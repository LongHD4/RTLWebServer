/*
 *  SimpleServer.cpp
 *
 *  Created on: Feb 22, 2024
 */

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <Webserver.h>

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define WIFI_STATUS     0    /* Set SSID status, 1 hidden, 0 not hidden */

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/

const String index_raw = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>RTL8720</title>
    <link rel="shortcut icon" href="#"/>
</head>
<body>
<script>
    function sendPostRequest() {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', '/post_param', true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        var data = {
            param : "Hello"
        };
        var jsonData = JSON.stringify(data);
        xhr.onload = function () {
            if (xhr.status === 200) {
                console.log('POST request successful');
                console.log(xhr.responseText);
            } else {
                console.error('POST request failed');
            }
        };
        xhr.send(jsonData);
    }
    setInterval(sendPostRequest, 3000);
</script>
    <h1>RTL8720 Hello world!</h1?
</body>
</html>
)";

char wifi_ssid[32];
char wifi_password[64] = "12345678";
char wifi_channel[] = "1";

RTLWebServer server(80);

/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/



/******************************************************************************/

void wifi_conf(void) {
    uint8_t mac[6];
    int status = WL_IDLE_STATUS;

    /* Start STA mode to get MAC address */
    WiFi.begin("WiFiDummy", "");
    WiFi.macAddress(mac);
    sprintf(wifi_ssid, "RTL8720 WiFi %02X%02X", mac[4], mac[5]);

    /* Start AP mode */
    while (status != WL_CONNECTED) {
        status = WiFi.apbegin(wifi_ssid, wifi_password, wifi_channel, WIFI_STATUS);
        delay(3000);
    }

    Serial.println("Start wifi AP mode successfully");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    wifi_conf();

    server.on("/", HTTP_GET, [](RTLWebServerRequest *request) {
        request->send(200, "text/html", index_raw);
    });

    server.on("/get_param", HTTP_GET, [](RTLWebServerRequest *request) {
        if (request->hasParam("param")) {
            Serial.print("GET param "); Serial.println(request->getParam("param"));
        }
        request->send(200, "text/plain", "Hello GET");
    });

    server.on("/post_param", HTTP_POST, [](RTLWebServerRequest *request) {
        String content = request->content();
        if (content.length()) {
            Serial.print("POST param "); Serial.println(content);
        }
        request->send(200, "text/plain", "Hello POST");
    });
    server.begin();
}

void loop() {
    server.loop();
}
