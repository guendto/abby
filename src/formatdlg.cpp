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

#include "formatdlg.h"

FormatDialog::FormatDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();
}

void
FormatDialog::writeSettings() {
    QSettings s;

    s.beginGroup("FormatDialog");

    s.setValue("size", size());

    s.setValue("youtubeBox", youtubeBox->currentIndex());
    s.setValue("googleBox", googleBox->currentIndex());
    s.setValue("dailymotionBox", dailymotionBox->currentIndex());
    s.setValue("vimeoBox", vimeoBox->currentIndex());

    s.endGroup();
}

void
FormatDialog::readSettings() {
    QSettings s;

    s.beginGroup("FormatDialog");

    resize( s.value("size", QSize(400,140)).toSize() );

    youtubeBox->setCurrentIndex( s.value("youtubeBox").toInt() );
    googleBox->setCurrentIndex( s.value("googleBox").toInt() );
    dailymotionBox->setCurrentIndex( s.value("dailymotionBox").toInt() );
    vimeoBox->setCurrentIndex( s.value("vimeoBox").toInt() );

    s.endGroup();
}
