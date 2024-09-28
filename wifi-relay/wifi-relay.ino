#define DECODE_HASH

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <IRremote.h>
#include <DS1307ESP.h>

#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define IR_RECV D4
IRrecv irrecv(IR_RECV);
decode_results results;

#define LDR_SENSOR D3

uint32_t lastTime;

const char* ssid = "esia hidayah 2";
const char* password = "123443212";

const byte relays[] = { D5, D6, D7, D0 };
int relayStatus[] = { 0, 0, 0, 0 };
bool relay3AutoMode = false;

ESP8266WebServer server(8877);

DS1307ESP rtc;

struct RelaySchedule {
  String time;
  int relayIndex;
  int status;
};

const int relaySchedulesLength = 20;
RelaySchedule relaySchedules[relaySchedulesLength] = {};

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //Serial.print("Connecting to WiFi ..");
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.print('.');
    delay(1000);
  }
  //Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  //Serial.print("RSSI: ");
  //Serial.println(WiFi.RSSI());
}

void initServer() {
  server.on("/", []() {
    blinkBuiltinLED();

    rtc.DSread();
    int ldr = digitalRead(LDR_SENSOR);

    String relays =
      "<br>Relay Status: "
      + String(relayStatus[0]) + ","
      + String(relayStatus[1]) + ","
      + String(relayStatus[2]) + ","
      + String(relayStatus[3]);

    String msg =
      "hello <br>"
      + rtc.getDate(true) + " " + rtc.getTime(true)
      + "<br>LDR: " + String(ldr)
      + relays
      + "<br>Relay-3 LDR mode: " + relay3AutoMode;

    server.send(200, "text/html", msg);
  });

  server.on("/save-states", []() {
    setDefaultStates();
    server.send(200, "text/plain", "ok");
  });

  server.on("/test-read", []() {
    server.send(200, "text/plain", readFileAsString("/states.json"));
  });

  server.on("/set-time", []() {
    int H = server.arg("h").toInt();
    int i = server.arg("i").toInt();
    int s = server.arg("s").toInt();
    int Y = server.arg("y").toInt();
    int m = server.arg("m").toInt();
    int d = server.arg("d").toInt();
    rtc.DSadjust(H, i, s, Y, m, d);

    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);

    server.send(200, "text/plain", "set time");
  });

  server.on("/toggle-relay", []() {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);

    String n = server.arg("n");

    if (n[0] == '\0') {
      return server.send(200, "text/plain", "nothing happened");
    }

    int relayIdx = n.toInt();

    relayIdx = relayIdx - 1;
    if (relayIdx < 0 || relayIdx > 3) {
      return server.send(200, "text/plain", "wrong relay number");
    }

    toggleRelay(relayIdx);

    server.send(200, "text/plain", "ok");
  });

  server.on("/toggle-relay3auto", []() {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);

    String msg = "relay-3 LDR mode: ";
    if (relay3AutoMode) {
      relay3AutoMode = false;
      msg += " off";
    } else {
      relay3AutoMode = true;
      msg += " on";
    }
    server.send(200, "text/plain", msg);
  });

  server.on("/set-relays", []() {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);

    String n = server.arg("s");

    if (n[0] == '\0') {
      return server.send(200, "text/plain", "nothing happened");
    }

    int status = n.toInt();

    if (status == 1) {
      setRelaysStatus(true);
    } else {
      setRelaysStatus(false);
    }

    server.send(200, "text/plain", "ok");
  });

  server.on("/get-schedules", []() {
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

    String jsonData = saveScheduleData();

    // response
    server.send(200, "text/plain", jsonData);
  });

  server.on("/remove-schedule", []() {
    blinkBuiltinLED();

    // params
    String time = server.arg("time");
    int relayIndex = server.arg("relay").toInt();

    // remove
    removeRelaySchedule(time, relayIndex);

    String jsonData = saveScheduleData();

    server.send(200, "text/plain", jsonData);
  });


  server.begin();
  //Serial.println("server up");
}

void initRemote() {
  IrReceiver.begin(IR_RECV, DISABLE_LED_FEEDBACK);
  printActiveIRProtocols(&Serial);
}

void toggleRelay(int relayIdx) {
  if (relayStatus[relayIdx] == 0) {
    digitalWrite(relays[relayIdx], LOW);
    relayStatus[relayIdx] = 1;
  } else {
    digitalWrite(relays[relayIdx], HIGH);
    relayStatus[relayIdx] = 0;
  }
}

void setRelaysStatus(bool active) {
  for (int i; i < 4; i++) {
    if (active) {
      digitalWrite(relays[i], LOW);
      relayStatus[i] = 1;
    } else {
      digitalWrite(relays[i], HIGH);
      relayStatus[i] = 0;
    }
  }
}

void setRelayStatus(int relayIndex, bool active) {
  if (active) {
    digitalWrite(relays[relayIndex], LOW);
    relayStatus[relayIndex] = 1;
  } else {
    digitalWrite(relays[relayIndex], HIGH);
    relayStatus[relayIndex] = 0;
  }
}

void setup() {
  //Serial.begin(9600);
  delay(500);

  rtc.begin();
  //rtc.DSadjust(20, 50, 10, 2023, 9, 21);  // 00:19:21 16 Mar 2022

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  for (int i = 0; i < 4; i++) {
    pinMode(relays[i], OUTPUT);
    digitalWrite(relays[i], HIGH);
  }

  initWiFi();
  initFiles();
  loadDefaultStates();
  loadTimers();
  initServer();
  initRemote();

  delay(2000);
}


void handleRemote() {
  if (irrecv.decode(&results)) {
    int value = results.value;
    //Serial.println(results.value, HEX);

    if (value == 0xE318261B) {
      //Serial.println("key 1");
      toggleRelay(0);
    } else if (value == 0X511DBB) {
      //Serial.println("key 2");
      toggleRelay(1);
    } else if (value == 0xEE886D7F) {
      //Serial.println("key 3");
      toggleRelay(2);
    } else if (value == 1386468383) {
      //Serial.println("key 4");
      toggleRelay(3);
    } else if (value == 0xD7E84B1B) {
      //Serial.println("key 5");
    } else if (value == 553536955) {
      //Serial.println("key 6");
    } else if (value == 0xF076C13B) {
      //Serial.println("key 7");
    } else if (value == 0xA3C8EDDB) {
      //Serial.println("key 8");
    } else if (value == 0xE5CFBD7F) {
      //Serial.println("key 9");
    } else if (value == 0x97483BFB) {
      //Serial.println("key 0");
    } else if (value == 1033561079) {
      //Serial.println("key up, turn on all relays");
      setRelaysStatus(true);
    } else if (value == 465573243) {
      //Serial.println("key down, turn off all relays");
      setRelaysStatus(false);
    } else if (value == 0x8C22657B) {
      //Serial.println("key left");
    } else if (value == 0x449E79F) {
      //Serial.println("key right");
    } else if (value == 0x488F3CBB) {
      //Serial.println("key ok");
    } else if (value == 0xC101E57B) {
      //Serial.println("*, save states");
      setDefaultStates();
    } else if (value == 0xF0C41643) {
      //Serial.println("#, , toggle relay 3 LDR");
      relay3AutoMode = !relay3AutoMode;
    }

    delay(150);
    irrecv.resume();
  }
}

void loop() {
  server.handleClient();

  handleRemote();

  if (millis() - lastTime >= 1000) {
    setRelaysByTime();
    setRelay3ByLDR();
    lastTime = millis();
  }
}

String offTimers[] = {
  "01:30"
};

void setRelaysByTime() {
  rtc.DSread();
  String time = rtc.getHour() + ":" + rtc.getMinute();

  ////Serial.println("Time Now: " + time);
  // //Serial.println(sizeof(offTimers));

  for (byte i = 0; i < relaySchedulesLength; i++) {
    ////Serial.println("Time Off: " + offTimers[i]);
    if (time == relaySchedules[i].time) {
      ////Serial.println("Turning relays off: ");
      boolean status = false;
      if (relaySchedules[i].status == 1) {
        status = true;
      }
      setRelayStatus(relaySchedules[i].relayIndex - 1, status);
    }
  }
}

void setRelay3ByLDR() {
  int ldr = digitalRead(LDR_SENSOR);

  if (!relay3AutoMode) {
    return;
  }

  // dark
  if (ldr == 1) {
    if (relayStatus[2] == 0) {
      setRelayStatus(2, true);
      relayStatus[2] == 1;
    }
  } else {
    if (relayStatus[2] == 1) {
      setRelayStatus(2, false);
      relayStatus[2] == 0;
    }
  }
}


void initFiles() {
  //LittleFS.format();
  //Serial.println("Mount LittleFS");
  if (!LittleFS.begin()) {
    //Serial.println("LittleFS mount failed");
    return;
  }
  if (!LittleFS.exists("/data.json")) {
    writeFile("/data.json", "{\"test\": \"hellow\"}");
  }
  if (!LittleFS.exists("/states.json")) {
    writeFile("/states.json", "{\"test\": \"hellow\"}");
  }
}

void loadTimers() {
  String timers = readFileAsString("/data.json");

  DynamicJsonDocument doc(2048);
  deserializeJson(doc, timers);

  int i = 0;
  for (JsonObject item : doc.as<JsonArray>()) {
    RelaySchedule sc = { item["time"], item["relayIndex"], item["status"] };
    relaySchedules[i] = sc;
    i++;
  }
}

void loadDefaultStates() {
  String states = readFileAsString("/states.json");

  DynamicJsonDocument doc(512);
  deserializeJson(doc, states);

  relay3AutoMode = doc["relay3AutoMode"];

  for (int i = 0; i < 4; i++) {
    bool active = true;
    if (doc["relayStatus"][i] == 0) {
      active = false;
    }
    setRelayStatus(i, active);
  }

  //Serial.println(states);
}

void setDefaultStates() {
  StaticJsonDocument<512> doc;
  doc["relay3AutoMode"] = relay3AutoMode;
  JsonArray relayStat = doc.createNestedArray("relayStatus");
  relayStat.add(relayStatus[0]);
  relayStat.add(relayStatus[1]);
  relayStat.add(relayStatus[2]);
  relayStat.add(relayStatus[3]);

  // save
  String jsonStr = "";
  serializeJson(doc, jsonStr);
  //Serial.println(jsonStr);
  writeFile("/states.json", jsonStr.c_str());
}

void blinkBuiltinLED() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

String readFileAsString(const char* path) {
  ////Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    //Serial.println("Failed to open file for reading");
    return "";
  }

  //Serial.print("Read from file: ");
  if (file.available()) {
    String str = file.readString();
    file.close();
    return str;
    // //Serial.println(str);
  }
  // while (file.available()) {
  // }
  file.close();
  return "";
}

void writeFile(const char* path, const char* message) {
  //Serial.printf("Writing file: %s\n", path);

  File file = LittleFS.open(path, "w");
  if (!file) {
    //Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    //Serial.println("File written");
  } else {
    //Serial.println("Write failed");
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

String saveScheduleData() {
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

    return test;
}

bool addRelaySchedule(String time, int relayIndex, int status) {
  bool isFull = true;

  //Serial.println(relaySchedulesLength);

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
