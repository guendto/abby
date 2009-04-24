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
#ifndef prefsdlg_h
#define prefsdlg_h

#include "ui_prefsdlg.h"

class QDialog;

class PreferencesDialog : public QDialog, public Ui::PreferencesDialog {
    Q_OBJECT
public:
    PreferencesDialog(QWidget *parent=0);
private slots:
    void onProxyChanged(int index);
    void onLimitStateChanged(int state);
    void onFinished(int result);
    void onBrowseSaveDir();
    void onBrowseStreamCommand();
    void onBrowseCommand();
    void onBrowseCclive();
    void onLangChanged(int index);
    void onTimeoutStateChanged(int state);
private:
    void readSettings();
    void writeSettings();
    QStringList findQmFiles();
    QString langName(const QString &qmFile);
private:
    QStringList qmFiles;
    QString qmFile;
};
#endif
