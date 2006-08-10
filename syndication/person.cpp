/*
 * This file is part of the syndication library
 *
 * Copyright (C) 2006 Frank Osterfeld <frank.osterfeld@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "person.h"

#include <QString>

namespace Syndication {

Person::~Person()
{
}

QString Person::debugInfo() const
{
    QString info;
    info += "# Person begin ####################\n";
    
    QString dname = name();
    
    if (!dname.isNull())
    {
        info += "name: #" + dname + "#\n";
    }
    
    QString duri = uri();
    
    if (!duri.isNull())
    {
        info += "uri: #" + duri + "#\n";
    }
    
    QString demail = email();
    
    if (!demail.isNull())
    {
        info += "email: #" + demail + "#\n";
    }

    info += "# Person end ######################\n";
    
    return info;
}

bool Person::operator==(const Person& other) const
{
    return name() == other.name() && email() == other.email() && uri() == other.uri();
}
        
} // namespace Syndication
