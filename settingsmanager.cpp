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

#include "settingsmanager.h"

SettingsManager::SettingsManager(const __FlashStringHelper *filename) : filename(filename) {
  
}

sonoffSetting *SettingsManager::getSetting(sonoffSettingIndex index) {
  return &settings[index];
}

sonoffSetting *SettingsManager::getSetting(uint8_t index) {
  return &settings[(sonoffSettingIndex)index];
}

/*
 * Add setting to settingsmanager
 */
void SettingsManager::addSetting(sonoffSettingIndex index, sonoffSettingType settingType, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, uint8_t settingLength) {
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
void SettingsManager::addSettingString(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, const char *defaultValue, uint8_t settingLength) {
  addSetting(index, typeString, visible, settingName, settingDescription, settingLength);
  setSettingString(index, (char *)defaultValue);
  memcpy(settings[index].settingDefaultValue, settings[index].settingValue, settingLength);
}

/*
 * Add password setting to settingsmanager, wrapper for addSetting
 */
void SettingsManager::addSettingPassword(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, char *defaultValue, uint8_t settingLength) {
  addSetting(index, typePassword, visible, settingName, settingDescription, settingLength);
  setSettingString(index, defaultValue);
  memcpy(settings[index].settingDefaultValue, settings[index].settingValue, settingLength);
}

/*
 * Add bool setting to settingsmanager, wrapper for addSetting
 */
void SettingsManager::addSettingBool(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, bool defaultValue) {
  addSetting(index, typeBool, visible, settingName, settingDescription, 1);
  setSettingBool(index, defaultValue);
  memcpy(settings[index].settingDefaultValue, settings[index].settingValue, 1);
}

/*
 * Add integer setting to settingsmanager, wrapper for addSetting
 */
void SettingsManager::addSettingInteger(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, int defaultValue) {
  addSetting(index, typeInteger, visible, settingName, settingDescription, 4);
  setSettingInteger(index, defaultValue);
  memcpy(settings[index].settingDefaultValue, settings[index].settingValue, 4);
}

/*
 * Get string value of setting
 */
char *SettingsManager::getSettingString(sonoffSettingIndex setting) {
  return (char *)settings[setting].settingValue;
}

/*
 * Set string value of setting
 */
void SettingsManager::setSettingString(sonoffSettingIndex setting, char *value) {
  memset(settings[setting].settingValue, 0x00, settings[setting].settingLength);
  strncpy((char *)settings[setting].settingValue, value, strlen(value));
}

/*
 * Get bool value of setting
 */
bool SettingsManager::getSettingBool(sonoffSettingIndex setting) {
  return (settings[setting].settingValue[0] ? true : false);
}

/*
 * Set bool value of setting
 */
void SettingsManager::setSettingBool(sonoffSettingIndex setting, bool value) {
  settings[setting].settingValue[0] = (value ? 0x01 : 0x00);
}

/*
 * Get integer value of setting
 */
int SettingsManager::getSettingInteger(sonoffSettingIndex setting) {
  return *(int *)(settings[setting].settingValue);
}

/*
 * Set bool value of setting
 */
void SettingsManager::setSettingInteger(sonoffSettingIndex setting, int value) {
  *(int *)(settings[setting].settingValue) = value;
}

/*
 * Attempt to restore known values from flash
 */
void SettingsManager::restoreSettings() {
  int8_t i;
  SPIFFS.begin();
  File f = SPIFFS.open(filename, "r");
  if (!f) {
    Serial.println("file open failed");
    saveSettings(true);
    restoreSettings();
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
bool SettingsManager::saveSettings(bool defaultValue) {
  int8_t i;
  SPIFFS.begin();
  File f = SPIFFS.open(filename, "w");
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
