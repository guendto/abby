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

#ifndef util_h
#define util_h

class Util {
public:
    static void detectCclive(
        QString& path,
        QString& version,
        QString& libVersion,
        QString& libName,
        bool *isCcliveFlag
    );
    static void verifyCclivePath(
        const QString& path,
        QString& version,
        QString& libVersion,
        QString& libName,
        bool *isCcliveFlag=NULL
    );
};

class NoCcliveException {
public:
    NoCcliveException(const QString& errmsg);
    NoCcliveException(const QString& path, const QString& errmsg);
    NoCcliveException(const QString& path,
                        const int& exitCode, const QString& output);
public:
    const QString& what() const;
private:
    QString errmsg;
};

#endif


