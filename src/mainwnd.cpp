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

#include "mainwnd.h"
#include "prefsdlg.h"
#include "passdlg.h"
#include "aboutdlg.h"

MainWindow::MainWindow():
    cancelled(false)
{
    // NOTE: This string is displayed in the preferences dialog
    // languages selection. It is not intended to be translated,
    // but replaced with the name of the target language (in native).
    // e.g. Suomi, Deutsch, Svenska, Turkce, Francais, ...
    const QString lang = tr("English");

    setupUi(this);
    setWindowIcon(QIcon(":/rc/abby.png"));
    readSettings();

    prefs = new PreferencesDialog(this);

    connect(&process, SIGNAL(started()),
        this, SLOT(onProcStarted()));
    connect(&process, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(onProcError(QProcess::ProcessError)));
    connect(&process, SIGNAL(readyReadStandardError()),
        this, SLOT(onProcStderrReady()));
    connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(onProcFinished(int,QProcess::ExitStatus)));
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
    s.setValue("size",size());
    s.setValue("pos",pos());
    s.setValue("formatCombo",formatCombo->currentIndex());
    s.endGroup();
}

void
MainWindow::readSettings() {
    QSettings s;
    s.beginGroup("MainWindow");
    resize(s.value("size",QSize(400,400)).toSize());
    move(s.value("pos",QPoint(200,200)).toPoint());
    formatCombo->setCurrentIndex(s.value("formatCombo").toInt());
    s.endGroup();
}

void
MainWindow::updateLog(QString newText) {
    QString text = logEdit->toPlainText() + newText;
    logEdit->setPlainText(text);
}


// Slots

void
MainWindow::onPreferences() {
    prefs->exec();
}

void
MainWindow::onCommandStateChanged(int state) {
    preferencesButton->setEnabled(state != 0);
}

void
MainWindow::onSaveasStateChanged(int state) {
    saveasEdit->setEnabled(state != 0);
}

void
MainWindow::onSaveasBrowse() {
    QString fname = QFileDialog::getSaveFileName(this,tr("Save as")); 
    if (!fname.isEmpty())
        saveasEdit->setText(fname);
}

void
MainWindow::onStart() {
    // Check video URL

    QString url = urlEdit->text();

    if (url.isEmpty()) {
        statusBar()->showMessage(tr("Enter a video link."));
        return;
    }

    url = url.trimmed();

    if (!url.startsWith("http://",Qt::CaseInsensitive))
        url.insert(0,"http://");

    // Check cclive

    QString cclive = prefs->ccliveEdit->text();
    if (cclive.isEmpty()) {
        QMessageBox::information(this,QCoreApplication::applicationName(),
            tr("Path to cclive command undefined. See preferences."));
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
            tr("Save directory undefined. See preferences."));
        onPreferences();
        return;
    }
    process.setWorkingDirectory(savedir);

    // Construct cclive args

    QStringList args;
    args << "--print-fname";

    QString s = prefs->additionalEdit->text();
    if (!s.isEmpty())
        args << s;

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

    if (prefs->youtubeGroup->isChecked()) {
        QString user = prefs->ytuserEdit->text();
        QString pass = prefs->ytpassEdit->text();
        if (pass.isEmpty()) {
            PasswordDialog pwd(this);
            if (pwd.exec())
                pass = pwd.passEdit->text();
        }
        if (!user.isEmpty() && !pass.isEmpty()) {
            args << QString("--youtube-user=%1").arg(user);
            args << QString("--youtube-pass=%1").arg(pass);
        }
    }

    s = saveasEdit->text();
    if (!s.isEmpty())
        args << QString("--output-video=%1").arg(s);

    if (continueBox->isChecked())
        args << "--continue";

    args << QString("--download=%1").arg(formatCombo->currentText());
    args << QString("%1").arg(url);

    // Prepare log

    logEdit->clear();
    updateLog("% cclive " +args.join(" ")+ "\n");

    // And finally start the process

    cancelled = false;
    process.start(cclive,args);
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
MainWindow::onProcStarted() {
    statusBar() ->clearMessage();
    fileLabel   ->setText("-");
    sizeLabel   ->setText("-- / --");
    rateLabel   ->setText("--.-");
    etaLabel    ->setText("--:--");
    progressBar ->setValue(0);

    startButton ->setEnabled(false);
    cancelButton->setEnabled(true);
}

void
MainWindow::onProcError(QProcess::ProcessError err) {
    if (err == QProcess::FailedToStart) {
        QString msg = tr("error: failed to start process");
        updateLog(msg);
        statusBar()->showMessage(msg);
    }
}

void
MainWindow::onProcStderrReady() {
    QString newText =
        QString::fromLocal8Bit(process.readAllStandardError());

    QString status;
    if (newText.contains("fetch"))
        status = tr("Fetching link...");
    else if (newText.contains("verify"))
        status = tr("Verifying video link...");
    else if (newText.contains("error:")) {
        // Probably better to connect finished etc. signal instead
        status = tr("error: see log for details");
    }
    else if (newText.contains("file:")) {
        status = tr("Extracting video...");
        QStringList lst = newText.split(" ",QString::SkipEmptyParts);
        fileLabel->setText(lst[1].remove("\n"));
    }

    if (!status.isEmpty())
        statusBar()->showMessage(status.remove("\n"));

    if (newText.contains("%")) {
        newText.replace("\n", " ");

        QStringList lst =
            newText.split(" ",QString::SkipEmptyParts);

        QString percent = lst[1].remove(QChar('%'));
        QString now     = lst[2];
        QString expected= lst[4];
        QString rate    = lst[5];
        QString eta     = lst[6];

        sizeLabel->setText(now +" / "+ expected);
        rateLabel->setText(rate);
        etaLabel->setText(eta);
        progressBar->setValue(percent.toInt());
    } else {
        updateLog(newText);
    }
}

void
MainWindow::onProcFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QString status;
    if (exitStatus == QProcess::NormalExit) {
        if (exitCode != 0)
            status = tr("Process exited with an error; see log");
        else
            status = tr("Process exited normally");
    } else {
        status = cancelled
            ? tr("Process terminated")
            : tr("Process crashed; see log");
    }

    if (!statusBar()->currentMessage().contains(tr("error:")))
        statusBar()->showMessage(status);

    updateLog(status + ".");

    startButton ->setEnabled(true);
    cancelButton->setEnabled(false);
}
