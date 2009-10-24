/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "contactgroupeditordelegate_p.h"

#include "contactcompletionmodel_p.h"
#include "contactgroupmodel_p.h"

#include <akonadi/entitytreemodel.h>
#include <kcombobox.h>
#include <kicon.h>
#include <klocale.h>

#include <QtGui/QAbstractItemView>
#include <QtGui/QCompleter>
#include <QtGui/QMouseEvent>
#include <QtGui/QToolButton>

using namespace Akonadi;

ContactLineEdit::ContactLineEdit( QWidget *parent )
  : KLineEdit( parent )
{
  setFrame( false );

  QCompleter *completer = new QCompleter( Akonadi::ContactCompletionModel::self(), this );
  completer->setCompletionColumn( Akonadi::ContactCompletionModel::NameColumn );
  connect( completer, SIGNAL( activated( const QModelIndex& ) ), SLOT( completed( const QModelIndex& ) ) );

  setCompleter( completer );
}

Akonadi::Item ContactLineEdit::completedItem() const
{
  return mItem;
}

void ContactLineEdit::completed( const QModelIndex &index )
{
  if ( index.isValid() )
    mItem = index.data( Akonadi::EntityTreeModel::ItemRole ).value<Akonadi::Item>();
  else
    mItem = Item();
}

class ContactGroupEditorDelegate::Private
{
  public:
    Private()
      : mButtonSize( 16, 16 ), mIcon( QLatin1String( "list-remove" ) )
    {
    }

    QSize mButtonSize;
    const KIcon mIcon;
    QAbstractItemView *mItemView;
};

ContactGroupEditorDelegate::ContactGroupEditorDelegate( QAbstractItemView *view, QObject *parent )
  : QStyledItemDelegate( parent ), d( new Private )
{
  d->mItemView = view;
}

ContactGroupEditorDelegate::~ContactGroupEditorDelegate()
{
  delete d;
}

QWidget* ContactGroupEditorDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem&,
                                                   const QModelIndex &index ) const
{
  if ( index.column() == 0 ) {
    return new ContactLineEdit( parent );
  } else {
    if ( index.data( ContactGroupModel::IsReferenceRole ).toBool() ) {
      KComboBox *comboBox = new KComboBox( parent );
      comboBox->setFrame( false );
      return comboBox;
    } else {
      KLineEdit *lineEdit = new KLineEdit( parent );
      lineEdit->setFrame( false );
      return lineEdit;
    }
  }
}

void ContactGroupEditorDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
  if ( index.data( ContactGroupModel::IsReferenceRole ).toBool() ) {
    if ( index.column() == 0 ) {
      KLineEdit *lineEdit = qobject_cast<KLineEdit*>( editor );
      if ( !lineEdit )
        return;

      lineEdit->setText( index.data( Qt::EditRole ).toString() );
    } else {
      KComboBox *comboBox = qobject_cast<KComboBox*>( editor );
      if ( !comboBox )
        return;

      const QStringList emails = index.data( ContactGroupModel::AllEmailsRole ).toStringList();
      comboBox->clear();
      comboBox->addItems( emails );
      comboBox->setCurrentIndex( comboBox->findText( index.data( Qt::EditRole ).toString() ) );
    }
  } else {
    if ( index.column() == 0 ) {

      KLineEdit *lineEdit = qobject_cast<KLineEdit*>( editor );
      if ( !lineEdit )
        return;

      lineEdit->setText( index.data( Qt::EditRole ).toString() );

    } else {
      KLineEdit *lineEdit = qobject_cast<KLineEdit*>( editor );
      if ( !lineEdit )
        return;

      lineEdit->setText( index.data( Qt::EditRole ).toString() );
    }
  }
}

void ContactGroupEditorDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
  if ( index.data( ContactGroupModel::IsReferenceRole ).toBool() ) {
    if ( index.column() == 0 ) {
      ContactLineEdit *lineEdit = static_cast<ContactLineEdit*>( editor );

      const Item item = lineEdit->completedItem();
      model->setData( index, item.isValid(), ContactGroupModel::IsReferenceRole );
      if ( item.isValid() )
        model->setData( index, item.id(), Qt::EditRole );
      else
        model->setData( index, lineEdit->text(), Qt::EditRole );
    }

    if ( index.column() == 1 ) {
      KComboBox *comboBox = qobject_cast<KComboBox*>( editor );
      if ( !comboBox )
        return;

      model->setData( index, comboBox->currentText(), Qt::EditRole );
    }
  } else {
    if ( index.column() == 0 ) {
      ContactLineEdit *lineEdit = static_cast<ContactLineEdit*>( editor );

      const Item item = lineEdit->completedItem();
      model->setData( index, item.isValid(), ContactGroupModel::IsReferenceRole );
      if ( item.isValid() )
        model->setData( index, item.id(), Qt::EditRole );
      else
        model->setData( index, lineEdit->text(), Qt::EditRole );
    }

    if ( index.column() == 1 ) {
      KLineEdit *lineEdit = qobject_cast<KLineEdit*>( editor );
      if ( !lineEdit )
        return;

      model->setData( index, lineEdit->text(), Qt::EditRole );
    }
  }
}

static bool isLastRow( const QModelIndex &index )
{
  return (index.row() == (index.model()->rowCount() - 1));
}

void ContactGroupEditorDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  QStyledItemDelegate::paint( painter, option, index );

  if ( index.column() == 1 && !isLastRow( index ) )
    d->mIcon.paint( painter, option.rect, Qt::AlignRight );
}

QSize ContactGroupEditorDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  Q_UNUSED( option );

  QSize hint = QStyledItemDelegate::sizeHint( option, index );
  hint.setHeight( qMax( hint.height(), d->mButtonSize.height() ) );

  if ( index.column() == 1 )
    hint.setWidth( hint.width() + d->mButtonSize.width() );

  return hint;
}

bool ContactGroupEditorDelegate::editorEvent( QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index )
{
  if ( index.column() == 1 && !isLastRow( index ) ) {
    if ( event->type() == QEvent::MouseButtonRelease ) {
      const QMouseEvent *mouseEvent = static_cast<QMouseEvent*>( event );
      QRect buttonRect = d->mItemView->visualRect( index );
      buttonRect.setLeft( buttonRect.right() - d->mButtonSize.width() );

      if ( buttonRect.contains( mouseEvent->pos() ) ) {
        model->removeRows( index.row(), 1 );
        return true;
      }
    }
  }
  return QStyledItemDelegate::editorEvent( event, model, option, index );
}

#include "contactgroupeditordelegate_p.moc"
