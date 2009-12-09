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
#ifndef rssdlg_h
#define rssdlg_h

#include "ui_rssdlg.h"

#include <QXmlStreamReader>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QPointer>

class RSSDialog : public QDialog, public Ui::rssDialog {
    Q_OBJECT
public:
    RSSDialog(QWidget *parent=0);
private slots:
    void onFetch();
    void onFeedMgr();
    void onSelectAll();
    void onInvert();
    void replyFinished(QNetworkReply*);
public:
    void writeSettings();
private:
    QNetworkAccessManager *createManager();
    void handleRedirect(QNetworkReply *reply);
    void readSettings();
    void parseRSS(QNetworkReply *reply);
private:
    QXmlStreamReader xml;
    QPointer<QNetworkAccessManager> mgr;
    QUrl redirectedToURL;
};
#endif
