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

#include <QString>
#include <QStringList>
#include <QRegExp>

#include "scan.h"

static void
match_pattern(
    const QString& html,
    QStringList& found,
    QRegExp re)
{
    re.setCaseSensitivity(Qt::CaseInsensitive);
    re.setMinimal(true);

    int pos = 0;
    while ((pos = re.indexIn(html,pos)) != -1) {
        const QString cap = re.cap(1).simplified();
        if (!found.contains(cap) && !cap.isEmpty())
            found << cap;
        pos += re.matchedLength();
    }
}

#ifdef _1_
static void
dump_found(const QStringList& lst) {
    const int size = lst.size();
    for (int i=0; i<size; ++i)
        qDebug() << lst[i];
    qDebug() << "total: " << lst.size();
}
#endif

void
Scan::youtube(const QString& html, QStringList& found) {
    QStringList ids;
    match_pattern(html, ids, QRegExp("\\/v\\/(.*)[\"&\n<]")); // embed
    match_pattern(html, ids, QRegExp("\\/watch\\?v=(.*)[\"&\n<]"));

    const int size = ids.size();
    for (int i=0; i<size; ++i)
        found << "http://www.youtube.com/watch?v="+ids[i];
}

bool
Scan::title(const QString& html, QString& title) {
    QRegExp re("<title>(.*)<");

    re.setCaseSensitivity(Qt::CaseInsensitive);
    re.setMinimal(true);

    if (re.indexIn(html))
        title = re.cap(1).simplified();

    return !title.isEmpty();
}


