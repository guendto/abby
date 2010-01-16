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

#include "rssparser.h"

RSSParser::RSSParser ()
    : currItem(0), feed(0), is_image(false)
{
}

RSSParser::~RSSParser() {
    if (feed) {
        while (!feed->items.isEmpty())
            delete feed->items.takeFirst();
        delete feed;
        feed = 0;
    }
    currItem = 0;
}

void
RSSParser::parse (const QString& data) {

    xml.clear();
    xml.addData(data);

    feed     = 0;
    currItem = 0;
    is_image = false;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement())
            atStartElement();
        else if (xml.isEndElement())
            atEndElement();
        else if (xml.isCharacters() && !xml.isWhitespace())
            atChar();
    }
    if (xml.error())
        throw RSSParserException(xml);
}

#define cmp(what) \
    (QString::compare(name,what,Qt::CaseInsensitive) == 0)

void
RSSParser::atStartElement() {

    const QString name = xml.name().toString();

    if (cmp("channel"))
        feed = new RSSFeed;

    else if (cmp("item") && feed) {
        currItem = new RSSItem;
        feed->items << currItem;
    }

    else if (cmp("image") && feed)
        is_image = true;
}

void
RSSParser::atEndElement() {

    if (!feed) return;

    const QString name = xml.name().toString();
    const QString tmp = text.simplified();

    if (cmp("item"))
        currItem = 0;

    else if (cmp("image"))
        is_image = false;

    else if (cmp("title")) {

        if (currItem)
            currItem->title = tmp;

        else if (is_image)
            feed->imageTitle = tmp;

        else
            feed->title = tmp;
    }

    else if (cmp("link")) {

        if (currItem)
            currItem->link = tmp;

        else if (is_image)
            feed->imageLink = tmp;

        else
            feed->link = tmp;
    }

    else if (cmp("description")) {

        if (currItem)
            currItem->description = tmp;

        else
            feed->description = tmp;
    }

    else if (cmp("url") && is_image)
        feed->imageUrl = tmp;

    else if (cmp("language"))
        feed->language = tmp;

    else if (cmp("generator"))
        feed->generator = tmp;

    else if (cmp("copyright"))
        feed->copyright = tmp;

    else if (cmp("pubDate") && currItem)
        currItem->pubDate = tmp;

#ifdef _1_
    else if (cmp("category") && currItem) { }
#endif

    text.clear();
}

void
RSSParser::atChar() {
    text += xml.text().toString();
}

RSSParserException::RSSParserException(
    const QXmlStreamReader& xml)
{
    what        = xml.errorString();
    lineNumber  = xml.lineNumber();
    errorCode   = xml.error();
}

const RSSFeed*
RSSParser::getFeed() const {
    return feed;
}


