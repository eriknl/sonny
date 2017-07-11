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

#ifndef SONNY_H
#define SONNY_H

#define SONOFF          1
#define SONOFF_DUAL     2
#define SONOFF_S20      3
#define SONOFF_TOUCH    4


#define LUMBERLOG_HOST  "192.168.0.2"
//#define SONOFF_DEVICE   SONOFF
//#define SONOFF_DEVICE   SONOFF_S20
#define SONOFF_DEVICE   SONOFF_DUAL
//#define SONOFF_DEVICE   SONOFF_TOUCH

#if SONOFF_DEVICE == SONOFF_TOUCH
  #error Set board to ESP8285 and flash mode to DOUT, 1M 64K SPIFFS
#else
//  #error Set board to ESP8266 and flash mode to DIO, 1M 64K SPIFFS
#endif

// https://github.com/adafruit/Adafruit_MQTT_Library/
#include <Adafruit_MQTT_Client.h>
#include <Adafruit_MQTT.h>

#include "logger.h"
#include "settingsmanager.h"

class Sonny;

/*
 * Contains information for an I/O, including MQTT pub/subs
 * An output might have both a pub and a sub to ensure all changes are fed back to the broker
 */
typedef struct {
  uint8_t                   pin;                                                  // Physical pin
  uint8_t                   currentState;                                         // Current (polled) state
  uint8_t                   lastState;                                            // Last known state, in order not to publish the same state again
  uint8_t                   triggerPublishState;                                  // What triggers a publish: high, low or any?
  bool                      reportInverted = false;                               // Report on on 0 and off on 1
  int                       lastStateTime;                                        // When was the last state change
  Adafruit_MQTT_Publish     *mqttPublisher;                                       // Publisher object
  char                      *publishTopic;                                        // Publish topic has to be kept here because it's not public in Adafruit_MQTT_Publish
  Adafruit_MQTT_Subscribe   *mqttSubscriber;                                      // Subscriber object
  void                      (*triggers[4])(Sonny *device, uint8_t index) = {0};   // Define firmware triggers on specific times between state changes, useful for buttons
} sonoffIO;

/*
 * Contains information for a LED
 */
typedef struct {
  uint8_t                   pin;                                  // Physical pin
  uint32_t                  dutyCycle = 0;                        // PWM duty cycle
} sonoffLED;

class Sonny {
public:
  Sonny(WiFiClient *wifiClient, SettingsManager *settings, uint8_t inputCount, uint8_t outputCount, uint8_t ledCount);
  static Sonny *setupDevice(WiFiClient *wifiClient, SettingsManager *settings);
  void addInputDevice(uint8_t index, uint8_t pin);
  void setInputTrigger(uint8_t index, uint8_t triggerIndex, void *trigger);
  void setInputTriggerPublishValue(uint8_t index, uint8_t triggerPublishState);
  void setOutputInverted(uint8_t index, bool inverted);
  void addOutputDevice(uint8_t index, uint8_t pin);
  void addLed(uint8_t index, uint8_t pin);
  void setLedState(uint8_t index, bool state);
  void setLedDutyCycle(uint8_t index, uint8_t percentage);
  sonoffIO *getOutputDevice(uint8_t index);
  sonoffIO *getInputDevice(uint8_t index);
  uint8_t getInputCount();
  uint8_t getOutputCount();

  void initialiseIO();
  void handleIO();

  void handleMQTT();

  bool getSetupMode();
  void setSetupMode(bool value);
  
  static void toggleOutputTrigger(Sonny *object, uint8_t index);
  static void resetConfigTrigger(Sonny *object, uint8_t index);
  static void countedOutputTrigger(Sonny *object, uint8_t index);

  void toggleOutput(uint8_t index);
  void resetConfig(uint8_t index);
  void countedOutput(uint8_t index);

  void logFormatted(Logger::logSeverity severity, char *format, ...);

  virtual uint8_t readInput(uint8_t index);
  virtual uint8_t readOutput(uint8_t index);
  virtual void writeOutput(uint8_t index, uint8_t value);
  virtual void readAll();
  virtual void writeAll();

protected:
  void addIoDevice(sonoffIO ** list, uint8_t index, uint8_t pin);
  bool connectMQTT();
  void tryMqttPublish(Adafruit_MQTT_Publish * publisher, bool value, bool state, int deltaTime);
  virtual void setupInput(uint8_t index);
  virtual void setupOutput(uint8_t index);

  WiFiClient                *wifiClient;
  uint8_t                   inputCount = 0;                       // Amount of inputs
  uint8_t                   outputCount = 0;                      // Amount of outputs
  uint8_t                   outputCounter = 0;                    // Counter for bit toggled outputs
  uint8_t                   outputLimitCounter = 0;               // Limit for counter for bit toggled outputs
  uint8_t                   ledCount = 0;                         // Amount of LEDs (ie blinking outputs)
  uint8_t                   ledCounter = 0;                       // Counter used for blinking pattern
  sonoffIO                  **inputs;                             // Array of input structs
  sonoffIO                  **outputs;                            // Array of output structs
  sonoffLED                 **leds;                               // Array of LED structs
  bool                      setupMode = false;                    // Device in setup mode
  SettingsManager           *settings;                            // Settings manager
  Adafruit_MQTT_Client      *mqtt;                                // MQTT connection
  Logger                    **loggers;                            // Debug and logging
  uint8_t                   loggerCount = 0;                      // Amount of loggers
  uint32_t                  pingInterval = 180000;                // Time that has to elapse between pings
  uint32_t                  lastPing;                             // Time of last ping
};

class SonnyS20 : public Sonny {
public:
  SonnyS20(WiFiClient *wifiClient, SettingsManager *settings);
};

class SonnyDual : public Sonny {
public:
  SonnyDual(WiFiClient *wifiClient, SettingsManager *settings);

  uint8_t readInput(uint8_t index);
  uint8_t readOutput(uint8_t index);
  void writeOutput(uint8_t index, uint8_t value);
  void readAll();
  void writeAll();

protected:
  void setupInput(uint8_t index);
  void setupOutput(uint8_t index);
  void (*stuckTriggers[2])(Sonny *device, uint8_t index) = {0};  // Define firmware triggers for stuck/unstuck inputs
};

#endif // SONNY_H
