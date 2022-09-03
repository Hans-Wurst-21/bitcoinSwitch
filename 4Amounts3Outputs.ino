#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <Wire.h>
using WebServerClass = WebServer;
fs::SPIFFSFS &FlashFS = SPIFFS;
#define FORMAT_ON_FAIL true
#include <JC_Button.h>
#include "qrcoded.h"

#include <AutoConnect.h>
#include <ArduinoJson.h>

#define PARAM_FILE "/elements.json"

/////////////////////////////////
///////////CHANGE////////////////
/////////////////////////////////
  
bool format = false; // true for formatting SPIFFS, use once, then make false and reflash
int portalPin = 4; // Pin HIGH at boot to start the Portal-Mode

///////////Hans-Wurst////////////

int amount1 = 2000; // Amount for LED 1 in mSats
int amount2 = 4000; // Amount for LED 2 in mSats
int amount3 = 6000; // Amount for LED 3 in mSats
int amount4 = 10000; // Amount for all LEDs in mSats
#define LED1 26 // Output Pin D26 for LED1
#define LED2 27 // Output Pin D27 for LED2
#define LED3 14 // Output Pin D14 for LED3

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////


// Access point variables
String password;
String lnbitsServer;
String lnurlP;
String invoiceKey;
String highPin;
String timePin;
String amount;
String dataId;
String description;
String payReq;

int balance;
int oldBalance;

bool paid;
bool down = false;
bool triggerAp = false; 

String content = "<h1>Base Access-point</br>For easy variable and wifi connection setting</h1>";

// custom access point pages
static const char PAGE_ELEMENTS[] PROGMEM = R"(
{
  "uri": "/config",
  "title": "Access Point Config",
  "menu": true,
  "element": [
    {
      "name": "text",
      "type": "ACText",
      "value": "AP options",
      "style": "font-family:Arial;font-size:16px;font-weight:400;color:#191970;margin-botom:15px;"
    },
    {
      "name": "password",
      "type": "ACInput",
      "label": "Password",
      "value": "ToTheMoon"
    },
    {
      "name": "text",
      "type": "ACText",
      "value": "Project options",
      "style": "font-family:Arial;font-size:16px;font-weight:400;color:#191970;margin-botom:15px;"
    },
    {
      "name": "server",
      "type": "ACInput",
      "label": "LNbits server",
      "value": "legend.lnbits.com"
    },
    {
      "name": "lnurl",
      "type": "ACInput",
      "label": "LNURLPay optional for static QR",
      "value": ""
    },
    {
      "name": "invoicekey",
      "type": "ACInput",
      "label": "LNbits invoice key",
      "value": ""
    },
    {
      "name": "pin",
      "type": "ACInput",
      "label": "Pin to turn on",
      "value": ""
    },
    {
      "name": "time",
      "type": "ACInput",
      "label": "Time to turn on pin",
      "value": ""
    },
    {
      "name": "amount",
      "type": "ACInput",
      "label": "Amount to check for",
      "value": ""
    },
    {
      "name": "load",
      "type": "ACSubmit",
      "value": "Load",
      "uri": "/config"
    },
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "Save",
      "uri": "/save"
    },
    {
      "name": "adjust_width",
      "type": "ACElement",
      "value": "<script type='text/javascript'>window.onload=function(){var t=document.querySelectorAll('input[]');for(i=0;i<t.length;i++){var e=t[i].getAttribute('placeholder');e&&t[i].setAttribute('size',e.length*.8)}};</script>"
    }
  ]
 }
)";

static const char PAGE_SAVE[] PROGMEM = R"(
{
  "uri": "/save",
  "title": "Elements",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "format": "Elements have been saved to %s",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "validated",
      "type": "ACText",
      "style": "color:red"
    },
    {
      "name": "echo",
      "type": "ACText",
      "style": "font-family:monospace;font-size:small;white-space:pre;"
    },
    {
      "name": "ok",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/config"
    }
  ]
}
)";

WebServerClass server;
AutoConnect portal(server);
AutoConnectConfig config;
AutoConnectAux elementsAux;
AutoConnectAux saveAux;

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  const byte BUTTON_PIN_A = 39;
  Button BTNA(BUTTON_PIN_A);
  BTNA.begin();
  int timer = 0;
  pinMode (2, OUTPUT);
  while (timer < 2000)
  {
    digitalWrite(2, LOW);
    Serial.println(touchRead(portalPin));
      if(touchRead(portalPin) < 60){
        Serial.println("Launch portal");
        triggerAp = true;
        timer = 5000;
      }
    digitalWrite(2, HIGH);
    timer = timer + 100;
    delay(300);
  }
    
 // h.begin();
  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);
  if(format == true){
    SPIFFS.format(); 
  }
  // get the saved details and store in global variables
  File paramFile = FlashFS.open(PARAM_FILE, "r");
  if (paramFile)
  {
    StaticJsonDocument<2500> doc;
    DeserializationError error = deserializeJson(doc, paramFile.readString());

    const JsonObject maRoot0 = doc[0];
    const char *maRoot0Char = maRoot0["value"];
    password = maRoot0Char;
    
    const JsonObject maRoot1 = doc[1];
    const char *maRoot1Char = maRoot1["value"];
    lnbitsServer = maRoot1Char;

    const JsonObject maRoot2 = doc[2];
    const char *maRoot2Char = maRoot2["value"];
    lnurlP = maRoot2Char;
    lnurlP.toUpperCase();

    const JsonObject maRoot3 = doc[3];
    const char *maRoot3Char = maRoot3["value"];
    invoiceKey = maRoot3Char;

    const JsonObject maRoot4 = doc[4];
    const char *maRoot4Char = maRoot4["value"];
    highPin = maRoot4Char;
    
    const JsonObject maRoot5 = doc[5];
    const char *maRoot5Char = maRoot5["value"];
    timePin = maRoot5Char;

    const JsonObject maRoot6 = doc[6];
    const char *maRoot6Char = maRoot6["value"];
    amount = maRoot6Char;
  }
  else{
    triggerAp = true;
  }
  paramFile.close();
    server.on("/", []() {
      content += AUTOCONNECT_LINK(COG_24);
      server.send(200, "text/html", content);
    });
    
    elementsAux.load(FPSTR(PAGE_ELEMENTS));
    elementsAux.on([](AutoConnectAux &aux, PageArgument &arg) {
      File param = FlashFS.open(PARAM_FILE, "r");
      if (param)
      {
        aux.loadElement(param, {"password", "server", "lnurl", "invoicekey", "pin", "time", "amount"});
        param.close();
      }

      if (portal.where() == "/config")
      {
        File param = FlashFS.open(PARAM_FILE, "r");
        if (param)
        {
          aux.loadElement(param, {"password", "server", "lnurl", "invoicekey", "pin", "time", "amount"});
          param.close();
        }
      }
      return String();
    });
    saveAux.load(FPSTR(PAGE_SAVE));
    saveAux.on([](AutoConnectAux &aux, PageArgument &arg) {
      aux["caption"].value = PARAM_FILE;
      File param = FlashFS.open(PARAM_FILE, "w");
      if (param)
      {
        // save as a loadable set for parameters.
        elementsAux.saveElement(param, {"password", "server", "lnurl", "invoicekey", "pin", "time", "amount"});
        param.close();
        // read the saved elements again to display.
        param = FlashFS.open(PARAM_FILE, "r");
        aux["echo"].value = param.readString();
        param.close();
      }
      else
      {
        aux["echo"].value = "Filesystem failed to open.";
      }
      return String();
    });
    config.auth = AC_AUTH_BASIC;
    config.authScope = AC_AUTHSCOPE_AUX;
    config.ticker = true;
    config.autoReconnect = true;
    config.apid = "Device-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    config.psk = password;
    config.menuItems = AC_MENUITEM_CONFIGNEW | AC_MENUITEM_OPENSSIDS | AC_MENUITEM_RESET;
    config.reconnectInterval = 1;

    if (triggerAp == true)
    {
      config.immediateStart = true;
      portal.join({elementsAux, saveAux});
      portal.config(config);
      portal.begin();
      while (true)
      {
        portal.handleClient();
      }
      timer = 2000;
    }
    timer = timer + 200;
    delay(200);
  if (invoiceKey != "")
  {
    portal.join({elementsAux, saveAux});
    config.autoRise = false;
    portal.config(config);
    portal.begin();
  }
  triggerAp = false;
  delay(1000);
  pinMode(highPin.toInt(), OUTPUT); // Pin Mode from Portal
  pinMode(LED1, OUTPUT); // Pin Mode for LED 1
  pinMode(LED2, OUTPUT); // Pin Mode for LED 2
  pinMode(LED3, OUTPUT); // Pin Mode for LED 3
}

void loop() {
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Failed to connect");
    delay(500);
  }
  delay(250);
  Serial.println(highPin.toInt());
  Serial.println(timePin.toInt());
  Serial.println(LED1);
  Serial.println(LED2);
  Serial.println(LED3);
  Serial.println(lnurlP.substring(0, 5));
  if(lnurlP.substring(0, 5) == "LNURL"){
    checkBalance();
    oldBalance = balance;
    while((oldBalance + amount.toInt()) > balance){
      checkBalance();
      delay(2000);
    }
  if (balance - oldBalance == amount1) { // Amount 1, in example LED 1
    oldBalance = balance;
    Serial.print("LED 1");
	delay(250);
    digitalWrite(LED1, HIGH);
    delay(150);
    digitalWrite(LED1, LOW);
    delay(150);
    digitalWrite(LED1, HIGH);
    delay(150);
    digitalWrite(LED1, LOW);
    delay(150);
    digitalWrite(LED1, HIGH);
    delay(150);
    digitalWrite(LED1, LOW);
    delay(150);
    digitalWrite(LED1, HIGH);
    delay(150);
    digitalWrite(LED1, LOW);
	delay(150);
    digitalWrite(LED1, HIGH);
    delay(150);
    digitalWrite(LED1, LOW);
    delay(2000);
    paid = false;
  }
  if (balance - oldBalance == amount2) { // Amount 2, in example LED 2
    oldBalance = balance;
    Serial.print("LED 2");
	delay(250);
    digitalWrite(LED2, HIGH);
    delay(500);
    digitalWrite(LED2, LOW);
    delay(500);
    digitalWrite(LED2, HIGH);
    delay(500);
    digitalWrite(LED2, LOW);
    delay(500);
    digitalWrite(LED2, HIGH);
    delay(500);
    digitalWrite(LED2, LOW);
    delay(500);
    digitalWrite(LED2, HIGH);
    delay(500);
    digitalWrite(LED2, LOW);
	delay(500);
    digitalWrite(LED2, HIGH);
    delay(500);
    digitalWrite(LED2, LOW);
	delay(2000);
    paid = false;
  }
    if (balance - oldBalance == amount3) { // Amount 3, in example LED 3
    oldBalance = balance;
    Serial.print("LED 3");
	delay(250);
    digitalWrite(LED3, HIGH);
    delay(3000);
    digitalWrite(LED3, LOW);
	delay(2000);
    paid = false;
  }
    if (balance - oldBalance == amount4) { // Amount 4, in example all LEDs
    oldBalance = balance;
    Serial.print("All LEDs flashing");
	delay(250);
	digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
	delay(500);
    digitalWrite(LED1, LOW);
	digitalWrite(LED2, LOW);
	digitalWrite(LED3, LOW);
    delay(500);
	digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
	delay(500);
    digitalWrite(LED1, LOW);
	digitalWrite(LED2, LOW);
	digitalWrite(LED3, LOW);
	delay(500);
	digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
	delay(500);
    digitalWrite(LED1, LOW);
	digitalWrite(LED2, LOW);
	digitalWrite(LED3, LOW);
    delay(500);
	digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
	delay(500);
    digitalWrite(LED1, LOW);
	digitalWrite(LED2, LOW);
	digitalWrite(LED3, LOW);
	delay(500);
	digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
	delay(500);
    digitalWrite(LED1, LOW);
	digitalWrite(LED2, LOW);
	digitalWrite(LED3, LOW);
	delay(2000);
    paid = false;
  }
  }
  else{
    getInvoice();
    if(down){
      getInvoice();
      delay(5000);
    }
    if(payReq != ""){
      delay(5000);
    }
    while(paid == false && payReq != ""){
      checkInvoice();
      if(paid){
        Serial.println("Paid");
        digitalWrite(highPin.toInt(), HIGH);
        delay(timePin.toInt());
        digitalWrite(highPin.toInt(), LOW); 
      }
      delay(2000);
    }
    payReq = "";
    dataId = "";
    paid = false;
    delay(4000);
  }
}

//////////////////NODE CALLS///////////////////

void checkConnection(){
  WiFiClientSecure client;
  client.setInsecure();
  const char* lnbitsserver = lnbitsServer.c_str();
  const char* invoicekey = invoiceKey.c_str();
  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }
}

void checkBalance(){
  WiFiClientSecure client;
  client.setInsecure();
  const char* lnbitsserver = lnbitsServer.c_str();
  const char* invoicekey = invoiceKey.c_str();
  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }

  String url = "/api/v1/wallet";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                "Host: " + lnbitsserver + "\r\n" +
                "X-Api-Key: "+ invoicekey +" \r\n" +
                "User-Agent: ESP32\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");
   while (client.connected()) {
   String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line == "\r") {
      break;
    }
  }
  String line = client.readString();
  Serial.println(line);
  StaticJsonDocument<500> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  int charBalance = doc["balance"];
  balance = charBalance;
}


void getInvoice() {
  WiFiClientSecure client;
  client.setInsecure();
  const char* lnbitsserver = lnbitsServer.c_str();
  const char* invoicekey = invoiceKey.c_str();
  const char* lnbitsamount = amount.c_str();
  const char* lnbitsdescription = description.c_str();

  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }

  String topost = "{\"out\": false,\"amount\" : " + String(lnbitsamount) + ", \"memo\" :\""+ String(lnbitsdescription) + String(random(1,1000)) + "\"}";
  String url = "/api/v1/payments";
  client.print(String("POST ") + url +" HTTP/1.1\r\n" +
                "Host: " + lnbitsserver + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key: "+ invoicekey +" \r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + topost.length() + "\r\n" +
                "\r\n" + 
                topost + "\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readString();

  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  const char* payment_hash = doc["checking_id"];
  const char* payment_request = doc["payment_request"];
  payReq = payment_request;
  dataId = payment_hash;
}


void checkInvoice(){
  WiFiClientSecure client;
  client.setInsecure();
  const char* lnbitsserver = lnbitsServer.c_str();
  const char* invoicekey = invoiceKey.c_str();
  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }

  String url = "/api/v1/payments/";
  client.print(String("GET ") + url + dataId +" HTTP/1.1\r\n" +
                "Host: " + lnbitsserver + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");
   while (client.connected()) {
   String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line == "\r") {
      break;
    }
  }
  String line = client.readString();
  Serial.println(line);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  bool charPaid = doc["paid"];
  paid = charPaid;
}
