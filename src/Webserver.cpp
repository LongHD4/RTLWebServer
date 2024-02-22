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
    _content = String();
    _params.clear();
}

void RTLWebServerRequest::stop() {
    _params.clear();
}

void RTLWebServerRequest::setContent(const String& content) {
    _content = content;
}

String RTLWebServerRequest::content() {
    return _content;
}

void RTLWebServerRequest::send(int code, const String& contentType, const String& content) {
    int max_len = 300;
    char buf[300];

    int index = snprintf(buf, max_len, "HTTP/1.%d %d %s\r\n", version, code, responseCodeToString(code));
    if(contentType.length()) {
        max_len -= index;
        index += snprintf(&buf[index], max_len, "Content-Type: %s\r\n", contentType.c_str());
    }
    _client->println(buf);

    if (content.length()) {
        _client->println(content);
    }
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

bool RTLWebServerRequest::hasParam(const String& name, bool post) {
    std::list<RTLWebParameter>::iterator next = _params.begin();

    if (post != is_post) {
        return false;
    }

    while (next != _params.end()) {
        String param = String(next->name);
        if (param == name) {
            return true;
        }
        next++;
    }
    return false;
}

const char *RTLWebServerRequest::getParam(const String& name, bool post) {
    std::list<RTLWebParameter>::iterator next = _params.begin();

    if (post != is_post) {
        return nullptr;
    }

    while (next != _params.end()) {
        String param = String(next->name);
        if (param == name) {
            return next->value.c_str();
        }
        next++;
    }

    return nullptr;
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

    _request->is_post = false;
    if (m == "GET") {
        _method = HTTP_GET;
    }
    else if (m == "POST") {
        _method = HTTP_POST;
        _request->is_post = true;
    }
    else {
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
    bool handled = false;

    /* Check if a client has connected */
    WiFiClient client = _server.available();
    if (!client.available()) {
        return;
    }
    _request = new RTLWebServerRequest(&client);
    
    /* Read the request header */
    _temp = client.readStringUntil('\r');

    if (_parseReqHead()) {
        if (_method == HTTP_POST) {
            /* Discard the headers */
            while (client.available()) {
                _temp = client.readStringUntil('\r');
                if (_temp == "\n") {
                    break;
                }
            }

            /* Read the POST data */
            _temp = client.readStringUntil('\r');
            _request->setContent(_temp);
        }
        
        /* Find handler in list if matched with url and method */
        std::list<RTLCallbackWebHandler>::iterator next = _handlers.begin();
        while (next != _handlers.end()) {
            if ((next->method() == _method) && (_url == next->url())) {
                next->handleRequest(_request);
                handled = true;
                break;
            }
            next++;
        }
    }
    else {
        _request->send(404);    /* Not Found */
    }

    if (!handled) {
        _request->send(405);    /* Method Not Allowed */
    }
    _request->stop();

    /* Close connection */
    client.flush();
    client.stop();
}
