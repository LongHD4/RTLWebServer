/*
 *  Webserver.cpp
 *
 *  Created on: Feb 22, 2024
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include "Webserver.h"

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/



/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/



/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

static const char* responseCodeToString(int code) {
    switch (code) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Time-out";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Large";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested range not satisfiable";
        case 417: return "Expectation Failed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Time-out";
        case 505: return "HTTP Version not supported";
        default:  return "";
    }
}

RTLWebServerRequest::RTLWebServerRequest(WiFiClient *client) {
    _client = client;
    _params.clear();
}

void RTLWebServerRequest::stop() {
    _params.clear();
}

void RTLWebServerRequest::send(int code, const String& contentType, const String& content) {
    char buf[300];
    snprintf(buf, sizeof(buf), "HTTP/1.%d %d %s\r\n", version, code, responseCodeToString(code));
    Serial.println(buf);
    _client->println(buf);
}

void RTLWebServerRequest::_addParam(RTLWebParameter p) {
    _params.push_back(p);
}

void RTLWebServerRequest::addGetParams(const String& params) {
    size_t start = 0;
    while (start < params.length()) {
        int end = params.indexOf('&', start);
        if (end < 0) end = params.length();
        int equal = params.indexOf('=', start);
        if (equal < 0 || equal > end) equal = end;

        RTLWebParameter param;
        param.name = params.substring(start, equal);
        param.value = equal + 1 < end ? params.substring(equal + 1, end) : String();
        _addParam(param);
        start = end + 1;
    }
}

/******************************************************************************/

RTLWebServer::RTLWebServer(uint16_t port)
    : _server(port)
    , _method(HTTP_ANY)
    , _temp()
    , _url()
{
    _handlers.clear();
}

RTLWebServer::~RTLWebServer() {
    end();
}

String RTLWebServer::urlDecode(const String& text) const {
    char temp[] = "0x00";
    unsigned int len = text.length();
    unsigned int i = 0;
    String decoded = String();
    decoded.reserve(len);    /* Allocate the string internal buffer - never longer from source text */

    while (i < len) {
        char decodedChar;
        char encodedChar = text.charAt(i++);
        if ((encodedChar == '%') && (i + 1 < len)){
            temp[2] = text.charAt(i++);
            temp[3] = text.charAt(i++);
            decodedChar = strtol(temp, NULL, 16);
        }
        else if (encodedChar == '+') {
            decodedChar = ' ';
        }
        else {
            decodedChar = encodedChar;
        }
        decoded.concat(decodedChar);
    }
    return decoded;
}

bool RTLWebServer::_parseReqHead() {
    int index = _temp.indexOf(' ');
    String m = _temp.substring(0, index);
    index = _temp.indexOf(' ', index + 1);
    String u = _temp.substring(m.length() + 1, index);
    _temp = _temp.substring(index + 1);

    if (m == "GET") {
        _method = HTTP_GET;
        Serial.println("Received GET method");
    }
    else if (m == "POST") {
        _method = HTTP_POST;
        Serial.println("Received POST method");
    }
    else {
        Serial.println("Not supported method");
        return false;    /* We only support GET/POST method now */
    }

    String g = String();
    index = u.indexOf('?');
    if (index > 0) {
        g = u.substring(index + 1);
        u = u.substring(0, index);
    }

    if (!_temp.startsWith("HTTP/1.0")) {
        _request->version = 1;
    }
    else {
        _request->version = 0;
    }

    _url = urlDecode(u);
    Serial.print("URL "); Serial.println(_url);
    _request->addGetParams(g);

    _temp = String();
    return true;
}

/******************************************************************************/

void RTLWebServer::on(const char* uri, RTLRequestHandlerFunction onRequest) {
    RTLCallbackWebHandler handler;
    handler.setUri(uri);
    handler.onRequest(onRequest);

    /* Push to list */
    _handlers.push_back(handler);
}

void RTLWebServer::on(const char* uri, RTLWebMethod method, RTLRequestHandlerFunction onRequest) {
    RTLCallbackWebHandler handler;
    handler.setUri(uri);
    handler.setMethod(method);
    handler.onRequest(onRequest);

    /* Push to list */
    _handlers.push_back(handler);
}

void RTLWebServer::begin() {
    _server.begin();
}

void RTLWebServer::end() {
    _server.end();
}

void RTLWebServer::loop() {
    /* Check if a client has connected */
    WiFiClient client = _server.available();

    /* Read the request */
    _temp = client.readStringUntil('\r');
    client.flush();

    if (_temp.length()) {
        _request = new RTLWebServerRequest(&client);
        if (_parseReqHead()) {
            /* Find handler in list if matched with url and method */
            std::list<RTLCallbackWebHandler>::iterator next = _handlers.begin();
            while (next != _handlers.end()) {
                Serial.print("Method "); Serial.println(next->method());
                if (next->method() == _method) {
                    Serial.print("Handle method "); Serial.println(next->method());
                    next->handleRequest(_request);
                    break;
                }
                next++;
            }
        }
        _request->stop();
    }

    /* Close connection */
    client.stop();
}
