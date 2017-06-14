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

#include "logger.h"

Logger::Logger() {
  
}

/*
 * Format for output - be sure to free() allocated string!
 */
char *Logger::format(logSeverity severity, char *format, va_list args) {
  const char *severityStrings[4] = {
    "DEBUG", "INFO", "WARNING", "ERROR"
  };
  char *buffer = (char *)malloc(LOGGER_LINELENGTH);
  sprintf(buffer, "[%s]: ", severityStrings[severity]);
  vsnprintf(buffer + strlen(buffer), LOGGER_LINELENGTH - strlen(buffer) + 1, format, args);
  return buffer;
}

SerialLogger::SerialLogger(int baudrate) : Logger() {
  Serial.begin(baudrate);
}

SerialLogger::~SerialLogger() {
  Serial.end();
}

/*
 * Write on serial port
 */
void SerialLogger::logFormattedVa(logSeverity severity, char *format, va_list args) {
  char *formattedText = Logger::format(severity, format, args);
  Serial.print(formattedText);
  free(formattedText);
}

UdpLogger::UdpLogger(const char *host, uint16_t port) : Logger(), port(port) {
  this->host = strdup(host);
}

/*
 * Write to LumberLog host
 */
void UdpLogger::logFormattedVa(logSeverity severity, char *format, va_list args) {
  char *formattedText = Logger::format(severity, format, args);
  UDP.beginPacket(host, port);
  UDP.write(formattedText);
  UDP.endPacket();
  free(formattedText);
}
