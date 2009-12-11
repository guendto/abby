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
#ifndef scandlg_h
#define scandlg_h

#include "ui_scandlg.h"
#include "httpmgr.h"

class QDialog;

class ScanDialog : public QDialog, public Ui::scanDialog {
    Q_OBJECT
public:
    ScanDialog(QWidget *parent);
private slots:
    void onScan();
    void onSelectAll();
    void onInvert();
    void onFetchFinished();
    void onFetchTitlesFinished();
    void onFetchError(QString);
    void onFetchLink(QString);
public:
    void writeSettings();
private:
    void readSettings();
    void enableWidgets(const bool state=true);
    void resetUI();
    void updateCount();
private:
    QPointer<QHttpManager> mgr;
    QPointer<QHttpManager> mgrt; // for fetching titles
    int fetchedTitles;
    int expectedTitles;
    QStringList videoLinks;
};

#endif

