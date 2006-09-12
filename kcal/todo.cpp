/*
    This file is part of the kcal library.

    Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>

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

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

#include "todo.h"

using namespace KCal;

Todo::Todo()
{
  mHasDueDate = false;
  mHasStartDate = false;

  mHasCompletedDate = false;
  mPercentComplete = 0;
}

Todo::Todo(const Todo &t) : Incidence(t)
{
  mDtDue = t.mDtDue;
  mHasDueDate = t.mHasDueDate;
  mHasStartDate = t.mHasStartDate;
  mCompleted = t.mCompleted;
  mHasCompletedDate = t.mHasCompletedDate;
  mPercentComplete = t.mPercentComplete;
  mDtRecurrence = t.mDtRecurrence;
}

Todo::~Todo()
{
}

Todo *Todo::clone()
{
  return new Todo( *this );
}


bool Todo::operator==( const Todo& t2 ) const
{
    return
        static_cast<const Incidence&>(*this) == static_cast<const Incidence&>(t2) &&
        dtDue() == t2.dtDue() &&
        hasDueDate() == t2.hasDueDate() &&
        hasStartDate() == t2.hasStartDate() &&
        completed() == t2.completed() &&
        hasCompletedDate() == t2.hasCompletedDate() &&
        percentComplete() == t2.percentComplete();
}

void Todo::setDtDue(const KDateTime &dtDue, bool first )
{
  //int diffsecs = mDtDue.secsTo(dtDue);

  /*if (mReadOnly) return;
  const Alarm::List& alarms = alarms();
  for (Alarm* alarm = alarms.first(); alarm; alarm = alarms.next()) {
    if (alarm->enabled()) {
      alarm->setTime(alarm->time().addSecs(diffsecs));
    }
  }*/
  if( doesRecur() && !first ) {
    mDtRecurrence = dtDue;
  } else {
    mDtDue = dtDue;
    // TODO: This doesn't seem right...
    recurrence()->setStartDateTime( dtDue );
    recurrence()->setFloats( doesFloat() );
  }

  if ( doesRecur() && dtDue < recurrence()->startDateTime() )
    setDtStart( dtDue );

  //kDebug(5800) << "setDtDue says date is " << mDtDue.toString() << endl;

  /*const Alarm::List& alarms = alarms();
  for (Alarm* alarm = alarms.first(); alarm; alarm = alarms.next())
    alarm->setAlarmStart(mDtDue);*/

  updated();
}

KDateTime Todo::dtDue( bool first ) const
{
  if ( doesRecur() && !first && mDtRecurrence.isValid() )
    return mDtRecurrence;

  return mDtDue;
}

QString Todo::dtDueTimeStr() const
{
  return KGlobal::locale()->formatTime( dtDue(!doesRecur()).time() );
}

QString Todo::dtDueDateStr(bool shortfmt) const
{
  return KGlobal::locale()->formatDate(dtDue( !doesRecur() ).date(),shortfmt);
}

QString Todo::dtDueStr() const
{
  return KGlobal::locale()->formatDateTime( dtDue( !doesRecur() ).dateTime() );
}

bool Todo::hasDueDate() const
{
  return mHasDueDate;
}

void Todo::setHasDueDate(bool f)
{
  if (mReadOnly) return;
  mHasDueDate = f;
  updated();
}


bool Todo::hasStartDate() const
{
  return mHasStartDate;
}

void Todo::setHasStartDate(bool f)
{
  if (mReadOnly) return;

  if ( doesRecur() && !f ) {
    if ( !comments().filter("NoStartDate").count() )
      addComment("NoStartDate"); //TODO: --> custom flag?
  } else {
    QString s("NoStartDate");
    removeComment(s);
  }
  mHasStartDate = f;
  updated();
}

KDateTime Todo::dtStart( bool first ) const
{
  if ( doesRecur() && !first )
    return mDtRecurrence.addDays( dtDue( true ).daysTo( IncidenceBase::dtStart() ) );
  else
    return IncidenceBase::dtStart();
}

void Todo::setDtStart( const KDateTime &dtStart )
{
  // TODO: This doesn't seem right (rfc 2445/6 says, recurrence is calculated from the dtstart...)
  if ( doesRecur() ) {
    recurrence()->setStartDateTime( mDtDue );
    recurrence()->setFloats( doesFloat() );
  }
  IncidenceBase::setDtStart( dtStart );
}

QString Todo::dtStartTimeStr( bool first ) const
{
  return KGlobal::locale()->formatTime(dtStart(first).time());
}

QString Todo::dtStartDateStr(bool shortfmt, bool first) const
{
  return KGlobal::locale()->formatDate(dtStart(first).date(),shortfmt);
}

QString Todo::dtStartStr(bool first) const
{
  return KGlobal::locale()->formatDateTime(dtStart(first).dateTime());
}

bool Todo::isCompleted() const
{
  if (mPercentComplete == 100) return true;
  else return false;
}

void Todo::setCompleted(bool completed)
{
  if (completed)
    mPercentComplete = 100;
  else {
    mPercentComplete = 0;
    mHasCompletedDate = false;
    mCompleted = KDateTime();
  }
  updated();
}

KDateTime Todo::completed() const
{
  if ( hasCompletedDate() )
    return mCompleted;
  else
    return KDateTime();
}

QString Todo::completedStr() const
{
  return KGlobal::locale()->formatDateTime(mCompleted.dateTime());
}

void Todo::setCompleted(const KDateTime &completed)
{
  if( !recurTodo() ) {
    mHasCompletedDate = true;
    mPercentComplete = 100;
    mCompleted = completed.toUtc();
  }
  updated();
}

bool Todo::hasCompletedDate() const
{
  return mHasCompletedDate;
}

int Todo::percentComplete() const
{
  return mPercentComplete;
}

void Todo::setPercentComplete(int v)
{
  mPercentComplete = v;
  if ( v != 100 ) mHasCompletedDate = false;
  updated();
}

void Todo::shiftTimes(const KDateTime::Spec &oldSpec, const KDateTime::Spec &newSpec)
{
  Incidence::shiftTimes( oldSpec, newSpec );
  mDtDue = mDtDue.toTimeSpec( oldSpec );
  mDtDue.setTimeSpec( newSpec );
  if ( doesRecur() ) {
    mDtRecurrence = mDtRecurrence.toTimeSpec( oldSpec );
    mDtRecurrence.setTimeSpec( newSpec );
  }
  if ( mHasCompletedDate ) {
    mCompleted = mCompleted.toTimeSpec( oldSpec );
    mCompleted.setTimeSpec( newSpec );
  }
}

void Todo::setDtRecurrence( const KDateTime &dt )
{
  mDtRecurrence = dt;
}

KDateTime Todo::dtRecurrence() const
{
  return mDtRecurrence.isValid() ? mDtRecurrence : mDtDue;
}

bool Todo::recursOn( const QDate &date, const KDateTime::Spec &timeSpec ) const
{
  QDate today = QDate::currentDate();
  return ( Incidence::recursOn(date, timeSpec) &&
           !( date < today && mDtRecurrence.date() < today &&
              mDtRecurrence > recurrence()->startDateTime() ) );
}

bool Todo::recurTodo()
{
  if ( doesRecur() ) {
    Recurrence *r = recurrence();
    KDateTime endDateTime = r->endDateTime();
    KDateTime nextDate = r->getNextDateTime( dtDue() );

    if ( ( r->duration() == -1 || ( nextDate.isValid() && endDateTime.isValid()
           && nextDate <= endDateTime ) ) ) {
      setDtDue( nextDate );
      while ( !recursAt( dtDue() ) || dtDue() <= KDateTime::currentUtcDateTime() ) {
        setDtDue( r->getNextDateTime( dtDue() ) );
      }

      setCompleted( false );
      setRevision( revision() + 1 );

      return true;
    }
  }

  return false;
}

bool Todo::isOverdue() const
{
  bool inPast = doesFloat() ? mDtDue.date() < QDate::currentDate()
                            : mDtDue < KDateTime::currentUtcDateTime();
  return ( inPast && !isCompleted() );
}

// DEPRECATED methods
void Todo::setDtDue(const QDateTime &dtDue, bool first)
{
  if (dtStart().isValid())
    setDtDue(KDateTime(dtDue, dtStart().timeSpec()), first);
  else
    setDtDue(KDateTime(dtDue), first);  // use local time zone
}
void Todo::setCompleted( const QDateTime &completed )
{
  if (dtStart().isValid())
    setCompleted(KDateTime(completed, dtStart().timeSpec()));
  else
    setCompleted(KDateTime(completed));  // use local time zone
}
