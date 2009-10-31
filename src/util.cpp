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

#include <QString>
#include <QProcess>
#include <QMessageBox>
#include <QFile>
#include <QTranslator>

#include "util.h"

void
Util::verifyCclivePath(
    const QString& path,
    QString& ccliveVersion,
    QString& curlVersion,
    QString& curlMod,
    bool *isCcliveFlag/*=NULL*/)
{
    // Parses the c/clive --version output.

    if (!QFile::exists(path)) {
        throw NoCcliveException(path,
            QObject::tr("Invalid path to c/clive") );
    }

    ccliveVersion.clear();
    curlVersion.clear();

    QProcess proc;
    proc.setEnvironment(QStringList() << "CCLIVE_NO_CONFIG=1");
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(path, QStringList() << "--version");

    if (isCcliveFlag)
        *isCcliveFlag = false;

    if (!proc.waitForFinished()) {
        throw NoCcliveException(path, proc.errorString());
    }
    else {
        const QString output =
            QString::fromLocal8Bit( proc.readAll() );

        const int exitCode =
            proc.exitCode();

        if (exitCode == 0) {
            const QString versionOutput = output;
//            qDebug() << versionOutput;

            const QStringList tmp =
                versionOutput.split("\n", QString::SkipEmptyParts);

            bool enoughOutputFlag = false;

            if (tmp.size() >= 1) {

                const QStringList lst =
                    tmp[0].split(" ", QString::SkipEmptyParts);

                if (lst.size() >= 6) {
                    if (isCcliveFlag)
                        *isCcliveFlag  = (lst[0] == "cclive");
                    ccliveVersion = lst[2];
                    curlMod       = lst[4];
                    curlVersion   = lst[6];

                    enoughOutputFlag  = true;
                }
            }

            if (!enoughOutputFlag) {
                throw NoCcliveException(path,
                    QObject::tr("Not c/clive or an unsupported version of it"));
            }
        }
        else
            throw NoCcliveException(path, exitCode, output);
    }
}

static const QString defmsg =
    QObject::tr("Undefined path to c/clive command");

NoCcliveException::NoCcliveException(
    const QString& path,
    const QString& errmsg)
    : errmsg(defmsg)
{
    if (!path.isEmpty()) {
        this->errmsg =
            QString("%1:\n%2").arg(path).arg(errmsg);
    }
}

NoCcliveException::NoCcliveException(
    const QString& path,
    const int& exitCode,
    const QString& output)
    : errmsg(defmsg)
{
    if (!path.isEmpty()) {
        this->errmsg = QString(
            QObject::tr(
                "%1 exited with %2 and the following"
                "output:\n%3")
                    .arg(path)
                    .arg(exitCode)
                    .arg(output)
        );
    }
}

const QString&
NoCcliveException::what() const {
    return errmsg;
}


