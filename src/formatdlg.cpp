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

/* The contents of this dialog changes dynamically based on
 * c/clive --hosts output. abby tries to make switching
 * between cclive and clive as transparent as possible as
 * there may be differences in supported websites. */
 
/* Because QComboBox does not have setCurrentText we have to
 * use two maps to keep track of the selections instead of
 * only one. See sel and selN. The former stores selected
 * "format ID", and the latter the matching QComboBox index
 * to that ID (needed for setCurrentIndex, due to lack of
 * setCurrentText). */

FormatDialog::FormatDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();
}

void
FormatDialog::resetHosts() {
    hostBox->clear();
}

void
FormatDialog::parseHosts(const QStringMap& hosts) {
    this->hosts = hosts;
    for (QStringMap::const_iterator iter = hosts.begin();
        iter != hosts.end(); ++iter)
    {
        // Fill host combo with supported hosts.
        hostBox->addItem(iter.key());
    }
    readFormatSettings();
    updateFormats();
}

void
FormatDialog::updateFormats() {

    formatBox->clear();

    const QString curr = hostBox->currentText();

    if (curr.isEmpty())
        return;

    const QStringList formats = hosts[curr].split("|");

    typedef unsigned int _uint;
    const register _uint size = formats.size();

    for (register _uint i=0; i<size; ++i)
        formatBox->addItem(formats[i]);

    if (size > 1)
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

    if (hostBox->count() == 0)
        return;

    QString fmt = formatBox->currentText();
    if (fmt.isEmpty())
        fmt = "flv";

    if (lastHost.isEmpty())
        lastHost = hosts.begin().key();

    sel[lastHost]  = fmt;
    selN[lastHost] = formatBox->currentIndex();

//    qDebug() << "saved:" << lastHost << sel[lastHost] << selN[lastHost];
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
    writeFormatSettings(s);
    s.endGroup();
}

void
FormatDialog::writeFormatSettings(QSettings& s) {
    s.beginGroup("hosts");
    for (QStringMap::const_iterator iter = sel.begin();
        iter != sel.end(); ++iter)
    {
        const QString curr = iter.key();
        s.setValue(iter.key(),
            QString("%1|%2").arg(sel[curr]).arg(selN[curr]));
    }
    s.endGroup();
}

void
FormatDialog::readSettings() {
    QSettings s;
    s.beginGroup("FormatDialog");
    resize( s.value("size", QSize(400,140)).toSize() );
    s.endGroup();
}

void
FormatDialog::readFormatSettings() {
    QSettings s;

    s.beginGroup("FormatDialog");
    s.beginGroup("hosts");

    for (QStringMap::const_iterator iter = hosts.begin();
        iter != hosts.end(); ++iter)
    {
        const QString curr    = iter.key();
        const QString v       = s.value(iter.key()).toString();
        const QStringList tmp = v.split("|");

        if (tmp.size() == 2) {
            sel[curr] = tmp[0];  // format string
            selN[curr] = tmp[1].toInt(); // last known index in combobox
        }
    }

    s.endGroup();
    s.endGroup();
}

const QString
FormatDialog::getFormatSetting(const QString& url) const {
    for (QStringMap::const_iterator iter = sel.begin();
        iter != sel.end(); ++iter)
    {
        QRegExp re(iter.key());
        if (re.indexIn(url) != -1)
            return sel[iter.key()];
    }
    return "flv";
}
