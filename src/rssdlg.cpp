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
#include <QMessageBox>

#include "rssdlg.h"

RSSDialog::RSSDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    mgr = createManager();
    itemsTree->setColumnHidden(1, true);
}

void
RSSDialog::onFetch() {
    QString links = linkEdit->text();

    if (links.isEmpty())
        return;

    itemsTree->clear();
    xml.clear();

    QNetworkRequest req(links);
    mgr->get(req);
}

void
RSSDialog::onAbort() {
}

void
RSSDialog::replyFinished(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QVariant tmp =
            reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        redirectUrl = 
            redirect(tmp.toUrl(), redirectUrl);

        if (!redirectUrl.isEmpty())
            mgr->get(QNetworkRequest(redirectUrl));
        else {
            redirectUrl.clear();
            xml.addData(reply->readAll());
            parseRSS();
        }
        reply->deleteLater();
    }
    else {
        QMessageBox mb;
        mb.setText("Network error occurred.");
        mb.setInformativeText(reply->errorString());
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setDefaultButton(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.exec();
    }
}

void
RSSDialog::parseRSS() {
    QString rssTitle, link, tag, title, pubdate;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "item")
                link = xml.attributes().value("rss::about").toString();
            tag = xml.name().toString();
        } else if (xml.isEndElement()) {
            if (xml.name() == "item") {
                if (!rssTitle.isEmpty()) {
                    QTreeWidgetItem *item = new QTreeWidgetItem;
                    item->setCheckState(0, Qt::Unchecked);
                    item->setText(0, title);
                    item->setText(1, link);
                    itemsTree->addTopLevelItem(item);
                }
                else
                    rssTitle = title;
                title.clear();
                link.clear();
                pubdate.clear();
            }
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (tag == "title")
                title += xml.text().toString();
            else if (tag == "link") 
                link += xml.text().toString();
            else if (tag == "pubDate")
                pubdate += xml.text().toString();
        }
    }

    if (xml.error()
        && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError)
    {
        QMessageBox mb;
        mb.setText("XML parsing error");
        mb.setInformativeText(xml.lineNumber() +":"+ xml.errorString());
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setDefaultButton(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.exec();
    }
}

QNetworkAccessManager*
RSSDialog::createManager() {
    QNetworkAccessManager *p = new QNetworkAccessManager(this);

    connect(p, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(replyFinished(QNetworkReply*)));

    return p;
}

QUrl
RSSDialog::redirect(const QUrl& to, const QUrl& from) const {
    QUrl redirectTo;
    if (!to.isEmpty() && to != from) {
        redirectTo = to;
    }
    return redirectTo;
}
