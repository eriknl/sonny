/*
This file is part of sonny Copyright (C) 2017 Erik de Jong

sonny is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

sonny is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with sonny.  If not, see <http://www.gnu.org/licenses/>.
*/

#define SONOFF          1
#define SONOFF_DUAL     2
#define SONOFF_S20      3

#define SONOFF_DEVICE   SONOFF_S20

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
// https://github.com/adafruit/Adafruit_MQTT_Library/
#include <Adafruit_MQTT_Client.h>
#include <Adafruit_MQTT.h>
// https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
#include <ArduinoOTA.h>
// https://bblanchon.github.io/ArduinoJson/
#include <ArduinoJson.h>
#include "FS.h"

#include "sonny_types.h"

ESP8266WebServer server(80);
DynamicJsonBuffer jsonBuffer;
WiFiClient client;
Adafruit_MQTT_Client *mqtt;

/*
 * Add IO device to array and assign pin
 */
void addIoDevice(sonoffIO ** list, int8_t index, int8_t pin) {
  list[index] = (sonoffIO*)malloc(sizeof(sonoffIO));
  list[index]->pin = pin;
}

/*
 * Toggle an output
 */
void toggleOutput(int8_t index) {
  if (device.outputCount > index) {
    digitalWrite(device.outputs[index]->pin, !digitalRead(device.outputs[index]->pin));
  }
}

/*
 * Set reset and reboot
 */
void resetConfig(int8_t index) {
  saveSettings(true);
  ESP.restart();
}

/*
 * Add LED to array and assing pin
 */
void addLed(sonoffLED ** list, int8_t index, int8_t pin) {
  list[index] = (sonoffLED*)malloc(sizeof(sonoffLED));
  list[index]->pin = pin;
  list[index]->divisor = 0;
}

/*
 * Set LED state
 */
void setLedState(int8_t index, bool state) {
  if (device.ledCount > index) {
    device.leds[0]->divisor = 0;
    digitalWrite(device.leds[0]->pin, !state);
  }
}

/*
 * Set LED pattern
 */
void setLedPattern(int8_t index, int8_t divisor) {
  if (device.ledCount > index) {
    device.leds[0]->divisor = divisor;
  }
}

/*
 * Setup device specific IOs and create their pub/sub handlers
 */
void setupDevice() {
#if SONOFF_DEVICE == SONOFF
  Serial.println(F("Setup Sonoff"));
#elif SONOFF_DEVICE == SONOFF_DUAL
  Serial.println(F("Setup Sonoff dual"));
#elif SONOFF_DEVICE == SONOFF_S20
  Serial.println(F("Setup Sonoff S20"));
  device.inputCount = 1;
  device.outputCount = 1;
  device.ledCount = 1;
  device.inputs = (sonoffIO**)malloc(sizeof(sonoffIO*) * device.inputCount);
  addIoDevice(device.inputs, 0, 0);               // button
  device.inputs[0]->triggers[0] = toggleOutput;
  device.inputs[0]->triggers[3] = resetConfig;
  device.inputs[0]->triggerPublishState = 1;      // trigger on release
  device.outputs = (sonoffIO**)malloc(sizeof(sonoffIO) * device.outputCount);
  addIoDevice(device.outputs, 0, 12);             // relay
  device.leds = (sonoffLED**)malloc(sizeof(sonoffLED) * device.ledCount);
  addLed(device.leds, 0, 13);
#else
  Serial.println(F("Unknown devicetype"));
#endif
  Serial.println("Configuring IO");
  int8_t i;
  char *topic;
  const int topicSize = 32;
  for (i = 0; i < device.inputCount; i++) {
    pinMode(device.inputs[i]->pin, INPUT);
    device.inputs[i]->lastState = digitalRead(device.inputs[i]->pin);
    device.inputs[i]->lastStateTime = millis();
    topic = (char *)malloc(topicSize);
    snprintf(topic, topicSize, "sonoff/%s/input/%d", getSettingString(settingHostname), i);
    device.inputs[i]->mqttPublisher = Adafruit_MQTT_Publish(mqtt, topic);
    device.inputs[i]->publishTopic = strdup(topic);
  }
  for (i = 0; i < device.outputCount; i++) {
    pinMode(device.outputs[i]->pin, OUTPUT);
    device.outputs[i]->lastState = digitalRead(device.outputs[i]->pin);
    topic = (char *)malloc(topicSize);
    snprintf(topic, topicSize, "sonoff/%s/output/%d", getSettingString(settingHostname), i);
    device.outputs[i]->mqttPublisher = Adafruit_MQTT_Publish(mqtt, topic);
    device.outputs[i]->publishTopic = strdup(topic);
    topic = (char *)malloc(topicSize);
    snprintf(topic, topicSize, "sonoff/%s/switch/%d", getSettingString(settingHostname), i);
    device.outputs[i]->mqttSubscriber = Adafruit_MQTT_Subscribe(mqtt, topic);
    mqtt->subscribe(&device.outputs[i]->mqttSubscriber);
  }
  for (i = 0; i < device.ledCount; i++) {
    pinMode(device.leds[i]->pin, OUTPUT);
  }
}

/*
 * Callback for software timer used for blinking LEDs
 */
void timerCallback(void*) {
  int8_t i;
  device.ledCounter++;
  for (i = 0; i < device.ledCount; i++) {
    if (device.leds[i]->divisor) {
      if (!(device.ledCounter % device.leds[i]->divisor)) {
        digitalWrite(device.leds[i]->pin, !digitalRead(device.leds[i]->pin));
      }
    }
  }
  if (device.ledCounter > 50) {
    device.ledCounter = 0;
  }
}

sonoffSetting settings[settingLast];

/*
 * Add setting to settingsmanager
 * settingName and settingDescription should not be free()d after using them on the settingsmanager!
 */
void addSetting(sonoffSettingIndex index, sonoffSettingType settingType, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, uint8_t settingLength) {
  settings[index].settingName = settingName;
  settings[index].settingDescription = settingDescription;
  settings[index].settingValue = (uint8_t*)malloc(settingLength);
  settings[index].settingDefaultValue = (uint8_t*)malloc(settingLength);
  settings[index].settingLength = settingLength;
  settings[index].settingType = settingType;
  settings[index].visible = visible;
  if (index == 0) {
    settings[index].offset = 0;
  } else {
    settings[index].offset = settings[index-1].offset + settings[index-1].settingLength;
  }
}

/*
 * Add string setting to settingsmanager, wrapper for addSetting
 */
void inline addSettingString(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, const char *defaultValue, uint8_t settingLength) {
  addSetting(index, typeString, visible, settingName, settingDescription, settingLength);
  setSettingString(index, (char *)defaultValue);
  memcpy(settings[index].settingDefaultValue, settings[index].settingValue, settingLength);
}

/*
 * Add password setting to settingsmanager, wrapper for addSetting
 */
void inline addSettingPassword(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, char *defaultValue, uint8_t settingLength) {
  addSetting(index, typePassword, visible, settingName, settingDescription, settingLength);
  setSettingString(index, defaultValue);
  memcpy(settings[index].settingDefaultValue, settings[index].settingValue, settingLength);
}

/*
 * Add bool setting to settingsmanager, wrapper for addSetting
 */
void inline addSettingBool(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, bool defaultValue) {
  addSetting(index, typeBool, visible, settingName, settingDescription, 1);
  setSettingBool(index, defaultValue);
  memcpy(settings[index].settingDefaultValue, settings[index].settingValue, 1);
}

/*
 * Add integer setting to settingsmanager, wrapper for addSetting
 */
void inline addSettingInteger(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, int defaultValue) {
  addSetting(index, typeInteger, visible, settingName, settingDescription, 4);
  setSettingInteger(index, defaultValue);
  memcpy(settings[index].settingDefaultValue, settings[index].settingValue, 4);
}

/*
 * Get string value of setting
 */
char *getSettingString(sonoffSettingIndex setting) {
  return (char *)settings[setting].settingValue;
}

/*
 * Set string value of setting
 */
void setSettingString(sonoffSettingIndex setting, char *value) {
  strncpy((char *)settings[setting].settingValue, value, strlen(value));
}

/*
 * Get bool value of setting
 */
bool getSettingBool(sonoffSettingIndex setting) {
  return (settings[setting].settingValue[0] ? true : false);
}

/*
 * Set bool value of setting
 */
void setSettingBool(sonoffSettingIndex setting, bool value) {
  settings[setting].settingValue[0] = (value ? 0x01 : 0x00);
}

/*
 * Get integer value of setting
 */
int getSettingInteger(sonoffSettingIndex setting) {
  return *(int *)(settings[setting].settingValue);
}

/*
 * Set bool value of setting
 */
void setSettingInteger(sonoffSettingIndex setting, int value) {
  *(int *)(settings[setting].settingValue) = value;
}


/*
 * Attempt to restore known values from flash
 */
void restoreSettings() {
  int8_t i;
  SPIFFS.begin();
  File f = SPIFFS.open("/settings.dat", "r");
  if (!f) {
    Serial.println("file open failed");
    return;
  }

  Serial.println("Restoring settings from flash");
  for (i = 0; i < settingLast; i++) {
    f.readBytes((char *)settings[i].settingValue, settings[i].settingLength);
    Serial.print(settings[i].settingName);
    Serial.print("(");
    Serial.print(settings[i].settingLength, DEC);
    Serial.print(")");
    Serial.print(": ");
    switch (settings[i].settingType) {
      case typeString:
      case typePassword:
        Serial.println(getSettingString((sonoffSettingIndex)i));
      break;
      case typeBool:
        Serial.println(getSettingBool((sonoffSettingIndex)i) ? F("true") : F("false"));
      break;
      case typeInteger:
        Serial.println(getSettingInteger((sonoffSettingIndex)i), DEC);
      break;
    }
  }
  f.close();
}

/*
 * Save settings to flash
 */
bool saveSettings(bool defaultValue) {
  int8_t i;
  SPIFFS.begin();
  File f = SPIFFS.open("/settings.dat", "w");
  if (!f) {
    Serial.println("file open failed");
    return false;
  }
  
  Serial.println("Saving settings to flash");
  for (i = 0; i < settingLast; i++) {
    f.write((const uint8_t*)(defaultValue ? settings[i].settingDefaultValue : settings[i].settingValue), settings[i].settingLength);
  }
  f.close();
  return true;
}

/*
 * HTML node super class
 */
class HtmlNode {
public:
  HtmlNode(const __FlashStringHelper *type, String id);
  ~HtmlNode();
  String toString();
protected:
  void addTagAttribute(const __FlashStringHelper *tag, String value);
  void addTagAttribute(const __FlashStringHelper *tag, const __FlashStringHelper *value);
  void addCloseTag();
  const __FlashStringHelper *type;
  String id;
  String content;
};

/*
 * Constructor
 */
HtmlNode::HtmlNode(const __FlashStringHelper *type, String id) : type(type), id(id) {
  content += '<';
  content += type;
  content += F(" ");
  if (id.length() > 0) {
    addTagAttribute(F("id"), id);
  }
}

/*
 * Destructor
 */
HtmlNode::~HtmlNode() {
}

/*
 * Add tag attribute
 */
void HtmlNode::addTagAttribute(const __FlashStringHelper *tag, String value) {
  content += tag;
  content += "=\"";
  content += value;
  content += "\" ";
}

/*
 * Add tag attribute
 */
void HtmlNode::addTagAttribute(const __FlashStringHelper *tag, const __FlashStringHelper *value) {
  content += tag;
  content += "=\"";
  content += value;
  content += "\" ";
}

/*
 * Add a close tag
 */
void HtmlNode::addCloseTag() {
  content += "</";
  content += type;
  content += '>';
}

/*
 * Return HTML code
 */
String HtmlNode::toString() {
  return content;
}

/*
 * HTML table node class
 */
class HtmlTable : public HtmlNode {
public:
  HtmlTable(String id, int columnCount, const __FlashStringHelper ** columnNames);
  void addRow(const String *columnValues);
  void addRow(const std::initializer_list<String>& columnValues);
  String toString();
private:
  int columnCount;
};

/*
 * Constructor, sets up table header
 */
HtmlTable::HtmlTable(String id, int columnCount, const __FlashStringHelper ** columnNames) : HtmlNode(F("table"), id), columnCount(columnCount) {
  int i;
  content += F("\"><thead><tr>");
  for (i = 0; i < columnCount; i++) {
    content += F("<td>");;
    content += columnNames[i];
    content += F("</td>");
  }
  content += F("</tr></thead></tbody>");
}

/*
 * Add row to table with given values
 */
void HtmlTable::addRow(const String *columnValues) {
  int i;
  content += F("<tr>");
  for (i = 0; i < columnCount; i++) {
    content += F("<td>");
    content += columnValues[i];
    content += F("</td>");
  }
  content += F("</tr>");
}

/*
 * Plumbing to allow initializer lists for addRow function
 */
void HtmlTable::addRow(const std::initializer_list<String>& columnValues) {
  addRow(columnValues.begin());
}

/*
 * Return table HTML code
 */
String HtmlTable::toString() {
  content += F("</tbody>");
  addCloseTag();
  return content;
}

/*
 * HTML form node class
 */
class HtmlForm : public HtmlNode {
public:
  HtmlForm(String id, const __FlashStringHelper *action);
  void addTextField(const __FlashStringHelper *fieldName, const __FlashStringHelper *description, int maxLength, String value, bool password);
  String toString();
};

/*
 * Constructor, sets up form
 */
HtmlForm::HtmlForm(String id, const __FlashStringHelper *action) : HtmlNode(F("form"), id) {
  addTagAttribute(F("action"), action);
  addTagAttribute(F("method"), F("post"));
  content += '>';
}

/*
 * Adds a text type field to form
 */
void HtmlForm::addTextField(const __FlashStringHelper *fieldName, const __FlashStringHelper *description, int maxLength, String value, bool password) {
  content += F("<label>");
  content += description;
  content += F(":</label>");
  content += F("<input ");
  addTagAttribute(F("type"), password ? F("password") : F("text"));
  addTagAttribute(F("value"), value);
  addTagAttribute(F("name"), fieldName);
  addTagAttribute(F("size"), String(maxLength, DEC));
  content += F("><br />");
}

/*
 * Return table HTML code
 */
String HtmlForm::toString() {
  content += F("<input ");
  addTagAttribute(F("type"), F("submit"));
  content += '>';
  addCloseTag();
  return content;
}


/*
 * HTML link node class
 */
class HtmlLink : public HtmlNode {
public:
  HtmlLink(String id, const __FlashStringHelper *text, const __FlashStringHelper *location);
};

/*
 * Build link
 */
HtmlLink::HtmlLink(String id, const __FlashStringHelper *text, const __FlashStringHelper *location) : HtmlNode(F("a"), id) {
  addTagAttribute(F("href"), location);
  content += F(">");
  content += text;
  addCloseTag();
}

/*
 * WWW related functions
 */
void wwwRoot() {
  String page;
  page += pageHeader("Sonny index");
  page += HtmlLink("", F("Configure"), F("configure")).toString();
  page += F("<br />"); 
  page += HtmlLink("", F("Control"), F("control")).toString();
  page += pageFooter();
  server.send(200, F("text/html"), page);
}

/*
 * Configuration page, default when in AP mode
 */
void wwwConfigure() {
  String page;
  int8_t i;
  int8_t len;
  page += pageHeader("Configure Sonny");
  switch (server.method()) {
    case HTTP_POST:
      for (i = 0; i < settingLast; i++) {
        if (settings[i].visible) {
          switch (settings[i].settingType) {
            case typeString:
            case typePassword:
              setSettingString((sonoffSettingIndex)i, (char *)server.arg(settings[i].settingName).c_str());
            break;
            case typeBool:

            break;
            case typeInteger:
              setSettingInteger((sonoffSettingIndex)i, server.arg(settings[i].settingName).toInt());
            break;
          }
        }
      }
      setSettingBool(settingReset, false);
      if (saveSettings(false)) {
        page += F("Settings saved<br />");
      } else {
        page += F("Error saving settings<br />");
      }
    case HTTP_GET:
      HtmlForm settingsForm("settings", F("configure"));
      for (i = 0; i < settingLast; i++) {
        if (settings[i].visible) {
          switch (settings[i].settingType) {
            case typeString:
            case typePassword:
            case typeInteger:
              settingsForm.addTextField(settings[i].settingName, settings[i].settingDescription, settings[i].settingLength, (settings[i].settingType == typeInteger ? String(getSettingInteger((sonoffSettingIndex)i), DEC) : getSettingString((sonoffSettingIndex)i)), (settings[i].settingType == typePassword ? true : false));
            break;
            case typeBool:

            break;
          }
        }
      }
      page += settingsForm.toString();
      page += (getSettingBool(settingReset) ? "Reset" : "No reset");
    break;
  }
  page += pageFooter();
  server.send(200, F("text/html"), page);
}

/*
 * Page for overview of IO and status
 */
void wwwControl() {
  String page;
  int8_t i;

  const __FlashStringHelper * inputTableHeaders[] = {
    F("ID"), F("State"), F("Publication topic"), F("Last change (ms)")
  };
  const __FlashStringHelper * outputTableHeaders[] = {
    F("ID"), F("State"), F("Publication topic"), F("Subscription topic"), F("Last change (ms)")
  };
  
  page += pageHeader("Control Sonny");
  switch (server.method()) {
    case HTTP_POST:
    break;
    case HTTP_GET:
      HtmlTable inputTable("inputTable", 4, inputTableHeaders);
      for (i = 0; i < device.inputCount; i++) {
        inputTable.addRow({String(i, DEC), String(device.inputs[i]->lastState, DEC), String(device.inputs[i]->publishTopic), String((millis() - device.inputs[i]->lastStateTime), DEC)});
      }
      page += inputTable.toString();
      
      HtmlTable outputTable("outputTable", 5, outputTableHeaders);
      for (i = 0; i < device.outputCount; i++) {
        outputTable.addRow({String(i, DEC), String(device.outputs[i]->lastState, DEC), String(device.outputs[i]->publishTopic), String(device.outputs[i]->mqttSubscriber.topic), String((millis() - device.outputs[i]->lastStateTime), DEC)});
      }
      page += outputTable.toString();

    break;
  }
  page += pageFooter();
  server.send(200, F("text/html"), page);
}

/*
 * CSS page
 */
void wwwStyle() {
  String page;
  page += F("label{display:inline-block;width:350px;}");
  server.send(200, F("text/css"), page);
}

/*
 * Page builder: header
 */
String pageHeader(const char *title) {
  String page;
  page += F("<!DOCTYPE html><html><head><title>");
  page += title;
  page += F("</title><link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"></head><body>");
  return page;
}

/*
 * Page builder: footer
 */
String pageFooter() {
  String page;
  page += F("</body></html>");
  return page;
}
//
//void handleNotFound(){
//  String message = "File Not Found\n\n";
//  message += "URI: ";
//  message += server.uri();
//  message += "\nMethod: ";
//  message += (server.method() == HTTP_GET) ? "GET" : "POST";
//  message += "\nArguments: ";
//  message += server.args();
//  message += "\n";
//  for (uint8_t i = 0; i < server.args(); i++) {
//    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
//  }
//  server.send(404, F("text/plain"), message);
//}

/*
 * Connect to broker
 */
void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt->connected() || device.setupMode) {
    return;
  }

  Serial.print(F("Connecting to MQTT... "));
  if (!(ret = mqtt->connect())) {
    Serial.println(F("MQTT Connected"));
    setLedState(0, true);
  } else {
    Serial.println(mqtt->connectErrorString(ret));
    device.leds[0]->divisor = 1;
    mqtt->disconnect();
  }
}

/*
 * Publish a state to broker together with delta time since last state change (on device).
 * If state change is a triggered input it's previous state won't have been published.
 * Eg: button is pressed (not published), button is released after 1000 msec (published with deltatime 1000 msec)
 */
void tryMqttPublish(Adafruit_MQTT_Publish * publisher, int8_t value, int deltaTime) {
  char payload[128];
  // calculate minimum @ https://bblanchon.github.io/ArduinoJson/assistant/
  StaticJsonBuffer<128> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["state"] = value ? "true" : "false";
  root["deltaTime"] = deltaTime;
  root.printTo(payload, sizeof(payload));  
  if (! publisher->publish(payload)) {
    Serial.println(F("MQTT publish failed"));
    setLedPattern(0, 2);
  }
}

/*
 * Called from loop() to publish any changes to IOs
 */
void MQTT_publish() {
  int8_t currentValue;
  int8_t i;
  int deltaTime;
  for (i = 0; i < device.inputCount; i++) {
    currentValue = digitalRead(device.inputs[i]->pin);
    if (currentValue != device.inputs[i]->lastState) {
      deltaTime = millis() - device.inputs[i]->lastStateTime;
      if (currentValue == device.inputs[i]->triggerPublishState) {
        // publish
        tryMqttPublish(&device.inputs[i]->mqttPublisher, currentValue, deltaTime);
        if ((deltaTime < 500) && (device.inputs[i]->triggers[0])) { // trigger 0
          device.inputs[i]->triggers[0](i);
        } else if ((deltaTime < 2000) && (device.inputs[i]->triggers[1])) { // trigger 1
          device.inputs[i]->triggers[1](i);
        } else if ((deltaTime < 5000) && (device.inputs[i]->triggers[2])) { // trigger 2
          device.inputs[i]->triggers[2](i);
        } else if ((deltaTime > 5000) && (device.inputs[i]->triggers[3])) { // trigger 3
          device.inputs[i]->triggers[3](i);
        }
      }
      device.inputs[i]->lastState = currentValue;
      device.inputs[i]->lastStateTime = millis();
    }
  }
  for (i = 0; i < device.outputCount; i++) {
    currentValue = digitalRead(device.outputs[i]->pin);
    if (currentValue != device.outputs[i]->lastState) {
      // publish
      tryMqttPublish(&device.outputs[i]->mqttPublisher, currentValue, 0);
      device.outputs[i]->lastState = currentValue;
    }
  }
}

/*
 * Setup device and libraries
 */
void setup(void){
  Serial.begin(115200);
  os_timer_setfn(&device.timer, timerCallback, NULL);
  os_timer_arm(&device.timer, 100, true);

  Serial.println("");

  addSettingString(settingSSID, true, F("ssid"), F("WiFi SSID"), "", 32);
  addSettingPassword(settingPSK, true, F("psk"), F("WiFi PSK"), "", 32);
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String hostname = String("Sonny-") + String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  addSettingString(settingHostname, true, F("host"), F("Device hostname"), hostname.c_str(), 32);
  addSettingBool(settingReset, false, F("reset"), F("Reset device"), true);
  addSettingString(settingMqttHost, true, F("mqtt_host"), F("MQTT broker hostname"), "127.0.0.1", 32);
  addSettingInteger(settingMqttPort, true, F("mqtt_port"), F("MQTT broker port"), 1883);
  addSettingString(settingMqttUsername, true, F("mqtt_user"), F("MQTT username"), "", 32);
  addSettingString(settingMqttPassword, true, F("mqtt_key"), F("MQTT password"), "", 64);
  addSettingString(settingMqttHostFingerprint, false, F("mqtt_host_fingerprint"), F("MQTT host SHA fingerprint"), "", 60);  // implemented later?
  restoreSettings();

  // Setup before device so IO can refere to mqtt object
  mqtt = new Adafruit_MQTT_Client(&client, getSettingString(settingMqttHost), getSettingInteger(settingMqttPort), getSettingString(settingMqttUsername), getSettingString(settingMqttPassword));
  setupDevice();                // device specific configuration
  setLedPattern(0, 4);          // show we're initialising

  if (getSettingBool(settingReset)) {
    Serial.println(F("Device was reset to system default settings"));
    // Setup AP mode
    WiFi.softAP(hostname.c_str());
    device.setupMode = true;
  } else {
    // Setup STA mode
    WiFi.begin(getSettingString(settingSSID), getSettingString(settingPSK));
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      MQTT_publish(); // Allow input triggers even when there is no connection
      yield();
    }
    Serial.println("");
    Serial.print(F("Connected to "));
    Serial.println(getSettingString(settingSSID));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
  }

  if (device.setupMode) {
    server.onNotFound(wwwConfigure);
  } else {
    server.on("/configure", wwwConfigure);
    server.on("/control", wwwControl);
    server.on("/style.css", wwwStyle);
    server.onNotFound(wwwRoot);
  }

  server.begin();
  Serial.println(F("HTTP server started"));

  ArduinoOTA.setHostname(getSettingString(settingHostname));
  ArduinoOTA.onStart([]() {
    Serial.println(F("Starting update OTA"));
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nEnd of update OTA"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Updating: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    setLedPattern(0, 10);
    Serial.printf("OTA Error[%u]: ", error);
    switch (error) {
      case OTA_AUTH_ERROR:
        Serial.print(F("Auth"));
      break;
      case OTA_BEGIN_ERROR:
        Serial.print(F("Begin"));
      break;
      case OTA_CONNECT_ERROR:
        Serial.print(F("Connect"));
      break;
      case OTA_RECEIVE_ERROR:
        Serial.print(F("Receive"));
      break;
      case OTA_END_ERROR:
        Serial.print(F("End"));
      break;
      default:
        Serial.println(F("Unknown failure"));
    }
    Serial.println(F(" failed"));
  });
  ArduinoOTA.begin();

  setLedState(0, true);          // initialisation done
}

/*
 * Main loop, handle incoming MQTT messages etc
 */

void loop(void){
  int8_t i;

  server.handleClient();
  MQTT_connect();
  MQTT_publish();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt->readSubscription(100))) {
    for (i = 0; i < device.outputCount; i++) {
      if (subscription == &device.outputs[i]->mqttSubscriber) {
        JsonObject& root = jsonBuffer.parseObject(subscription->lastread);
        if (!root.success()) {
          Serial.println(F("parseObject() failed"));
        } else {
          const char* state = root["state"];
          int8_t value = 0;
          if (!strcmp(state, "false")) {
            value = 1;
          }
          digitalWrite(device.outputs[i]->pin, value);
        }
      }
    }
  }
  ArduinoOTA.handle();
  
  yield(); // Needed for software timer, readsubscription takes a chunk of time already though so timer won't be too accurate(!)
}
