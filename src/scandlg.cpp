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
//#include <QDebug>
#include <QMessageBox>
#include <QRegExp>

#include "util.h"
#include "scan.h"
#include "scandlg.h"

ScanDialog::ScanDialog(QWidget *parent)
    : QDialog(parent), errorOccurred(false)
{
    setupUi(this);

    itemsTree->setColumnHidden(1, true);

    readSettings();

    mgr= new QHttpManager(this);
    mgr->setProgressBar(progressBar);

    connect(mgr, SIGNAL(fetchLink(QString)),
        this, SLOT(onFetchLink(QString)));

    connect(mgr, SIGNAL(fetchFinished()),
        this, SLOT(onFetchFinished()));

    connect(mgr, SIGNAL(fetchError(QString)),
        this, SLOT(onFetchError(QString)));
}

static void
update_item_count(const ScanDialog *d) {
    int count = 0;
    QTreeWidgetItemIterator iter(d->itemsTree);
    while (*iter) {
        ++count;
        ++iter;
    }
    d->totalLabel->setText( QString("Total: %1").arg(count) );
}

void
ScanDialog::enableWidgets(const bool state/*=true*/) {
    linkEdit->setEnabled (state);
    titlesBox->setEnabled(state);
}

void
ScanDialog::onScan() {

    if (!linkEdit->isEnabled()) {
        // Assumes it is disabled when we're scanning.
        if (mgr)
            mgr->abort();
        if (mgrt)
            mgrt->abort();
        return;
    }

    QString lnk = linkEdit->text().simplified();

    if (lnk.isEmpty())
        return;

    if (!lnk.startsWith("http://", Qt::CaseInsensitive))
        lnk.insert(0,"http://");

    itemsTree->clear();
    update_item_count(this);

    logEdit->clear();

    enableWidgets(false);
    progressBar->setTextVisible(false);
    scanButton->setText(tr("&Abort"));

    errorOccurred = false;

    mgr->fetch(lnk);
}

void
ScanDialog::writeSettings() {
    QSettings s;
    s.beginGroup("ScanDialog");
    s.setValue("size", size());
    s.endGroup();
}

void
ScanDialog::readSettings() {
    QSettings s;
    s.beginGroup("ScanDialog");
    resize( s.value("size", QSize(514,295)).toSize() );
    s.endGroup();
}

void
ScanDialog::resetUI() {
    enableWidgets(true);
    scanButton->setText(tr("&Scan"));
    update_item_count(this);
}

void
ScanDialog::onSelectAll() {
    Util::checkAllItems(itemsTree, Qt::Checked);
}

void
ScanDialog::onInvert() {
    Util::invertAllCheckableItems(itemsTree);
}

void
ScanDialog::onFetchFinished() {

    if (errorOccurred) {
        resetUI();
        return;
    }

    QStringList found;
    Scan::youtube(mgr->getData(), found);

    const int size = found.size();

    Util::appendLog(logEdit,
        QString(tr("Found %1 video links.")).arg(size));

    if ( !titlesBox->checkState() ) {

        for (int i=0; i<size; ++i) {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setCheckState(0, Qt::Unchecked);
            for (int j=0; j<2; ++j)
                item->setText(j, found[i]);
            itemsTree->addTopLevelItem(item);
        }

        resetUI();
        Util::appendLog(logEdit, tr("Done."));
    }
    else {
        Util::appendLog(logEdit,
            tr("Fetch titles for the found videos."));

        mgrt = new QHttpManager(this);

        connect(mgrt, SIGNAL(fetchLink(QString)),
            this, SLOT(onFetchLink(QString)));

        connect(mgrt, SIGNAL(fetchError(QString)),
            this, SLOT(onFetchError(QString)));

        // Note the use of different slot here.

        connect(mgrt, SIGNAL(fetchFinished()),
            this, SLOT(onFetchTitlesFinished()));

        // Do not set progressbar for this manager.
        // We'll use fetched/expected for that instead.

        fetchedTitles   = -1;
        expectedTitles  = size;

        progressBar->setValue(fetchedTitles);
        progressBar->setMaximum(expectedTitles);

        videoLinks = found;

        progressBar->setTextVisible(true);

        emit onFetchTitlesFinished(); // Start iteration.
    }
}

void
ScanDialog::onFetchTitlesFinished() {

    if (errorOccurred) {
        resetUI();
        return;
    }

    if (fetchedTitles >= 0) {
        QString title;
        if (!Scan::title(mgrt->getData(), title))
            title = videoLinks[fetchedTitles]; // Fallback to link.

        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setCheckState(0, Qt::Unchecked);
        item->setText(0, title);
        item->setText(1, videoLinks[fetchedTitles]);
        itemsTree->addTopLevelItem(item);

        update_item_count(this);
        progressBar->setValue(++fetchedTitles);

        if (fetchedTitles == expectedTitles) {
            resetUI();
            videoLinks.clear();
            mgrt->deleteLater();
            Util::appendLog(logEdit, tr("Done."));
            return;
        }
    }
    else
        fetchedTitles++;

    mgrt->fetch(videoLinks[fetchedTitles]);
}

void
ScanDialog::onFetchError(QString errorString) {
    errorOccurred = true;
    Util::appendLog(logEdit, errorString);
}

void
ScanDialog::onFetchLink(QString url) {
    Util::appendLog(logEdit, tr("Fetch ... ")+url);
}


