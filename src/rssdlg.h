/*
 * abby Copyright (C) 2009,2010 Toni Gundogdu.
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
#ifndef rssdlg_h
#define rssdlg_h

#include "ui_rssdlg.h"

#include "httpmgr.h"

class RSSDialog : public QDialog, public Ui::rssDialog {
    Q_OBJECT
public:
    RSSDialog(QWidget *parent=0);
private slots:
    void onAdd();
    void onRemove();
    void onPaste();
    void onClear();
    void onFetch();
    void onFeedMgr();
    void onSelectAll();
    void onInvert();
    void onFetchFinished();
    void onFetchError(QString);
    void onFetchLink(QString);
public:
    void writeSettings();
private:
    void readSettings();
    void enableWidgets(const bool state=true);
    void resetUI();
    void addRSSLink(QString lnk);
private:
    QPointer<QHttpManager> mgr;
    int currentFeed;
    int expectedFeeds;
protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
};
#endif


