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
#ifndef mainwnd_h
#define mainwnd_h

#include <QProcess>

#include "ui_mainwnd.h"

class QMainWindow;
class PreferencesDialog;
class RSSDialog;
class ScanDialog;

class MainWindow : public QMainWindow, private Ui::MainWindow {
    Q_OBJECT
public:
    MainWindow();
private slots:
    void onPreferences();
    void onStart();
    void onCancel();
    void onAbout();
    void onStreamStateChanged(int state);
    void onRSS();
    void onScan();
    void onPasteURL();
    void onAdd();
    void onRemove();
    void onProcStarted();
    void onProcError(QProcess::ProcessError);
    void onProcStdoutReady();
    void onProcFinished(int exitCode, QProcess::ExitStatus exitStatus);
private:
    PreferencesDialog *prefs;
    RSSDialog *rss;
    ScanDialog *scan;
    QProcess process;
    bool cancelled;
    bool errorOccurred;
private:
    void addPageLink(QString lnk);
    void writeSettings();
    void readSettings();
    void updateLog(const QString& text);
    bool ccliveSupports(const QString& buildOption);
    void updateWidgets();
    void updateFormats();
    bool isCclive(const QString& path, QString& output);
protected:
    void closeEvent(QCloseEvent *event);
};
#endif
