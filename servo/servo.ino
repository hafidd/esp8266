#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <Servo.h>


ESP8266WebServer server(8877);
Servo servo;




const char* ssid = "esia hidayah 2";
const char* password = "123443212";

int servoAngle = 180;


void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  initWiFi();
  initServer();
  initFiles();

  servo.attach(D2, 500, 2500);
  servo.write(servoAngle);
}


void loop() {
  server.handleClient();
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Serial.print("Connecting to WiFi ..");
  delay(3000);
  while (WiFi.status() != WL_CONNECTED) {
    // Serial.print('.');
    delay(1000);
  }
  // Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Serial.print("RSSI: ");
  // Serial.println(WiFi.RSSI());
}

struct RelaySchedule {
  String time;
  int relayIndex;
  int status;
};

RelaySchedule relaySchedules[20] = {};
int relaySchedulesLength = sizeof(relaySchedules) / sizeof(relaySchedules[0]);


void initServer() {
  server.on("/", []() {
    blinkBuiltinLED();
    server.send(200, "text/plain", "hello ");
  });

  server.on("/test-deserialize", []() {
    blinkBuiltinLED();
    String json = readFileAsString("/data.json");

    DynamicJsonDocument doc(2048);
    deserializeJson(doc, json);

    int i = 0;
    for (JsonObject item : doc.as<JsonArray>()) {
      RelaySchedule sc = { item["time"], item["relayIndex"], item["status"] };
      relaySchedules[i] = sc;
      i++;
    }


    server.send(200, "text/plain", "hmm");
  });

  server.on("/test-read-struct", []() {
    blinkBuiltinLED();

    StaticJsonDocument<2048> doc;
    JsonArray array = doc.to<JsonArray>();

    for (int x = 0; x < relaySchedulesLength; x++) {
      JsonObject data = array.createNestedObject();
      data["time"] = relaySchedules[x].time;
      data["relayIndex"] = relaySchedules[x].relayIndex;
      data["status"] = relaySchedules[x].status;
    }

    String test = "";
    serializeJson(array, test);
    //writeFile("/data.json", test.c_str());
    server.send(200, "text/plain", test);
  });

  server.on("/add-schedule", []() {
    blinkBuiltinLED();

    // params
    String time = server.arg("time");
    int relayIndex = server.arg("relay").toInt();
    int status = server.arg("status").toInt();

    // check
    if (iskScheduleExist(time, relayIndex)) {
      server.send(400, "text/plain", "Schedule exist!");
      return;
    }

    // add
    if (!addRelaySchedule(time, relayIndex, status)) {
      server.send(400, "text/plain", "Full boss!");
      return;
    }

    // serialize to json
    StaticJsonDocument<2048> doc;
    JsonArray array = doc.to<JsonArray>();
    for (int x = 0; x < relaySchedulesLength; x++) {
      JsonObject data = array.createNestedObject();
      data["time"] = relaySchedules[x].time;
      data["relayIndex"] = relaySchedules[x].relayIndex;
      data["status"] = relaySchedules[x].status;
    }
    String test = "";
    serializeJson(array, test);

    // save
    writeFile("/data.json", test.c_str());

    // response
    server.send(200, "text/plain", test);
  });

  server.on("/remove-schedule", []() {
    blinkBuiltinLED();

    // params
    String time = server.arg("time");
    int relayIndex = server.arg("relay").toInt();

    // remove
    removeRelaySchedule(time, relayIndex);

    // // serialize to json
    // StaticJsonDocument<2048> doc;
    // JsonArray array = doc.to<JsonArray>();
    // for (int x = 0; x < relaySchedulesLength; x++) {
    //   JsonObject data = array.createNestedObject();
    //   data["time"] = relaySchedules[x].time;
    //   data["relayIndex"] = relaySchedules[x].relayIndex;
    //   data["status"] = relaySchedules[x].status;
    // }
    // String test = "";
    // serializeJson(array, test);

    // // save
    // writeFile("/data.json", test.c_str());

    // // response
    // server.send(200, "text/plain", test);

    server.send(200, "text/plain", "ok");
  });

  server.on("/servo", []() {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);

    int rot = server.arg("r").toInt();
    servo.write(rot);

    server.send(200, "text/plain", String(rot));
  });

  server.begin();
  Serial.println("server up");
}

void blinkBuiltinLED() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

void initFiles() {
  //LittleFS.format();
  Serial.println("Mount LittleFS");
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  //writeFile("/data.json", "{\"test\": \"hellow\"}");
}

String readFileAsString(const char* path) {
  //Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }

  Serial.print("Read from file: ");
  if (file.available()) {
    String str = file.readString();
    file.close();
    return str;
    // Serial.println(str);
  }
  // while (file.available()) {
  // }
  file.close();
  return "";
}

void writeFile(const char* path, const char* message) {
  Serial.printf("Writing file: %s\n", path);

  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  delay(2000);  // Make sure the CREATE and LASTWRITE times are different
  file.close();
}

bool iskScheduleExist(String time, int relayIndex) {
  bool isExist = false;

  for (int i = 0; i < relaySchedulesLength; i++) {
    if (relaySchedules[i].time == time && relaySchedules[i].relayIndex == relayIndex) {
      isExist = true;
      break;
    }
  }

  return isExist;
}

bool addRelaySchedule(String time, int relayIndex, int status) {
  bool isFull = true;

  Serial.println(relaySchedulesLength);

  for (int i = 0; i < relaySchedulesLength; i++) {
    if (relaySchedules[i].time == "") {
      relaySchedules[i].time = time;
      relaySchedules[i].relayIndex = relayIndex;
      relaySchedules[i].status = status;
      isFull = false;
      break;
    }
  }

  if (isFull) {
    return false;
  }

  return true;
}

void removeRelaySchedule(String time, int relayIndex) {
  for (int i = 0; i < relaySchedulesLength; i++) {
    if (relaySchedules[i].time == time && relaySchedules[i].relayIndex == relayIndex) {
      relaySchedules[i].time = "";
      relaySchedules[i].relayIndex = 0;
      relaySchedules[i].status = 0;
      break;
    }
  }
}
