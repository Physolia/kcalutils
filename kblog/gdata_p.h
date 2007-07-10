/*
    This file is part of the kblog library.

    Copyright (c) 2007 Christian Weilbach <christian@whiletaker.homeip.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef API_GDATA_P_H
#define API_GDATA_P_H

#include <QtCore/QString>
#include <syndication/loader.h>
#include "gdata.h"

using namespace KBlog;

class APIGData::APIGDataPrivate : public QObject {
  Q_OBJECT
  private:
    QString mCreatePostingsPath;
    QString mFetchPostingsPath;
  public:
    QString mFetchPostingId;//HACK
    APIGData* parent;
    APIGDataPrivate();
    ~APIGDataPrivate();
    void getIntrospection();
    QString getFetchPostingsPath(){ return mFetchPostingsPath; }
    QString getCreatePostingPath(){ return mCreatePostingsPath; }
  public slots:
    void slotLoadingPostingsComplete(Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode);
    void slotFetchingPostingComplete(Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode);
    void slotLoadingBlogsComplete(Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode);
};

#endif