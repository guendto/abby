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
#include <QDragEnterEvent>

#include "util.h"
#include "scan.h"
#include "scandlg.h"

ScanDialog::ScanDialog(QWidget *parent)
    : QDialog(parent)
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

    // Enable drops.
    setAcceptDrops(true);
}

void
ScanDialog::updateCount() {
    totalLabel->setText(
        QString(tr("Total: %1"))
            .arg(Util::countItems(itemsTree))
    );
}

void
ScanDialog::enableWidgets(const bool state/*=true*/) {
    linkEdit->setEnabled (state);
    titlesBox->setEnabled(state);
    buttonBox->setEnabled(state);
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
    updateCount();

    logEdit->clear();

    enableWidgets(false);
    progressBar->setTextVisible(false);
    scanButton->setText(tr("&Abort"));
    scanButton->setIcon(QIcon(":process-stop.png"));

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
    scanButton->setIcon(QIcon(":edit-find.png"));
    updateCount();
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

    if (mgr->errorOccurred()) {
        resetUI();
        return;
    }

    QStringList found;
    Scan::youtube(mgr->getData(), found);

    const int size = found.size();

    if (size == 0) {
        Util::appendLog(logEdit, tr("Nothing found."));
        resetUI();
        return;
    }

    Util::appendLog(logEdit,
        QString(tr("Found %1 video links.")).arg(size));

    if ( !titlesBox->checkState() ) {

        for (int i=0; i<size; ++i) {
            QTreeWidgetItem *item = new QTreeWidgetItem(itemsTree);
            item->setCheckState(0, Qt::Unchecked);
            for (int j=0; j<2; ++j)
                item->setText(j, found[i]);
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

        // Note the use of a different slot here.

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

    if (mgrt->errorOccurred()) {
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

        updateCount();
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
    Util::appendLog(logEdit, errorString);
}

void
ScanDialog::onFetchLink(QString url) {
    Util::appendLog(logEdit, tr("Fetch ... ")+url);
}

void
ScanDialog::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

void
ScanDialog::dropEvent(QDropEvent *event) {
    QStringList lst = event->mimeData()->text().split("\n");
    const int size = lst.size();

    for (int i=0; i<size; ++i)
        linkEdit->setText(lst[i]);

    event->acceptProposedAction();
}


