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

#include <QNetworkRequest>
//#include <QDebug>

#include "httpmgr.h"

QHttpManager::QHttpManager(QObject *parent)
    : QNetworkAccessManager(parent), errorFlag(false), pb(0)
{
    connect(this, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(onFinished(QNetworkReply*)));
}

void
QHttpManager::setProgressBar(QProgressBar *pb) {
    this->pb = pb;
}

void
QHttpManager::fetch(const QString& url) {
    data.clear();
    errorFlag = false;
    emit fetchLink(url);
    re = get( QNetworkRequest(url) );
    connectReplySignals();
}

void
QHttpManager::abort() {
    if (re && re->isOpen())
        re->abort();
}

QString
QHttpManager::getData() const {
    return data;
}

const bool&
QHttpManager::errorOccurred() const {
    return errorFlag;
}

void
QHttpManager::clearErrors() {
    errorFlag = false;
}

void
QHttpManager::connectReplySignals() {

    connect(re, SIGNAL(readyRead()),
        this, SLOT(onReadyRead()));

    connect(re, SIGNAL(error(QNetworkReply::NetworkError)),
        this, SLOT(onError(QNetworkReply::NetworkError)));

    connect(re, SIGNAL(downloadProgress(qint64, qint64)),
        this, SLOT(onProgress(qint64, qint64)));
}

void
QHttpManager::resetProgressBar() {
    if (pb) {
        if (pb->maximum() == 0)
            pb->setMaximum(1);
        pb->reset();
    }
}

void
QHttpManager::onFinished(QNetworkReply *reply) {
    resetProgressBar();
    reply->deleteLater();
    re->deleteLater();
    emit fetchFinished();
}

void
QHttpManager::onReadyRead() {
    QUrl redir =
        re->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

    if (redir.isEmpty()) {
        const int httpcode =
            re->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (httpcode == 200)
            data += QString::fromLocal8Bit(re->readAll());
        else {
            errorFlag = true;
            emit fetchError(
                QString("httpcode %1: %2")
                    .arg(httpcode)
                    .arg(re->errorString())
            );
            re->deleteLater();
            emit fetchFinished();
//            qDebug() << __func__ << "httpcode:" << httpcode;
        }
    }
    else {
        emit fetchLink(redir.toString());
        resetProgressBar();
        QNetworkReply *tmp = get( QNetworkRequest(redir) );
        re->deleteLater();
        re = tmp;
        connectReplySignals();
    }
}

void
QHttpManager::onError(QNetworkReply::NetworkError) {
    errorFlag = true;
    emit fetchError(re->errorString());
}

void
QHttpManager::onProgress(qint64 received, qint64 expected) {
    if (pb) {
        if (expected == -1)
            pb->setMaximum(0);
        pb->setValue(received);
    }
}


