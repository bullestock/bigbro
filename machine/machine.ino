// Libraries: 
// https://github.com/DaKaZ/esp8266-restclient
// https://github.com/plerup/espsoftwareserial
// Library manager:
// ArduinoJson
// FastLed

#define SERIAL_DBG  0

#include <ArduinoJson.h>
#include <EEPROM.h>
extern "C" {
#include <user_interface.h>
}

#include "acsrestclient.h"
#include "cardreader.h"
#include "display.h"
#include "eeprom.h"
#include "led.h"
#include "wifi.h"

const char* VERSION = "0.2.0";

// TX is not connected
#define PIN_TX       11
#define PIN_RX       14
#define PIN_SWITCH   12
#define PIN_LED      13
#define PIN_RELAY    15

Display display;

Led<PIN_LED> led;

WiFiHandler wifi_handler;

unsigned long start_tick = 0;
unsigned long last_info_tick = 0;
bool showing_version = true;

String get_reset_reason()
{
    static const struct
    {
        int reason;
        const char* desc;
    } reasons[] = {
        { REASON_DEFAULT_RST, "Def" },
        { REASON_WDT_RST, "WD" },
        { REASON_EXCEPTION_RST, "Ex" },
        { REASON_SOFT_WDT_RST, "SWD" },
        { REASON_SOFT_RESTART, "Res" },
        { REASON_DEEP_SLEEP_AWAKE, "DS" },
        { REASON_EXT_SYS_RST, "Sys" }
    };
    
    const auto r = ESP.getResetInfoPtr()->reason;

    for (size_t i = 0; i < sizeof(reasons)/sizeof(reasons[0]); ++i)
    {
        if (r == reasons[i].reason)
            return reasons[i].desc;
    }
    return String(r);
}

void update_info(unsigned long now)
{
    auto rem = now/1000;
    const auto h = rem/(60*60);
    rem -= h*60*60;
    const auto m = rem/60;
    rem -= m*60;
    char buf[3+3+4+2+1+2+1+2+10];
    sprintf(buf, "RC %s Up %02d:%02d:%02d",
            get_reset_reason().c_str(), h, m, rem);
    display.set_info(buf);
}

void setup()
{
    delay(1000);
    Serial.begin(115200);
    Serial.print("Machine v ");
    Serial.println(VERSION);

    pinMode(PIN_RELAY, OUTPUT);

    // We need room for machine name (not more than 20 bytes) and API token (64 bytes)
    Eeprom::init();

    display.set_machine_id(Eeprom::get_machine_id().c_str());
    String s = "Version ";
    s += VERSION;
    display.set_status(s);
    start_tick = millis();

    // Connect to WiFi network
    wifi_handler.init(led, display);
}

CardReader reader(PIN_RX, PIN_TX, PIN_SWITCH);
String last_card_id;

void decode_line(const char* line)
{
    int i = 0;
    switch (tolower(line[i]))
    {
    case 'h':
        Serial.println("Commands:");
        Serial.println("  s  Set SSID");
        Serial.println("  p  Set password");
        Serial.println("  k  Set API token");
        Serial.println("  m  Set machine ID");
        Serial.println("  t  Send test request");
        Serial.println("  x  Erase EEPROM");
        break;
        
    case 's':
        // Set SSID
        while (line[++i] == ' ')
            ;
        Eeprom::set_ssid(line+i);
        Serial.println("SSID set");
        return;

    case 'p':
        // Set password
        while (line[++i] == ' ')
            ;
        Eeprom::set_password(line+i);
        Serial.println("Password set");
        return;

    case 'k':
        // Set API token
        while (line[++i] == ' ')
            ;
        Eeprom::set_api_token(line+i);
        Serial.println("API token set");
        return;

    case 'm':
        // Set machine ID
        while (line[++i] == ' ')
            ;
        Eeprom::set_machine_id(line+i);
        Serial.println("Machine ID set");
        display.set_machine_id(Eeprom::get_machine_id().c_str());
        return;

    case 't':
        {
            Serial.println("Sending test request");
            String message, user_name;
            bool allowed = false;
            int user_id = 0;
            if (!query_permission("0000BB96C5", allowed, user_name, user_id, message))
            {
                Serial.print("Error: ");
                Serial.println(message);
            }
            else
            {
                Serial.print("Success: User ");
                Serial.println(user_name);
            }
        }
        break;

    case 'x':
        Eeprom::erase();
        Serial.println("EEPROM erased");
        break;
        
    case 'z':
        Eeprom::dump();
        break;
        
    default:
        Serial.print("Unknown command: ");
        Serial.println(line);
        return;
    }
}

const int MAX_LINE_LENGTH = 80;
char line[MAX_LINE_LENGTH+1];
int line_len = 0;

bool query_permission(const String& card_id,
                      bool& allowed,
                      String& user_name,
                      int& user_id,
                      String& message)
{
    AcsRestClient rc("permissions");
    DynamicJsonDocument doc(200);
    doc["api_token"] = Eeprom::get_api_token();
    doc["card_id"] = card_id;
    const auto status = rc.post(doc);
    led.update();
    Serial.print("HTTP status ");
    Serial.println(status);
    if (status == 200)
    {
        auto resp = rc.get_response();
        // Remove garbage (why is it there?)
        int i = 0;
        while ((resp[i] != '{') && (i < resp.length()))
            ++i;
        int j = i;
        while ((resp[j] != '}') && (j < resp.length()))
            ++j;
        resp = resp.substring(i, j+1);
        StaticJsonDocument<200> doc;
        auto error = deserializeJson(doc, resp);
        if (error)
        {
            Serial.println("Bad JSON:");
            Serial.println(resp);
            message = "Bad JSON";
        }
        else
        {
            allowed = doc["allowed"];
            user_name = (const char*) doc["name"];
            user_id = doc["id"];
        }
        return true;
    }
    else if (status == 404)
    {
        message = "Unknown card";
        allowed = false;
        return true;
    }
    String s = "Bad HTTP reply:";
    s += String(status);
    message = s;
    return false;
}

void loop()
{
    yield();
    reader.update();
    
    const auto card_id = reader.get_card_id();
    if (card_id != last_card_id)
    {
        last_card_id = card_id;
        if (card_id.length())
        {
            Serial.println("Card present");
            display.set_status("Card present");
            yield();
            String message, user_name;
            bool allowed = false;
            int user_id = 0;
            if (!query_permission(card_id, allowed, user_name, user_id, message))
                display.set_status(message);
            else
            {
                if (allowed)
                {
                    digitalWrite(PIN_RELAY, 1);
                    led.set_colour(CRGB::Green);
                }
                else
                    led.set_colour(CRGB::Red);
            }
            yield();
            led.set_duty_cycle(100);
            led.update();
            String name_trunc = user_name;
            if (name_trunc.length() > 12)
                name_trunc = name_trunc.substring(0, 12) + String("...");
            display.set_status(name_trunc,
                               allowed ? "OK" : "Denied");

            AcsRestClient logger("logs");
            DynamicJsonDocument doc(200);
            yield();
            doc["api_token"] = Eeprom::get_api_token();
            auto log = doc.createNestedObject("log");
            log["user_id"] = user_id;
            if (allowed)
                log["message"] = "Successful machine access";
            else
                log["message"] = "Machine access denied";
            const auto status = logger.post(doc);
            yield();
            if (status != 200)
            {
                String s = "Bad HTTP log reply:";
                s += String(status);
                display.set_status(s);
            }
            else if (status == 404)
                // Unknown card
                display.set_status("Unknown card:", card_id);
            yield();
        }
    }

    if (!card_id.length())
    {
        if (!showing_version)
            display.set_status("No card");
        digitalWrite(PIN_RELAY, 0);
        led.set_colour(CRGB::Green);
        led.set_duty_cycle(1);
        led.set_period(10);
    }

    const auto now = millis();
    if (now - start_tick > 5000)
        showing_version = false;
    if (now - last_info_tick > 1000)
    {
        update_info(now);
        last_info_tick = now;
    }
    
    delay(1);
    led.update();
    yield();

    if (Serial.available())
    {
        yield();
        const char c = Serial.read();
        if ((c == '\r') || (c == '\n'))
        {
            line[line_len] = 0;
            line_len = 0;
            decode_line(line);
        }
        else if (line_len < MAX_LINE_LENGTH)
            line[line_len++] = c;
        else
        {
            Serial.print("Line too long: ");
            Serial.println(line);
            line_len = 0;
        }
    }
}
