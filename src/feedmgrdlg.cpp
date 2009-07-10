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
#include <QInputDialog>
#include <QMessageBox>
#include <QClipboard>

#include "feedmgrdlg.h"

FeedMgrDialog::FeedMgrDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();

    connect(feedsList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
        this, SLOT(onItemDoubleClicked(QListWidgetItem*)));
}

void
FeedMgrDialog::onAdd() {
    QString lnk = QInputDialog::getText(this,
        tr("Add new RSS feed link"), tr("Enter link:"));
    addLink(lnk);
}

void
FeedMgrDialog::onPaste() {
    QClipboard *cb = QApplication::clipboard();
    addLink( cb->text().split("\n")[0] );
}

void
FeedMgrDialog::onRemove() {

    QList<QListWidgetItem*> sel = feedsList->selectedItems();

    if (sel.size() == 0)
        return;
 
    if (QMessageBox::warning(this, QCoreApplication::applicationName(),
        tr("Really remove the selected links?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
        == QMessageBox::No)
    {
        return;
    }

    for (register int i=0; i<sel.size(); ++i) {
        const int row = feedsList->row(sel[i]);
        delete feedsList->takeItem(row);
    }
}

void
FeedMgrDialog::onClear() {

    if (feedsList->count() == 0)
        return;

    if (QMessageBox::warning(this, QCoreApplication::applicationName(),
        tr("Really clear list?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
        == QMessageBox::No)
    {
        return;
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

void
FeedMgrDialog::onItemDoubleClicked(QListWidgetItem *item) {
    bool ok;

    QString lnk = QInputDialog::getText(this,
        tr("Edit link"), "", QLineEdit::Normal, item->text(), &ok);

    if (ok && !lnk.isEmpty())
        item->setText(lnk);
}

void
FeedMgrDialog::addLink(QString& lnk) {

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
