#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_camera.h>

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_PIN         4

WebServer server(80);

void setup() {
    Serial.begin(115200);
    
    // Вспышка
    pinMode(FLASH_PIN, OUTPUT);
    digitalWrite(FLASH_PIN, HIGH);
    
    // КАМЕРА - БАЛАНС КАЧЕСТВА И СКОРОСТИ
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA;    // 800x600 - золотая середина
    config.jpeg_quality = 8;               // Качество 8 из 63
    config.fb_count = 2;                   // 2 буфера для скорости
    
    esp_camera_init(&config);
    
    // Настройки изображения
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    s->set_brightness(s, 1);
    s->set_contrast(s, 1);
    s->set_saturation(s, 1);
    
    // WiFi
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-CAM", "12345678");
    
    server.on("/", []() {
        String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'><title>ESP32-CAM</title><style>body{margin:0;background:#000}img{width:100%}</style></head><body>";
        html += "<img src='/stream'>";
        html += "</body></html>";
        server.send(200, "text/html", html);
    });
    
    server.on("/stream", []() {
        WiFiClient client = server.client();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
        client.println();
        while(client.connected()) {
            camera_fb_t *fb = esp_camera_fb_get();
            if(fb) {
                client.print("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: ");
                client.print(fb->len);
                client.print("\r\n\r\n");
                client.write(fb->buf, fb->len);
                client.print("\r\n");
                esp_camera_fb_return(fb);
            }
        }
    });
    
    server.begin();
    
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
}

void loop() {
    server.handleClient();
}