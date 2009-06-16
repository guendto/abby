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
#include <QIcon>
#include <QClipboard>
#include <QDebug>
#include <QInputDialog>

#include "mainwnd.h"
#include "prefsdlg.h"
#include "rssdlg.h"
#include "aboutdlg.h"

MainWindow::MainWindow():
    cancelled(false)
{
/*
 The word "English" is not meant to be translated literally.
 Replace the word "English" with target language, e.g. Suomi
 or Deutsch. Note that the only purpose of this string is to
 list it as an available language in the Preferences dialog.
*/
    const QString lang = tr("English");

    setupUi(this);
    readSettings();

    setWindowIcon(QIcon(":/rc/abby.png"));

    connect(&process, SIGNAL(started()),
        this, SLOT(onProcStarted()));
    connect(&process, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(onProcError(QProcess::ProcessError)));

    // NOTE: We read both channels stdout and stderr.
    connect(&process, SIGNAL(readyReadStandardOutput()),
        this, SLOT(onProcStdoutReady()));
    connect(&process, SIGNAL(readyReadStandardError()),
        this, SLOT(onProcStdoutReady()));

    connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(onProcFinished(int,QProcess::ExitStatus)));

    prefs = new PreferencesDialog(this);
    rss = new RSSDialog(this);

    updateWidgets();
}

bool
MainWindow::isCclive(const QString& path, QString& output) {
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(path, QStringList() << "--version");

    bool state = false;
    if (!process.waitForFinished())
        qDebug() << path << ": " << process.errorString();
    else {
        output = QString::fromLocal8Bit(process.readAll());
        QStringList lst = output.split(" ", QString::SkipEmptyParts);
        state = lst[0] == "cclive";
    }
    return state;
}

bool
MainWindow::ccliveSupports(const QString& buildOption) {
    QString output, path = prefs->ccliveEdit->text();
    const bool _isCclive = isCclive(path,output);

    if (!_isCclive && buildOption == "--with-perl")
        return false; // To keep the titleBox hidden always 

    return output.contains(buildOption);
}

void
MainWindow::updateWidgets() {
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

    if (ccliveSupports("--with-perl")) {
        titleBox->show();
    } else {
        titleBox->setCheckState(Qt::Unchecked);
        titleBox->hide();
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

void
MainWindow::updateFormats() {

    // TODO: See TODO file.

    // Alter widgets dynamically based on the video URL.

    QString url; // = urlEdit->toPlainText();

    if (url.isEmpty())
        return;

    struct lookup_s {
        const char *host;
        const char *formats;
    };
    static const struct lookup_s lookup[] = {
        {"youtube.com",     "fmt17|fmt18|fmt22|fmt35"},
//        {"video.google.",   "mp4"}, // missing in cclive 0.4.3+
        {"dailymotion.com", "spak-mini|vp6-hq|vp6-hd|vp6|h264"},
    };

    const int c = sizeof(lookup)/sizeof(struct lookup_s);

    QStringList formats;
    formats << "flv" << "best";

    for (register int i=0; i<c; ++i) {
        if (url.contains(lookup[i].host)) {
            QString s = lookup[i].formats;
            formats << s.split("|");
            break;
        }
    }

    QString last = formatCombo->currentText();

    formatCombo->clear();
    formatCombo->addItems(formats);

    int n = formatCombo->findText(last);
    if (n != -1)
        formatCombo->setCurrentIndex(n);
}


// Slots

void
MainWindow::onPreferences() {
    prefs->exec();
    updateWidgets();
}

void
MainWindow::onStreamStateChanged(int state) {
    streamSpin->setEnabled(state != 0);
}

void
MainWindow::onStart() {

    // Check cclive

    QString path = prefs->ccliveEdit->text();
    if (path.isEmpty()) {
        QMessageBox::information(this,QCoreApplication::applicationName(),
            tr("Specify path to either clive or cclive command in the "
                "Preferences."));
        onPreferences();
        return;
    }

    // Check video save directory

    // NOTE: cclive itself does not support this concept so we
    // work around it by changing the working directory to the
    // save directory.

    QString savedir = prefs->savedirEdit->text();
    if (savedir.isEmpty()) {
        QMessageBox::information(this,QCoreApplication::applicationName(),
            tr("Specify path to save directory for the downloaded videos "
                "in the Preferences."));
        onPreferences();
        return;
    }

    if (linksList->count() == 0)
        return;

    QString output;
    const bool _isCclive = isCclive(path,output);

    // clive can use this same approach even if --savedir option exists.
    process.setWorkingDirectory(savedir);

    // Construct cclive/clive args

    QStringList args;
    QStringList env;

    if (_isCclive) {
        args << "--print-fname";
    } else {
        args << "--stderr";
        // Set environment variables for clive
        env  << "COLUMNS=80" << "LINES=24" // Term::ReadKey
             << QString("HOME=%1").arg(QDir::homePath()); // $env{HOME}
    }

    QString s = prefs->additionalEdit->text();
    if (!s.isEmpty())
        args << s;

    s = prefs->streamEdit->text();
    if (!s.isEmpty() && streamBox->isChecked()) {
        args << QString("--stream-exec=%1").arg(s);
        args << QString("--stream=%1").arg(streamSpin->value());
    }

    s = prefs->commandEdit->text();
    if (!s.isEmpty() && commandBox->isChecked()) {
        if (!s.endsWith(";"))
            s += ";";
        args << QString("--exec=%1").arg(s);
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

    if (_isCclive) { // clive defaults to this
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

    s = formatCombo->currentText();
    if (s.isEmpty())
        s = "flv";

    args << QString("--format=%1").arg(s);

    for (register int i=0; i<linksList->count(); ++i)
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
    AboutDialog about(this,prefs->ccliveEdit->text());
    about.exec();
}

void
MainWindow::onRSS() {
    if (rss->exec() == QDialog::Accepted) {
        QTreeWidgetItemIterator iter(rss->itemsTree);
        while (*iter) {
            if ((*iter)->checkState(0) == Qt::Checked)
                addPageLink((*iter)->text(1));
            ++iter;
        }
    }
    rss->writeSettings();
}

void
MainWindow::onScan() {
}

void
MainWindow::onPasteURL() {
    QClipboard *cb = QApplication::clipboard();
    QStringList lst = cb->text().split("\n");
    for (register int i=0; i<lst.size(); ++i)
        addPageLink(lst[i]);
}

void
MainWindow::onAdd() {
    QString lnk = QInputDialog::getText(this,
        tr("Add new video page link"), tr("Enter link:"));
    addPageLink(lnk);
}

void
MainWindow::onRemove() {
    QList<QListWidgetItem*> sel = linksList->selectedItems();
    for (register int i=0; i<sel.size(); ++i) {
        const int row = linksList->row(sel[i]);
        delete linksList->takeItem(row);
    }
}

void
MainWindow::addPageLink(QString lnk) {
    if (lnk.isEmpty())
        return;

    lnk = lnk.trimmed();

    if (!lnk.startsWith("http://",Qt::CaseInsensitive))
        lnk.insert(0,"http://");

    QList<QListWidgetItem *> found
        = linksList->findItems(lnk, Qt::MatchExactly);

    if (found.size() == 0)
        linksList->addItem(lnk);

    updateFormats();
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

    startButton ->setEnabled(false);
    cancelButton->setEnabled(true);

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
    //qDebug() << last;

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
                ? tr("Process exited with an error. See Log for details.")
                : tr("Process exited normally.");
        } else {
            status = cancelled
                ? tr("Process terminated.")
                : tr("Process crashed. See Log for details.");
        }
        updateLog(status + ".");
    }
    else
        status = tr("Error(s) occurred. See Log for details.");

    statusBar()->showMessage(status);

    startButton ->setEnabled(true);
    cancelButton->setEnabled(false);
}
