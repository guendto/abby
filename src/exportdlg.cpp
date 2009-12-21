/* 
* Copyright (C) 2009 Toni Gundogdu.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QDialog>
#include <QFileDialog>

#include "exportdlg.h"

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
}

void
ExportDialog::onBrowse() {
    const QString fname =
        QFileDialog::getSaveFileName(this, tr("Choose file"));

    if (fname.isEmpty())
        return;

    fileEdit->setText(fname);
}


