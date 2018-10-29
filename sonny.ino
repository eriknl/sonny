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

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
#include <ArduinoOTA.h>

#include "sonny.h"
#include "html.h"

ESP8266WebServer server(80);
WiFiClient client;

Sonny *device;
SettingsManager *settings;

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
  uint8_t i;
  page += pageHeader("Configure Sonny");
  switch (server.method()) {
    case HTTP_POST:
      for (i = 0; i < settingLast; i++) {
        if (settings->getSetting(i)->visible) {
          switch (settings->getSetting(i)->settingType) {
            case typeString:
            case typePassword:
              settings->setSettingString((sonoffSettingIndex)i, (char *)server.arg(settings->getSetting(i)->settingName).c_str());
            break;
            case typeBool:

            break;
            case typeInteger:
              settings->setSettingInteger((sonoffSettingIndex)i, server.arg(settings->getSetting(i)->settingName).toInt());
            break;
          }
        }
      }
      settings->setSettingBool(settingReset, false);
      if (settings->saveSettings(false)) {
        page += F("Settings saved<br />");
      } else {
        page += F("Error saving settings<br />");
      }
    case HTTP_GET:
      HtmlForm settingsForm("settings", F("configure"));
      for (i = 0; i < settingLast; i++) {
        if (settings->getSetting(i)->visible) {
          switch (settings->getSetting(i)->settingType) {
            case typeString:
            case typePassword:
            case typeInteger:
              settingsForm.addTextField(settings->getSetting(i)->settingName, settings->getSetting(i)->settingDescription, settings->getSetting(i)->settingLength, (settings->getSetting(i)->settingType == typeInteger ? String(settings->getSettingInteger((sonoffSettingIndex)i), DEC) : settings->getSettingString((sonoffSettingIndex)i)), (settings->getSetting(i)->settingType == typePassword ? true : false));
            break;
            case typeBool:

            break;
          }
        }
      }
      page += "<h2>Settings</h2><p>";
      page += settingsForm.toString();
      page += (settings->getSettingBool(settingReset) ? "Reset" : "No reset");
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
      page += "<h2>Inputs</h2><p>";
      HtmlTable inputTable("inputTable", 4, inputTableHeaders);
      for (i = 0; i < device->getInputCount(); i++) {
        inputTable.addRow({String(i, DEC), String(device->getInputDevice(i)->lastState, DEC), String(device->getInputDevice(i)->publishTopic), String((millis() - device->getInputDevice(i)->lastStateTime), DEC)});
      }
      page += inputTable.toString();

      page += "<h2>Outputs</h2><p>";
      HtmlTable outputTable("outputTable", 5, outputTableHeaders);
      for (i = 0; i < device->getOutputCount(); i++) {
        outputTable.addRow({String(i, DEC), String(device->getOutputDevice(i)->lastState, DEC), String(device->getOutputDevice(i)->publishTopic), String(device->getOutputDevice(i)->mqttSubscriber->topic), String((millis() - device->getOutputDevice(i)->lastStateTime), DEC)});
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

/*
 * Setup device and libraries
 */
void setup(void){
  settings = new SettingsManager(F("/settings.dat"));
//  Serial.begin(115200);
//  Serial.println("");

  settings->addSettingString(settingSSID, true, F("ssid"), F("WiFi SSID"), "", 32);
  settings->addSettingPassword(settingPSK, true, F("psk"), F("WiFi PSK"), "", 32);
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String hostname = String("Sonny-") + String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  settings->addSettingString(settingHostname, true, F("host"), F("Device hostname"), hostname.c_str(), 32);
  settings->addSettingBool(settingReset, false, F("reset"), F("Reset device"), true);
  settings->addSettingString(settingMqttHost, true, F("mqtt_host"), F("MQTT broker hostname"), "127.0.0.1", 32);
  settings->addSettingInteger(settingMqttPort, true, F("mqtt_port"), F("MQTT broker port"), 1883);
  settings->addSettingString(settingMqttUsername, true, F("mqtt_user"), F("MQTT username"), "", 32);
  settings->addSettingString(settingMqttPassword, true, F("mqtt_key"), F("MQTT password"), "", 64);
  settings->addSettingString(settingMqttHostFingerprint, false, F("mqtt_host_fingerprint"), F("MQTT host SHA fingerprint"), "", 60);  // implemented later?
  settings->restoreSettings();
//  Serial.println("Complete");
  device = Sonny::setupDevice(&client, settings); // device specific configuration
  device->setLedDutyCycle(0, 50);           // show we're initialising

  if (settings->getSettingBool(settingReset)) {
//    Serial.println(F("Device was reset to system default settings"));
    // Setup AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostname.c_str());
    device->setSetupMode(true);
  } else {
    // Setup STA mode
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings->getSettingString(settingSSID), settings->getSettingString(settingPSK));
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      device->handleIO(); // Allow input triggers even when there is no connection
      yield();
    }
//    Serial.println("");
//    Serial.print(F("Connected to "));
//    Serial.println(settings->getSettingString(settingSSID));
//    Serial.print(F("IP address: "));
//    Serial.println(WiFi.localIP());
  }

  if (device->getSetupMode()) {
    server.onNotFound(wwwConfigure);
  } else {
    server.on("/configure", wwwConfigure);
    server.on("/control", wwwControl);
    server.on("/style.css", wwwStyle);
    server.onNotFound(wwwRoot);
  }

  server.begin();
//  Serial.println(F("HTTP server started"));

  ArduinoOTA.setHostname(settings->getSettingString(settingHostname));
  ArduinoOTA.onStart([]() {
    device->logFormatted(Logger::severityInfo, "Starting update OTA\r\n");
  });
  ArduinoOTA.onEnd([]() {
    device->logFormatted(Logger::severityInfo, "\nEnd of update OTA\r\n");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percentage = (progress / (total / 100));
    if (percentage == 100) {
      device->logFormatted(Logger::severityInfo, "Updating: %d%%\r\n", percentage);
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    device->setLedDutyCycle(0, 10);
    switch (error) {
      case OTA_AUTH_ERROR:
        device->logFormatted(Logger::severityError, "OTA Error[%u]: Auth failed\r\n", error);
      break;
      case OTA_BEGIN_ERROR:
        device->logFormatted(Logger::severityError, "OTA Error[%u]: Begin failed\r\n", error);
      break;
      case OTA_CONNECT_ERROR:
        device->logFormatted(Logger::severityError, "OTA Error[%u]: Connect failed\r\n", error);
      break;
      case OTA_RECEIVE_ERROR:
        device->logFormatted(Logger::severityError, "OTA Error[%u]: Receive failed\r\n", error);
      break;
      case OTA_END_ERROR:
        device->logFormatted(Logger::severityError, "OTA Error[%u]: End failed\r\n", error);
      break;
      default:
        device->logFormatted(Logger::severityError, "OTA Error[%u]: Unknown failure\r\n", error);
    }
  });
  ArduinoOTA.begin();
  device->logFormatted(Logger::severityInfo, "%s ready\r\n", settings->getSettingString(settingHostname));
}

/*
 * Main loop, handle incoming MQTT messages etc
 */

void loop(void){
  device->handleIO();
#ifndef SONNY_P1
  device->handleMQTT();
#endif
  server.handleClient();
  ArduinoOTA.handle();
}
