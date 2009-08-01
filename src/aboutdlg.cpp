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

#include "aboutdlg.h"

AboutDialog::AboutDialog(QWidget *parent,
    const QString& ccliveVersion, const QString& curlVersion)
    : QDialog(parent)
{
    setupUi(this);

    abbyVersionLabel->setText("0.4.3");
    //QCoreApplication::applicationVersion()); // 4.4+
    qtVersionLabel->setText(qVersion());

    ccliveVersionLabel->setText(ccliveVersion);
    curlVersionLabel->setText(curlVersion);
}
