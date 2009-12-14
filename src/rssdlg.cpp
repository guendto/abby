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
//#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>

#include "util.h"
#include "rssdlg.h"
#include "feedmgrdlg.h"

#include <QDebug>
RSSDialog::RSSDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    readSettings();

    itemsTree->setColumnHidden(1, true);

    mgr = new QHttpManager(this);
    mgr->setProgressBar(progressBar);

    connect(mgr, SIGNAL(fetchLink(QString)),
        this, SLOT(onFetchLink(QString)));

    connect(mgr, SIGNAL(fetchFinished()),
        this, SLOT(onFetchFinished()));

    connect(mgr, SIGNAL(fetchError(QString)),
        this, SLOT(onFetchError(QString)));

#ifdef _1_
    for (int i=0; i<5; ++i) {
        QTreeWidgetItem *p = new QTreeWidgetItem(itemsTree);
        p->setText(0, QString("Parent %1").arg(i+1));
        for (int j=0; j<5; ++j) {
            QTreeWidgetItem *item = new QTreeWidgetItem(p);
            item->setText(0, QString("Child %1").arg(j+1));
        }
    }
#endif
}

void
RSSDialog::onFetch() {

    if (!itemsList->isEnabled()) {
        // Assumes it is disabled when we're fetching xml.
        if (mgr)
            mgr->abort();
        return;
    }

    currentFeed = -1;
    expectedFeeds = itemsList->count();

    if (!expectedFeeds)
        return;

    itemsTree->clear();
    logEdit->clear();

    enableWidgets(false);
    fetchButton->setText(tr("&Abort"));

    emit onFetchFinished();
}

void
RSSDialog::onFeedMgr() {
    FeedMgrDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QTreeWidgetItemIterator iter(dlg.itemsTree);
        while (*iter) {
            if ((*iter)->checkState(0) == Qt::Checked)
                Util::addItem(itemsList, (*iter)->text(1)); // 1=url
            ++iter;
        }
        dlg.writeSettings();
    }
}

void
RSSDialog::writeSettings() {
    QSettings s;
    s.beginGroup("RSSDialog");
    s.setValue("size", size());
    s.endGroup();
}

void
RSSDialog::readSettings() {
    QSettings s;
    s.beginGroup("RSSDialog");
    resize( s.value("size", QSize(514,295)).toSize() );
    s.endGroup();
}

void
RSSDialog::onSelectAll() {
    Util::checkAllItems(itemsTree, Qt::Checked);
}

void
RSSDialog::onInvert() {
    Util::invertAllCheckableItems(itemsTree);
}

void
RSSDialog::onFetchFinished() {

    if (mgr->errorOccurred()) {
        resetUI();
        return;
    }

    if (currentFeed >= 0) {
        parseRSS(mgr->getData());

        if (++currentFeed == expectedFeeds) {
            resetUI();
            mgr->deleteLater();
            Util::appendLog(logEdit, tr("Done."));
            return;
        }
    }
    else
        ++currentFeed;

    QListWidgetItem *item = itemsList->item(currentFeed);
    mgr->fetch(item->text());
}

void
RSSDialog::onFetchError(QString errorString) {
    Util::appendLog(logEdit, errorString);
}

void
RSSDialog::onFetchLink(QString url) {
    Util::appendLog(logEdit, tr("Fetch ... ")+url);
}

void
RSSDialog::parseRSS(const QString& rss) {
    xml.clear();

    QTreeWidgetItem *parent = 0;
    QString feedTitle, lnk, tag, itemTitle;
#ifdef _1_
    QString pubDate;
#endif

    xml.addData(rss);

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "item")
                lnk = xml.attributes().value("rss::about").toString();
            tag = xml.name().toString();
        }
        else if (xml.isEndElement()) {
            if (xml.name() == "item") {
                if (!feedTitle.isEmpty()) {
                    // Add parent:
                    if (!parent) {
                        parent = new QTreeWidgetItem(itemsTree);
                        parent->setText(0, feedTitle);
                    }
                    // Add child:
                    QTreeWidgetItem *c = new QTreeWidgetItem(parent);
                    c->setCheckState(0, Qt::Unchecked);
                    c->setText(0, itemTitle);
                    c->setText(1, lnk);
                }
                else
                    feedTitle = itemTitle;

                itemTitle.clear();
                lnk.clear();
#ifdef _1_
                pubDate.clear();
#endif
            }
        }
        else if (xml.isCharacters() && !xml.isWhitespace()) {
            const QString tmp = xml.text().toString();
            if (tag == "title")
                itemTitle += tmp;
            else if (tag == "link")
                lnk += tmp;
#ifdef _1_
            else if (tag == "pubDate")
                pubDate += tmp;
#endif
        }
    }

    if (xml.error()
        && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError)
    {
        Util::appendLog(logEdit,
            QString(tr("XML parsing error: %1: %2"))
                .arg(xml.lineNumber())
                .arg(xml.errorString())
        );
    }
}

void
RSSDialog::enableWidgets(const bool state/*=true*/) {
    addButton->setEnabled    (state);
    removeButton->setEnabled (state);
    pasteButton->setEnabled  (state);
    clearButton->setEnabled  (state);
    itemsList->setEnabled    (state);
    feedmgrButton->setEnabled(state);
}

void
RSSDialog::resetUI() {
    enableWidgets(true);
    fetchButton->setText(tr("&Fetch"));
}

void
RSSDialog::onAdd() {
    Util::addItem(
        itemsList,
        QInputDialog::getText(this,
            QCoreApplication::applicationName(), tr("Add link:"))
    );
}

void
RSSDialog::onRemove() {
    Util::removeSelectedItems(this, itemsList);
}

void
RSSDialog::onPaste() {
    Util::paste(itemsList);
}

void
RSSDialog::onClear() {
    Util::clearItems(this, itemsList);
}



