#include "acsrestclient.h"

#include <RestClient.h>

#define SERVER "panopticon.hal9k.dk"
#define USE_SSL 1

AcsRestClient::AcsRestClient(const char* p)
    : path(p)
{
}

int AcsRestClient::post(const DynamicJsonDocument& doc)
{
    String s;
    serializeJson(doc, s);
    // Work around RestClient bug
    s = String("\r\n") + s;
    RestClient client(SERVER, USE_SSL ? 443 : 80, USE_SSL);
    client.setContentType("application/json");
    String url("/api/v1/");
    url += path;
    return client.post(url.c_str(), s.c_str(), &response);
}

String AcsRestClient::get_response() const
{
    return response;
}

