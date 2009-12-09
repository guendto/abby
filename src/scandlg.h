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
#ifndef scandlg_h
#define scandlg_h

#include "ui_scandlg.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QPointer>

class QDialog;

class ScanDialog : public QDialog, public Ui::scanDialog {
    Q_OBJECT
public:
    ScanDialog(QWidget *parent);
private slots:
    void onScan();
    void onSelectAll();
    void onInvert();
    void replyFinished(QNetworkReply*);
    void scanComplete();
public:
    void writeSettings();
private:
    void readSettings();

    QNetworkAccessManager *createManager();
    void handleRedirect(const QNetworkReply *reply);

    void parseHtmlTitle(QNetworkReply *reply);
    void scanContent(QNetworkReply *reply);
private:
    QPointer<QNetworkAccessManager> mgr;
    QUrl redirectedToURL;
    bool titleMode;
    bool scanFlag;
    int scannedPages;
    int expectedScans;
};

#endif

