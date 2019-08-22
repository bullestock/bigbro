#pragma once

#include <Arduino.h>

class Eeprom
{
public:
    static void init();

    static String get_ssid();
    static void set_ssid(const char*);

    static String get_password();
    static void set_password(const char*);

    static String get_machine_id();
    static void set_machine_id(const char*);

    static String get_api_token();
    static void set_api_token(const char*);

    static void erase();
    static void dump();

private:
    static String get(size_t offset, size_t max_size);

    static void set(const char* token, size_t offset, size_t max_size);
};
