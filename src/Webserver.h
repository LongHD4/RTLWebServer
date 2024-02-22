/*
 *  Webserver.h
 *
 *  Created on: Feb 22, 2024
 */

#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <WiFi.h>

/* undefine invalid defines from stl_algobase.h */
#undef max
#undef min
#include <list>

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

typedef enum {
    HTTP_GET  = 0b00000001,
    HTTP_POST = 0b00000010,
    HTTP_ANY  = 0b11111111,
} RTLWebMethod;

/*!
 * @brief  Chainable object to hold GET/POST and FILE parameters
 */
typedef struct {
    String name;
    String value;
} RTLWebParameter;

/*!
 * @brief  Chainable object to hold the headers
 */
class RTLWebHeader {
    private:
        String _name;
        String _value;

    public:
        RTLWebHeader(const String& name, const String& value): _name(name), _value(value){}
        RTLWebHeader(const String& data): _name(), _value(){
        if(!data) return;

        int index = data.indexOf(':');
        if (index < 0) return;
            _name = data.substring(0, index);
            _value = data.substring(index + 2);
        }
        ~RTLWebHeader(){}
        const String& name() const { return _name; }
        const String& value() const { return _value; }
        String toString() const { return String(_name + ": " + _value + "\r\n"); }
};

class RTLWebServerRequest {
    private:
        std::list<RTLWebParameter> _params;
        WiFiClient *_client;
        void _addParam(RTLWebParameter p);

    public:
        uint8_t version;
        RTLWebServerRequest(WiFiClient *client);
        void stop();

        void send(int code, const String& contentType=String(), const String& content=String());
        void addGetParams(const String& params);
};

/******************************************************************************/

typedef void (*RTLRequestHandlerFunction)(RTLWebServerRequest *request);

class RTLCallbackWebHandler {
    private:
        String _uri;
        RTLWebMethod _method;
        RTLRequestHandlerFunction _onRequest;

    public:
        RTLCallbackWebHandler() : _uri(), _method(HTTP_ANY), _onRequest(NULL) {}
        void setUri(const String& uri) {
            _uri = uri;
        }
        void setMethod(RTLWebMethod method){ _method = method; }
        void onRequest(RTLRequestHandlerFunction fn){ _onRequest = fn; }

        RTLWebMethod method() { return _method; }

        void handleRequest(RTLWebServerRequest *request) {
            if (_onRequest) {
                _onRequest(request);
            }
            else {
                request->send(500);
            }
        }
};

/******************************************************************************/

class RTLWebServer {
    public:
        RTLWebServer(uint16_t port);
        ~RTLWebServer();

        void begin();
        void end();
        void loop();

        void on(const char* uri, RTLRequestHandlerFunction onRequest);
        void on(const char* uri, RTLWebMethod method, RTLRequestHandlerFunction onRequest);

        String urlDecode(const String& text) const;
    
    private:
        WiFiServer _server;
        std::list<RTLCallbackWebHandler> _handlers;
        RTLWebMethod _method;
        String _temp;
        String _url;
        RTLWebServerRequest *_request;

        bool _parseReqHead();
};

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/

#endif /* _WEBSERVER_H_ */
