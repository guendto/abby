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
#include <QSettings>
//#include <QDebug>
#include <QMessageBox>
#include <QRegExp>

#include "scandlg.h"

ScanDialog::ScanDialog(QWidget *parent)
    : QDialog(parent), titleMode(false)
{
    setupUi(this);

    readSettings();

    mgr = createManager();

    itemsTree->setColumnHidden(1, true);
}

void
ScanDialog::onScan() {
    QString lnk = linkEdit->text();

    lnk = lnk.trimmed();

    if (lnk.isEmpty())
        return;

    if (!lnk.startsWith("http://", Qt::CaseInsensitive))
        lnk.insert(0,"http://");

    itemsTree->clear();

    titleMode = false;

    linkEdit->setEnabled    (false);
    scanButton->setEnabled  (false);
    titlesBox->setEnabled   (false);
    buttonBox->setEnabled   (false);

    mgr->get(QNetworkRequest(lnk));
}

void
ScanDialog::replyFinished(QNetworkReply* reply) {

    bool state = false;

    if (reply->error() == QNetworkReply::NoError) {

        handleRedirect(reply);

        if (!redirectedToURL.isEmpty())
            mgr->get( QNetworkRequest(redirectedToURL) );
        else {
            if (!titleMode)
                scanContent(reply);
            else
                parseHtmlTitle(reply);
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
        linkEdit->setEnabled    (state);
        scanButton->setEnabled  (state);
        titlesBox->setEnabled   (state);
        buttonBox->setEnabled   (state);
    }

    reply->deleteLater();
}

static QStringList
matchScanContent (const QStringList& lst, QRegExp& re, const QString& content) {

    re.setCaseSensitivity(Qt::CaseInsensitive);
    re.setMinimal(true);

    QStringList matches;

    int pos = 0;
    while ((pos = re.indexIn(content, pos)) != -1) {
        if (!matches.contains(re.cap(1)) && !lst.contains(re.cap(1)))
            matches << re.cap(1);
        pos += re.matchedLength();
    }
    return matches;
}

typedef unsigned int _uint;

#ifdef _1_
static void
dumpScanMatches (const QStringList& lst) {
    const register _uint size = lst.size();
    for (register _uint i=0; i<size; ++i)
        qDebug() << lst[i];
    qDebug() << "total: " << lst.size();
}
#endif

static void
scanYoutubeEmbed(QStringList& lst, const QString& content) {
    QRegExp re("\\/v\\/(.*)[\"&\n]");
    QStringList matches = matchScanContent(lst, re, content);
    //dumpScanMatches(matches);
    lst << matches;
}

static void
scanYoutubeRegular(QStringList& lst, const QString& content) {
    QRegExp re("\\/watch\\?v=(.*)[\"&\n]");
    QStringList matches = matchScanContent(lst, re, content);
    //dumpScanMatches(matches);
    lst << matches;
}

void
ScanDialog::scanContent(QNetworkReply *reply) {

    const QString content = QString::fromLocal8Bit(reply->readAll());

    QStringList IDs, links;

    scanYoutubeEmbed    (IDs, content);
    scanYoutubeRegular  (IDs, content);

    const register _uint ids_size = IDs.size();
    register _uint i;

    for (i=0; i<ids_size; ++i)
        links << "http://www.youtube.com/watch?v="+IDs[i];

    const register _uint links_size = links.size();

    if (titlesBox->checkState()) {
        titleMode = true;
        for (i=0; i<links_size; ++i)
            mgr->get( QNetworkRequest(links[i]) );
    }
    else {
        for (i=0; i<links_size; ++i) {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setCheckState(0, Qt::Unchecked);
            item->setText(0, links[i]);
            item->setText(1, links[i]);
            itemsTree->addTopLevelItem(item);
        }
    }
}

void
ScanDialog::parseHtmlTitle(QNetworkReply *reply) {

    const QString content = QString::fromLocal8Bit(reply->readAll());
    const QString link = reply->url().toString();

    QRegExp re("<title>(.*)<\\/title>"); // TODO: improve.
    re.setCaseSensitivity(Qt::CaseInsensitive);
    re.setMinimal(true);
    re.indexIn(content);

    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setCheckState(0, Qt::Unchecked);
    item->setText(0, re.cap(1));
    item->setText(1, link);
    itemsTree->addTopLevelItem(item);
}

QNetworkAccessManager*
ScanDialog::createManager() {
    QNetworkAccessManager *p = new QNetworkAccessManager(this);

    connect(p, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(replyFinished(QNetworkReply*)));

    return p;
}

void
ScanDialog::handleRedirect(const QNetworkReply *reply) {

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
ScanDialog::writeSettings() {
    QSettings s;
    s.beginGroup("ScanDialog");
    s.setValue("size", size());
    s.endGroup();
}

void
ScanDialog::readSettings() {
    QSettings s;
    s.beginGroup("ScanDialog");
    resize( s.value("size", QSize(514,295)).toSize() );
    s.endGroup();
}
