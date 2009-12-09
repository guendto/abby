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
class FormatDialog;

typedef QMap<QString,QString> QStringMap;

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
    void onClear();
    void onFormats();
    void onProcStarted();
    void onProcError(QProcess::ProcessError);
    void onProcStdoutReady();
    void onProcFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onItemDoubleClicked(QListWidgetItem *item);
private:
    QStringMap hosts;
    PreferencesDialog *prefs;
    RSSDialog *rss;
    ScanDialog *scan;
    FormatDialog *format;
    QProcess process;
    bool cancelledFlag;
    bool isCcliveFlag;
    QString ccliveVersion;
    QString libVersion;
    QString libName;
private:
    void addPageLink(QString lnk);
    void writeSettings();
    void readSettings();
    void updateWidgets(const bool updateCcliveDepends);
    void setProxy();
    bool ccliveSupportsHost(const QString &lnk);
    bool parseCcliveHostsOutput();
    bool parseCcliveVersionOutput();
protected:
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
};
#endif


