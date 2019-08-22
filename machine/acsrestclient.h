#pragma once

#include <ArduinoJson.h>

class AcsRestClient
{
public:
    AcsRestClient(const char* path);

    void set_param(const char* name, const String& value);

    int post(const DynamicJsonDocument& obj);

    String get_response() const;

private:
    String path;
    String response;
};
