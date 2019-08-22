#include "wifi.h"

#include "eeprom.h"
#include "display.h"
#include "led.h"

WiFiHandler::WiFiHandler()
{
}

void WiFiHandler::init(AbstractLed& led,
                       Display& disp)
{
    led.set_colour(CRGB::Orange);
    led.set_duty_cycle(10);
    led.set_period(1);

    const auto ssid = Eeprom::get_ssid();

    WiFi.mode(WIFI_STA);
    int tries = 0;
    while (ssid.length() != 0 && tries < 5)
    {
        led.update();
        Serial.println();
        Serial.println();
        String s = "Trying ";
        s += ssid;
        disp.set_network_status(s.c_str());
        Serial.println(s);

        WiFi.begin(ssid, Eeprom::get_password());
        led.update();

        int i = 0;
        bool connected = false;
        while (i < 5000)
        {
            delay(1);
            led.update();
            if (WiFi.status() == WL_CONNECTED)
            {
                connected = true;
                break;
            }
            ++i;
        }
        if (connected)
        {
            const auto token = Eeprom::get_api_token();
            disp.set_network_status(token.length() ? "Online" : "(online)");
            Serial.println("");
            Serial.print("Connected to ");
            Serial.println(ssid);
            led.set_colour(CRGB::Green);
            led.set_duty_cycle(1);
            led.set_period(10);
            break;
        }
        Serial.println("");
        ++tries;
    }

    led.update();
    
    // Print the IP address
    Serial.println(WiFi.localIP());

    led.update();

    // Set up mDNS responder
    String dns_name = "machine-";
    dns_name += Eeprom::get_machine_id();
    if (!m_mdns.begin(dns_name.c_str()))
        Serial.println("Error setting up mDNS responder!");
    else
    {
        Serial.println("mDNS responder started");
        Serial.print("My name is ");
        Serial.println(dns_name);
    }
}
