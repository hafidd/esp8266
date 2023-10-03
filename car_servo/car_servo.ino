#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

#define ENA D0
#define IN1 D1
#define IN2 D2

#define IN3 D5
#define IN4 D6

#define SERVO D5
#define SERVO2 D6

ESP8266WebServer server(8877);
Servo servo;

const char* ssid = "esia hidayah 2";
const char* password = "123443212";

const int carStraightAngle = 90;

int servoAngle = carStraightAngle;
int speed = 0;  // -255 - 255

int pwmFreq = 3500;

void setup() {
  setPinsMode();
  initServo();
  initWiFi();
  initServer();

  analogWriteFreq(pwmFreq);

  setMotorEnabled(true);
  stop(0);
  stop(1);
}

void loop() {
  server.handleClient();
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

void initServer() {
  server.on("/", []() {
    blinkBuiltinLED();
    server.send(200, "text/plain", "hello ");
  });

  server.on("/freq", []() {
    blinkBuiltinLED();

    int f = server.arg("f").toInt();

    // Valid values are from 100Hz to 40000Hz.
    if (f < 100 || f > 40000) {
      server.send(200, "text/plain", "Valid values are from 100Hz to 40000Hz");
      return;
    }

    pwmFreq = f;
    analogWriteFreq(pwmFreq);

    server.send(200, "text/plain", "PWM freq set to: " + String(pwmFreq));
  });

  server.on("/get-freq", []() {
    blinkBuiltinLED();

    server.send(200, "text/plain", "PWM freq: " + String(pwmFreq));
  });

  server.on("/move", []() {
    blinkBuiltinLED();
    int speed = server.arg("speed").toInt();
    int idx = server.arg("idx").toInt();

    String direction = "forward";

    if (speed > 0) {
      moveForward(speed, idx);
    } else if (speed < 0) {
      direction = "reverse";
      moveReverse(speed, idx);
    } else {
      direction = "";
      stop(idx);
    }

    server.send(200, "text/plain", String("{\"dr\":" + direction + "\"spd\":" + String(abs(speed)) + "}"));
  });

  server.on("/left", []() {
    toLeft();
    server.send(200, "text/plain", String(servoAngle));
  });

  server.on("/right", []() {
    toRight();
    server.send(200, "text/plain", String(servoAngle));
  });

  server.on("/turn", []() {
    int deg = server.arg("d").toInt();
    setServo(deg);
    server.send(200, "text/plain", String(servoAngle));
  });

  server.enableCORS(true);
  server.begin();
  //Serial.println("server up");
}

void setPinsMode() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
}

void initServo() {
  servo.attach(SERVO);
  servo.write(servoAngle);
}

void setServo(int deg) {
  servo.write(deg);
  servoAngle = deg;
}

void moveForward(int speed, int idx) {
  if (idx == 0) {
    digitalWrite(IN1, LOW);
    analogWrite(IN2, speed);
    return;
  }

  // digitalWrite(IN3, LOW);
  // analogWrite(IN4, speed);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void moveReverse(int speed, int idx) {
  if (idx == 0) {
    analogWrite(IN1, -speed);
    digitalWrite(IN2, LOW);
    return;
  }
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  // analogWrite(IN1, -speed);
  // digitalWrite(IN2, LOW);
}

void setMotorEnabled(bool enabled) {
  if (enabled) {
    return digitalWrite(ENA, HIGH);
  }
  digitalWrite(ENA, LOW);
}

void stop(int idx) {
  if (idx == 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    return;
  }

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
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

void blinkBuiltinLED() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}
