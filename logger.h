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

#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define LOGGER_LINELENGTH 128

class Logger {
public:
  typedef enum {
    severityDebug = 0,
    severityInfo,
    severityWarning,
    severityError
  } logSeverity;

  Logger();
  ~Logger();
  virtual void logFormattedVa(logSeverity severity, char *format, va_list args);
protected:
  static char *format(logSeverity severity, char *format, va_list args);
};

class SerialLogger : public Logger {
public:
  SerialLogger(int baudrate);
  ~SerialLogger();
  void logFormattedVa(logSeverity severity, char *format, va_list args);
};

class UdpLogger : public Logger {
public:
  UdpLogger(const char *host, uint16_t port);
  void logFormattedVa(logSeverity severity, char *format, va_list args);
private:
  WiFiUDP UDP;
  const char *host;
  uint16_t port;
};

#endif // LOGGER_H
