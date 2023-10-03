#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <Servo.h>

#define ENA D7
#define IN1 D1
#define IN2 D2

// #define SERVO D0

ESP8266WebServer server(8877);
Servo servo;

const char* ssid = "esia hidayah 2";
const char* password = "123443212";

int speed = 0;
int prevSpeed = 0;
bool forward = true;
bool prevForward = true;

int servoAngle = 90;

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  setDirectionAndSpeed(true, 0);

  servo.attach(D4, 500, 2500);
  servo.write(servoAngle);

  initWiFi();
  initServer();
}

void loop() {
  server.handleClient();
  //handleMove();


  //delay(200);
}

void initServer() {
  server.on("/", []() {
    blinkBuiltinLED();
    server.send(200, "text/plain", "hello ");
  });

  server.on("/move", []() {
    blinkBuiltinLED();
    int speed = server.arg("speed").toInt();
    String direction = server.arg("direction");
    bool forward = direction == "f";
    setDirectionAndSpeed(forward, speed);

    server.send(200, "text/plain", String(speed));
  });

  server.on("/left", []() {
    toLeft();
    server.send(200, "text/plain", String(servoAngle));
  });

  server.on("/right", []() {
    toRight();
    server.send(200, "text/plain", String(servoAngle));
  });

  server.begin();
  //Serial.println("server up");
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // //Serial.print("Connecting to WiFi ..");
  delay(3000);
  while (WiFi.status() != WL_CONNECTED) {
    // //Serial.print('.');
    delay(1000);
  }
  // //Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // //Serial.print("RSSI: ");
  // //Serial.println(WiFi.RSSI());
}

void handleMove() {
  if (speed == prevSpeed && forward == prevForward) {
    return;
  }

  prevSpeed = speed;
  prevForward = forward;

  if (forward) {
    motorForward();
  } else {
    motorReverse();
  }

  if (speed == 0) {
    motorStop();
  }
}

void motorStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

void motorForward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}

void motorReverse() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void toRight() {
  int newAngle = servoAngle + 10;
  if (newAngle <= 140) {
    servoAngle = newAngle;
    servo.write(servoAngle);
  }
}

void toLeft() {
  int newAngle = servoAngle - 10;
  if (newAngle >= 40) {
    servoAngle = newAngle;
    servo.write(servoAngle);
  }
}

void resetServo() {
  servoAngle = 90;
}

void setDirectionAndSpeed(bool isForward, int speedValue) {
  // 0-255
  speed = speedValue;
  forward = isForward;

  if (speed == 0) {
    analogWrite(ENA, 0);
    motorStop();
    return;
  }

  if (isForward) {
    motorForward();
  } else {
    motorReverse();
  }

  if (isForward != forward) {
    for (int i = 0; i <= speed; i++) {
      analogWrite(ENA, i);
      delay(20);
    }
  } else if (speed > prevSpeed) {
    for (int i = prevSpeed; i <= speed; i++) {
      analogWrite(ENA, i);
      delay(20);
    }
  } else {
    for (int i = prevSpeed; i >= speed; --i) {
      analogWrite(ENA, i);
      delay(20);
    }
  }
}

void blinkBuiltinLED() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}
