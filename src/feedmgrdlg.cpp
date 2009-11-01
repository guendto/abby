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
#include <QDebug>

#include "feededitdlg.h"
#include "feedmgrdlg.h"

typedef unsigned int _uint;

FeedMgrDialog::FeedMgrDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();

    connect(itemsTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
        this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    itemsTree->setColumnHidden(1, true);
}

void
FeedMgrDialog::onAdd() {
    FeedEditDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        addLink(dlg.nameEdit->text(), dlg.linkEdit->text());
}

static int
confirm_remove(QWidget *parent) {
    return QMessageBox::warning(
        parent,
        QObject::tr("Warning"),
        QObject::tr("Really remove the selected links?"),
        QMessageBox::Yes|QMessageBox::No,
        QMessageBox::No
    );
}

void
FeedMgrDialog::onRemove() {

    bool ok = false;

    QTreeWidgetItemIterator iter(itemsTree);
    QList<QTreeWidgetItem*> lst;
    while (*iter) {
        if ((*iter)->checkState(0) == Qt::Checked) {
            if (!ok) {
                if (confirm_remove(this) == QMessageBox::No)
                    return;
                ok = true;
            }
            lst << (*iter);
        }
        ++iter;
    }

    QList<QTreeWidgetItem*>::const_iterator i;
    for (i=lst.constBegin(); i!=lst.constEnd(); ++i) {
        itemsTree->removeItemWidget((*i), 0);
        delete (*i);
    }
}

void
FeedMgrDialog::onClear() {

    if (itemsTree->topLevelItemCount() == 0)
        return;

    if (QMessageBox::warning(this, QCoreApplication::applicationName(),
        tr("Really clear list?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
        == QMessageBox::No)
    {
        return;
    }

    itemsTree->clear();
}

void
FeedMgrDialog::writeSettings() {
    QSettings s;

    s.beginGroup("FeedMgrDialog");
    s.setValue("size", size());

    s.beginWriteArray("feeds");
    QTreeWidgetItemIterator iter(itemsTree);
    for (register _uint i=0; (*iter); ++iter, ++i) {
        s.setArrayIndex(i);
        s.setValue("link",
            QString("%1||%2") // Note: replaces "||" which we'll use as delim
                .arg((*iter)->text(0).replace("||","|"))
                .arg((*iter)->text(1).replace("||","|"))
        );
    }
    s.endArray();

    s.endGroup();
}

void
FeedMgrDialog::readSettings() {

    QSettings s;
    s.beginGroup("FeedMgrDialog");
    resize( s.value("size", QSize(505,255)).toSize() );

    const register _uint size = s.beginReadArray("feeds");
    register _uint unnamed_count = 1;

    for (register _uint i=0; i<size; ++i) {
        s.setArrayIndex(i);

        const QString entry =
            s.value("link").toString();

        QStringList tmp = entry.split("||");

        if (tmp.size() < 2) {
            tmp.prepend( // Name column was added in 0.4.5.
                QString( tr("Unnamed feed #%1") )
                    .arg(unnamed_count++)
            );
        }

        addLink(tmp[0], tmp[1]);
    }
    s.endArray();

    s.endGroup();
}

void
FeedMgrDialog::onItemDoubleClicked(
    QTreeWidgetItem *item,
    int /*column*/)
{
    FeedEditDialog dlg(this, item->text(0), item->text(1));
    if (dlg.exec() == QDialog::Accepted) {
        item->setText(0, dlg.nameEdit->text());
        item->setText(1, dlg.linkEdit->text());
    }
}

void
FeedMgrDialog::addLink(QString name, QString lnk) {

    if (name.isEmpty())
        return;

    name = name.trimmed();

    if (lnk.isEmpty())
        return;

    lnk = lnk.trimmed();

    if (!lnk.startsWith("http://",Qt::CaseInsensitive))
        lnk.insert(0,"http://");

    QList<QTreeWidgetItem *> found =
        itemsTree->findItems(lnk, Qt::MatchExactly, 1); // 1=url column

    if (found.size() == 0) {
        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setCheckState(0, Qt::Unchecked);
        item->setText(0, name);
        item->setText(1, lnk);
        itemsTree->addTopLevelItem(item);
    }
}


