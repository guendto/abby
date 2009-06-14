/*
 * abby Copyright (C) 2009 Toni Gundogdu.
 * This file is part of abby.
 *
 * abby is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * abby is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QDialog>
#include <QDebug>

#include "rssdlg.h"

RSSDialog::RSSDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
}

void
RSSDialog::onFetch() {
    QString links = linkEdit->toPlainText();

    if (links.isEmpty())
        return;

    xml.clear();
}

void
RSSDialog::onAbort() {
}

void
RSSDialog::parseRSS() {
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "item")
                linkString = xml.attributes().value("rss::about").toString();
            currentTag = xml.name().toString();
        } else if (xml.isEndElement()) {
            if (xml.name() == "item") {
                QTreeWidgetItem *item = new QTreeWidgetItem;
                item->setText(0, titleString);
                item->setText(1, "");
                item->setText(2, linkString);
                itemsTree->addTopLevelItem(item);

                titleString.clear();
                linkString.clear();
            }
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (currentTag == "title")
                titleString += xml.text().toString();
            else if (currentTag == "link") 
                linkString += xml.text().toString();
        }
    }

    if (xml.error()
        && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError)
    {
        qWarning() << "xml error: "
                   << xml.lineNumber()
                   << ": "
                   << xml.errorString();
    }
}
