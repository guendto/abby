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

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();

    langCombo->addItem("English");

    int sel = 0;
    qmFiles = findQmFiles();
    for (int i=0; i<qmFiles.size(); ++i) {
        langCombo->addItem(langName(qmFiles[i]));
        if (qmFile == qmFiles[i])
            sel = i+1;
    }
    langCombo->setCurrentIndex(sel);

    connect(langCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onLangChanged(int)));
}

void
PreferencesDialog::onFinished(int) {
    writeSettings();
}

void
PreferencesDialog::writeSettings() {
    QSettings s;

    s.beginGroup("PreferencesDialog");

    s.setValue("size",size());
    //s.setValue("pos",pos());

    s.setValue("savedirEdit",savedirEdit->text());
    s.setValue("streamEdit",streamEdit->text());
    s.setValue("commandEdit",commandEdit->text());
    s.setValue("ccliveEdit",ccliveEdit->text());
    s.setValue("additionalEdit",additionalEdit->text());

    s.setValue("proxyCombo",proxyCombo->currentIndex());
    s.setValue("proxyEdit",proxyEdit->text());
    s.setValue("limitBox",limitBox->checkState());
    s.setValue("limitSpin",limitSpin->value());

    s.setValue("youtubeGroup",youtubeGroup->isChecked());
    s.setValue("ytuserEdit",ytuserEdit->text());
    s.setValue("ytpassEdit",ytpassEdit->text());

    s.setValue("qmFile",qmFile);

    s.endGroup();
}

void
PreferencesDialog::readSettings() {
    QSettings s;

    s.beginGroup("PreferencesDialog");

    resize(s.value("size",QSize(525,205)).toSize());
    //move(s.value("pos",QPoint(200,200)).toPoint());

    savedirEdit->setText(s.value("savedirEdit").toString());
    streamEdit->setText(s.value("streamEdit").toString());
    commandEdit->setText(s.value("commandEdit").toString());
    ccliveEdit->setText(s.value("ccliveEdit").toString());
    additionalEdit->setText(s.value("additionalEdit").toString());

    proxyCombo->setCurrentIndex(s.value("proxyCombo").toInt());
    proxyEdit->setText(s.value("proxyEdit").toString());
    limitBox->setCheckState(
        s.value("limitBox").toBool()
        ? Qt::Checked
        : Qt::Unchecked);
    limitSpin->setValue(s.value("limitSpin").toInt());

    youtubeGroup->setChecked(s.value("youtubeGroup").toBool());
    ytuserEdit->setText(s.value("ytuserEdit").toString());
    ytpassEdit->setText(s.value("ytpassEdit").toString());

    qmFile = s.value("qmFile").toString();

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
PreferencesDialog::langName(const QString &qmFile) {
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
    QFileDialog dlg(this);

    dlg.setFileMode(QFileDialog::Directory);
    dlg.setViewMode(QFileDialog::Detail);
    dlg.setDirectory(savedirEdit->text());

    if (dlg.exec())
        savedirEdit->setText(dlg.selectedFiles()[0]);
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

void
PreferencesDialog::onLangChanged(int index) {
    qmFile = "";
    if (index > 0)
        qmFile = qmFiles[index-1];

    QMessageBox::information(this, QCoreApplication::applicationName(),
        tr("You need to restart the application for the language change "
            "to take effect."));
}
