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

#ifndef HTML_H
#define HTML_H

#include <initializer_list>
#include <Arduino.h>

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
 * HTML form node class
 */
class HtmlForm : public HtmlNode {
public:
  HtmlForm(String id, const __FlashStringHelper *action);
  void addTextField(const __FlashStringHelper *fieldName, const __FlashStringHelper *description, int maxLength, String value, bool password);
  String toString();
};

/*
 * HTML link node class
 */
class HtmlLink : public HtmlNode {
public:
  HtmlLink(String id, const __FlashStringHelper *text, const __FlashStringHelper *location);
};

#endif // HTML_H
