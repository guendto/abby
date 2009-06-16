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

ScanDialog::ScanDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();
    mgr = createManager();
    itemsTree->setColumnHidden(1, true);
}

void
ScanDialog::onScan() {
    QString lnk = linkEdit->text();

    if (lnk.isEmpty())
        return;

    itemsTree->clear();

    scanButton->setEnabled(false);
    buttonBox->setEnabled(false);

    QNetworkRequest req(lnk);
    req.setRawHeader("User-Agent", "Mozilla/5.0");
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
            req.setRawHeader("User-Agent", "Mozilla/5.0");
            mgr->get(req);
        }
        else {
            redirectUrl.clear();
            scanHTML(reply->readAll());
        }
        reply->deleteLater();
    }
    else {
        QMessageBox mb(this);
        mb.setText("Network error occurred");
        mb.setInformativeText(reply->errorString());
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setDefaultButton(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.exec();
    }

    scanButton->setEnabled(true);
    buttonBox->setEnabled(true);
}

void
ScanDialog::scanHTML(QString html) {
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
