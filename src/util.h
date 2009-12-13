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

#ifndef util_h
#define util_h

class QTreeWidget;
class QTextEdit;
class QListWidget;

class Util {
public:
    static void detectCclive(
        QString& path,
        QString& version,
        QString& libVersion,
        QString& libName,
        bool *isCcliveFlag
    );
    static void verifyCclivePath(
        const QString& path,
        QString& version,
        QString& libVersion,
        QString& libName,
        bool *isCcliveFlag=NULL
    );
    static void checkAllItems(
        const QTreeWidget *w,
        const Qt::CheckState& st,
        const int column=0
    );
    static void invertAllCheckableItems(
        const QTreeWidget *w,
        const int column=0
    );
    static void appendLog(const QTextEdit *w, const QString& s);
    static int countItems(const QTreeWidget *w);
    static void addItem(const QListWidget *w, QString lnk);
    static void removeSelectedItems(const QWidget *parent, const QListWidget *w);
    static void clearItems(const QWidget *parent, const QListWidget *w);
    static void paste(const QListWidget *w);
};

class NoCcliveException {
public:
    NoCcliveException(const QString& errmsg);
    NoCcliveException(const QString& path, const QString& errmsg);
    NoCcliveException(const QString& path,
                        const int& exitCode, const QString& output);
public:
    const QString& what() const;
private:
    QString errmsg;
};

#endif


