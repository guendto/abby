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
#include <QSettings>

#include "rssdlg.h"
#include "feedmgrdlg.h"

RSSDialog::RSSDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();
    mgr = createManager();
    itemsTree->setColumnHidden(1, true);
}

void
RSSDialog::onFetch() {
    QString lnk = linkEdit->text();

    if (lnk.isEmpty())
        return;

    itemsTree->clear();

    fetchButton->setEnabled(false);
    buttonBox->setEnabled(false);

    QNetworkRequest req(lnk);
    req.setRawHeader("User-Agent", "Mozilla/5.0");
    mgr->get(req);
}

void
RSSDialog::onFeedMgr() {
    FeedMgrDialog *p = new FeedMgrDialog(this);
    if (p->exec() == QDialog::Accepted) {
        QList<QListWidgetItem*> sel = p->feedsList->selectedItems();
        if (sel.size() > 0)
            linkEdit->setText(sel[0]->text());
        p->writeSettings();
    }
}

void
RSSDialog::replyFinished(QNetworkReply* reply) {

    if (reply->error() == QNetworkReply::NoError) {
        QVariant tmp =
            reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        redirectUrl = 
            redirect(tmp.toUrl(), redirectUrl);

        if (!redirectUrl.isEmpty()) {
            QNetworkRequest req(redirectUrl);
            req.setRawHeader("User-Agent", "Mozilla/5.0");
            mgr->get(req);
        }
        else {
            redirectUrl.clear();
            xml.addData(reply->readAll());
            parseRSS();
        }
        reply->deleteLater();
    }
    else {
        QMessageBox::critical(this, QCoreApplication::applicationName(),
            QString(tr("Network error: %1")).arg(reply->errorString()));
    }

    fetchButton->setEnabled(true);
    buttonBox->setEnabled(true);
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
        QMessageBox::critical(this, QCoreApplication::applicationName(),
            QString(tr("XML parsing error:%1:%1"))
                .arg(xml.lineNumber())
                .arg(xml.errorString()));
    }
    xml.clear();
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
    if (!to.isEmpty() && to != from)
        redirectTo = to;
    return redirectTo;
}

void
RSSDialog::writeSettings() {
    QSettings s;
    s.beginGroup("RSSDialog");
    s.setValue("size", size());
    s.endGroup();
}

void
RSSDialog::readSettings() {
    QSettings s;
    s.beginGroup("RSSDialog");
    resize( s.value("size", QSize(514,295)).toSize() );
    s.endGroup();
}
