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
#ifndef formatdlg_h
#define formatdlg_h

#include "ui_formatdlg.h"

class QDialog;
class QSettings;

typedef QMap<QString,QString> QStringMap;

class FormatDialog : public QDialog, public Ui::formatDialog {
    Q_OBJECT
public:
    FormatDialog(QWidget *parent);
public:
    void resetHosts();
    void parseHosts(const QStringMap& hosts);
    void saveCurrent();
    void writeSettings();
    const QString getFormatSetting(const QString& url) const;
private:
    void readSettings();
    void readFormatSettings();
    void writeFormatSettings(QSettings& s);
    void updateFormats();
private slots:
    void onHostChanged(const QString& host);
private:
    QMap<QString, int> selN;
    QStringMap hosts, sel;
    QString lastHost;
};

#endif
