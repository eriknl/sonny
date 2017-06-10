// Used for software timer (http://www.switchdoc.com/2015/10/iot-esp8266-timer-tutorial-arduino-ide/), hareware timer0 interferes with OTA
extern "C" {
#include "user_interface.h"
}

/*
 * Contains information for an I/O, including MQTT pub/subs
 * An output might have both a pub and a sub to ensure all changes are fed back to the broker
 */
typedef struct {
  int8_t                    pin;                                  // Physical pin
  int8_t                    lastState;                            // Last known state, in order not to publish the same state again
  int8_t                    triggerPublishState;                  // What triggers a publish, high or low?
  int                       lastStateTime;                        // When was the last state change
  Adafruit_MQTT_Publish     mqttPublisher;                        // Publisher object
  char                      *publishTopic;                        // Publish topic has to be kept here because it's not public in Adafruit_MQTT_Publish
  Adafruit_MQTT_Subscribe   mqttSubscriber;                       // Subscriber object
  void                      (*triggers[4])(int8_t index) = {0};   // Define firmware triggers on specific times between state changes, useful for buttons
} sonoffIO;

/*
 * Contains information for a LED
 */
typedef struct {
  int8_t                    pin;                                  // Physical pin
  int8_t                    divisor = 0;                          // Divisor for blinking
} sonoffLED;

/*
 * Device specific information, set in setup routine with device specific data
 */
typedef struct {
  int8_t                    inputCount = 0;                       // Amount of inputs
  int8_t                    outputCount = 0;                      // Amount of outputs
  int8_t                    ledCount = 0;                         // Amount of LEDs (ie blinking outputs)
  int8_t                    ledCounter = 0;                       // Counter used for blinking pattern
  sonoffIO                  **inputs;                             // Array of input structs
  sonoffIO                  **outputs;                            // Array of output structs
  sonoffLED                 **leds;                               // Array of LED structs
  os_timer_t                timer;                                // Software timer
  bool                      setupMode = false;                    // Device in setup mode
} sonoffDevice;
sonoffDevice device;

/*
 * Identifiers for settings to be stored in flash, to be backwards compatible just add new settings before settingLast
 */
typedef enum {
  settingSSID = 0,
  settingPSK,
  settingHostname,
  settingReset,
  settingMqttHost,
  settingMqttPort,
  settingMqttUsername,
  settingMqttPassword,
  settingMqttHostFingerprint,
  settingLast
} sonoffSettingIndex;

/*
 * Types of settings to be stored
 */
typedef enum {
  typeString = 0,
  typePassword,
  typeBool,
  typeInteger,
//  typeHexString
} sonoffSettingType;

/*
 * Memory representation of a setting as read from EEPROM
 */
typedef struct {
  const __FlashStringHelper *settingName;
  const __FlashStringHelper *settingDescription;
  uint8_t                   *settingValue;
  uint8_t                   *settingDefaultValue;
  uint8_t                   settingLength;
  uint16_t                  offset;
  uint8_t                   visible = 1;
  uint8_t                   settingType;
} sonoffSetting;

