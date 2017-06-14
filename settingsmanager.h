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

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include "FS.h"

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

class SettingsManager {
public:
  SettingsManager(const __FlashStringHelper *filename);
  void addSettingString(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, const char *defaultValue, uint8_t settingLength);
  void addSettingPassword(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, char *defaultValue, uint8_t settingLength);
  void addSettingBool(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, bool defaultValue);
  void addSettingInteger(sonoffSettingIndex index, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, int defaultValue);

  char *getSettingString(sonoffSettingIndex setting);
  void setSettingString(sonoffSettingIndex setting, char *value);
  bool getSettingBool(sonoffSettingIndex setting);
  void setSettingBool(sonoffSettingIndex setting, bool value);
  int getSettingInteger(sonoffSettingIndex setting);
  void setSettingInteger(sonoffSettingIndex setting, int value);

  sonoffSetting *getSetting(sonoffSettingIndex index);
  sonoffSetting *getSetting(uint8_t index);

  void restoreSettings();
  bool saveSettings(bool defaultValue);

private:
  void addSetting(sonoffSettingIndex index, sonoffSettingType settingType, bool visible, const __FlashStringHelper *settingName, const __FlashStringHelper *settingDescription, uint8_t settingLength);

  const __FlashStringHelper *filename;
  sonoffSetting settings[settingLast];
};

#endif // SETTINGSMANAGER_H
