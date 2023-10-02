#define HWSerial Serial0
// #define LTESerial Serial1
#define USBSerial Serial

#include <WiFi.h>
#include <HTTPClient.h>

int rxPin = 1;
int txPin = 0;

bool sending = false;

const char* ssid = "Chocolate";        // Change this to your WiFi SSID
const char* password = "961002+Mike";  // Change this to your WiFi password

HardwareSerial LTESerial(1);
int nextLineIsPdu = 0;
int inited = 0;
String IMEI = "";
String ICCID = "";
String IMSI = "";
String deviceId = "123";
String url = "http://47.102.87.155:3006/newDevice";
String newSMSurl = "http://47.102.87.155:3006/newSMS";


String sentAt(String command) {
  if (sending == false) {
    sending = true;
    Serial.print("send:");
    LTESerial.println(command);
    int i = 0;
    while (1) {
      if (LTESerial.available()) {
        String respond = LTESerial.readString();
        Serial.print(respond);
        sending = false;
        return respond;
      }
      i ++;
      if (i > 5000) {
        Serial.println("超时");
        sending = false;
        return "";
      }
    }
  } else {
    Serial.println("BUSY");
    return "";
  }

}


void setup() {
  USBSerial.begin(115200);
  HWSerial.begin(115200);
  LTESerial.begin(115200, SERIAL_8N1, rxPin, txPin);
  USBSerial.println("Hello, ESP32-C3 ADC Example!");
  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(5000);
}

void loop() {
  WiFiClient client;
  HTTPClient http;
  if (inited == 0) {
    String report = sentAt("AT+CCID");
    USBSerial.println(report);
    report = sentAt("AT");
    USBSerial.println(report);
    String json = "{\"deviceID\":\"" + deviceId + "\"}";
    if (!http.begin(client, url)) {
      Serial.println("Failed HTTPClient begin!");
      return;
    }
    Serial.println(json);
    http.addHeader("Content-Type", "application/json");
    int responseCode = http.POST(json);
    String body = http.getString();

    Serial.println(responseCode);
    Serial.println(body);

    http.end();
    inited = 1;
  }
  while (USBSerial.available()) {
    String incomingStream = USBSerial.readStringUntil('\n');
    LTESerial.println(incomingStream);
  }
  while (LTESerial.available()) {
    String incomingStream = LTESerial.readStringUntil('\n');
    if (incomingStream.indexOf("+CMT") != -1) {
      USBSerial.println("new msg");
      nextLineIsPdu = 1;
      break;
    }
    if (nextLineIsPdu == 1) {
      USBSerial.println("new msg is :");
      USBSerial.println(incomingStream);
      incomingStream.replace("\n","");
      incomingStream.replace("\r","");
      String json = "{\"deviceID\":\"" + deviceId + "\",\"SMS\":\""+ incomingStream +"\"}";
      if (!http.begin(client, newSMSurl)) {
        Serial.println("Failed HTTPClient begin!");
        return;
      }
      Serial.println(json);
      http.addHeader("Content-Type", "application/json");
      int responseCode = http.POST(json);
      String body = http.getString();

      Serial.println(responseCode);
      Serial.println(body);

      http.end();
      nextLineIsPdu = 0;
    }
  }
}
