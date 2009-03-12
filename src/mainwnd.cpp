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
#include <QDebug>

#include "mainwnd.h"
#include "prefsdlg.h"
#include "passdlg.h"
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

    updateWidgets();
}

bool
MainWindow::ccliveSupports(QString buildOption) {
    bool state = false;
    QString cclivePath = prefs->ccliveEdit->text();
    if (!cclivePath.isEmpty()) {
        QProcess cclive;
        cclive.setProcessChannelMode(QProcess::MergedChannels);
        cclive.start(cclivePath, QStringList() << "--version");

        if (!cclive.waitForFinished())
            qDebug() << "cclive failed:" << cclive.errorString();
        else {
            QString output = QString::fromLocal8Bit(cclive.readAll());
            return output.contains(buildOption);
        }
    }
    return state;
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

    if (ccliveSupports("--with-perl"))
        titleBox->show();
    else {
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
    s.setValue("size",size());
    s.setValue("pos",pos());
    s.setValue("titleBox",titleBox->checkState());
    s.setValue("formatCombo",formatCombo->currentIndex());
    s.endGroup();
}

void
MainWindow::readSettings() {
    QSettings s;
    s.beginGroup("MainWindow");
    resize(s.value("size",QSize(525,265)).toSize());
    move(s.value("pos",QPoint(200,200)).toPoint());
    titleBox->setCheckState(
        s.value("titleBox").toBool()
        ? Qt::Checked
        : Qt::Unchecked);
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
    updateWidgets();
}

void
MainWindow::onSaveasStateChanged(int state) {
    saveasEdit  ->setEnabled(state != 0);
    saveasButton->setEnabled(state != 0);

    if (state != 0 && titleBox->isChecked())
        titleBox->setCheckState(Qt::Unchecked);

    titleBox->setEnabled(state == 0);
}

void
MainWindow::onStreamStateChanged(int state) {
    streamSpin->setEnabled(state != 0);
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
    if (!s.isEmpty() && saveasBox->isChecked())
        args << QString("--output-video=%1").arg(s);

    if (continueBox->isChecked())
        args << "--continue";

    if (titleBox->isChecked()) {
        args << "--title";
        s = prefs->cclassEdit->text();
        if (!s.isEmpty())
            args << QString("--title-cclass=%1").arg(s);
    }

    s = formatCombo->currentText();
    if (s.isEmpty())
        s = "flv";
    args << QString("--download=%1").arg(s);
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
MainWindow::onURLEditingFinished() {

    // Change format combobox contents dynamically based on the video URL.

    QString url = urlEdit->text();
    if (url.isEmpty())
        return;

    struct lookup_s {
        const char *host;
        const char *formats;
    };
    static const struct lookup_s lookup[] = {
        {"youtube.com",     "mp4|xflv|3gpp"},
        {"video.google.",   "mp4"},
        {"dailymotion.com", "spak-mini|vp6-hq|vp6-hd|vp6|h264"},
    };

    const int c = sizeof(lookup)/sizeof(struct lookup_s);

    QStringList formats;
    formats << "flv";

    for (int i=0; i<c; ++i) {
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

void
MainWindow::onFormatStateChanged(int) {

    // Disable --continue for the specified hosts if format is "flv".
    // Make changes based on the video URL.

    QString url = urlEdit->text();
    if (url.isEmpty())
        return;

    struct lookup_s {
        const char *host;
    };
    static const struct lookup_s lookup[] = {
        {"youtube.com"},
        {"video.google."},
    };

    const int c = sizeof(lookup)/sizeof(struct lookup_s);
    bool enable = true;

    for (int i=0; i<c; ++i) {
        if (url.contains(lookup[i].host)) {
            if (formatCombo->currentText() == "flv") {
                enable = false;
                break;
            }
        }
    }

    continueBox->setEnabled(enable);

    if (continueBox->isChecked() && !enable)
        continueBox->setCheckState(Qt::Unchecked);
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

    errorOccurred = false;
}

void
MainWindow::onProcError(QProcess::ProcessError err) {
    if (err == QProcess::FailedToStart) {
        QString msg = tr("error: failed to start process");
        statusBar()->showMessage(msg);
        updateLog(msg);
    }
}

void
MainWindow::onProcStderrReady() {
    QString newText =
        QString::fromLocal8Bit(process.readAllStandardError());

    QStringList tmp = newText.split("\n", QString::SkipEmptyParts);
    if (tmp.isEmpty())
        return;

    QString status, last = tmp.last();

    if (last.startsWith("fetch"))
        status = tr("Fetching link...");
    else if (last.startsWith("verify"))
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
                ? tr("Process exited with an error; see log")
                : tr("Process exited normally");
        } else {
            status = cancelled
                ? tr("Process terminated")
                : tr("Process crashed; see log");
        }
        updateLog(status + ".");
    }
    else
        status = tr("error: see log for details");

    statusBar()->showMessage(status);

    startButton ->setEnabled(true);
    cancelButton->setEnabled(false);
}
