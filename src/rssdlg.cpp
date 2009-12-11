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

#include "util.h"
#include "rssdlg.h"
#include "feedmgrdlg.h"

RSSDialog::RSSDialog(QWidget *parent)
    : QDialog(parent), errorOccurred(false)
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
}

void
RSSDialog::onFetch() {

    if (!linkEdit->isEnabled()) {
        // Assumes it is disabled when we're fetching xml.
        if (mgr)
            mgr->abort();
        return;
    }

    QString lnk = linkEdit->text().simplified();

    if (lnk.isEmpty())
        return;

    if (!lnk.startsWith("http://", Qt::CaseInsensitive))
        lnk.insert(0,"http://");

    itemsTree->clear();
    updateCount();

    logEdit->clear();

    enableWidgets(false);
    fetchButton->setText(tr("&Abort"));

    errorOccurred = false;

    mgr->fetch(lnk);
}

void
RSSDialog::onFeedMgr() {
    FeedMgrDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QTreeWidgetItemIterator iter(dlg.itemsTree);
        while (*iter) {
            if ((*iter)->checkState(0) == Qt::Checked) {
                linkEdit->setText((*iter)->text(1)); // 1=url
                break;
            }
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

    resetUI();

    if (errorOccurred)
        return;

    parseRSS(mgr->getData());
    updateCount();

    Util::appendLog(logEdit, tr("Done."));
}

void
RSSDialog::onFetchError(QString errorString) {
    errorOccurred = true;
    Util::appendLog(logEdit, errorString);
}

void
RSSDialog::onFetchLink(QString url) {
    Util::appendLog(logEdit, tr("Fetch ... ")+url);
}

void
RSSDialog::parseRSS(const QString& rss) {
    xml.clear();

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
                    QTreeWidgetItem *item = new QTreeWidgetItem;
                    item->setCheckState(0, Qt::Unchecked);
                    item->setText(0, itemTitle);
                    item->setText(1, lnk);
                    itemsTree->addTopLevelItem(item);
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
    linkEdit->setEnabled     (state);
    feedmgrButton->setEnabled(state);
}

void
RSSDialog::resetUI() {
    enableWidgets(true);
    fetchButton->setText(tr("&Fetch"));
    updateCount();
}

void
RSSDialog::updateCount() {
    totalLabel->setText(
        QString(tr("Total: %1"))
            .arg(Util::countItems(itemsTree))
    );
}


