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
#include <QDebug>

#include "formatdlg.h"

FormatDialog::FormatDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();
}

void
FormatDialog::parseHosts(const QStringMap& hosts) {

    hostBox->clear();

    this->hosts = hosts;

    for (QStringMap::const_iterator iter = hosts.begin();
        iter != hosts.end(); ++iter)
    {
        // Fill host combo with supported hosts.
        hostBox->addItem(iter.key());
    }

    updateFormats();
}

void
FormatDialog::updateFormats() {
    formatBox->clear();

    QString curr        = hostBox->currentText();
    QStringList formats = hosts[curr].split("|");

    for (register int i=0; i<formats.size(); ++i)
        formatBox->addItem(formats[i]);

    if (formats.size() > 1)
        formatBox->addItem("best");

    if (!sel[curr].isEmpty()) {
        // Qt should provide setCurrentText for QComboBox.
        // Instead we have to use a workaround in its absence.
        if (selN[curr] == -1)
            selN[curr] = 0;
        formatBox->setCurrentIndex(selN[curr]);
    }
}

void
FormatDialog::saveCurrent() {
    QString fmt = formatBox->currentText();
    if (fmt.isEmpty())
        fmt = "flv";
    if (lastHost.isEmpty())
        lastHost = hosts.begin().key();
    sel[lastHost]  = fmt;
    selN[lastHost] = formatBox->currentIndex();
    qDebug() << "saved:" << lastHost << sel[lastHost] << selN[lastHost];
}

void
FormatDialog::onHostChanged(const QString& host) {
    saveCurrent();
    updateFormats();
    lastHost = host;
}

void
FormatDialog::writeSettings() {
    QSettings s;

    s.beginGroup("FormatDialog");

    s.setValue("size", size());

/*    s.setValue("youtubeBox", youtubeBox->currentIndex());
    s.setValue("googleBox", googleBox->currentIndex());
    s.setValue("dailymotionBox", dailymotionBox->currentIndex());
    s.setValue("vimeoBox", vimeoBox->currentIndex());*/

    s.endGroup();
}

void
FormatDialog::readSettings() {
    QSettings s;

    s.beginGroup("FormatDialog");

    resize( s.value("size", QSize(400,140)).toSize() );

/*    youtubeBox->setCurrentIndex( s.value("youtubeBox").toInt() );
    googleBox->setCurrentIndex( s.value("googleBox").toInt() );
    dailymotionBox->setCurrentIndex( s.value("dailymotionBox").toInt() );
    vimeoBox->setCurrentIndex( s.value("vimeoBox").toInt() );*/

    s.endGroup();
}
