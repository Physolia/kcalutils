// -*- c-basic-offset: 2 -*-
/*
    This file is part of libkabc.
    Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2004 Szombathelyi György <gyurco@freemail.hu>

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
#include <stdlib.h>

#include <QBuffer>
#include <QEventLoop>
#include <QFile>

#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstringhandler.h>
#include <ktemporaryfile.h>
#include <kio/netaccess.h>

#include "kldap/ldif.h"
#include "kldap/ldapurl.h"

#include "resourceldapkio.h"
#include "resourceldapkioconfig.h"

using namespace KABC;

class ResourceLDAPKIO::ResourceLDAPKIOPrivate
{
  public:
    KLDAP::Ldif mLdif;
    bool mTLS,mSSL,mSubTree;
    QString mResultDn;
    Addressee mAddr;
    Address mAd;
    Resource::Iterator mSaveIt;
    bool mSASL;
    QString mMech;
    QString mRealm, mBindDN;
    KLDAP::LdapUrl mLDAPUrl;
    int mVer, mSizeLimit, mTimeLimit, mRDNPrefix;
    int mError;
    int mCachePolicy;
    bool mReadOnly;
    bool mAutoCache;
    QString mCacheDst;
    KTemporaryFile *mTmp;
};

ResourceLDAPKIO::ResourceLDAPKIO()
  : Resource()
{
  d = new ResourceLDAPKIOPrivate;
  mPort = 389;
  mAnonymous = true;
  mUser = mPassword = mHost =  mFilter =  mDn = "";
  d->mMech = d->mRealm = d->mBindDN = "";
  d->mTLS = d->mSSL = d->mSubTree = d->mSASL = false;
  d->mVer = 3; d->mRDNPrefix = 0;
  d->mTimeLimit = d->mSizeLimit = 0;
  d->mCachePolicy = Cache_No;
  d->mAutoCache = true;
  d->mCacheDst = KGlobal::dirs()->saveLocation("cache", "ldapkio") + '/' +
    type() + '_' + identifier();
  init();
}

ResourceLDAPKIO::ResourceLDAPKIO( const KConfigGroup &group )
  : Resource( group )
{
  d = new ResourceLDAPKIOPrivate;
  QMap<QString, QString> attrList;
  QStringList attributes = group.readEntry( "LdapAttributes", QStringList() );
  for ( int pos = 0; pos < attributes.count(); pos += 2 )
    mAttributes.insert( attributes[ pos ], attributes[ pos + 1 ] );

  mUser = group.readEntry( "LdapUser" );
  mPassword = KStringHandler::obscure( group.readEntry( "LdapPassword" ) );
  mDn = group.readEntry( "LdapDn" );
  mHost = group.readEntry( "LdapHost" );
  mPort = group.readEntry( "LdapPort", 389 );
  mFilter = group.readEntry( "LdapFilter" );
  mAnonymous = group.readEntry( "LdapAnonymous" , false );
  d->mTLS = group.readEntry( "LdapTLS" , false );
  d->mSSL = group.readEntry( "LdapSSL" , false );
  d->mSubTree = group.readEntry( "LdapSubTree" , false );
  d->mSASL = group.readEntry( "LdapSASL" , false );
  d->mMech = group.readEntry( "LdapMech" );
  d->mRealm = group.readEntry( "LdapRealm" );
  d->mBindDN = group.readEntry( "LdapBindDN" );
  d->mVer = group.readEntry( "LdapVer", 3 );
  d->mTimeLimit = group.readEntry( "LdapTimeLimit", 0 );
  d->mSizeLimit = group.readEntry( "LdapSizeLimit", 0 );
  d->mRDNPrefix = group.readEntry( "LdapRDNPrefix", 0 );
  d->mCachePolicy = group.readEntry( "LdapCachePolicy", 0 );
  d->mAutoCache = group.readEntry("LdapAutoCache", true );
  d->mCacheDst = KGlobal::dirs()->saveLocation("cache", "ldapkio") + '/' +
    type() + '_' + identifier();
  init();
}

ResourceLDAPKIO::~ResourceLDAPKIO()
{
  delete d;
}

void ResourceLDAPKIO::enter_loop()
{
  QEventLoop eventLoop;
  connect(this, SIGNAL(leaveModality()),
          &eventLoop, SLOT(quit()));
  eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

void ResourceLDAPKIO::entries( KIO::Job*, const KIO::UDSEntryList & list )
{
  KIO::UDSEntryList::ConstIterator it = list.begin();
  KIO::UDSEntryList::ConstIterator end = list.end();
  for (; it != end; ++it) {
    const QString urlStr = (*it).stringValue( KIO::UDS_URL );
    if ( !urlStr.isEmpty() ) {
      KUrl tmpurl( urlStr );
      d->mResultDn = tmpurl.path();
      kDebug(7125) << "findUid(): " << d->mResultDn << endl;
      if ( d->mResultDn.startsWith("/") ) d->mResultDn.remove(0,1);
      return;
    }
  }
}

void ResourceLDAPKIO::listResult( KJob *job)
{
  d->mError = job->error();
  if ( d->mError && d->mError != KIO::ERR_USER_CANCELED )
    mErrorMsg = job->errorString();
  else
    mErrorMsg = "";
  emit leaveModality();
}

QString ResourceLDAPKIO::findUid( const QString &uid )
{
  KLDAP::LdapUrl url( d->mLDAPUrl );
  KIO::UDSEntry entry;

  mErrorMsg = d->mResultDn = "";

  url.setAttributes(QStringList( "dn" ));
  url.setFilter( '(' + mAttributes[ "uid" ] + '=' + uid + ')' + mFilter );
  url.setExtension( "x-dir", "one" );

  kDebug(7125) << "ResourceLDAPKIO::findUid() uid: " << uid << " url " <<
    url.prettyUrl() << endl;

  KIO::ListJob * listJob = KIO::listDir( url, false /* no GUI */ );
  connect( listJob,
    SIGNAL( entries( KIO::Job *, const KIO::UDSEntryList& ) ),
    SLOT( entries( KIO::Job*, const KIO::UDSEntryList& ) ) );
  connect( listJob, SIGNAL( result( KJob* ) ),
    this, SLOT( listResult( KJob* ) ) );

  enter_loop();
  return d->mResultDn;
}

QByteArray ResourceLDAPKIO::addEntry( const QString &attr, const QString &value, bool mod )
{
  QByteArray tmp;
  if ( !attr.isEmpty() ) {
    if ( mod ) tmp += KLDAP::Ldif::assembleLine( "replace", attr ) + '\n';
    tmp += KLDAP::Ldif::assembleLine( attr, value ) + '\n';
    if ( mod ) tmp += "-\n";
  }
  return ( tmp );
}

bool ResourceLDAPKIO::AddresseeToLDIF( QByteArray &ldif, const Addressee &addr,
  const QString &olddn )
{
  QByteArray tmp;
  QString dn;
  QByteArray data;
  bool mod = false;

  if ( olddn.isEmpty() ) {
    //insert new entry
    switch ( d->mRDNPrefix ) {
      case 1:
        dn = mAttributes[ "uid" ] + '=' + addr.uid() + ',' +mDn;
        break;
      case 0:
      default:
        dn = mAttributes[ "commonName" ] + '=' + addr.assembledName() + ',' +mDn;
        break;
    }
  } else {
    //modify existing entry
    mod = true;
    if ( olddn.startsWith( mAttributes[ "uid" ] ) ) {
      dn = mAttributes[ "uid" ] + '=' + addr.uid() + ',' + olddn.section( ',', 1 );
    } else if ( olddn.startsWith( mAttributes[ "commonName" ] ) ) {
      dn = mAttributes[ "commonName" ] + '=' + addr.assembledName() + ',' +
        olddn.section( ',', 1 );
    } else {
      dn = olddn;
    }

    if ( olddn.toLower() != dn.toLower() ) {
      tmp = KLDAP::Ldif::assembleLine( "dn", olddn ) + '\n';
      tmp += "changetype: modrdn\n";
      tmp += KLDAP::Ldif::assembleLine( "newrdn", dn.section( ',', 0, 0 ) ) + '\n';
      tmp += "deleteoldrdn: 1\n\n";
    }
  }


  tmp += KLDAP::Ldif::assembleLine( "dn", dn ) + '\n';
  if ( mod ) tmp += "changetype: modify\n";
  if ( !mod ) {
    tmp += "objectClass: top\n";
    QStringList obclass = mAttributes[ "objectClass" ].split(',', QString::SkipEmptyParts);
    for ( QStringList::const_iterator it = obclass.constBegin(); it != obclass.constEnd(); ++it ) {
      tmp += KLDAP::Ldif::assembleLine( "objectClass", *it ) + '\n';
    }
  }

  tmp += addEntry( mAttributes[ "commonName" ], addr.assembledName(), mod );
  tmp += addEntry( mAttributes[ "formattedName" ], addr.formattedName(), mod );
  tmp += addEntry( mAttributes[ "givenName" ], addr.givenName(), mod );
  tmp += addEntry( mAttributes[ "familyName" ], addr.familyName(), mod );
  tmp += addEntry( mAttributes[ "uid" ], addr.uid(), mod );

  PhoneNumber number;
  number = addr.phoneNumber( PhoneNumber::Home );
  tmp += addEntry( mAttributes[ "phoneNumber" ], number.number().toUtf8(), mod );
  number = addr.phoneNumber( PhoneNumber::Work );
  tmp += addEntry( mAttributes[ "telephoneNumber" ], number.number().toUtf8(), mod );
  number = addr.phoneNumber( PhoneNumber::Fax );
  tmp += addEntry( mAttributes[ "facsimileTelephoneNumber" ], number.number().toUtf8(), mod );
  number = addr.phoneNumber( PhoneNumber::Cell );
  tmp += addEntry( mAttributes[ "mobile" ], number.number().toUtf8(), mod );
  number = addr.phoneNumber( PhoneNumber::Pager );
  tmp += addEntry( mAttributes[ "pager" ], number.number().toUtf8(), mod );

  tmp += addEntry( mAttributes[ "description" ], addr.note(), mod );
  tmp += addEntry( mAttributes[ "title" ], addr.title(), mod );
  tmp += addEntry( mAttributes[ "organization" ], addr.organization(), mod );

  Address ad = addr.address( Address::Home );
  if ( !ad.isEmpty() ) {
    tmp += addEntry( mAttributes[ "street" ], ad.street(), mod );
    tmp += addEntry( mAttributes[ "state" ], ad.region(), mod );
    tmp += addEntry( mAttributes[ "city" ], ad.locality(), mod );
    tmp += addEntry( mAttributes[ "postalcode" ], ad.postalCode(), mod );
  }

  QStringList emails = addr.emails();
  QStringList::ConstIterator mailIt = emails.begin();

  if ( !mAttributes[ "mail" ].isEmpty() ) {
    if ( mod ) tmp +=
      KLDAP::Ldif::assembleLine( "replace", mAttributes[ "mail" ] ) + '\n';
    if ( mailIt != emails.end() ) {
      tmp += KLDAP::Ldif::assembleLine( mAttributes[ "mail" ], *mailIt ) + '\n';
      mailIt ++;
    }
    if ( mod && mAttributes[ "mail" ] != mAttributes[ "mailAlias" ] ) tmp += "-\n";
  }

  if ( !mAttributes[ "mailAlias" ].isEmpty() ) {
    if ( mod && mAttributes[ "mail" ] != mAttributes[ "mailAlias" ] ) tmp +=
      KLDAP::Ldif::assembleLine( "replace", mAttributes[ "mailAlias" ] ) + '\n';
    for ( ; mailIt != emails.end(); ++mailIt ) {
      tmp += KLDAP::Ldif::assembleLine( mAttributes[ "mailAlias" ], *mailIt ) + '\n' ;
    }
    if ( mod ) tmp += "-\n";
  }

  if ( !mAttributes[ "jpegPhoto" ].isEmpty() ) {
    QByteArray pic;
    QBuffer buffer( &pic );
    buffer.open( QIODevice::WriteOnly );
    addr.photo().data().save( &buffer, "JPEG" );

    if ( mod ) tmp +=
      KLDAP::Ldif::assembleLine( "replace", mAttributes[ "jpegPhoto" ] ) + '\n';
    tmp += KLDAP::Ldif::assembleLine( mAttributes[ "jpegPhoto" ], pic, 76 ) + '\n';
    if ( mod ) tmp += "-\n";
  }

  tmp += '\n';
  kDebug(7125) << "ldif: " << QString::fromUtf8(tmp) << endl;
  ldif = tmp;
  return true;
}

void ResourceLDAPKIO::setReadOnly( bool value )
{
  //save the original readonly flag, because offline using disables writing
  d->mReadOnly = true;
  Resource::setReadOnly( value );
}

void ResourceLDAPKIO::init()
{
  if ( mPort == 0 ) mPort = 389;

  /**
    If you want to add new attributes, append them here, add a
    translation string in the ctor of AttributesDialog and
    handle them in the load() method below.
    These are the default values
   */
  if ( !mAttributes.contains("objectClass") )
    mAttributes.insert( "objectClass", "inetOrgPerson" );
  if ( !mAttributes.contains("commonName") )
    mAttributes.insert( "commonName", "cn" );
  if ( !mAttributes.contains("formattedName") )
    mAttributes.insert( "formattedName", "displayName" );
  if ( !mAttributes.contains("familyName") )
    mAttributes.insert( "familyName", "sn" );
  if ( !mAttributes.contains("givenName") )
    mAttributes.insert( "givenName", "givenName" );
  if ( !mAttributes.contains("mail") )
    mAttributes.insert( "mail", "mail" );
  if ( !mAttributes.contains("mailAlias") )
    mAttributes.insert( "mailAlias", "" );
  if ( !mAttributes.contains("phoneNumber") )
    mAttributes.insert( "phoneNumber", "homePhone" );
  if ( !mAttributes.contains("telephoneNumber") )
    mAttributes.insert( "telephoneNumber", "telephoneNumber" );
  if ( !mAttributes.contains("facsimileTelephoneNumber") )
    mAttributes.insert( "facsimileTelephoneNumber", "facsimileTelephoneNumber" );
  if ( !mAttributes.contains("mobile") )
    mAttributes.insert( "mobile", "mobile" );
  if ( !mAttributes.contains("pager") )
    mAttributes.insert( "pager", "pager" );
  if ( !mAttributes.contains("description") )
    mAttributes.insert( "description", "description" );

  if ( !mAttributes.contains("title") )
    mAttributes.insert( "title", "title" );
  if ( !mAttributes.contains("street") )
    mAttributes.insert( "street", "street" );
  if ( !mAttributes.contains("state") )
    mAttributes.insert( "state", "st" );
  if ( !mAttributes.contains("city") )
    mAttributes.insert( "city", "l" );
  if ( !mAttributes.contains("organization") )
    mAttributes.insert( "organization", "o" );
  if ( !mAttributes.contains("postalcode") )
    mAttributes.insert( "postalcode", "postalCode" );

  if ( !mAttributes.contains("uid") )
    mAttributes.insert( "uid", "uid" );
  if ( !mAttributes.contains("jpegPhoto") )
    mAttributes.insert( "jpegPhoto", "jpegPhoto" );

  d->mLDAPUrl = KLDAP::LdapUrl( KUrl() );
  if ( !mAnonymous ) {
    d->mLDAPUrl.setUser( mUser );
    d->mLDAPUrl.setPass( mPassword );
  }
  d->mLDAPUrl.setProtocol( d->mSSL ? "ldaps" : "ldap");
  d->mLDAPUrl.setHost( mHost );
  d->mLDAPUrl.setPort( mPort );
  d->mLDAPUrl.setDn( mDn );

  if (!mAttributes.empty()) {
    QMap<QString,QString>::Iterator it;
    QStringList attr;
    for ( it = mAttributes.begin(); it != mAttributes.end(); ++it ) {
      if ( !it.value().isEmpty() && it.key() != "objectClass" )
        attr.append( it.value() );
    }
    d->mLDAPUrl.setAttributes( attr );
  }

  d->mLDAPUrl.setScope( d->mSubTree ? KLDAP::LdapUrl::Sub : KLDAP::LdapUrl::One );
  if ( !mFilter.isEmpty() && mFilter != "(objectClass=*)" )
    d->mLDAPUrl.setFilter( mFilter );
  d->mLDAPUrl.setExtension( "x-dir", "base" );
  if ( d->mTLS ) d->mLDAPUrl.setExtension( "x-tls", "" );
  d->mLDAPUrl.setExtension( "x-ver", QString::number( d->mVer ) );
  if ( d->mSizeLimit )
    d->mLDAPUrl.setExtension( "x-sizelimit", QString::number( d->mSizeLimit ) );
  if ( d->mTimeLimit )
    d->mLDAPUrl.setExtension( "x-timelimit", QString::number( d->mTimeLimit ) );
  if ( d->mSASL ) {
    d->mLDAPUrl.setExtension( "x-sasl", "" );
    if ( !d->mBindDN.isEmpty() ) d->mLDAPUrl.setExtension( "bindname", d->mBindDN );
    if ( !d->mMech.isEmpty() ) d->mLDAPUrl.setExtension( "x-mech", d->mMech );
    if ( !d->mRealm.isEmpty() ) d->mLDAPUrl.setExtension( "x-realm", d->mRealm );
  }

  d->mReadOnly = readOnly();

  kDebug(7125) << "resource_ldapkio url: " << d->mLDAPUrl.prettyUrl() << endl;
}

void ResourceLDAPKIO::writeConfig( KConfigGroup &group )
{
  Resource::writeConfig( group );

  group.writeEntry( "LdapUser", mUser );
  group.writeEntry( "LdapPassword", KStringHandler::obscure( mPassword ) );
  group.writeEntry( "LdapDn", mDn );
  group.writeEntry( "LdapHost", mHost );
  group.writeEntry( "LdapPort", mPort );
  group.writeEntry( "LdapFilter", mFilter );
  group.writeEntry( "LdapAnonymous", mAnonymous );
  group.writeEntry( "LdapTLS", d->mTLS );
  group.writeEntry( "LdapSSL", d->mSSL );
  group.writeEntry( "LdapSubTree", d->mSubTree );
  group.writeEntry( "LdapSASL", d->mSASL );
  group.writeEntry( "LdapMech", d->mMech );
  group.writeEntry( "LdapVer", d->mVer );
  group.writeEntry( "LdapTimeLimit", d->mTimeLimit );
  group.writeEntry( "LdapSizeLimit", d->mSizeLimit );
  group.writeEntry( "LdapRDNPrefix", d->mRDNPrefix );
  group.writeEntry( "LdapRealm", d->mRealm );
  group.writeEntry( "LdapBindDN", d->mBindDN );
  group.writeEntry( "LdapCachePolicy", d->mCachePolicy );
  group.writeEntry( "LdapAutoCache", d->mAutoCache );

  QStringList attributes;
  QMap<QString, QString>::const_iterator it;
  for ( it = mAttributes.constBegin(); it != mAttributes.constEnd(); ++it )
    attributes << it.key() << it.value();

  group.writeEntry( "LdapAttributes", attributes );
}

Ticket *ResourceLDAPKIO::requestSaveTicket()
{
  if ( !addressBook() ) {
    kDebug(7125) << "no addressbook" << endl;
    return 0;
  }

  return createTicket( this );
}

void ResourceLDAPKIO::releaseSaveTicket( Ticket *ticket )
{
  delete ticket;
}

bool ResourceLDAPKIO::doOpen()
{
  return true;
}

void ResourceLDAPKIO::doClose()
{
}

void ResourceLDAPKIO::createCache()
{
  d->mTmp = NULL;
  if ( d->mCachePolicy == Cache_NoConnection && d->mAutoCache ) {
    d->mTmp = new KTemporaryFile;
    d->mTmp->setPrefix(d->mCacheDst);
    d->mTmp->setSuffix("tmp");
    d->mTmp->open();
  }
}

void ResourceLDAPKIO::activateCache()
{
  if ( d->mTmp && d->mError == 0 ) {
    QString filename = d->mTmp->fileName();
    delete d->mTmp;
    d->mTmp = 0;
    rename( QFile::encodeName( filename ), QFile::encodeName( d->mCacheDst ) );
  }
}

KIO::Job *ResourceLDAPKIO::loadFromCache()
{
  KIO::Job *job = NULL;
  if ( d->mCachePolicy == Cache_Always ||
     ( d->mCachePolicy == Cache_NoConnection &&
      d->mError == KIO::ERR_COULD_NOT_CONNECT ) ) {

    d->mAddr = Addressee();
    d->mAd = Address( Address::Home );
    //initialize ldif parser
    d->mLdif.startParsing();

    Resource::setReadOnly( true );

    KUrl url( d->mCacheDst );
    job = KIO::get( url, true, false );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
      this, SLOT( data( KIO::Job*, const QByteArray& ) ) );
  }
  return job;
}

bool ResourceLDAPKIO::load()
{
  kDebug(7125) << "ResourceLDAPKIO::load()" << endl;
  KIO::Job *job;

  clear();
  //clear the addressee
  d->mAddr = Addressee();
  d->mAd = Address( Address::Home );
  //initialize ldif parser
  d->mLdif.startParsing();

  //set to original settings, offline use will disable writing
  Resource::setReadOnly( d->mReadOnly );

  createCache();
  if ( d->mCachePolicy != Cache_Always ) {
    job = KIO::get( d->mLDAPUrl, true, false );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
      this, SLOT( data( KIO::Job*, const QByteArray& ) ) );
    connect( job, SIGNAL( result( KJob* ) ),
      this, SLOT( syncLoadSaveResult( KJob* ) ) );
    enter_loop();
  }

  job = loadFromCache();
  if ( job ) {
    connect( job, SIGNAL( result( KJob* ) ),
      this, SLOT( syncLoadSaveResult( KJob* ) ) );
    enter_loop();
  }
  if ( mErrorMsg.isEmpty() ) {
    kDebug(7125) << "ResourceLDAPKIO load ok!" << endl;
    return true;
  } else {
    kDebug(7125) << "ResourceLDAPKIO load finished with error: " << mErrorMsg << endl;
    addressBook()->error( mErrorMsg );
    return false;
  }
}

bool ResourceLDAPKIO::asyncLoad()
{
  clear();
  //clear the addressee
  d->mAddr = Addressee();
  d->mAd = Address( Address::Home );
  //initialize ldif parser
  d->mLdif.startParsing();

  Resource::setReadOnly( d->mReadOnly );

  createCache();
  if ( d->mCachePolicy != Cache_Always ) {
    KIO::Job *job = KIO::get( d->mLDAPUrl, true, false );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
      this, SLOT( data( KIO::Job*, const QByteArray& ) ) );
    connect( job, SIGNAL( result( KJob* ) ),
      this, SLOT( result( KJob* ) ) );
  } else {
    result( NULL );
  }
  return true;
}

void ResourceLDAPKIO::data( KIO::Job *, const QByteArray &data )
{
  if ( data.size() ) {
    d->mLdif.setLdif( data );
    if ( d->mTmp ) {
      d->mTmp->write( data );
    }
  } else {
    d->mLdif.endLdif();
  }

  KLDAP::Ldif::ParseValue ret;
  QString name;
  QByteArray value;
  do {
    ret = d->mLdif.nextItem();
    switch ( ret ) {
      case KLDAP::Ldif::NewEntry:
        kDebug(7125) << "new entry: " << d->mLdif.dn() << endl;
        break;
      case KLDAP::Ldif::Item:
        name = d->mLdif.attr().toLower();
        value = d->mLdif.value();
        if ( name == mAttributes[ "commonName" ].toLower() ) {
          if ( !d->mAddr.formattedName().isEmpty() ) {
            QString fn = d->mAddr.formattedName();
            d->mAddr.setNameFromString( QString::fromUtf8( value, value.size() ) );
            d->mAddr.setFormattedName( fn );
          } else
            d->mAddr.setNameFromString( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "formattedName" ].toLower() ) {
          d->mAddr.setFormattedName( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "givenName" ].toLower() ) {
          d->mAddr.setGivenName( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "mail" ].toLower() ) {
          d->mAddr.insertEmail( QString::fromUtf8( value, value.size() ), true );
        } else if ( name == mAttributes[ "mailAlias" ].toLower() ) {
          d->mAddr.insertEmail( QString::fromUtf8( value, value.size() ), false );
        } else if ( name == mAttributes[ "phoneNumber" ].toLower() ) {
          PhoneNumber phone;
          phone.setNumber( QString::fromUtf8( value, value.size() ) );
          d->mAddr.insertPhoneNumber( phone );
        } else if ( name == mAttributes[ "telephoneNumber" ].toLower() ) {
          PhoneNumber phone( QString::fromUtf8( value, value.size() ),
            PhoneNumber::Work );
          d->mAddr.insertPhoneNumber( phone );
        } else if ( name == mAttributes[ "facsimileTelephoneNumber" ].toLower() ) {
          PhoneNumber phone( QString::fromUtf8( value, value.size() ),
            PhoneNumber::Fax );
          d->mAddr.insertPhoneNumber( phone );
        } else if ( name == mAttributes[ "mobile" ].toLower() ) {
          PhoneNumber phone( QString::fromUtf8( value, value.size() ),
            PhoneNumber::Cell );
          d->mAddr.insertPhoneNumber( phone );
        } else if ( name == mAttributes[ "pager" ].toLower() ) {
          PhoneNumber phone( QString::fromUtf8( value, value.size() ),
            PhoneNumber::Pager );
          d->mAddr.insertPhoneNumber( phone );
        } else if ( name == mAttributes[ "description" ].toLower() ) {
          d->mAddr.setNote( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "title" ].toLower() ) {
          d->mAddr.setTitle( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "street" ].toLower() ) {
          d->mAd.setStreet( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "state" ].toLower() ) {
          d->mAd.setRegion( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "city" ].toLower() ) {
          d->mAd.setLocality( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "postalcode" ].toLower() ) {
          d->mAd.setPostalCode( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "organization" ].toLower() ) {
          d->mAddr.setOrganization( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "familyName" ].toLower() ) {
          d->mAddr.setFamilyName( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "uid" ].toLower() ) {
          d->mAddr.setUid( QString::fromUtf8( value, value.size() ) );
        } else if ( name == mAttributes[ "jpegPhoto" ].toLower() ) {
          KABC::Picture photo;
          QImage img = QImage::fromData( value );
          if ( !img.isNull() ) {
            photo.setData( img );
            photo.setType( "image/jpeg" );
            d->mAddr.setPhoto( photo );
          }
        }

        break;
      case KLDAP::Ldif::EndEntry: {
        d->mAddr.setResource( this );
        d->mAddr.insertAddress( d->mAd );
        d->mAddr.setChanged( false );
        insertAddressee( d->mAddr );
        //clear the addressee
        d->mAddr = Addressee();
        d->mAd = Address( Address::Home );
        }
        break;
      default:
        break;
    }
  } while ( ret != KLDAP::Ldif::MoreData );
}

void ResourceLDAPKIO::loadCacheResult( KJob *job )
{
  mErrorMsg = "";
  d->mError = job->error();
  if ( d->mError && d->mError != KIO::ERR_USER_CANCELED ) {
    mErrorMsg = job->errorString();
  }
  if ( !mErrorMsg.isEmpty() )
    emit loadingError( this, mErrorMsg );
  else
    emit loadingFinished( this );
}

void ResourceLDAPKIO::result( KJob *job )
{
  mErrorMsg = "";
  if ( job ) {
    d->mError = job->error();
    if ( d->mError && d->mError != KIO::ERR_USER_CANCELED ) {
      mErrorMsg = job->errorString();
    }
  } else {
    d->mError = 0;
  }
  activateCache();

  KIO::Job *cjob;
  cjob = loadFromCache();
  if ( cjob ) {
    connect( cjob, SIGNAL( result( KJob* ) ),
      this, SLOT( loadCacheResult( KJob* ) ) );
  } else {
    if ( !mErrorMsg.isEmpty() )
      emit loadingError( this, mErrorMsg );
    else
      emit loadingFinished( this );
  }
}

bool ResourceLDAPKIO::save( Ticket* )
{
  kDebug(7125) << "ResourceLDAPKIO save" << endl;

  d->mSaveIt = begin();
  KIO::Job *job = KIO::put( d->mLDAPUrl, -1, true, false, false );
  connect( job, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ),
    this, SLOT( saveData( KIO::Job*, QByteArray& ) ) );
  connect( job, SIGNAL( result( KJob* ) ),
    this, SLOT( syncLoadSaveResult( KJob* ) ) );
  enter_loop();
  if ( mErrorMsg.isEmpty() ) {
    kDebug(7125) << "ResourceLDAPKIO save ok!" << endl;
    return true;
  } else {
    kDebug(7125) << "ResourceLDAPKIO finished with error: " << mErrorMsg << endl;
    addressBook()->error( mErrorMsg );
    return false;
  }
}

bool ResourceLDAPKIO::asyncSave( Ticket* )
{
  kDebug(7125) << "ResourceLDAPKIO asyncSave" << endl;
  d->mSaveIt = begin();
  KIO::Job *job = KIO::put( d->mLDAPUrl, -1, true, false, false );
  connect( job, SIGNAL( dataReq( KIO::Job*, QByteArray& ) ),
    this, SLOT( saveData( KIO::Job*, QByteArray& ) ) );
  connect( job, SIGNAL( result( KJob* ) ),
    this, SLOT( saveResult( KJob* ) ) );
  return true;
}

void ResourceLDAPKIO::syncLoadSaveResult( KJob *job )
{
  d->mError = job->error();
  if ( d->mError && d->mError != KIO::ERR_USER_CANCELED )
    mErrorMsg = job->errorString();
  else
    mErrorMsg = "";
  activateCache();

  emit leaveModality();
}

void ResourceLDAPKIO::saveResult( KJob *job )
{
  d->mError = job->error();
  if ( d->mError && d->mError != KIO::ERR_USER_CANCELED )
    emit savingError( this, job->errorString() );
  else
    emit savingFinished( this );
}

void ResourceLDAPKIO::saveData( KIO::Job*, QByteArray& data )
{
  while ( d->mSaveIt != end() &&
       !(*d->mSaveIt).changed() ) d->mSaveIt++;

  if ( d->mSaveIt == end() ) {
    kDebug(7125) << "ResourceLDAPKIO endData" << endl;
    data.resize(0);
    return;
  }

  kDebug(7125) << "ResourceLDAPKIO saveData: " << (*d->mSaveIt).assembledName() << endl;

  AddresseeToLDIF( data, *d->mSaveIt, findUid( (*d->mSaveIt).uid() ) );
//  kDebug(7125) << "ResourceLDAPKIO save LDIF: " << QString::fromUtf8(data) << endl;
  // mark as unchanged
  (*d->mSaveIt).setChanged( false );

  d->mSaveIt++;
}

void ResourceLDAPKIO::removeAddressee( const Addressee& addr )
{
  QString dn = findUid( addr.uid() );

  kDebug(7125) << "ResourceLDAPKIO: removeAddressee: " << dn << endl;

  if ( !mErrorMsg.isEmpty() ) {
    addressBook()->error( mErrorMsg );
    return;
  }
  if ( !dn.isEmpty() ) {
    kDebug(7125) << "ResourceLDAPKIO: found uid: " << dn << endl;
    KLDAP::LdapUrl url( d->mLDAPUrl );
    url.setPath( '/' + dn );
    url.setExtension( "x-dir", "base" );
    url.setScope( KLDAP::LdapUrl::Base );
    if ( KIO::NetAccess::del( url, NULL ) ) mAddrMap.remove( addr.uid() );
  } else {
    //maybe it's not saved yet
    mAddrMap.remove( addr.uid() );
  }
}


void ResourceLDAPKIO::setUser( const QString &user )
{
  mUser = user;
}

QString ResourceLDAPKIO::user() const
{
  return mUser;
}

void ResourceLDAPKIO::setPassword( const QString &password )
{
  mPassword = password;
}

QString ResourceLDAPKIO::password() const
{
  return mPassword;
}

void ResourceLDAPKIO::setDn( const QString &dn )
{
  mDn = dn;
}

QString ResourceLDAPKIO::dn() const
{
  return mDn;
}

void ResourceLDAPKIO::setHost( const QString &host )
{
  mHost = host;
}

QString ResourceLDAPKIO::host() const
{
  return mHost;
}

void ResourceLDAPKIO::setPort( int port )
{
  mPort = port;
}

int ResourceLDAPKIO::port() const
{
  return mPort;
}

void ResourceLDAPKIO::setVer( int ver )
{
  d->mVer = ver;
}

int ResourceLDAPKIO::ver() const
{
  return d->mVer;
}

void ResourceLDAPKIO::setSizeLimit( int sizelimit )
{
  d->mSizeLimit = sizelimit;
}

int ResourceLDAPKIO::sizeLimit()
{
  return d->mSizeLimit;
}

void ResourceLDAPKIO::setTimeLimit( int timelimit )
{
  d->mTimeLimit = timelimit;
}

int ResourceLDAPKIO::timeLimit()
{
  return d->mTimeLimit;
}

void ResourceLDAPKIO::setFilter( const QString &filter )
{
  mFilter = filter;
}

QString ResourceLDAPKIO::filter() const
{
  return mFilter;
}

void ResourceLDAPKIO::setIsAnonymous( bool value )
{
  mAnonymous = value;
}

bool ResourceLDAPKIO::isAnonymous() const
{
  return mAnonymous;
}

void ResourceLDAPKIO::setIsTLS( bool value )
{
  d->mTLS = value;
}

bool ResourceLDAPKIO::isTLS() const
{
  return d->mTLS;
}
void ResourceLDAPKIO::setIsSSL( bool value )
{
  d->mSSL = value;
}

bool ResourceLDAPKIO::isSSL() const
{
  return d->mSSL;
}

void ResourceLDAPKIO::setIsSubTree( bool value )
{
  d->mSubTree = value;
}

bool ResourceLDAPKIO::isSubTree() const
{
  return d->mSubTree;
}

void ResourceLDAPKIO::setAttributes( const QMap<QString, QString> &attributes )
{
  mAttributes = attributes;
}

QMap<QString, QString> ResourceLDAPKIO::attributes() const
{
  return mAttributes;
}

void ResourceLDAPKIO::setRDNPrefix( int value )
{
  d->mRDNPrefix = value;
}

int ResourceLDAPKIO::RDNPrefix() const
{
  return d->mRDNPrefix;
}

void ResourceLDAPKIO::setIsSASL( bool value )
{
  d->mSASL = value;
}

bool ResourceLDAPKIO::isSASL() const
{
  return d->mSASL;
}

void ResourceLDAPKIO::setMech( const QString &mech )
{
  d->mMech = mech;
}

QString ResourceLDAPKIO::mech() const
{
  return d->mMech;
}

void ResourceLDAPKIO::setRealm( const QString &realm )
{
  d->mRealm = realm;
}

QString ResourceLDAPKIO::realm() const
{
  return d->mRealm;
}

void ResourceLDAPKIO::setBindDN( const QString &binddn )
{
  d->mBindDN = binddn;
}

QString ResourceLDAPKIO::bindDN() const
{
  return d->mBindDN;
}

void ResourceLDAPKIO::setCachePolicy( int pol )
{
  d->mCachePolicy = pol;
}

int ResourceLDAPKIO::cachePolicy() const
{
  return d->mCachePolicy;
}

void ResourceLDAPKIO::setAutoCache( bool value )
{
  d->mAutoCache = value;
}

bool ResourceLDAPKIO::autoCache()
{
  return d->mAutoCache;
}

QString ResourceLDAPKIO::cacheDst() const
{
  return d->mCacheDst;
}


#include "resourceldapkio.moc"
