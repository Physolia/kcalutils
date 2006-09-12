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
#ifndef KCAL_TODO_H
#define KCAL_TODO_H

#include <QByteArray>

#include "incidence.h"

namespace KCal {

/**
  This class provides a Todo in the sense of RFC2445.
*/
class KCAL_EXPORT Todo : public Incidence
{
  public:
    typedef ListBase<Todo> List;

    Todo();
    Todo( const Todo & );
    ~Todo();
    bool operator==( const Todo& ) const;

    QByteArray type() const { return "Todo"; }

    /**
      Returns an exact copy of this todo. The returned object is owned by the
      caller.
    */
    Todo *clone();

    /**
      Sets due date and time.

      @param dtDue The due date/time.
      @param first Set the date of the first occurrence (if the todo is recurrent).
    */
    void setDtDue(const KDateTime &dtDue, bool first = false);
    KDE_DEPRECATED void setDtDue(const QDateTime &dtDue, bool first = false);
    /**
      Returns due date and time.

      @param first If true and the todo recurs, the due date of the first
      occurrence will be returned. If false and recurrent, the date of the
      current occurrence will be returned. If non-recurrent, the normal due date
      will be returned.
    */
    KDateTime dtDue( bool first = false ) const;
    /**
      Returns due time as string formatted according to the user's locale
      settings.
    */
    QString dtDueTimeStr() const;
    /**
      Returns due date as string formatted according to the user's locale
      settings.

      @param shortfmt If set to true, use short date format, if set to false use
                      long format.
    */
    QString dtDueDateStr( bool shortfmt = true ) const;
    /**
      Returns due date and time as string formatted according to the user's locale
      settings.
    */
    QString dtDueStr() const;

    /**
      Returns true if the todo has a due date, otherwise return false.
    */
    bool hasDueDate() const;
    /**
      Set if the todo has a due date.

      @param hasDueDate true if todo has a due date, otherwise false
    */
    void setHasDueDate( bool hasDueDate );

    /**
      Returns true if the todo has a start date, otherwise return false.
    */
    bool hasStartDate() const;
    /**
      Set if the todo has a start date.

      @param hasStartDate true if todo has a start date, otherwise false
    */
    void setHasStartDate( bool hasStartDate );

    /**
      Returns the start date of the todo.
      @param first If true, the start date of the todo will be returned. If the
      todo recurs, the start date of the first occurrence will be returned.
      If false and the todo recurs, the relative start date will be returned,
      based on the date returned by dtRecurrence().
    */
    KDateTime dtStart( bool first = false ) const;

    /**
      Sets the start date of the todo.
    */
    void setDtStart( const KDateTime &dtStart );
    KDE_DEPRECATED void setDtStart( const QDateTime &dtStart )  { setDtStart(KDateTime(dtStart)); }  // use local time zone

    /** Returns a todo's starting time as a string formatted according to the
     user's locale settings.
     @param first If true, the start date of the todo will be returned. If the
     todo recurs, the start date of the first occurrence will be returned.
     If false and the todo recurs, the relative start date will be returned,
     based on the date returned by dtRecurrence().
    */
    QString dtStartTimeStr( bool first = false ) const;
    /** Returns a todo's starting date as a string formatted according to the
     user's locale settings.
     @param shortfmt If true, use short date format, if set to false use
     long format.
     @param first If true, the start date of the todo will be returned. If the
     todo recurs, the start date of the first occurrence will be returned.
     If false and the todo recurs, the relative start date will be returned,
     based on the date returned by dtRecurrence().
    */
    QString dtStartDateStr( bool shortfmt = true, bool first = false ) const;
    /** Returns a todo's starting date and time as a string formatted according
     to the user's locale settings.
     @param first If true, the start date of the todo will be returned. If the
     todo recurs, the start date of the first occurrence will be returned.
     If false and the todo recurs, the relative start date will be returned,
     based on the date returned by dtRecurrence().
    */
    QString dtStartStr( bool first = false ) const;

    /**
      Returns true if the todo is 100% completed, otherwise return false.
    */
    bool isCompleted() const;
    /**
      Set completed state.

      @param completed If true set completed state to 100%, if false set
                       completed state to 0%.
    */
    void setCompleted( bool completed );

    /**
      Returns what percentage of the task is completed. Returns a value
      between 0 and 100.
    */
    int percentComplete() const;
    /**
      Set what percentage of the task is completed. Valid values are in the
      range from 0 to 100.
    */
    void setPercentComplete( int );

    /**
      Returns date and time when todo was completed.
    */
    KDateTime completed() const;
    /**
      Returns string contaiting date and time when the todo was completed
      formatted according to the user's locale settings.
    */
    QString completedStr() const;
    /**
      Set date and time of completion.
    */
    void setCompleted( const KDateTime &completed );
    KDE_DEPRECATED void setCompleted( const QDateTime &completed );

    /**
      Returns true, if todo has a date associated with completion, otherwise
      return false.
    */
    bool hasCompletedDate() const;

    /**
      @copydoc
      IncidenceBase::shiftTimes()
    */
    virtual void shiftTimes(const KDateTime::Spec &oldSpec, const KDateTime::Spec &newSpec);

    /**
      Sets the due date/time of the current occurrence if recurrent.
    */
    void setDtRecurrence( const KDateTime &dt );

    /**
      Returns the due date/time of the current occurrence if recurrent.
    */
    KDateTime dtRecurrence() const;

    /**
      Returns true if the date specified is one on which the todo will
      recur. Todos are a special case, hence the overload. It adds an extra
      check, which make it return false if there's an occurrence between
      the recur start and today.
    */
    virtual bool recursOn( const QDate &date, const KDateTime::Spec &timeSpec = KDateTime::LocalZone ) const;

    /**
      Returns true if this todo is overdue (e.g. due date is lower than today
      and not completed), else false.
     */
      bool isOverdue() const;

  protected:
    /** Return the end date/time of the base incidence. */
    virtual KDateTime endDateRecurrenceBase() const { return dtDue(); }

  private:
    bool accept(Visitor &v) { return v.visit( this ); }
    /** Returns true if the todo got a new date, else false will be returned. */
    bool recurTodo();

    KDateTime mDtDue;                    // due date of todo
                                         // (first occurrence if recurrent)
    KDateTime mDtRecurrence;             // due date of recurrence

    bool mHasDueDate;                    // if todo has associated due date
    bool mHasStartDate;                  // if todo has associated start date

    KDateTime mCompleted;
    bool mHasCompletedDate;

    int mPercentComplete;

    class Private;
    Private *d;
};

}

#endif
