
#define BLYNK_TEMPLATE_ID "TMPL4dNc7OnrD"
#define BLYNK_TEMPLATE_NAME "Lingu v2"
#define BLYNK_AUTH_TOKEN "aSNGNh-a2T0223g0P805FjasWe8eOQs3"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include "esp_camera.h"


#define RXD2 14  
#define TXD2 15   

#define SMOKE_PIN 15 

#define DHTPIN 13 // DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;
char ssid[] = "iPhone (Mateusz)";
char pass[] = "1234567890";


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

void startCameraServer();


void checkSensors() {
  // --- A. DHT11 ---
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    Blynk.virtualWrite(V0, t); // Temp
    Blynk.virtualWrite(V1, h); // Wilg
  }


  int smokeStatus = digitalRead(SMOKE_PIN);

  if (smokeStatus == LOW) {
    Serial.println("!!! WYKRYTO DYM !!!");
    
    Blynk.virtualWrite(V3, 255); 
    
    Blynk.logEvent("fire_alert", "Pozar w smietniku!"); 
  } else {

    Blynk.virtualWrite(V3, 0);
  }


  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n'); 
    data.trim(); 

    if (data.length() > 0) {
      int distance = data.toInt();
      
      Serial.print("Odebrano z Arduino: ");
      Serial.print(distance);
      Serial.println(" cm");

      if (distance > 0) {
        Blynk.virtualWrite(V2, distance); 
      }
    }
  }
}

// ==========================================
// 5. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  

  pinMode(SMOKE_PIN, INPUT_PULLUP); 


  Serial2.begin(9600, SERIAL_8N1, RXD2, -1);

  dht.begin();


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
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  esp_camera_init(&config);


  Serial.println("Laczenie z Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("Polaczono!");

  startCameraServer();
  
  Serial.print("Stream: http://");
  Serial.println(WiFi.localIP());

  timer.setInterval(1000L, checkSensors);
}


void loop() {
  Blynk.run();
  timer.run();
}
