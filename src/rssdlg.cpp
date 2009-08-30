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

    lnk = lnk.trimmed();

    if (lnk.isEmpty())
        return;

    if (!lnk.startsWith("http://", Qt::CaseInsensitive))
        lnk.insert(0,"http://");

    itemsTree->clear();

    linkEdit->setEnabled     (false);
    feedmgrButton->setEnabled(false);
    fetchButton->setEnabled  (false);
    buttonBox->setEnabled    (false);

    mgr->get(QNetworkRequest(lnk));
}

void
RSSDialog::onFeedMgr() {
    FeedMgrDialog *p = new FeedMgrDialog(this);

    if (p->exec() == QDialog::Accepted) {

        QList<QListWidgetItem*> sel =
            p->feedsList->selectedItems();

        if (sel.size() > 0)
            linkEdit->setText(sel[0]->text());

        p->writeSettings();
    }
}

void
RSSDialog::replyFinished(QNetworkReply* reply) {

    bool state = false;

    if (reply->error() == QNetworkReply::NoError) {

        handleRedirect(reply);

        if (!redirectedToURL.isEmpty())
            mgr->get( QNetworkRequest(redirectedToURL) );
        else {
            parseRSS(reply);
            redirectedToURL.clear();
            state = true;
        }
    }
    else {
        QMessageBox::critical(this, QCoreApplication::applicationName(),
            QString(tr("Network error: %1")).arg(reply->errorString()));
        state = true;
    }

    if (state) {
        linkEdit->setEnabled     (state);
        feedmgrButton->setEnabled(state);
        fetchButton->setEnabled  (state);
        buttonBox->setEnabled    (state);
    }
    
    reply->deleteLater();
}

void
RSSDialog::parseRSS(QNetworkReply *reply) {

    QString rssTitle, link, tag, title;//, pubdate;

    xml.clear();
    xml.addData(QString::fromLocal8Bit(reply->readAll()));

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

                title  .clear();
                link   .clear();
                //pubdate.clear();
            }
        } else if (xml.isCharacters() && !xml.isWhitespace()) {

            if (tag == "title")
                title += xml.text().toString();

            else if (tag == "link") 
                link += xml.text().toString();
/*
            else if (tag == "pubDate")
                pubdate += xml.text().toString();*/
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
}

QNetworkAccessManager*
RSSDialog::createManager() {
    QNetworkAccessManager *p = new QNetworkAccessManager(this);

    connect(p, SIGNAL(finished(QNetworkReply *)),
        this, SLOT(replyFinished(QNetworkReply *)));

    return p;
}

void
RSSDialog::handleRedirect(QNetworkReply *reply) {

//    QUrl location =
//      reply->header(QNetworkRequest::LocationHeader).toUrl();

    QUrl location =
        reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

    if (!location.isEmpty() && location != redirectedToURL)
        redirectedToURL = location;
    else
        redirectedToURL.clear();
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
