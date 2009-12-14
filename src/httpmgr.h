/* 
* Copyright (C) 2009 Toni Gundogdu.
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

#ifndef httpmgr_h
#define httpmgr_h

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QProgressBar>

class QHttpManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    QHttpManager(QObject *parent);
public:
    void setProgressBar(QProgressBar *pb);
    void fetch(const QString& url);
    void abort();
    QString getData() const;
    const bool& errorOccurred() const;
    void clearErrors();
private:
    QPointer<QNetworkReply> re;
    bool errorFlag;
    QProgressBar *pb;
    QString data;
private slots:
    void onFinished(QNetworkReply *reply);
    void onReadyRead();
    void onError(QNetworkReply::NetworkError code);
    void onProgress(qint64 received, qint64 expected);
private:
    void connectReplySignals();
    void resetProgressBar();
signals:
    void fetchLink(QString url);
    void fetchFinished();
    void fetchError(QString errorString);
};
#endif


