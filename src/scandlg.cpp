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
#include <QDebug>
#include <QMessageBox>
#include <QRegExp>

#include "scandlg.h"

#define USERAGENT "Mozilla/5.0"

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

    QNetworkRequest req(lnk);
    req.setRawHeader("User-Agent", USERAGENT);
    mgr->get(req);
}

void
ScanDialog::replyFinished(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QVariant tmp =
            reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        redirectUrl = 
            redirect(tmp.toUrl(), redirectUrl);

        if (!redirectUrl.isEmpty()) {
            QNetworkRequest req(redirectUrl);
            req.setRawHeader("User-Agent", USERAGENT);
            mgr->get(req);
        }
        else {
            if (!titleMode)
                scanContent(reply->readAll());
            else
                parseHtmlTitle(reply->readAll(), reply->url().toString());
            redirectUrl.clear();
        }
        reply->deleteLater();
    }
    else {
        QMessageBox::critical(this, QCoreApplication::applicationName(),
            QString(tr("Network error: %1")).arg(reply->errorString()));
    }

    linkEdit->setEnabled    (true);
    scanButton->setEnabled  (true);
    titlesBox->setEnabled   (true);
    buttonBox->setEnabled   (true);
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

#ifdef _1_
static void
dumpScanMatches (const QStringList& lst) {
    for (register int i=0; i<lst.size(); ++i)
        qDebug() << lst[i];
    qDebug() << "total: " << lst.size();
}
#endif

static void
scanYoutubeEmbed(QStringList& lst, const QString& content) {
    QRegExp re("\\/v\\/(.*)[\"&]");
    QStringList matches = matchScanContent(lst, re, content);
    //dumpScanMatches(matches);
    lst << matches;
}

static void
scanYoutubeRegular(QStringList& lst, const QString& content) {
    QRegExp re("\\/watch\\?v=(.*)[\"&]");
    QStringList matches = matchScanContent(lst, re, content);
    //dumpScanMatches(matches);
    lst << matches;
}

void
ScanDialog::scanContent(const QString& content) {
    QStringList IDs, links;
    scanYoutubeEmbed    (IDs, content);
    scanYoutubeRegular  (IDs, content);

    register int i;
    for (i=0; i<IDs.size(); ++i)
        links << "http://www.youtube.com/watch?v="+IDs[i];

    if (titlesBox->checkState()) {
        titleMode = true;
        for (i=0; i<links.size(); ++i) {
            QNetworkRequest req(links[i]);
            req.setRawHeader("User-Agent", USERAGENT);
            mgr->get(req);
        }
    }
    else {
        for (i=0; i<links.size(); ++i) {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setCheckState(0, Qt::Unchecked);
            item->setText(0, links[i]);
            item->setText(1, links[i]);
            itemsTree->addTopLevelItem(item);
        }
    }
}

void
ScanDialog::parseHtmlTitle(const QString& content, const QString& link) {
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

QUrl
ScanDialog::redirect(const QUrl& to, const QUrl& from) const {
    QUrl redirectTo;
    if (!to.isEmpty() && to != from)
        redirectTo = to;
    return redirectTo;
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
