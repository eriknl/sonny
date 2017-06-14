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

#include "html.h"

/*
 * Constructor
 */
HtmlNode::HtmlNode(const __FlashStringHelper *type, String id) : type(type), id(id) {
  content += '<';
  content += type;
  content += F(" ");
  if (id.length() > 0) {
    addTagAttribute(F("id"), id);
  }
}

/*
 * Destructor
 */
HtmlNode::~HtmlNode() {
}

/*
 * Add tag attribute
 */
void HtmlNode::addTagAttribute(const __FlashStringHelper *tag, String value) {
  content += tag;
  content += "=\"";
  content += value;
  content += "\" ";
}

/*
 * Add tag attribute
 */
void HtmlNode::addTagAttribute(const __FlashStringHelper *tag, const __FlashStringHelper *value) {
  content += tag;
  content += "=\"";
  content += value;
  content += "\" ";
}

/*
 * Add a close tag
 */
void HtmlNode::addCloseTag() {
  content += "</";
  content += type;
  content += '>';
}

/*
 * Return HTML code
 */
String HtmlNode::toString() {
  return content;
}

/*
 * Constructor, sets up table header
 */
HtmlTable::HtmlTable(String id, int columnCount, const __FlashStringHelper ** columnNames) : HtmlNode(F("table"), id), columnCount(columnCount) {
  int i;
  content += F("\"><thead><tr>");
  for (i = 0; i < columnCount; i++) {
    content += F("<td>");;
    content += columnNames[i];
    content += F("</td>");
  }
  content += F("</tr></thead></tbody>");
}

/*
 * Add row to table with given values
 */
void HtmlTable::addRow(const String *columnValues) {
  int i;
  content += F("<tr>");
  for (i = 0; i < columnCount; i++) {
    content += F("<td>");
    content += columnValues[i];
    content += F("</td>");
  }
  content += F("</tr>");
}

/*
 * Plumbing to allow initializer lists for addRow function
 */
void HtmlTable::addRow(const std::initializer_list<String>& columnValues) {
  addRow(columnValues.begin());
}

/*
 * Return table HTML code
 */
String HtmlTable::toString() {
  content += F("</tbody>");
  addCloseTag();
  return content;
}

/*
 * Constructor, sets up form
 */
HtmlForm::HtmlForm(String id, const __FlashStringHelper *action) : HtmlNode(F("form"), id) {
  addTagAttribute(F("action"), action);
  addTagAttribute(F("method"), F("post"));
  content += '>';
}

/*
 * Adds a text type field to form
 */
void HtmlForm::addTextField(const __FlashStringHelper *fieldName, const __FlashStringHelper *description, int maxLength, String value, bool password) {
  content += F("<label>");
  content += description;
  content += F(":</label>");
  content += F("<input ");
  addTagAttribute(F("type"), password ? F("password") : F("text"));
  addTagAttribute(F("value"), value);
  addTagAttribute(F("name"), fieldName);
  addTagAttribute(F("size"), String(maxLength, DEC));
  content += F("><br />");
}

/*
 * Return table HTML code
 */
String HtmlForm::toString() {
  content += F("<input ");
  addTagAttribute(F("type"), F("submit"));
  content += '>';
  addCloseTag();
  return content;
}

/*
 * Build link
 */
HtmlLink::HtmlLink(String id, const __FlashStringHelper *text, const __FlashStringHelper *location) : HtmlNode(F("a"), id) {
  addTagAttribute(F("href"), location);
  content += F(">");
  content += text;
  addCloseTag();
}

