# RTLWebServer

[![Contact Support](https://img.shields.io/badge/Contact-Support-blue?style=for-the-badge)](mailto:longhd4196@gmail.com)
[![Buy Me a Coffee](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.paypal.me/ldragon196)

The Arduino Web Server Library simplifies the process of creating a web server on Arduino devices. This library allows you to quickly set up a web interface for controlling and monitoring your Arduino projects.

This library has been tested on RTL8720.

### Common
```cpp
void setup() {
    server.on("/", HTTP_GET, [](RTLWebServerRequest *request) {
        request->send(200, "text/html", index_raw);
    });
    server.begin();
}

void loop() {
    /* server.loop() should be running in the loop() or in a task */
    server.loop();
}
```

### Add GET/POST handlers
```cpp
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
```
