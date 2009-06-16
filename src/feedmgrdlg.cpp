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
#include <QInputDialog>

#include "feedmgrdlg.h"

FeedMgrDialog::FeedMgrDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();
}

void
FeedMgrDialog::onAdd() {
    QString lnk = QInputDialog::getText(this,
        tr("Add new RSS feed link"), tr("Enter link:"));

    if (lnk.isEmpty())
        return;

    lnk = lnk.trimmed();

    if (!lnk.startsWith("http://",Qt::CaseInsensitive))
        lnk.insert(0,"http://");

    QList<QListWidgetItem *> found
        = feedsList->findItems(lnk, Qt::MatchExactly);

    if (found.size() == 0)
        feedsList->addItem(lnk);
}

void
FeedMgrDialog::onRemove() {
    QList<QListWidgetItem*> sel = feedsList->selectedItems();
    for (register int i=0; i<sel.size(); ++i) {
        const int row = feedsList->row(sel[i]);
        delete feedsList->takeItem(row);
    }
}

void
FeedMgrDialog::writeSettings() {
    QSettings s;

    s.beginGroup("FeedMgrDialog");
    s.setValue("size", size());

    s.beginWriteArray("feeds");
    for (register int i=0; i<feedsList->count(); ++i) {
        s.setArrayIndex(i);
        QListWidgetItem *item = feedsList->item(i);
        s.setValue("link",item->text());
    }
    s.endArray();

    s.endGroup();
}

void
FeedMgrDialog::readSettings() {
    QSettings s;

    s.beginGroup("FeedMgrDialog");
    resize( s.value("size", QSize(505,255)).toSize() );
    const int size = s.beginReadArray("feeds");
    for (register int i=0; i<size; ++i) {
        s.setArrayIndex(i);
        feedsList->addItem(s.value("link").toString());
    }
    s.endArray();
    s.endGroup();
}
