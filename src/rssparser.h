/* 
* Copyright (C) 2010 Toni Gundogdu.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef rssparser_h
#define rssparser_h

#include <QXmlStreamReader>
#include <QLinkedList>
#include <QString>

class RSSItem;
class RSSParserException;

class RSSItem {
public:
    QString title;
    QString link;
    QString description;
    QString pubDate;
};

class RSSFeed {
public:
    QString title;
    QString description;
    QString link;
    QString language;
    QString generator;
    QString copyright;
    QString imageUrl;
    QString imageTitle;
    QString imageLink;
    QLinkedList<RSSItem*> items;
};

class RSSParser {
public:
    RSSParser();
    virtual ~RSSParser();
public:
    void parse(const QString& data);
    const RSSFeed* getFeed() const;
private:
    void atStartElement();
    void atEndElement();
    void atChar();
private:
    QXmlStreamReader xml;
    RSSItem *currItem;
    RSSFeed *feed;
    QString text;
    bool is_image;
};

class RSSParserException {
public:
    RSSParserException(const QXmlStreamReader& xml);
public:
    QString what;
    qint64 lineNumber;
    QXmlStreamReader::Error errorCode;
};

#endif


