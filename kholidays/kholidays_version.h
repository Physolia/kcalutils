/*
    This file is part of libkholidays.
    Copyright (c) 2004 Allen Winter <winter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef KHOLIDAYS_KHOLIDAYS_VERSION_H
#define KHOLIDAYS_KHOLIDAYS_VERSION_H

#define LIBKHOLIDAYS_VERSION_STRING "1.0.0"
#define LIBKHOLIDAYS_VERSION_MAJOR 1
#define LIBKHOLIDAYS_VERSION_MINOR 1
#define LIBKHOLIDAYS_VERSION_RELEASE 0

#define LIBKHOLIDAYS_VERSION \
  KDE_MAKE_VERSION(LIBKHOLIDAYS_VERSION_MAJOR,\
                   LIBKHOLIDAYS_VERSION_MINOR,\
                   LIBKHOLIDAYS_VERSION_RELEASE)

#define LIBKHOLIDAYS_IS_VERSION(a,b,c) \
  ( LIBKHOLIDAYS_VERSION >= KDE_MAKE_VERSION(a,b,c) )

#endif
