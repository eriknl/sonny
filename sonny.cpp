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

Sonny *Sonny::SingleSonny = NULL;



#ifdef SONNY_REMEHA
uint16_t crcTable[] = {
  0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700, 0x0800, 0x0900, 0x0A00, 0x0B00, 0x0C00, 0x0D00, 0x0E00, 0x0F00,
  0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1500, 0x1600, 0x1700, 0x1800, 0x1900, 0x1A00, 0x1B00, 0x1C00, 0x1D00, 0x1E00, 0x1F00,
  0x2000, 0x2100, 0x2200, 0x2300, 0x2400, 0x2500, 0x2600, 0x2700, 0x2800, 0x2900, 0x2A00, 0x2B00, 0x2C00, 0x2D00, 0x2E00, 0x2F00,
  0x3000, 0x3100, 0x3200, 0x3300, 0x3400, 0x3500, 0x3600, 0x3700, 0x3800, 0x3900, 0x3A00, 0x3B00, 0x3C00, 0x3D00, 0x3E00, 0x3F00,
  0x4000, 0x4100, 0x4200, 0x4300, 0x4400, 0x4500, 0x4600, 0x4700, 0x4800, 0x4900, 0x4A00, 0x4B00, 0x4C00, 0x4D00, 0x4E00, 0x4F00,
  0x5000, 0x5100, 0x5200, 0x5300, 0x5400, 0x5500, 0x5600, 0x5700, 0x5800, 0x5900, 0x5A00, 0x5B00, 0x5C00, 0x5D00, 0x5E00, 0x5F00,
  0x6000, 0x6100, 0x6200, 0x6300, 0x6400, 0x6500, 0x6600, 0x6700, 0x6800, 0x6900, 0x6A00, 0x6B00, 0x6C00, 0x6D00, 0x6E00, 0x6F00,
  0x7000, 0x7100, 0x7200, 0x7300, 0x7400, 0x7500, 0x7600, 0x7700, 0x7800, 0x7900, 0x7A00, 0x7B00, 0x7C00, 0x7D00, 0x7E00, 0x7F00,
  0x8000, 0x8100, 0x8200, 0x8300, 0x8400, 0x8500, 0x8600, 0x8700, 0x8800, 0x8900, 0x8A00, 0x8B00, 0x8C00, 0x8D00, 0x8E00, 0x8F00,
  0x9000, 0x9100, 0x9200, 0x9300, 0x9400, 0x9500, 0x9600, 0x9700, 0x9800, 0x9900, 0x9A00, 0x9B00, 0x9C00, 0x9D00, 0x9E00, 0x9F00,
  0xA000, 0xA100, 0xA200, 0xA300, 0xA400, 0xA500, 0xA600, 0xA700, 0xA800, 0xA900, 0xAA00, 0xAB00, 0xAC00, 0xAD00, 0xAE00, 0xAF00,
  0xB000, 0xB100, 0xB200, 0xB300, 0xB400, 0xB500, 0xB600, 0xB700, 0xB800, 0xB900, 0xBA00, 0xBB00, 0xBC00, 0xBD00, 0xBE00, 0xBF00,
  0xC000, 0xC100, 0xC200, 0xC300, 0xC400, 0xC500, 0xC600, 0xC700, 0xC800, 0xC900, 0xCA00, 0xCB00, 0xCC00, 0xCD00, 0xCE00, 0xCF00,
  0xD000, 0xD100, 0xD200, 0xD300, 0xD400, 0xD500, 0xD600, 0xD700, 0xD800, 0xD900, 0xDA00, 0xDB00, 0xDC00, 0xDD00, 0xDE00, 0xDF00,
  0xE000, 0xE100, 0xE200, 0xE300, 0xE400, 0xE500, 0xE600, 0xE700, 0xE800, 0xE900, 0xEA00, 0xEB00, 0xEC00, 0xED00, 0xEE00, 0xEF00,
  0xF000, 0xF100, 0xF200, 0xF300, 0xF400, 0xF500, 0xF600, 0xF700, 0xF800, 0xF900, 0xFA00, 0xFB00, 0xFC00, 0xFD00, 0xFE00, 0xFF00 
};
uint16_t *Sonny::remehaCrcTable = crcTable;
#endif


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
#elif SONOFF_DEVICE == ESP_12S
  device = new SonnyEsp(wifiClient, settings);
#else
//  Serial.println(F("Unknown devicetype"));
#endif
  analogWriteRange(PWMRANGE);
  analogWriteFreq(1);
  device->initialiseIO();
  Sonny::SingleSonny = device;
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
#ifdef SONNY_P1
  p1Io = (sonoffIO*)malloc(sizeof(sonoffIO));
#endif
#ifdef SONNY_REMEHA
  remehaIo = (sonoffIO*)malloc(sizeof(sonoffIO));
#endif
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
  inputs[index]->triggers[triggerIndex] = (void (*)(uint8_t))trigger;  
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
void Sonny::toggleOutputTrigger(uint8_t index) {
  SingleSonny->toggleOutput(index);
}

/*
 * Increment counter and set outputs according to high bits
 */
void Sonny::countedOutputTrigger(uint8_t index) {
  SingleSonny->countedOutput(index);
}

/*
 * Set reset and reboot
 */
void Sonny::resetConfigTrigger(uint8_t index) {
  SingleSonny->resetConfig(index);
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
#ifdef SONNY_P1
  p1Serial = new SoftwareSerial(4, -1, true, SOFTSERIAL_BUFFERSIZE); // (RX, TX. inverted, buffer);
  p1Serial->begin(115200);
  topic = (char *)malloc(topicSize);
  snprintf(topic, topicSize, "sonoff/%s/p1/read", settings->getSettingString(settingHostname));
  p1Io->mqttPublisher = new Adafruit_MQTT_Publish(mqtt, topic);
  p1Io->publishTopic = strdup(topic);
#endif
#ifdef SONNY_REMEHA
  remehaSerial = new SoftwareSerial(4, 5, false, SOFTSERIAL_BUFFERSIZE); // (RX, TX. inverted, buffer);
  remehaSerial->begin(9600);
  topic = (char *)malloc(topicSize);
  snprintf(topic, topicSize, "sonoff/%s/remeha/read", settings->getSettingString(settingHostname));
  remehaIo->mqttPublisher = new Adafruit_MQTT_Publish(mqtt, topic);
  remehaIo->publishTopic = strdup(topic);
#endif
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
          inputs[i]->triggers[0](i);
        }
      } else if (currentValue == inputs[i]->triggerPublishState) {
        // publish
        tryMqttPublish(inputs[i]->mqttPublisher, currentValue, currentValue ^ inputs[i]->reportInverted, deltaTime);
        if ((deltaTime < 500) && (inputs[i]->triggers[0])) { // trigger 0
          inputs[i]->triggers[0](i);
        } else if ((deltaTime < 2000) && (inputs[i]->triggers[1])) { // trigger 1
          inputs[i]->triggers[1](i);;
        } else if ((deltaTime < 5000) && (inputs[i]->triggers[2])) { // trigger 2
          inputs[i]->triggers[2](i);
        } else if ((deltaTime > 5000) && (inputs[i]->triggers[3])) { // trigger 3
          inputs[i]->triggers[3](i);
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

#ifdef SONNY_P1
  if (p1Serial->available()) {
    uint8_t lineLength = p1Serial->readBytesUntil('\n', softSerialBuffer, SOFTSERIAL_BUFFERSIZE);
    // Append \n and terminate string
    softSerialBuffer[lineLength++] = '\n';
    softSerialBuffer[lineLength] = 0x00;
    // Start of telegram?
    if (softSerialBuffer[0] == '/') {
      p1CRC = 0;
      p1CalculateCRC16(softSerialBuffer, lineLength);
      memset(powerIn, 0x00, 7);
      memset(powerOut, 0x00, 7);
      memset(gasIn, 0x00, 10);
      memset(gasTime, 0x00, 13);
    } else if (softSerialBuffer[0] == '!') {
      p1CalculateCRC16(softSerialBuffer, 1);
      softSerialBuffer[5] = 0x00;
      uint16_t telegramCRC = strtol((const char*)softSerialBuffer + 1, NULL, 16);
      if (telegramCRC == p1CRC) {
//        logFormatted(Logger::severityDebug, "CRC ok 0x%x, 0x%x\r\n", telegramCRC, p1CRC);
        // publish telegram to MQTT
        if (connectMQTT()) {
          char payload[128];
          StaticJsonBuffer<128> jsonBuffer;
          JsonObject& root = jsonBuffer.createObject();
          root["powerIn"] = powerIn;
          root["powerOut"] = powerOut;
          root["gasIn"] = gasIn;
          root["gasTime"] = gasTime;
          root.printTo(payload, sizeof(payload));
          if (!p1Io->mqttPublisher->publish(payload)) {
            logFormatted(Logger::severityWarning, "MQTT publish failed\r\n");
          }
        }
      } else {
//        logFormatted(Logger::severityDebug, "CRC not ok 0x%x, 0x%x\r\n", telegramCRC, p1CRC);
      }
    } else {
      p1CalculateCRC16(softSerialBuffer, lineLength);
      if (!strncmp((const char*)softSerialBuffer, "1-0:1.7.0", strlen("1-0:1.7.0"))) {
        memcpy(powerIn, softSerialBuffer + 10, 6);
      } else if (!strncmp((const char*)softSerialBuffer, "1-0:2.7.0", strlen("1-0:2.7.0"))) {
        memcpy(powerOut, softSerialBuffer + 10, 6);
      } else if (!strncmp((const char*)softSerialBuffer, "0-1:24.2.1", strlen("0-1:24.2.1"))) {
        memcpy(gasTime, softSerialBuffer + 11, 12);
        memcpy(gasIn, softSerialBuffer + 26, 9);
      }
    }
  }
#endif

#ifdef SONNY_REMEHA
  if ((millis() - lastRemeha) > remehaInterval) {
    lastRemeha = millis();
    // Query: 0252050602005303 
    uint8_t query[] = {0x02, 0x52, 0x05, 0x06, 0x02, 0x00, 0x53, 0x03};
    remehaSerial->write(query, 8);
    delay(200);
    
    if (remehaSerial->available()) {
      bool payloadValid = false;
      uint16_t crc = 0xffff;
      softSerialBuffer[0] = 0x00;
      while (softSerialBuffer[0] != 0x02) {
        softSerialBuffer[0] = remehaSerial->read();
      }
      for (uint8_t i = 1; i < 64; i++) {
        softSerialBuffer[i] = remehaSerial->read();
        if ((i > 0) && (i < 62)) {
          // Payload
          crc = (crc << 8) ^ remehaCrcTable[((crc >> 8) ^ softSerialBuffer[i])];
        }
        if (i == 62) {
          // CRC
          if (((uint8_t*)&crc)[1] == softSerialBuffer[i]) {
            payloadValid = true;
          }
        }
      }
      if (payloadValid) {
//        logFormatted(Logger::severityDebug, "CRC match\r\n");
        // Parse
        uint16_t temp;
        temp = (*(softSerialBuffer + 21) << 8) + (*(softSerialBuffer + 22));
        roomTemp = (float)temp/100;
        temp = (*(softSerialBuffer + 27) << 8) + (*(softSerialBuffer + 28));
        roomSetpoint = (float)temp/100;
        if (connectMQTT()) {
          char payload[128];
          StaticJsonBuffer<128> jsonBuffer;
          JsonObject& root = jsonBuffer.createObject();
          root["roomTemp"] = roomTemp;
          root["roomSetpoint"] = roomSetpoint;
          root.printTo(payload, sizeof(payload));
          if (!remehaIo->mqttPublisher->publish(payload)) {
            logFormatted(Logger::severityWarning, "MQTT publish failed\r\n");
          }
        }
      } else {
//        logFormatted(Logger::severityDebug, "CRC mismatch\r\n");
      }
    }
  }
#endif
}

/*
 * Calculate CRC16 for x^16 + x^15 + x^2 + 1
 */
#ifdef SONNY_P1
// https://github.com/jantenhove/P1-Meter-ESP8266/blob/master/CRC16.h
uint16_t Sonny::p1CalculateCRC16(uint8_t *buffer, uint16_t length) {
  for (uint16_t index = 0; index < length; index++) {
    p1CRC ^= (unsigned int)buffer[index];       // XOR byte into least sig. byte of crc
    for (uint8_t bit = 8; bit != 0; bit--) {    // Loop over each bit
      if ((p1CRC & 0x0001) != 0) {              // If the LSB is set
        p1CRC >>= 1;                            // Shift right and XOR 0xA001
        p1CRC ^= 0xA001;
      } else {                                  // Else LSB is not set
        p1CRC >>= 1;                            // Just shift right
      }
    }
  }
  return p1CRC;
}
#endif

/*
 * Check subscriptions
 */
void Sonny::handleMQTT() {
  DynamicJsonBuffer jsonBuffer;
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
  if ((millis() - lastPing) > pingInterval) {
    mqtt->ping(5);
    lastPing = millis();
  }
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
    lastPing = millis();
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
          stuckTriggers[0](0);
        }
        input = Serial.read();
      } else if (input == 0xF6) { // unstuck button
        logFormatted(Logger::severityInfo, "Button unstuck\r\n");
        if (stuckTriggers[1]) {
          stuckTriggers[1](1);
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

/*
 * Set up for generic ESP8266 devices
 */
#ifdef defined(SONNY_P1) || defined(SONNY_REMEHA)
SonnyEsp::SonnyEsp(WiFiClient *wifiClient, SettingsManager *settings) : Sonny(wifiClient, settings, 0, 0, 0) {
#else
SonnyEsp::SonnyEsp(WiFiClient *wifiClient, SettingsManager *settings) : Sonny(wifiClient, settings, 0, 1, 0) {
#endif
  loggerCount = 2;
  loggers = (Logger**)malloc(sizeof(Logger*) * loggerCount);
  loggers[0] = new SerialLogger(115200);
  loggers[1] = new UdpLogger(LUMBERLOG_HOST, 12345);
#ifndef SONNY_P1
  addOutputDevice(0, 4);               // relay
#endif
#ifdef SONNY_REMEHA

#endif
  logFormatted(Logger::severityInfo, "Setup generic ESP device without IO\r\n");
}

