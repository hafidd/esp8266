#include <ESP8266WiFi.h>
#include <Servo.h>
#include "DHT.h"

/*
#define DHTPIN D5
#define DHTTYPE DHT11
#define LDRPIN D1
#define LEDPIN D0
#define SERVO D6
*/

// #define RELAY1 D0
// #define RELAY2 D1
// #define RELAY3 D2
// #define RELAY4 D3


//DHT dht(DHTPIN, DHTTYPE);

//Servo servo;

const char* ssid = "esia hidayah 2";
const char* password = "123443212";

unsigned long previousMillis = 0;
unsigned long interval = 30000;

const byte relays[] = { D0, D1, D2, D3 };

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  delay(3000);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

// typedef struct kv {
//    int key;
//    char value[NAMELENMAX+1];
// } kv;

//int relayStatus[4] = { 1, 0, 0, 0 };



setup() {
  Serial.begin(9600);
  //dht.begin();

  //servo.attach(SERVO);
  //servo.write(0);

  //pinMode(LEDPIN, OUTPUT);
  pinMode(RELAY0, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  initWiFi();

  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  delay(2000);
}


void loop() {
  delay(1000);

  /*
  int activeRelayIdx = 0;
  for (int i = 0; i < 4; i++) {
    byte status = HIGH;
    if (i == activaRelayIndex) {
      status = LOW;
    }

    digitalWrite(relays[i], status);

    activeRelayIdx++;
  }
  */

  /*
  int newServoRot = servoRot + 10;
  if (newServoRot > 180) {
    newServoRot = 0;
  }
  servoRot = newServoRot;
  servo.write(servoRot);
  */

  /*
  int ldrValue = digitalRead(LDRPIN);
  Serial.print("LDR value: ");
  Serial.println(ldrValue);
  if (ldrValue == 1) {
    digitalWrite(LEDPIN, HIGH);
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY3, LOW);
    servo.write(180);
  } else {
    digitalWrite(LEDPIN, LOW);
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY3, HIGH);
    servo.write(0);
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  */

  // Compute heat index in Fahrenheit (the default)
  //float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  //float hic = dht.computeHeatIndex(t, h, false);

  /*
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
  */
}
