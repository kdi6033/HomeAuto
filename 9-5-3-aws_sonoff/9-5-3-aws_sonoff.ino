// Example of the different modes of the X.509 validation options
// in the WiFiClientBearSSL object
//
// Jul 2019 by Taejun Kim at www.kist.ac.kr

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>

//json을 위한 설정
StaticJsonDocument<200> doc;
DeserializationError error;
JsonObject root;

const char *ssid = "***";  // 와이파이 이름
const char *pass = "***";      // 와이파이 비밀번호
const char *thingId = "abc";          // 사물 이름 (thing ID) 
const char *host = "***.amazonaws.com"; // AWS IoT Core 주소
const char* outTopic = "outTopic"; 
const char* inTopic = "inTopic"; 

String sIP;
String sChipID;
char cChipID[20];
int gpio13Led = 13;
int gpio12Relay = 12;


// 사물 인증서 (파일 이름: xxxxxxxxxx-certificate.pem.crt)
const char cert_str[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWjCCAkKgAwIBAgIVAOQ3aNyDDRz1urVllhcEPX7S8wJeMA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xOTA4MDIwNTI5
MTRaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC7F1eU1Vtab4MHXzlx
oH0DJelGYVFgDIja/d1VOb9vP/kjUKwBTINsJuccFGyXC1NFK23sVwHqwgPyyPl3
6ssUYMsoHtyZ3D20Rw5LhDV3QfFK6BfI9oCKOfNdmzEEMCg9OkOPmtxcpjuNCF18
-----END CERTIFICATE-----
)EOF";
// 사물 인증서 프라이빗 키 (파일 이름: xxxxxxxxxx-private.pem.key)
const char key_str[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAuxdXlNVbWm+DB185caB9AyXpRmFRYAyI2v3dVTm/bz/5I1Cs
AUyDbCbnHBRslwtTRStt7FcB6sID8sj5d+rLFGDLKB7cmdw9tEcOS4Q1d0HxSugX
yPaAijnzXZsxBDAoPTpDj5rcXKY7jQhdfLZqWPBbNqz/JBlrX9WD0Cz0JfqroaBQ
5LsZFpBa0vytSYS+EheyjLkOwQB8+h2Y7bCi1lZIOIeOh14EcJ1tjvxCaOOlOksm
H4mZJi1XWc6azPDJLduf7zcX36s8+xT2z/r2CFOfJKMjk/DxkkY8cp7bhW25vKyQ
YrcBrtf+yKkapG948qnF9x6jUA9XU0o26125SwIDAQABAoIBAARm6jKgSoP4N7cG
sI1R318hlzmGtKlz4gx1CK4mq7Bsauo/zaxCJp121N0+RcfQBmeMPAvhiDQD2J/v
xp7hsWGLXXxWLY6ZNgJ14Yo5VCC4NnsytsyNsDyQXH+JVT/p+ihmpIxOcnzjlGcf
GUQD7sCk9yB0NZSd3H7mwTE2vY/fKbowRxSDBjpfBo07qIRu8s+1yBYB/qjeMk28
Ckt/zd80AfUcxcmK98hIvYucCdbQuB9DAdf/Ii7ogZ5549RujF859j+ZAkDrUDVM
ObmrGItBX9m1Bo3MKJpSYUkvt2WyGMLduMMN3/rANbzvlWMEww1NfBChQa3xAszn
j/wSeDECgYEA2t5MOGR6xJVMCKeijr2PaRtorVTX2QyRu0RntNvDQ1hN2hijVGBU
-----END RSA PRIVATE KEY-----

)EOF";
// Amazon Trust Services(ATS) 엔드포인트 CA 인증서 (서버인증 > "RSA 2048비트 키: Amazon Root CA 1" 다운로드)
const char ca_str[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
-----END CERTIFICATE-----
)EOF";

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

  const char* s = root["chip"];
  Serial.println("=================");
  Serial.println(s);
  if(strcmp(s,cChipID)==0) {
    int value = root["on"];
    Serial.println(value);
    if(value==1) {
      digitalWrite(gpio13Led, HIGH);
      digitalWrite(gpio12Relay, HIGH);
    }
    else {
      digitalWrite(gpio13Led, LOW);
      digitalWrite(gpio12Relay, LOW);
    }
  }
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

void setup() {
  pinMode(gpio13Led, OUTPUT);
  pinMode(gpio12Relay, OUTPUT);
  digitalWrite(gpio13Led, HIGH);
  digitalWrite(gpio12Relay, HIGH);
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println();

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  sIP=toStringIp(WiFi.localIP());
  Serial.println(sIP);
  
  //이름 자동으로 생성
  uint8_t chipid[6]="";
  WiFi.macAddress(chipid);
  sprintf(cChipID,"%02x%02x%02x%02x%02x%02x%c",chipid[5], chipid[4], chipid[3], chipid[2], chipid[1], chipid[0],0);
  sChipID=String(cChipID);
  Serial.println(sChipID);

  wifiClient.setTrustAnchors(&ca);
  wifiClient.setClientRSACert(&cert, &key);
  Serial.println("Certifications and key are set");

  setClock();
  //client.setServer(host, 8883);
  client.setCallback(callback);
}

long lastMsg = 0;
char msg[50];
int value = 0;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld",value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    Serial.println(sIP);
    Serial.println(sChipID);
    client.publish(outTopic, msg);
    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
  }
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
