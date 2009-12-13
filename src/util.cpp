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
#include <QDir>
#include <QTreeWidget>
#include <QTextEdit>
#include <QDateTime>
#include <QListWidget>
#include <QClipboard>
#include <QCoreApplication>
#include <QApplication>
//#include <QDebug>

#include "util.h"

static QString
check_path(const QStringList& paths, const QString& exec) {
    QList<QString>::const_iterator iter;
    QString path;

    for (iter=paths.constBegin();
         iter!=paths.constEnd();
         ++iter)
    {
        QString tmp =
            QString( "%1/%2" )
                .arg(QDir::fromNativeSeparators(*iter))
                .arg(exec);

        tmp = QDir::toNativeSeparators(tmp);

        if (QFile::exists(tmp)) {
            path = tmp;
            break;
        }
    }
    return path;
}

static void
verify_version_output(
    const QString& path,
    QString& version,
    QString& libVersion,
    QString& libName,
    bool *isCcliveFlag)
{

    version.clear();
    libVersion.clear();
    libName.clear();

    QProcess proc;
    proc.setEnvironment(QStringList() << "CCLIVE_NO_CONFIG=1");
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(path, QStringList() << "--version");

    if (isCcliveFlag)
        *isCcliveFlag = false;

    if (!proc.waitForFinished()) {
        throw NoCcliveException(path,
            proc.exitCode(), proc.errorString());
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

                    version    = lst[2];
                    libName    = lst[4];
                    libVersion = lst[6];

                    enoughOutputFlag  = true;
                }
            }

            if (!enoughOutputFlag) {
                throw NoCcliveException(path,
                    QObject::
                        tr("Not c/clive or it is an unsupported "
                            "version of it"));
            }
        }
        else
            throw NoCcliveException(path, exitCode, output);
    }
}

void
Util::detectCclive(
    QString& path,
    QString& version,
    QString& libVersion,
    QString& libName,
    bool *isCcliveFlag)
{

    const QStringList env =
        QProcess::systemEnvironment();

    QRegExp re("^PATH=(.*)", Qt::CaseInsensitive);
    env.indexOf(re);

    const QString capt = re.capturedTexts()[1].simplified();
    QStringList paths  = capt.split(":");

    if (paths.size() < 2) // w32
        paths = capt.split(";");

    if (paths.size() < 2)
        return;

    path = check_path(paths, "cclive");
    if (path.isEmpty())
        path = check_path(paths, "clive");

    if (path.isEmpty()) {
        // Detection from $PATH failed. Prompt user
        // to specify the path in preferences manually.
        throw NoCcliveException(
            QObject::tr(
                "Neither cclive or clive not was found in the path.\n"
                "Please specify the path to either command in the\n"
                "preferences."
            )
        );
    }
    else {
        // Check --version output.
        verify_version_output(
            path,
            version,
            libVersion,
            libName,
            isCcliveFlag
        );
    }
}

void
Util::verifyCclivePath(
    const QString& path,
    QString& version,
    QString& libVersion,
    QString& libName,
    bool *isCcliveFlag/*=NULL*/)
{
    verify_version_output(
        path,
        version,
        libVersion,
        libName,
        isCcliveFlag
    );
}

void
Util::checkAllItems(
    const QTreeWidget *w,
    const Qt::CheckState& st,
    const int column/*=0*/)
{
    QTreeWidgetItemIterator iter(
        const_cast<QTreeWidget*>(w),
        QTreeWidgetItemIterator::NotChecked
    );

    while (*iter) {
        if ((*iter)->childCount() == 0)
            (*iter)->setCheckState(column, st);
        ++iter;
    }
}

void
Util::invertAllCheckableItems(
    const QTreeWidget *w,
    const int column/*=0*/)
{
    QTreeWidgetItemIterator iter(const_cast<QTreeWidget*>(w));
    while (*iter) {
        if ((*iter)->childCount() == 0) {
            (*iter)->setCheckState(column,
                (*iter)->checkState(column) == Qt::Checked
                ? Qt::Unchecked
                : Qt::Checked
            );
        }
        ++iter;
    }
}

void
Util::appendLog(const QTextEdit* w, const QString& s) {
    QDateTime now = QDateTime::currentDateTime();
    QString dt = now.toString(Qt::ISODate);
    const_cast<QTextEdit*>(w)->append(dt+": "+s);
}

int
Util::countItems(const QTreeWidget *w) {
    int count = 0;
    QTreeWidgetItemIterator iter(const_cast<QTreeWidget*>(w));
    while (*iter) {
        ++count;
        ++iter;
    }
    return count;
}

void
Util::addItem(const QListWidget *w, QString lnk) {
    lnk = lnk.simplified();

    if (lnk.isEmpty())
        return;

    if (!lnk.startsWith("http://", Qt::CaseInsensitive))
        lnk.insert(0, "http://");

    QList<QListWidgetItem *> matched =
        w->findItems(lnk, Qt::MatchExactly);

    if (matched.size() == 0)
        const_cast<QListWidget*>(w)->addItem(lnk);
}

void
Util::removeSelectedItems(const QWidget *parent, const QListWidget *w) {
    QList<QListWidgetItem *> s = w->selectedItems();

    const int size = s.size();

    if (!size)
        return;

    if (QMessageBox::warning(
        const_cast<QWidget*>(parent),
        QCoreApplication::applicationName(),
        QObject::tr("Really remove the selected links?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
        == QMessageBox::No)
    {
        return;
    }

    for (int i=0; i<size; ++i)
        delete const_cast<QListWidget*>(w)->takeItem( w->row(s[i]) );
}

void
Util::clearItems(const QWidget *parent, const QListWidget *w) {
    if (w->count() == 0)
        return;

    if (QMessageBox::warning(
        const_cast<QWidget*>(parent),
        QCoreApplication::applicationName(),
        QObject::tr("Really clear list?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
        == QMessageBox::No)
    {
        return;
    }

    const_cast<QListWidget*>(w)->clear();
}

void
Util::paste(const QListWidget *w) {
    QClipboard *cb  = QApplication::clipboard();
    QStringList lst = cb->text().split("\n");
    const int size  = lst.size();

    for (int i=0; i<size; ++i)
        Util::addItem(w, lst[i]);
}

NoCcliveException::NoCcliveException(const QString& errmsg)
    : errmsg(errmsg)
{
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
            QString("error: %1:\n%2").arg(path).arg(errmsg);
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
            QObject::
                tr("%1 exited with %2:\n%3")
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


