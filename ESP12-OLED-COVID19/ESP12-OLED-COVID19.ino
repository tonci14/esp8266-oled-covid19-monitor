#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


/// CONFIG
#define STATE         "sk"
#define REFRESH_RATE  1 // minutes
#define WIFI_SSID     ""
#define WIFI_PASS     ""



ESP8266WiFiMulti WiFiMulti;
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

bool connectedToAp = false;
String url = "https://coronavirus-tracker-api.herokuapp.com/v2/locations?country_code=" + String(STATE);
const uint8_t fingerprint[20] = {0x08, 0x3B, 0x71, 0x72, 0x02, 0x43, 0x6E, 0xCA, 0xED, 0x42, 0x86, 0x93, 0xBA, 0x7E, 0xDF, 0x81, 0xC4, 0xBC, 0x62, 0x30};


void setup() {
  Serial.begin(115200);
  u8g2.begin();
  drawBoot();
  Serial.println();
  Serial.println();
  
  WiFi.mode(WIFI_STA);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);
  connectToAp();
}

void loop() {
  if(WiFiMulti.run()!= WL_CONNECTED) {
    connectToAp();
  } else {
    downloadData();
  }
}


void drawBoot()
{
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_open_iconic_www_6x_t);
    u8g2.drawGlyph(44, 48, 81); 
    u8g2.setFont(u8g2_font_8x13_mf);
    u8g2.drawStr(40, 62, "Hello :)");
  } while ( u8g2.nextPage() );
}

void draw(String datas) {
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, datas);
  const int confirmed = doc["latest"]["confirmed"];
  const int recovered = doc["latest"]["recovered"];
  const int deaths = doc["latest"]["deaths"];
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    drawError("Error parse data!");
    return;
  } else {
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_fur14_tf);  
      u8g2.setCursor(0, 15);
      u8g2.print("Conf:   " + String(confirmed));     
      u8g2.setCursor(0, 40);
      u8g2.print("Reco:   " + String(recovered)); 
      u8g2.setCursor(0, 64);
      u8g2.print("Deads: " + String(deaths));
    } while ( u8g2.nextPage() );
  }
}


void drawError(const char *error)
{
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t);
    u8g2.drawGlyph(35, 48, 71);
    u8g2.setFont(u8g2_font_8x13_mf);
    u8g2.drawStr(35, 62, error);
  } while ( u8g2.nextPage() );
}

void downloadData() {
  
  String data;
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    Serial.println("Download DATA...");

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setFingerprint(fingerprint);
    HTTPClient https;
    
    if (https.begin(*client, url)) {  // HTTPS
      https.setTimeout(30000);
      int httpCode = https.GET();
      if (httpCode > 0 && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
          data = https.getString();
          Serial.println(data);
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        drawError("Error load data!");
        delay(5000);
        return;
      }
      https.end();
      if(data.length() > 0) {
        draw(data);
        delay(REFRESH_RATE * 60 * 1000);
      } else {
        Serial.println("no data");
        drawError("Error load data!");
        delay(5000);
        return;
      }
    }else {
      Serial.printf("[HTTPS} Unable to connect\n");
      drawError("Connect Error!");
      delay(5000);
      return;
    }
  } else {
    drawError("Connect Error!");
  }
}

bool connectToAp() {
  bool prevConnectedToAp = connectedToAp;
  Serial.print("Connecting.");
  
  while(WiFiMulti.run() != WL_CONNECTED){
    Serial.print("....");
    delay(100);
  }
  connectedToAp = WiFiMulti.run() == WL_CONNECTED;
  Serial.println("DONE");
  
  if(connectedToAp){
    if(!prevConnectedToAp) {
      Serial.println("Created connection to AP: ");
      Serial.print("SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("Client IP: ");
      Serial.println(WiFi.localIP().toString());
    }
  }
  else {
    if(prevConnectedToAp) {
      Serial.println("Lost Connection!");
    }
  }
}
