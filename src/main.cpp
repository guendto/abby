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
#include <QApplication>
#include <QTranslator>
#include <QSettings>

#include "mainwnd.h"

int
main (int argc, char *argv[]) {
    QApplication app(argc,argv);

    QCoreApplication::setOrganizationName("abby");
    QCoreApplication::setApplicationName("abby");
    //QCoreApplication::setApplicationVersion("0.1.0"); // 4.4+
    QCoreApplication::setOrganizationDomain("code.google.com/p/abby");

    QSettings settings;
    settings.beginGroup("PreferencesDialog");
    QString qmFile = settings.value("qmFile").toString();
    settings.endGroup();

    QTranslator transl;
    if (qmFile.isEmpty())
        transl.load("abby_" + QLocale::system().name(), ":/i18n");
    else
        transl.load(qmFile);
    app.installTranslator(&transl);

    (new MainWindow)->show();
    return app.exec();
}
