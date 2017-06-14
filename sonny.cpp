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

#include "sonny.h"

// https://bblanchon.github.io/ArduinoJson/
#include <ArduinoJson.h>

DynamicJsonBuffer jsonBuffer;

/*
 * Setup device specific IOs and create their pub/sub handlers
 */
Sonny *Sonny::setupDevice(WiFiClient *wifiClient, SettingsManager *settings) {
  Sonny *device = NULL;
#if SONOFF_DEVICE == SONOFF
  device = new SonnyS20(wifiClient, settings);
#elif SONOFF_DEVICE == SONOFF_DUAL
  device = new SonnyDual(wifiClient, settings);
#elif SONOFF_DEVICE == SONOFF_S20
  device = new SonnyS20(wifiClient, settings);
#elif SONOFF_DEVICE == SONOFF_TOUCH
  device = new SonnyS20(wifiClient, settings);
#else
//  Serial.println(F("Unknown devicetype"));
#endif
  analogWriteRange(PWMRANGE);
  analogWriteFreq(1);
  device->initialiseIO();
  return device;
}

/*
 * Reserve heap space for IO etc
 */
Sonny::Sonny(WiFiClient *wifiClient, SettingsManager *settings, uint8_t inputCount, uint8_t outputCount, uint8_t ledCount) : wifiClient(wifiClient), settings(settings), inputCount(inputCount), outputCount(outputCount), ledCount(ledCount) {
  inputs = (sonoffIO**)malloc(sizeof(sonoffIO*) * inputCount);
  outputs = (sonoffIO**)malloc(sizeof(sonoffIO*) * outputCount);
  leds = (sonoffLED**)malloc(sizeof(sonoffLED*) * ledCount);
  mqtt = new Adafruit_MQTT_Client(wifiClient, settings->getSettingString(settingMqttHost), settings->getSettingInteger(settingMqttPort), settings->getSettingString(settingMqttUsername), settings->getSettingString(settingMqttPassword));
}

/*
 * Wrapper for logger
 */
void Sonny::logFormatted(Logger::logSeverity severity, char *format, ...) {
  va_list args;
  va_start (args, format);
  for (uint8_t i = 0; i < loggerCount; i++) {
    loggers[i]->logFormattedVa(severity, format, args);
  }
  va_end (args);
}

/*
 * Are we in setup mode?
 */
bool Sonny::getSetupMode() {
  return setupMode;
}

/*
 * Set in setup mode
 */
void Sonny::setSetupMode(bool value) {
  setupMode = value;
}

/*
 * How many inputs are there?
 */
uint8_t Sonny::getInputCount() {
  return inputCount;    
}

/*
 * How many outputs are there?
 */
uint8_t Sonny::getOutputCount() {
  return outputCount;
}

/*
 * Add device to input list
 */
void Sonny::addInputDevice(uint8_t index, uint8_t pin) {
  addIoDevice(inputs, index, pin);
}

/*
 * Setup trigger for specified slot
 */
void Sonny::setInputTrigger(uint8_t index, uint8_t triggerIndex, void *trigger) {
  inputs[index]->triggers[triggerIndex] = (void (*)(Sonny*, uint8_t))trigger;  
}

/*
 * When to publish? High, low or any
 */
void Sonny::setInputTriggerPublishValue(uint8_t index, uint8_t triggerPublishState) {
  inputs[index]->triggerPublishState = triggerPublishState;
}

/*
 * Report inverted in 'state' field
 */
void Sonny::setOutputInverted(uint8_t index, bool inverted) {
  outputs[index]->reportInverted = inverted;
}

/*
 * Add device to output list
 */
void Sonny::addOutputDevice(uint8_t index, uint8_t pin) {
  addIoDevice(outputs, index, pin);
}

/*
 * Read ESP pin
 */
uint8_t Sonny::readInput(uint8_t index) {
  return digitalRead(inputs[index]->pin);
}

/*
 * Read ESP pin
 */
uint8_t Sonny::readOutput(uint8_t index) {
  return digitalRead(outputs[index]->pin);
}

/*
 * Write ESP pin
 */
void Sonny::writeOutput(uint8_t index, uint8_t value) {
  digitalWrite(outputs[index]->pin, value);
}

/*
 * Add IO device to array and assign pin
 */
void Sonny::addIoDevice(sonoffIO **list, uint8_t index, uint8_t pin) {
  list[index] = (sonoffIO*)malloc(sizeof(sonoffIO));
  memset(list[index], 0x00, sizeof(sonoffIO));
  list[index]->pin = pin;
}

/*
 * Add LED to array and assign pin
 */
void Sonny::addLed(uint8_t index, uint8_t pin) {
  if (ledCount > index) {
    leds[index] = (sonoffLED*)malloc(sizeof(sonoffLED));
    leds[index]->pin = pin;
    leds[index]->dutyCycle = PWMRANGE;
  }
}

/*
 * Set LED state
 */
void Sonny::setLedState(uint8_t index, bool state) {
  if (ledCount > index) {
    leds[index]->dutyCycle = (state ? 0 : PWMRANGE);
    analogWrite(leds[index]->pin, leds[index]->dutyCycle);
  }
}

/*
 * Set LED dutycycle
 */
void Sonny::setLedDutyCycle(uint8_t index, uint8_t percentage) {
  if (ledCount > index) {
    leds[index]->dutyCycle = (PWMRANGE / 100) * percentage;
    analogWrite(leds[index]->pin, leds[index]->dutyCycle);
  }
}

/*
 * Toggle an output
 */
void Sonny::toggleOutputTrigger(Sonny *object, uint8_t index) {
  object->toggleOutput(index);
}

/*
 * Increment counter and set outputs according to high bits
 */
void Sonny::countedOutputTrigger(Sonny *object, uint8_t index) {
  object->countedOutput(index);

}

/*
 * Set reset and reboot
 */
void Sonny::resetConfigTrigger(Sonny *object, uint8_t index) {
  object->resetConfig(index);
}

/*
 * Toggle output matching input pin
 */
void Sonny::toggleOutput(uint8_t index) {
  if (outputCount > index) {
    writeOutput(index, !readOutput(index));
  }
  writeAll();
}

/*
 * Reset to defaults
 */
void Sonny::resetConfig(uint8_t index) {
  settings->saveSettings(true);
  ESP.restart();
}

/*
 * Increment counter and set outputs according to bits
 */
void Sonny::countedOutput(uint8_t index) {
  logFormatted(Logger::severityInfo, "Counted output triggered: %d\r\n", outputCounter);
  if (outputCounter == outputLimitCounter) {
    outputCounter = 0;
  } else {
    outputCounter++;
  }
  for (uint8_t i = 0; i < outputCount; i++) {
    if ((outputCounter >> i) & 0x01) {
      writeOutput(i, 1);
    } else {
      writeOutput(i, 0);
    }
  }
  writeAll();
}

/*
 * Set up all IO
 */
void Sonny::initialiseIO() {
  uint8_t i;
  char *topic;
  const int topicSize = 32;
  logFormatted(Logger::severityInfo, "Configuring IO\r\n");
  for (i = 0; i < inputCount; i++) {
    setupInput(i);
    inputs[i]->lastState = readInput(i);
    inputs[i]->lastStateTime = millis();
    topic = (char *)malloc(topicSize);
    snprintf(topic, topicSize, "sonoff/%s/input/%d", settings->getSettingString(settingHostname), i);
    inputs[i]->mqttPublisher = new Adafruit_MQTT_Publish(mqtt, topic);
    inputs[i]->publishTopic = strdup(topic);
  }
  for (i = 0; i < outputCount; i++) {
    setupOutput(i);
    outputs[i]->lastState = readOutput(i);
    topic = (char *)malloc(topicSize);
    snprintf(topic, topicSize, "sonoff/%s/output/%d", settings->getSettingString(settingHostname), i);
    outputs[i]->mqttPublisher = new Adafruit_MQTT_Publish(mqtt, topic);
    outputs[i]->publishTopic = strdup(topic);
    topic = (char *)malloc(topicSize);
    snprintf(topic, topicSize, "sonoff/%s/switch/%d", settings->getSettingString(settingHostname), i);
    outputs[i]->mqttSubscriber = new Adafruit_MQTT_Subscribe(mqtt, topic);
    mqtt->subscribe(outputs[i]->mqttSubscriber);
  }
  for (i = 0; i < ledCount; i++) {
    pinMode(leds[i]->pin, OUTPUT);
    analogWrite(leds[i]->pin, leds[i]->dutyCycle);
  }
}

/*
 * Set up ESP input pin
 */
void Sonny::setupInput(uint8_t index) {
  pinMode(inputs[index]->pin, INPUT);
}

/*
 * Set up ESP output pin
 */
void Sonny::setupOutput(uint8_t index) {
  pinMode(outputs[index]->pin, OUTPUT);
}

/*
 * Placeholder for read all in subclasses
 */
void Sonny::readAll() {
  
}

/*
 * Placeholder for write all in subclasses
 */
void Sonny::writeAll() {
  
}

/*
 * Read I/O, trigger and publish
 */
void Sonny::handleIO() {
  uint8_t currentValue;
  uint8_t i;
  int deltaTime;
  readAll();
  for (i = 0; i < inputCount; i++) {
    currentValue = readInput(i);
    if (currentValue != inputs[i]->lastState) {
      deltaTime = millis() - inputs[i]->lastStateTime;
      logFormatted(Logger::severityDebug, "Input %d now has state %d (delta %d)\r\n", i, currentValue, deltaTime);
      if ((inputs[i]->triggerPublishState == 2) && (deltaTime > 500)) {
        tryMqttPublish(inputs[i]->mqttPublisher, currentValue, currentValue ^ inputs[i]->reportInverted, deltaTime);
        if (inputs[i]->triggers[0]) { // trigger 0
          inputs[i]->triggers[0](this, i);
        }
      } else if (currentValue == inputs[i]->triggerPublishState) {
        // publish
        tryMqttPublish(inputs[i]->mqttPublisher, currentValue, currentValue ^ inputs[i]->reportInverted, deltaTime);
        if ((deltaTime < 500) && (inputs[i]->triggers[0])) { // trigger 0
          inputs[i]->triggers[0](this, i);
        } else if ((deltaTime < 2000) && (inputs[i]->triggers[1])) { // trigger 1
          inputs[i]->triggers[1](this, i);;
        } else if ((deltaTime < 5000) && (inputs[i]->triggers[2])) { // trigger 2
          inputs[i]->triggers[2](this, i);
        } else if ((deltaTime > 5000) && (inputs[i]->triggers[3])) { // trigger 3
          inputs[i]->triggers[3](this, i);
        }
      }
      inputs[i]->lastState = currentValue;
      inputs[i]->lastStateTime = millis();
    }
  }
  for (i = 0; i < outputCount; i++) {
    currentValue = readOutput(i);
    if (currentValue != outputs[i]->lastState) {
      logFormatted(Logger::severityDebug, "Output %d now has state %d, was %d\r\n", i, outputs[i]->lastState, currentValue);
      // publish
      tryMqttPublish(outputs[i]->mqttPublisher, currentValue, currentValue ^ outputs[i]->reportInverted, 0);
      outputs[i]->lastState = currentValue;
    }
  }
}

/*
 * Check subscriptions
 */
void Sonny::handleMQTT() {
  if (!connectMQTT()) {
    return;
  }
  Adafruit_MQTT_Subscribe *subscription;
  while (subscription = mqtt->readSubscription(100)) {
    for (uint8_t i = 0; i < outputCount; i++) {
      if (subscription == outputs[i]->mqttSubscriber) {
        logFormatted(Logger::severityDebug, "Received MQTT message \"%s\"\r\n", subscription->lastread);
        JsonObject& root = jsonBuffer.parseObject(subscription->lastread);
        if (!root.success()) {
          logFormatted(Logger::severityWarning, "parseObject() failed\r\n");
        } else {
          const char* state = root["state"];
          logFormatted(Logger::severityDebug, "State \"%s\"\r\n", state);
          int8_t value = 0;
          if (!strcasecmp(state, "false") || !strcasecmp(state, "off")) {
            value = 1;
          }
          writeOutput(i, value);
        }
      }
    }
    writeAll();
  }
//  mqtt->ping();
}

/*
 * Connect in case we got disconnected
 */
bool Sonny::connectMQTT() {
  int8_t ret;
  if (mqtt->connected() || setupMode) {
    return true;
  }
  
  logFormatted(Logger::severityInfo, "Connecting to MQTT...\r\n");
  if (!(ret = mqtt->connect())) {
    logFormatted(Logger::severityInfo, "MQTT Connected\r\n");
    setLedState(0, true);
  } else {
    logFormatted(Logger::severityError, "%s\r\n", mqtt->connectErrorString(ret));
    setLedDutyCycle(0, 75);
    mqtt->disconnect();
  }
  return false;
}

/*
 * Direct access to output device
 */
sonoffIO *Sonny::getOutputDevice(uint8_t index) {
  return outputs[index];
}

/*
 * Direct access to input device
 */
sonoffIO *Sonny::getInputDevice(uint8_t index) {
  return inputs[index];
}

/*
 * Publish a state to broker together with delta time since last state change (on device).
 * If state change is a triggered input it's previous state won't have been published.
 * Eg: button is pressed (not published), button is released after 1000 msec (published with deltatime 1000 msec)
 */
void Sonny::tryMqttPublish(Adafruit_MQTT_Publish * publisher, bool value, bool state, int deltaTime) {
  char payload[128];
  // calculate minimum @ https://bblanchon.github.io/ArduinoJson/assistant/
  StaticJsonBuffer<128> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "bool";
  root["value"] = value;
  root["state"] = state ? "on" : "off";
  root["deltaTime"] = deltaTime;
  root.printTo(payload, sizeof(payload));  
  if (!publisher->publish(payload)) {
    logFormatted(Logger::severityWarning, "MQTT publish failed\r\n");
    setLedDutyCycle(0, 75);
  }
}

/*
 * Set up for Sonoff S20 and certain other boards
 */
SonnyS20::SonnyS20(WiFiClient *wifiClient, SettingsManager *settings) : Sonny(wifiClient, settings, 1, 1, 1) {
  loggerCount = 2;
  loggers = (Logger**)malloc(sizeof(Logger*) * loggerCount);
  loggers[0] = new SerialLogger(115200);
  loggers[1] = new UdpLogger(LUMBERLOG_HOST, 12345);
  
  logFormatted(Logger::severityInfo, "Setup Sonoff S20\r\n");
  addInputDevice(0, 0);                 // button
  setInputTrigger(0, 0, (void*)Sonny::toggleOutputTrigger);
  setInputTrigger(0, 3, (void*)Sonny::resetConfigTrigger);
  setInputTriggerPublishValue(0, 1);    // trigger on release
  addOutputDevice(0, 12);               // relay
  addLed(0, 13);
}

/*
 * Set up for Sonoff dual (IO devices above index 4 are ESP pins)
 */
SonnyDual::SonnyDual(WiFiClient *wifiClient, SettingsManager *settings) : Sonny(wifiClient, settings, 4, 4, 1) {
  loggerCount = 1;
  loggers = (Logger**)malloc(sizeof(Logger*) * loggerCount);
  loggers[0] = new UdpLogger(LUMBERLOG_HOST, 12345);
  
  logFormatted(Logger::severityInfo, "Setup Sonoff Dual\r\n");
  addInputDevice(0, 1);                 // button0
  addInputDevice(1, 2);                 // button1
  addInputDevice(2, 4);                 // button2
  addInputDevice(3, 8);                 // button3
//  addInputDevice(4, 0);                 // GPIO 0
//  setInputTrigger(4, 0, (void*)Sonny::countedOutputTrigger);
//  setInputTriggerPublishValue(4, 1);    // trigger on release
  
  setInputTrigger(2, 0, (void*)Sonny::countedOutputTrigger);
  setInputTriggerPublishValue(2, 2);    // trigger on release

  addOutputDevice(0, 1);                // relay0
  addOutputDevice(1, 2);                // relay1
  addOutputDevice(2, 4);                // relay2
  addOutputDevice(3, 8);                // relay3
  outputLimitCounter = 3;
  
  addLed(0, 13);
  stuckTriggers[0] = Sonny::resetConfigTrigger;
  
  Serial.end();
  Serial.begin(19200);
}

/*
 * Call base class setup for ESP pins
 */
void SonnyDual::setupInput(uint8_t index) {
  if (index >= 4) {
    Sonny::setupInput(index);
  }  
}

/*
 * Call base class setup for ESP pins
 */
void SonnyDual::setupOutput(uint8_t index) {
  if (index >= 4) {
    Sonny::setupOutput(index);
  }
}

/*
 * Read non ESP IO devices
 */
void SonnyDual::readAll() {
  // 0xA0, 0x00, bitfield buttons, 0xA1
  // 0xA0, 0xf5, 0x00, 0xA1 - stuck
  // 0xA0, 0xf6, 0x00, 0xA1 - unstuck
  uint8_t input = 0;
  if (Serial.available() == 4) {
    input = Serial.read();
    if (input == 0xA0) { // start of command
      input = Serial.read();
      if (input == 0x00 || input == 0x04) { // button message / relay status
        input = Serial.read();
        for (uint8_t i = 0; i < inputCount; i++) {
          if (inputs[i]->pin & input) {
            inputs[i]->currentState = 1;
            outputs[i]->currentState = 1;
          } else {
            inputs[i]->currentState = 0;
            outputs[i]->currentState = 0;
          }
        }
      } else if (input == 0xF5) { // stuck button
        logFormatted(Logger::severityInfo, "Button stuck\r\n");
        if (stuckTriggers[0]) {
          stuckTriggers[0](this, 0);
        }
        input = Serial.read();
      } else if (input == 0xF6) { // unstuck button
        logFormatted(Logger::severityInfo, "Button unstuck\r\n");
        if (stuckTriggers[1]) {
          stuckTriggers[1](this, 1);
        }
        input = Serial.read();
      } else {
        logFormatted(Logger::severityWarning, "Unexpected value for offset 1: 0x%x\r\n", input);
        input = Serial.read();
        logFormatted(Logger::severityWarning, "Unexpected value for offset 2: 0x%x\r\n", input);
      }
      input = Serial.read();
      if (input != 0xA1) {
        logFormatted(Logger::severityWarning, "Unexpected value for offset 3: 0x%x\r\n", input);
      }
    }
  }
}

/*
 * Return inputs
 */
uint8_t SonnyDual::readInput(uint8_t index) {
  if (index >= 4) {
    return Sonny::readInput(index);
  } else {
    return inputs[index]->currentState;
  }
}

/*
 * Return outputs
 */
uint8_t SonnyDual::readOutput(uint8_t index) {
  if (index >= 4) {
    return Sonny::readOutput(index);
  } else {
    return outputs[index]->currentState;
  }
}

/*
 * Write outputs
 */
void SonnyDual::writeOutput(uint8_t index, uint8_t value) {
  if (index >= 4) {
    Sonny::writeOutput(index, value);
  } else {
    outputs[index]->currentState = value;
  }
}

/*
 * Write non ESP IO devices
 */
void SonnyDual::writeAll() {
  // 0xA0, 0x04, bitfield outputs, 0xA1
  uint8_t bitfield = 0;
  for (uint8_t i = 0; i < outputCount; i++) {
    if (outputs[i]->currentState) {
      bitfield += (1 << i);
    }
  }
  Serial.write(0xa0);
  Serial.write(0x04);
  Serial.write(bitfield);
  Serial.write(0xa1);
}

