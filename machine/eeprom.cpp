#include "eeprom.h"

#include <EEPROM.h>

const int MACHINE_ID_OFFSET = 0;
const int MACHINE_ID_SIZE = 32;
const int API_TOKEN_OFFSET = MACHINE_ID_OFFSET + MACHINE_ID_SIZE;
const int API_TOKEN_SIZE = 80;
const int SSID_OFFSET = API_TOKEN_OFFSET + API_TOKEN_SIZE;
const int SSID_SIZE = 32;
const int PASSWORD_OFFSET = SSID_OFFSET + SSID_SIZE;
const int PASSWORD_SIZE = 32;

void Eeprom::init()
{
    EEPROM.begin(PASSWORD_OFFSET + PASSWORD_SIZE);
}

String Eeprom::get_ssid()
{
    return get(SSID_OFFSET, SSID_SIZE);
}

void Eeprom::set_ssid(const char* id)
{
    set(id, SSID_OFFSET, SSID_SIZE);
}

String Eeprom::get_password()
{
    return get(PASSWORD_OFFSET, PASSWORD_SIZE);
}

void Eeprom::set_password(const char* id)
{
    set(id, PASSWORD_OFFSET, PASSWORD_SIZE);
}

String Eeprom::get_machine_id()
{
    return get(MACHINE_ID_OFFSET, MACHINE_ID_SIZE);
}

void Eeprom::set_machine_id(const char* id)
{
    set(id, MACHINE_ID_OFFSET, MACHINE_ID_SIZE);
}

String Eeprom::get_api_token()
{
    return get(API_TOKEN_OFFSET, API_TOKEN_SIZE);
}

void Eeprom::set_api_token(const char* token)
{
    set(token, API_TOKEN_OFFSET, API_TOKEN_SIZE);
}

String Eeprom::get(size_t offset, size_t max_size)
{
    String s;
    int i = 0;
    while (i < max_size)
    {
        const auto c = static_cast<char>(EEPROM.read(offset+i));
        if (!c)
            break;
        s.concat(c);
        ++i;
    }
    return s;
}

void Eeprom::set(const char* token, size_t offset, size_t max_size)
{
    if (strlen(token) > max_size)
    {
        Serial.println("ERROR: API token too long");
        return;
    }
    int i = 0;
    while (i < strlen(token))
    {
        EEPROM.write(offset+i, token[i]);
        ++i;
    }
    EEPROM.write(offset+i, 0);
    if (!EEPROM.commit())
        Serial.println("ERROR: Cannot commit to EEPROM");
}

void Eeprom::erase()
{
    Serial.println(EEPROM.length());
    for (int i = 0; i < PASSWORD_OFFSET + PASSWORD_SIZE; ++i)
        EEPROM.write(i, 0);
    if (!EEPROM.commit())
        Serial.println("ERROR: Cannot commit to EEPROM");
}

void Eeprom::dump()
{
    for (int i = 0; i < PASSWORD_OFFSET + PASSWORD_SIZE; ++i)
    {
        Serial.printf("%02x ", EEPROM.read(i));
        if (i > 0 && ((i + 1) % 16) == 0)
            Serial.println();
    }
}
