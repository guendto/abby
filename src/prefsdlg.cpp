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
#include <QFileDialog>
#include <QTranslator>
#include <QMessageBox>

#include "prefsdlg.h"
#include "util.h"

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();

    langCombo->addItem("English");

    int sel = 0;
    qmFiles = findQmFiles();
    typedef unsigned int _uint;
    const register _uint size = qmFiles.size();
    for (register _uint i=0; i<size; ++i) {
        langCombo->addItem(langName(qmFiles[i]));
        if (qmFile == qmFiles[i])
            sel = i+1;
    }
    langCombo->setCurrentIndex(sel);

    connect(langCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onLangChanged(int)));

#ifdef WIN32
    streamLabel ->setHidden(true);
    streamEdit  ->setHidden(true);
    streamButton->setHidden(true);
#endif

    langLabel->setHidden(true);
    langCombo->setHidden(true);
}

void
PreferencesDialog::onFinished(int) {
    writeSettings();
}

void
PreferencesDialog::writeSettings() {
    QSettings s;

    s.beginGroup("PreferencesDialog");

    s.setValue("size", size());

    s.setValue("savedirEdit", savedirEdit->text());
    s.setValue("streamEdit", streamEdit->text());
    s.setValue("commandEdit", commandEdit->text());
    s.setValue("ccliveEdit", ccliveEdit->text());

    s.setValue("proxyCombo", proxyCombo->currentIndex());
    s.setValue("proxyEdit", proxyEdit->text());
    s.setValue("limitBox", limitBox->checkState());
    s.setValue("limitSpin", limitSpin->value());
    s.setValue("timeoutBox", timeoutBox->checkState());
    s.setValue("timeoutSpin", timeoutSpin->value());
    s.setValue("socksBox", socksBox->checkState());

    s.setValue("qmFile", qmFile);

    s.setValue("mintrayBox", mintrayBox->checkState());

    s.endGroup();
}

void
PreferencesDialog::readSettings() {
    QSettings s;

    s.beginGroup("PreferencesDialog");

    resize( s.value("size", QSize(525,205)).toSize() );

    savedirEdit->setText( s.value("savedirEdit").toString() );
    streamEdit->setText( s.value("streamEdit").toString() );
    commandEdit->setText( s.value("commandEdit").toString() );
    ccliveEdit->setText( s.value("ccliveEdit").toString() );

    proxyCombo->setCurrentIndex( s.value("proxyCombo").toInt() );
    proxyEdit->setText( s.value("proxyEdit").toString() );
    limitBox->setCheckState(
        s.value("limitBox").toBool()
        ? Qt::Checked
        : Qt::Unchecked);
    limitSpin->setValue( s.value("limitSpin").toInt() );
    timeoutBox->setCheckState(
        s.value("timeoutBox").toBool()
        ? Qt::Checked
        : Qt::Unchecked);
    timeoutSpin->setValue( s.value("timeoutSpin").toInt() );
    socksBox->setCheckState(
        s.value("socksBox").toBool()
        ? Qt::Checked
        : Qt::Unchecked);

    qmFile = s.value("qmFile").toString();

    mintrayBox->setCheckState(
        s.value("mintrayBox").toBool()
        ? Qt::Checked
        : Qt::Unchecked
    );

    s.endGroup();
}

QStringList
PreferencesDialog::findQmFiles() {
    QDir dir(":/i18n");

    QStringList fnames =
        dir.entryList(QStringList("*.qm"), QDir::Files, QDir::Name);

    QMutableStringListIterator iter(fnames);
    while (iter.hasNext()) {
        iter.next();
        iter.setValue(dir.filePath(iter.value()));
    }
    return fnames;
}

QString
PreferencesDialog::langName(const QString& qmFile) {
    QTranslator transl;
    transl.load(qmFile);
    return transl.translate("MainWindow","English");
}


// Slots

void
PreferencesDialog::onProxyChanged(int index) {
    proxyEdit->setEnabled(index == 1);
}

void
PreferencesDialog::onLimitStateChanged(int state) {
    limitSpin->setEnabled(state != 0);
}

void
PreferencesDialog::onBrowseSaveDir() {
    QString path =
        QFileDialog::getExistingDirectory(
            this,
            tr("Directory for saved videos"),
            QDir::currentPath()
        );

    if (!path.isEmpty())
        savedirEdit->setText(path);
}

void
PreferencesDialog::onBrowseStreamCommand() {
    QString fname = QFileDialog::getOpenFileName(this,tr("Choose command"));
    if (!fname.isEmpty())
        streamEdit->setText(fname);
}

void
PreferencesDialog::onBrowseCommand() {
    QString fname = QFileDialog::getOpenFileName(this,tr("Choose command"));
    if (!fname.isEmpty())
        commandEdit->setText(fname);
}

void
PreferencesDialog::onBrowseCclive() {
    QString fname = QFileDialog::getOpenFileName(this,tr("Choose cclive"));
    if (!fname.isEmpty())
        ccliveEdit->setText(fname);
}

static void
dump_detection_results(
    QWidget *parent,
    const QString& path,
    const QString& version,
    const QString& libVersion,
    const QString& libName,
    const bool isCcliveFlag,
    const bool showPathFlag=false)
{
    QString msg = QString(
        QObject::tr("Detected: %1 version %2 with %3 %4")
        .arg(isCcliveFlag ? "cclive" : "clive")
        .arg(version)
        .arg(libName)
        .arg(libVersion)
    );

    if (showPathFlag)
        msg += QString(QObject::tr("\nPath: %1")).arg(path);

    QMessageBox::information(
        parent,
        QCoreApplication::applicationName(),
        msg
    );
}

void
PreferencesDialog::onVerifyCclive() {
    try {
        QString path, version, libVersion, libName;
        bool isCcliveFlag;

        path = ccliveEdit->text();

        Util::verifyCclivePath(
            path,
            version,
            libVersion,
            libName,
            &isCcliveFlag
        );

        dump_detection_results(
            this,
            path,
            version,
            libVersion,
            libName,
            isCcliveFlag
        );
    }
    catch (const NoCcliveException& x) {
        QMessageBox::warning(this, tr("Warning"), x.what());
    }
}

void
PreferencesDialog::onAutodetectCclive() {
    try {
        QString path, version, libVersion, libName;
        bool isCcliveFlag = false;

        Util::detectCclive(
            path,
            version,
            libVersion,
            libName,
            &isCcliveFlag
        );

        ccliveEdit->setText(path);

        dump_detection_results(
            this,
            path,
            version,
            libVersion,
            libName,
            isCcliveFlag,
            true
        );
    }
    catch (const NoCcliveException& x) {
        QMessageBox::warning(this, tr("Warning"), x.what());
    }
}

void
PreferencesDialog::onLangChanged(int index) {
    qmFile = "";
    if (index > 0)
        qmFile = qmFiles[index-1];

    QMessageBox::information(this, QCoreApplication::applicationName(),
        tr("You need to restart the application for the language change "
            "to take effect."));
}

void
PreferencesDialog::onTimeoutStateChanged(int state) {
    timeoutSpin->setEnabled(state != 0);
    socksBox->setEnabled(state != 0);
}


