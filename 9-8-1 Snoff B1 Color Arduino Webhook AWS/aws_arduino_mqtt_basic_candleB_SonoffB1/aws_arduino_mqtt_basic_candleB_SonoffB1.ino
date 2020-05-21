#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <my92xx.h> //https://github.com/xoseperez/my92xx
#define TRIGGER_PIN 0 // trigger pin 0(D3) 2(D4)
#define MY92XX_MODEL    MY92XX_MODEL_MY9231     // The MY9291 is a 4-channel driver, usually for RGBW lights
#define MY92XX_CHIPS    2                       // No daisy-chain
#define MY92XX_DI_PIN   12                      // DI GPIO
#define MY92XX_DCKI_PIN 14                      // DCKI GPIO
#define UNIVERSE 1                              // First DMX Universe to listen for
#define UNIVERSE_COUNT 1                        // Total number of Universes to listen for, starting at UNIVERSE

my92xx _my92xx = my92xx(MY92XX_MODEL, MY92XX_CHIPS, MY92XX_DI_PIN, MY92XX_DCKI_PIN, MY92XX_COMMAND_DEFAULT);

int Bright=0,R=0,G=0,B=0,W=0; // Bringht Red Green Blue
unsigned long lastConnectRGB=0;
int i=0;

ESP8266WebServer server(80);

//json을 위한 설정
StaticJsonDocument<200> doc;
DeserializationError error;
JsonObject root;

// AP 설정을 위한 변수
const char *softAP_ssid = "candle-";
const char *softAP_password = "";
char ssid[32] = "";
char password[32] = "";
/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

/* AP 설정을 위한 변수 선언 */
String sAP_ssid,sChipID;
char cAP_ssid[20];
char cChipID[20]; // Json에서 chipid를 고유 ID로 사용

boolean bConnect=0;
unsigned long lastConnectTry = 0;
const int led = 2;
int bootMode=0; //0:station  1:AP

//mqtt 통신 변수
const char *thingId = "";          // 사물 이름 (thing ID) 
const char *host = "***-ats.iot.us-east-2.amazonaws.com"; // AWS IoT Core 주소
const char* outTopic = "outCandleAWS"; // 사용자가 결정해서 기록
const char* inTopic = "inCandleAWS"; // 사용자가 결정해서 기록
long now;
int sA0 = A0; // Analog Input
int nowBlindState=0; //현재 브라인드 상태

long lastMsg = 0;
char msg[100];
int value = 0;

int iOn=0,Auto=0,Dust=0; //mqtt로 들어오는 데이터
String inputString;

// 사물 인증서 (파일 이름: xxxxxxxxxx-certificate.pem.crt)
const char cert_str[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
VR0OBBYEFLnGFWy5cKgJMTMB3zlKLWJqcEXEMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQA0PFRmJVc4f93cZTil+m79AvPO
6fly4ldYqBrg1nGsNo/F50RetNwwXzme2aI2vn5Rb/nKYu/Gk1glfTlvBDhwBDco
swsLvG2h5RG9nISSev9OxWcTOj6yvX6InwS98LzZEwdB4JC+rZIcBmGnbc8DWWzS
bEpQ7sJIaPv8U309RdeMl1SgKeZFOgo3QCY6YotDlL1HGzHuNcXdbQM2tM0wdsde
tje/0xy06qSlG5rYfERj0aoAp8x7tr/+oLKyuLKzmhuEYtj96SWLk3krLC3hShhb
sxHk9zImZf7oa54vdN2meJ/pr+jkfNqKap2vKGKarU61444jUsdznDxGsN7H
-----END CERTIFICATE-----
)EOF";
// 사물 인증서 프라이빗 키 (파일 이름: xxxxxxxxxx-private.pem.key)
const char key_str[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
8NgoCgq/yEfo9+jwmQWx53fE0S+REg+A/opBi1riDCkqoQTTJtnpYKyfTpKZ2k1l
KduoAib3GX1nsTF5vsLLBb3APDXdy4USvhrv0fleu/s1DMSmHY5iKDcCgYEA8wzF
cPwqGGf+u1PglLe1/9vCOryG2LaYbKpALUOkTUwsIzhfkCdbu8x04kA7ZZYCtPdq
uhBPB7MjZSeWF//ZBuWe5F2+EtFDbWNOzO9ll/QrKzwEVPG4hj+eGbrAhw54Z/7d
qTzRifeXzqTwU6BqddxvFqHeWjmqwsMTrG1nCYcCgYEAhRIs5aulDAW+D8FUMVN+
zmxYh8h5XEWryBg/px0g06ZxJJxPRX0ho6/mJf24fEuLkgg3pKE6X9g1KqBknSuy
3aKv+YrFSFILkqO9fmUZOmLEHKz0918H1j+Gt6dNg8e9Nvy9CERxDR5GYJsqQfpA
lzxOsFGP8WbL14V1VB5wWR8CgYAs0nAe9AH3WkZZ2ZATHQYNV0OsfVQI5zOY5pTL
RCwqrR9+p0jIVtnN+lib2OibRVzebrpZ8eQBMYIXh4NgjahCY1o4FymUYs8ifyvr
E0MTEM5dPMY3vBQhfd30NMKIpZyC4TeTnEmwPd7bFwPTCERZ0/sQm21cCkJ5hGw+
YjacRQKBgEZdGoWk3vsM4PDHL79MWYo+SDfprtTu2OcdBomYc5s686MdWWAgEVkc
YfKAqPdJidXneToYE/D5ZPiXKd5BFxDYvQrVnqAb2jMh33r+NoBTm4JRI+Uub1Dl
Pb5jHHHdSihPXRtO6eXRGhuoBzI22PYF2VcZaOeuzxZsXWTqLNmH
-----END RSA PRIVATE KEY-----
)EOF";
// Amazon Trust Services(ATS) 엔드포인트 CA 인증서 (서버인증 > "RSA 2048비트 키: Amazon Root CA 1" 다운로드)
const char ca_str[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  digitalWrite(led, 1);
  //Serial.setDebugOutput(true);
  
  // AP 이름 자동으로 만듬
  sChipID=String(ESP.getChipId(),HEX);
  sChipID.toCharArray(cChipID,sChipID.length()+1);
  sAP_ssid=String(softAP_ssid)+String(ESP.getChipId(),HEX);
  sAP_ssid.toCharArray(cAP_ssid,sAP_ssid.length()+1);
  softAP_ssid=&cAP_ssid[0];
  thingId=&sChipID[0]; //mqtt 통신에서 사용
  Serial.println("chipID:");
  Serial.println(thingId);
  loadCredentials();
  
  if(ssid[0]==0)
    setupAp();
  else
    connectWifi();
  
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.on("/scan", handleScan);
  server.on("/auto", handleAuto);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  deserializeJson(doc,payload);
  root = doc.as<JsonObject>();
  const char* rChip = root["chip"];
    Bright = root["Bright"];
    R = root["R"];
    G = root["G"];
    B = root["B"];
    W = Bright;
  snprintf (msg, 100, "{%s %d %d %d %d}",cChipID,Bright,R,G,B);
  Serial.println(msg);

  if(!rChip)
    return;

  //if(strcmp(rChip,cChipID)==0) {
  //}
}

void updateLights()
{
      _my92xx.setChannel(4, R);
      _my92xx.setChannel(3, G);
      _my92xx.setChannel(5, B); 
      _my92xx.setChannel(0, W); 
      _my92xx.setChannel(1, W); 
      _my92xx.setState(true);
      _my92xx.update();
}

X509List ca(ca_str);
X509List cert(cert_str);
PrivateKey key(key_str);
WiFiClientSecure wifiClient;
PubSubClient client(host, 8883, callback, wifiClient); //set  MQTT port number to 8883 as per //standard

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(thingId)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "hello world");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      wifiClient.getLastSSLError(buf,256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void setupAp() {
  bootMode=1;
  Serial.println("AP Mode Connecting...");
  Serial.println(softAP_ssid);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  digitalWrite(led, 0); //상태 led로 표시
}

void connectWifi() {
  bootMode=0;
  Serial.println("STATION Mode Connecting...");
  WiFi.disconnect();
  //WiFi.mode(WIFI_STA); //이 모드를 설정 않해야 AP가 살아있습니다.
  WiFi.begin(ssid, password);

  //referance: https://www.arduino.cc/en/Reference/WiFiStatus
  //WL_NO_SHIELD:255 WL_IDLE_STATUS:0 WL_NO_SSID_AVAIL:1 WL_SCAN_COMPLETED:2
  //WL_CONNECTED:3 WL_CONNECT_FAILED:4 WL_CONNECTION_LOST:5 WL_DISCONNECTED:6
  int connRes = WiFi.waitForConnectResult();
  Serial.print ( "connRes: " );
  Serial.println ( connRes );
  if(connRes == WL_CONNECTED){
    wifiClient.setTrustAnchors(&ca);
    wifiClient.setClientRSACert(&cert, &key);
    setClock();
    client.setCallback(callback);
    //Serial.println("Certifications and key are set");
    Serial.println("AWS connected");
    Serial.println(WiFi.localIP());
    // 상태 led로 표시
    for(int i=0;i<5;i++) {
      digitalWrite(led, 0);
      delay(500);
      digitalWrite(led, 1);
      delay(500);
    }
  }
  //else
  //  WiFi.disconnect();
}

void loop() {
  server.handleClient();
  
  unsigned int sWifi = WiFi.status();
  now = millis();
  updateLights();

  //6초에 한번 와이파이 끊기면 다시 연결
  if(sWifi == WL_CONNECTED)
    doMqtt();
  else if (sWifi == WL_IDLE_STATUS && now > (lastConnectTry + 50000) && strlen(ssid) > 0 ) {
    lastConnectTry=now;
    Serial.println ( "Connect requested" );
    connectWifi();
  }

  //Factory Reset
  if ( digitalRead(TRIGGER_PIN) == LOW ) 
    factoryDefault();
}


void doMqtt() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (now - lastMsg > 5000) {
    lastMsg = now;
    ++value;
    //snprintf (msg, 75, "hello world #%ld", value);
    //Serial.print("Publish message: ");
    //Serial.println(msg);
    //client.publish(outTopic, msg);
    //Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
  }
}
