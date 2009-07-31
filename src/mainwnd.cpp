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
#include <QDebug>
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

#define critCcliveNotFound \
    do { \
    QMessageBox::critical(this, QCoreApplication::applicationName(), \
        QString( tr("c/clive executable not found, please check the path.") )); \
    } while (0)

MainWindow::MainWindow()
    : cancelled(false), isCclive(false)
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
    if (parseCcliveHostsOutput())
        parseCcliveVersionOutput();

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

    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(path, QStringList() << "--hosts");

    if (!proc.waitForFinished()) {
        qDebug() << path << ": " << proc.errorString();
        critCcliveNotFound;
        return false;
    }
    else {
        QString output  = QString::fromLocal8Bit(proc.readAll());
        QStringList lst = output.split("\n", QString::SkipEmptyParts);

        lst.removeLast(); // Note line.

        const int size = lst.size();
        for (register int i=0; i<size; ++i) {

            QString ln      = lst[i].remove("\r");
            QStringList tmp = ln.split("\t");

            if (!tmp[0].isEmpty() && !tmp[1].isEmpty())
                hosts[tmp[0]]   = tmp[1];
        }

        format->parseHosts(hosts);
    }

    return true;
}

void
MainWindow::parseCcliveVersionOutput() {

    versionOutput.clear();
    ccliveVersion.clear();
    curlVersion.clear();

    QString path = prefs->ccliveEdit->text();

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(path, QStringList() << "--version");

    isCclive = false;

    if (!proc.waitForFinished())
        qDebug() << path << ": " << proc.errorString();
    else {
        versionOutput = QString::fromLocal8Bit(proc.readAll());

        QStringList tmp = versionOutput.split("\n", QString::SkipEmptyParts);
        QStringList lst = tmp[0].split(" ", QString::SkipEmptyParts);

        isCclive = (lst[0] == "cclive");

        ccliveVersion = lst[2];
        curlVersion   = lst[6];
    }
}

bool
MainWindow::ccliveSupportsFeature(const QString& buildOption) {
    return versionOutput.contains(buildOption);
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
        if (ccliveSupportsFeature("--with-perl"))
            titleBox->show();
        else {
            titleBox->setCheckState(Qt::Unchecked);
            titleBox->hide();
        }
    }
}

void
MainWindow::closeEvent(QCloseEvent *event) {
    writeSettings();
    event->accept();
}

void
MainWindow::writeSettings() {
    QSettings s;
    s.beginGroup("MainWindow");
    s.setValue("size", size());
    s.setValue("pos", pos());
    s.setValue("titleBox", titleBox->checkState());
    s.endGroup();
}

void
MainWindow::readSettings() {
    QSettings s;
    s.beginGroup("MainWindow");
    resize( s.value("size", QSize(525,265)).toSize() );
    move( s.value("pos", QPoint(200,200)).toPoint() );
    titleBox->setCheckState(
        s.value("titleBox").toBool()
        ? Qt::Checked
        : Qt::Unchecked);
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
        parseCcliveHostsOutput();
        parseCcliveVersionOutput();
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

    if (linksList->count() == 0) {
        onAdd();
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
    QStringList env;

    if (isCclive) {
        args << "--print-fname";
    } else {
        args << "--stderr";
        // Set environment variables for clive
        env  << "COLUMNS=80" << "LINES=24" // Term::ReadKey
             << QString("HOME=%1").arg(QDir::homePath()); // $env{HOME}
    }

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

    args << "--continue"; // default to continue

    if (isCclive) { // clive defaults to this
        if (titleBox->isChecked()) {
            args << "--title";
            s = prefs->cclassEdit->text();
                if (!s.isEmpty())
            args << QString("--cclass=%1").arg(s);
        }
    } else { // clive can use this
        s = prefs->cclassEdit->text();
        if (!s.isEmpty())
            args << QString("--cclass=%1").arg(s);
    }

    // Check if all video page links are of the same host.

    QUrl first(linksList->item(0)->text());

    bool allSame    = true;
    const int count = linksList->count();

    for (register int i=0; i<count; ++i) {

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

    for (register int i=0; i<count; ++i)
        args << QString("%1").arg(linksList->item(i)->text());

    totalProgressbar->setMaximum(linksList->count());

    // Prepare log

    logEdit->clear();
    updateLog("% " +path+ " " +args.join(" ")+ "\n");

    // And finally start the process

    cancelled = false;
    process.setEnvironment(env);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(path,args);
}

void
MainWindow::onCancel() {
    cancelled = true;
    process.kill();
}

void
MainWindow::onAbout() {
    AboutDialog(this, ccliveVersion, curlVersion).exec();
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
    const int size  = lst.size();

    for (register int i=0; i<size; ++i)
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

    const int size = sel.size();

    for (register int i=0; i<size; ++i) {
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
    if (hosts.isEmpty()) {
        critCcliveNotFound;
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

    errorOccurred = false;
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
    QString newText =
        QString::fromLocal8Bit(process.readAll());

    QStringList tmp = newText.split("\n", QString::SkipEmptyParts);
    if (tmp.isEmpty())
        return;

    QString status, last = tmp.last();

    if (last.startsWith("fetch http://")) {
        status = tr("Fetching link...");
        totalProgressbar->setValue(totalProgressbar->value()+1);
    }
    else if (last.startsWith("verify") || last.startsWith("query length") )
        status = tr("Verifying video link...");
    else if (last.startsWith("error:"))
        errorOccurred = true;

    if (newText.contains("file:")) {
        QStringList tmp = newText.split(" ", QString::SkipEmptyParts);
        fileLabel->setText(tmp[1].remove("\n"));
        status = tr("Extracting video...");
    }

    if (!status.isEmpty())
        statusBar()->showMessage(status.remove("\n"));

    if (last.contains("%")) {
        newText.replace("\n", " ");

        QStringList tmp = newText.split(" ", QString::SkipEmptyParts);
        QString percent = tmp[1].remove(QChar('%'));
        QString now     = tmp[2];
        QString expected= tmp[4];
        QString rate    = tmp[5];
        QString eta     = tmp[6];

        sizeLabel->setText(now +" / "+ expected);
        progressBar->setValue(percent.toInt());
        rateLabel->setText(rate);
        etaLabel->setText(eta);
    }
    else
        updateLog(newText);
}

void
MainWindow::onProcFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QString status;
    if (!errorOccurred) {
        if (exitStatus == QProcess::NormalExit) {
            status = exitCode != 0
                ? tr("Process exited with an error. See Log for details")
                : tr("Process exited normally");
        } else {
            status = cancelled
                ? tr("Process terminated")
                : tr("Process crashed. See Log for details");
        }
        updateLog(status + ".");
    }
    else
        status = tr("Error(s) occurred. See Log for details.");

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
