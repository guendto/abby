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
#include <QMainWindow>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
//#include <QDebug>
#include <QInputDialog>
#include <QNetworkProxy>
#include <QRegExp>
#include <QMap>

#include "mainwnd.h"
#include "prefsdlg.h"
#include "rssdlg.h"
#include "scandlg.h"
#include "formatdlg.h"
#include "aboutdlg.h"
#include "util.h"

#define critCcliveProcessFailed(parent, msg) \
    do { \
    QMessageBox::critical(parent, QCoreApplication::applicationName(), \
        QString( tr("Error while trying to run c/clive:\n%1") ).arg(msg)); \
    } while (0)

#define critCcliveNotSpecified(parent) \
    do { \
    QMessageBox::critical(parent, QCoreApplication::applicationName(), \
      QString( tr("c/clive executable not found, please check the path.") )); \
    } while (0)

#define critCcliveExitedWithError(parent,code,msg) \
    do { \
    QMessageBox::critical(parent, QCoreApplication::applicationName(), \
        QString( tr("c/clive exited with error code %1:\n%2") ) \
            .arg(code).arg(msg)); \
    } while (0)


typedef unsigned int _uint;

MainWindow::MainWindow()
    : cancelledFlag(false), isCcliveFlag(false)
{
/*
 The word "English" is not meant to be translated literally.
 Instead, replace "English" with the target translation language,
 e.g. "Suomi", "Deutch", etc. abby uses this word in the
 preferences dialog to select current language.
*/
    const QString lang = tr("English");

    setupUi(this);

    // Dialogs. Be extravagant about system memory.
    prefs   = new PreferencesDialog (this);
    rss     = new RSSDialog         (this);
    scan    = new ScanDialog        (this);
    format  = new FormatDialog      (this);

    // Settings.
    readSettings();
    setProxy();

    // Process.
    connect(&process, SIGNAL( started() ),
        this, SLOT( onProcStarted() ));

    connect(&process, SIGNAL( error(QProcess::ProcessError) ),
        this, SLOT( onProcError(QProcess::ProcessError) ));

    // NOTE: Merge stdout/stderr from c/clive
    connect(&process, SIGNAL( readyReadStandardOutput() ),
        this, SLOT( onProcStdoutReady() ));

    connect(&process, SIGNAL( readyReadStandardError() ),
        this, SLOT( onProcStdoutReady() ));

    connect(&process, SIGNAL( finished(int, QProcess::ExitStatus) ),
        this, SLOT( onProcFinished(int, QProcess::ExitStatus) ));

    // Misc.
    connect(linksList, SIGNAL( itemDoubleClicked(QListWidgetItem *) ),
        this, SLOT( onItemDoubleClicked(QListWidgetItem *) ));

    // Parse.
    if (parseCcliveVersionOutput())
        parseCcliveHostsOutput();

    // Widget voodoo.
    updateWidgets           (true);

#ifdef WIN32
    streamBox ->setHidden(true);
    streamSpin->setHidden(true);
#endif
}

bool
MainWindow::parseCcliveHostsOutput() {
    hosts.clear();

    QString path = prefs->ccliveEdit->text();

    if (path.isEmpty()) {

        QMessageBox::information(
            this,
            QCoreApplication::applicationName(),
            tr("abby requires `clive' or `cclive'. "
                "Please define path to either executable.")
        );

        onPreferences();

        return false;
    }

    QProcess proc;
    proc.setEnvironment(QStringList() << "CCLIVE_NO_CONFIG=1");
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(path, QStringList() << "--hosts");

    format->resetHosts();

    if (!proc.waitForFinished()) {
        critCcliveProcessFailed(this, proc.errorString() );
        return false;
    }
    else {
        const QString output =
            QString::fromLocal8Bit( proc.readAll() );

        const int exitCode =
            proc.exitCode();

        if (exitCode == 0) {

            QStringList lst =
                output.split("\n", QString::SkipEmptyParts);

            lst.removeLast(); // The note line.

            const register _uint size = lst.size();
            for (register _uint i=0; i<size; ++i) {

                QString ln      = lst[i].remove("\r");
                QStringList tmp = ln.split("\t");

                if (!tmp[0].isEmpty() && !tmp[1].isEmpty())
                    hosts[tmp[0]] = tmp[1];
            }

            format->parseHosts(hosts);
        }
        else {
            critCcliveExitedWithError(this, exitCode, output);
            return false;
        }
    }

    return true;
}

bool
MainWindow::parseCcliveVersionOutput() {

    const QString path =
        prefs->ccliveEdit->text();

    try {
        Util::verifyCclivePath(
            path,
            ccliveVersion,
            curlVersion,
            curlMod,
            &isCcliveFlag
        );
    }
    catch (const NoCcliveException& x) {
        QMessageBox::warning(this, tr("Warning"), x.what());
        return false;
    }
    return true;
}

bool
MainWindow::ccliveSupportsHost(const QString& lnk) {

    const QString host = QUrl(lnk).host();

    for (QStringMap::const_iterator iter = hosts.begin();
        iter != hosts.end(); ++iter)
    {
        QRegExp re( iter.key());

        if (re.indexIn(host) != -1)
            return true;
    }

    return false;
}

void
MainWindow::updateWidgets(const bool updateCcliveDepends) {
    // Enable widgets based on preferences and other settings.
    QString s;

    s = prefs->streamEdit->text();
    streamBox->setEnabled(!s.isEmpty());
    if (s.isEmpty())
        streamBox->setCheckState(Qt::Unchecked);

    s = prefs->commandEdit->text();
    commandBox->setEnabled(!s.isEmpty());
    if (s.isEmpty())
        commandBox->setCheckState(Qt::Unchecked);

    if (updateCcliveDepends) {
        // The most time consuming check is to run (c)clive.
        // Run it only when we cannot work around it.
        if (isCcliveFlag) {
            regexpLabel ->show();
            regexpEdit  ->show();
            findallBox  ->show();
            cclassLabel ->hide();
            cclassEdit  ->hide();
        }
        else {
            regexpLabel ->hide();
            regexpEdit  ->hide();
            findallBox  ->hide();
            cclassLabel ->show();
            cclassEdit  ->show();
        }
    }
}

void
MainWindow::closeEvent(QCloseEvent *event) {
    int rc = QMessageBox::Yes;
    if (process.state() != QProcess::NotRunning) {
        rc = QMessageBox::warning(
            this,
            tr("Warning"),
            tr("c/clive process is still active, really close abby?"),
            QMessageBox::Yes|QMessageBox::No
        );
    }

    if (rc == QMessageBox::Yes) {
        process.kill();
        writeSettings();
        event->accept();
    }
    else
        event->ignore();
}

void
MainWindow::writeSettings() {
    QSettings s;
    s.beginGroup("MainWindow");
    s.setValue("size", size());
    s.setValue("pos", pos());
    s.setValue("regexpEdit", regexpEdit->text());
    s.setValue("findallBox", findallBox->checkState());
    s.setValue("cclassEdit", cclassEdit->text());
    s.setValue("fnamefmtEdit", fnamefmtEdit->text());
    s.endGroup();
}

void
MainWindow::readSettings() {
    QSettings s;
    s.beginGroup("MainWindow");
    resize( s.value("size", QSize(525,265)).toSize() );
    move( s.value("pos", QPoint(200,200)).toPoint() );
    regexpEdit->setText( s.value("regexpEdit").toString() );
    findallBox->setCheckState(
        s.value("findallBox").toBool()
        ? Qt::Checked
        : Qt::Unchecked);
    cclassEdit->setText( s.value("cclassEdit").toString() );
    fnamefmtEdit->setText( s.value("fnamefmtEdit").toString() );
    s.endGroup();
}

void
MainWindow::updateLog(const QString& newText) {
    QString text = logEdit->toPlainText() + newText;
    logEdit->setPlainText(text);
}


// Slots

void
MainWindow::onPreferences() {

    QString old = prefs->ccliveEdit->text();

    prefs->exec();

    QString _new = prefs->ccliveEdit->text();

    if (old != _new) {
        if (parseCcliveVersionOutput())
            parseCcliveHostsOutput();
    }

    updateWidgets(old != _new);

    setProxy();
}

void
MainWindow::setProxy() {
    if (!prefs->proxyEdit->text().isEmpty()
        && prefs->proxyCombo->currentIndex() > 0)
    {
        QUrl url(prefs->proxyEdit->text());

        QNetworkProxy proxy(
            QNetworkProxy::HttpProxy,
            url.host(),
            url.port()
        );

        QNetworkProxy::setApplicationProxy(proxy);
    }
    else
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
}

void
MainWindow::onStreamStateChanged(int state) {
    streamSpin->setEnabled(state != 0);
}

void
MainWindow::onStart() {

    if ( ccliveVersion.isEmpty() ) {
        critCcliveNotSpecified(this);
        onPreferences();
        return;
    }

    if (linksList->count() == 0) {
        onAdd();
        if (linksList->count() == 0)
            return;
    }

    QString path = prefs->ccliveEdit->text();

    // Check video save directory

    // cclive has no option for this but we can work around it by
    // changing the current working directory.

    QString savedir = prefs->savedirEdit->text();
    if (savedir.isEmpty()) {
        QMessageBox::information(this, QCoreApplication::applicationName(),
            tr("Please define a directory for downloaded videos."));
        onPreferences();
        return;
    }

    process.setWorkingDirectory(savedir);

    // Construct cclive/clive args

    QStringList args;

    args << "--print-fname" << "--continue";

    QString s = prefs->streamEdit->text();
    if (!s.isEmpty() && streamBox->isChecked()) {
        args << QString("--stream-exec=%1").arg(s);
        args << QString("--stream=%1").arg(streamSpin->value());
    }

    s = prefs->commandEdit->text();
    if (!s.isEmpty() && commandBox->isChecked()) {
        if (!s.endsWith(";"))
            s += ";";
        args << QString("--exec=%1").arg(s);
        args << QString("--exec-run");
    }

    if (prefs->proxyCombo->currentIndex() == 0)
        args << "--no-proxy";
    else {
        s = prefs->proxyEdit->text();
        if (!s.isEmpty())
            args << QString("--proxy=%1").arg(s);
    }

    if (prefs->limitBox->checkState()) {
        int n = prefs->limitSpin->value();
        args << QString("--limit-rate=%1").arg(n);
    }

    if (prefs->timeoutBox->checkState()) {
        int n = prefs->timeoutSpin->value();
        if (!prefs->socksBox->checkState())
            args << QString("--connect-timeout=%1").arg(n);
        else
            args << QString("--connect-timeout-socks=%1").arg(n);
    }

    QStringList env;

    if (isCcliveFlag) {
        s = regexpEdit->text();
        if (!s.isEmpty())
            args << QString("--regexp=%1").arg(s);
        if (findallBox->checkState())
            args << QString("--find-all");
    } else {

        args << "--stderr";

        // Set environment variables for clive
        env  << "COLUMNS=80" << "LINES=24"               // Term::ReadKey
             << QString("HOME=%1").arg(QDir::homePath()) // $env{HOME}
             << "CCLIVE_NO_CONFIG=1";                    // cclive 0.5.0+

        s = cclassEdit->text();
        if (!s.isEmpty())
            args << QString("--cclass=%1").arg(s);
    }

    s = fnamefmtEdit->text();
    if (!s.isEmpty())
        args << QString("--filename-format=%1").arg(s);

    // Check if all video page links are of the same host.

    QUrl first(linksList->item(0)->text());

    bool allSame    = true;
    const register _uint count = linksList->count();

    for (register _uint i=0; i<count; ++i) {

        QUrl url(linksList->item(i)->text());

        if (url.host() != first.host()) {
            allSame = false;
            break;
        }
    }

    s = "flv";

    // Use format dialog setting for the host.
    if (allSame)
        s = format->getFormatSetting(first.host());

    args << QString("--format=%1").arg(s);

    for (register _uint i=0; i<count; ++i)
        args << QString("%1").arg(linksList->item(i)->text());

    totalProgressbar->setMaximum(linksList->count());

    // Prepare log

    logEdit->clear();
    updateLog("% " +path+ " " +args.join(" ")+ "\n");

    // And finally start the process

    cancelledFlag = false;
    process.setEnvironment(env);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(path,args);
}

void
MainWindow::onCancel() {
    cancelledFlag = true;
    process.kill();
}

void
MainWindow::onAbout() {
    AboutDialog(this, ccliveVersion, curlMod, curlVersion).exec();
}

#define fillList(dlg) \
    do { \
        QTreeWidgetItemIterator iter(dlg->itemsTree); \
        while  (*iter) { \
            if ((*iter)->checkState(0) == Qt::Checked) \
                addPageLink((*iter)->text(1)); \
            ++iter; \
        } \
    } while (0)

void
MainWindow::onRSS() {
    if (rss->exec() == QDialog::Accepted) {
        fillList(rss);
    }
    rss->writeSettings();
}

void
MainWindow::onScan() {
    if (scan->exec() == QDialog::Accepted) {
        fillList(scan);
    }
    scan->writeSettings();
}

void
MainWindow::onPasteURL() {

    QClipboard *cb  = QApplication::clipboard();
    QStringList lst = cb->text().split("\n");
    const register _uint size  = lst.size();

    for (register _uint i=0; i<size; ++i)
        addPageLink(lst[i]);
}

void
MainWindow::onAdd() {
    addPageLink(QInputDialog::getText(this,
        QCoreApplication::applicationName(), tr("Add link:")));
}

void
MainWindow::onRemove() {

    QList<QListWidgetItem*> sel = linksList->selectedItems();

    if (sel.size() == 0)
        return;
 
    if (QMessageBox::warning(this, QCoreApplication::applicationName(),
        tr("Really remove the selected links?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
        == QMessageBox::No)
    {
        return;
    }

    const register _uint size = sel.size();

    for (register _uint i=0; i<size; ++i) {
        const int row = linksList->row(sel[i]);
        delete linksList->takeItem(row);
    }
}

void
MainWindow::onClear() {

    if (linksList->count() == 0)
        return;

    if (QMessageBox::warning(this, QCoreApplication::applicationName(),
        tr("Really clear list?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
        == QMessageBox::No)
    {
        return;
    }

    linksList->clear();
}

void
MainWindow::addPageLink(QString lnk) {

    lnk = lnk.trimmed();

    if (lnk.isEmpty())
        return;

    if (!lnk.startsWith("http://", Qt::CaseInsensitive))
        lnk.insert(0,"http://");

    if (!ccliveSupportsHost(lnk)) {
        QMessageBox::critical(this, QCoreApplication::applicationName(),
            QString(tr("%1: unsupported website")).arg(QUrl(lnk).host()));
        return;
    }

    QList<QListWidgetItem *> found
        = linksList->findItems(lnk, Qt::MatchExactly);

    if (found.size() == 0)
        linksList->addItem(lnk);
}

void
MainWindow::onFormats() {
    if ( hosts.isEmpty() || ccliveVersion.isEmpty() ) {
        critCcliveNotSpecified(this);
        onPreferences();
        return;
    }

    format->exec();
    format->saveCurrent();
    format->writeSettings();
}

void
MainWindow::onProcStarted() {
    statusBar() ->clearMessage();
    fileLabel   ->setText("-");
    sizeLabel   ->setText("-- / --");
    rateLabel   ->setText("--.-");
    etaLabel    ->setText("--:--");
    progressBar ->setValue(0);
    totalProgressbar->setValue(0);

    addButton   ->setEnabled(false);
    pasteButton ->setEnabled(false);
    removeButton->setEnabled(false);
    clearButton ->setEnabled(false);
    rssButton   ->setEnabled(false);
    scanButton  ->setEnabled(false);
    startButton ->setEnabled(false);
    cancelButton->setEnabled(true);

    action_Download->setEnabled(false);

    action_Link ->setEnabled(false);
    action_RSS  ->setEnabled(false);
    action_Scan ->setEnabled(false);
    action_Paste->setEnabled(false);

    action_Remove->setEnabled(false);
    action_Clear->setEnabled(false);

    linksList   ->setEnabled(false);

    tabWidget->setTabEnabled(1, false);
}

void
MainWindow::onProcError(QProcess::ProcessError err) {
    if (err == QProcess::FailedToStart) {
        QString msg = tr("Error: Failed to start the process.");
        statusBar()->showMessage(msg);
        updateLog(msg);
    }
}

void
MainWindow::onProcStdoutReady() {

    // NOTE: We read both channels stdout and stderr.

    char data[1024];
    memset(&data, 0, sizeof(data));

    QStatusBar *sb = statusBar();
    bool appendLogFlag = true;

    while (process.readLine(data, sizeof(data))) {

        appendLogFlag = true;
        
        QString ln = QString::fromLocal8Bit(data);
        ln.remove("\n");

        if (ln.startsWith("fetch http://")) {
            sb->showMessage( tr("Fetching ...") );
            totalProgressbar->setValue( totalProgressbar->value()+1 );
        }

        else if (ln.startsWith("verify"))
            sb->showMessage( tr("Verifying link ...") );

        else if (ln.startsWith("file:")) {
            QRegExp re("file: (.*)  (\\d+.\\d+)M");
            re.indexIn(ln);
            fileLabel->setText( re.capturedTexts()[1].simplified() );
            sb->showMessage( tr("Downloading video ...") );
        }

        else if (ln.startsWith("error:")) {
            // Log it.
        }

        else {

            appendLogFlag = false;

            // In some parallel world I have written a cleaner regexp.
            static const char progress_re[] =
                "(\\d+)%" // percent 
                "\\s+(\\d+)\\.(\\d+)M\\s+\\/\\s+(\\d+)\\.(\\d+)M" // xxM / yyM
                "\\s+(\\d+)\\.(\\d+)(\\w)\\/\\w" // speed
                "\\s+(.*)"; // eta

            QRegExp re(progress_re);

            if (re.indexIn(ln)) {
                QStringList cap = re.capturedTexts();

                cap.removeFirst();

                if (cap[0].isEmpty())
                    continue;

                //qDebug() << cap;

                enum {
                    PERCENT = 0,
                    SIZE_NOW_X,
                    SIZE_NOW_Y,
                    SIZE_EXPECTED_X,
                    SIZE_EXPECTED_Y,
                    SPEED_X,
                    SPEED_Y,
                    SPEED_TYPE,
                    ETA,
                };

                progressBar ->setValue( cap[PERCENT].toInt() );

                sizeLabel   ->setText(QString("%1.%2M / %3.%4M")
                    .arg(cap[SIZE_NOW_X])
                    .arg(cap[SIZE_NOW_Y])
                    .arg(cap[SIZE_EXPECTED_X])
                    .arg(cap[SIZE_EXPECTED_Y]));

                rateLabel   ->setText(QString("%1.%2%3/s")
                    .arg(cap[SPEED_X])
                    .arg(cap[SPEED_Y])
                    .arg(cap[SPEED_TYPE]));

                etaLabel    ->setText(cap[ETA].simplified());
            }
        }

        if (appendLogFlag)
            updateLog(ln + "\n");

        memset(&data, 0, sizeof(data));
    }
}

void
MainWindow::onProcFinished(int exitCode, QProcess::ExitStatus exitStatus) {

    QString status;

    switch (exitStatus) {
    case QProcess::NormalExit:
        switch (exitCode) {
        case 0:
            status = tr("c/clive exited normally.");
            break;
        default:
            status =
                QString(tr("c/clive exited with code %1, see log."))
                    .arg(exitCode);
            break;
        }
        break;
    default:
        status = cancelledFlag
            ? tr("c/clive terminated by user.")
            : tr("c/clive crashed, see log.");
        break;
    }

    updateLog(status);
    statusBar()->showMessage(status);

    addButton   ->setEnabled(true);
    pasteButton ->setEnabled(true);
    removeButton->setEnabled(true);
    clearButton ->setEnabled(true);
    rssButton   ->setEnabled(true);
    scanButton  ->setEnabled(true);
    startButton ->setEnabled(true);
    cancelButton->setEnabled(false);

    action_Download->setEnabled(true);

    action_Link ->setEnabled(true);
    action_RSS  ->setEnabled(true);
    action_Scan ->setEnabled(true);
    action_Paste->setEnabled(true);

    action_Remove->setEnabled(true);
    action_Clear->setEnabled(true);

    linksList   ->setEnabled(true);

    tabWidget   ->setTabEnabled(1, true);
}

void
MainWindow::onItemDoubleClicked(QListWidgetItem *item) {
    bool ok;

    QString lnk = QInputDialog::getText(this,
        QCoreApplication::applicationName(), tr("Edit link:"),
        QLineEdit::Normal, item->text(), &ok);

    if (ok && !lnk.isEmpty())
        item->setText(lnk);
}


