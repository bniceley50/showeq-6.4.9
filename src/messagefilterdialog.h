/*
 *  messagefilterdialog.h
 *  Copyright 2003-2009, 2019 by the respective ShowEQ Developers
 *
 *  This file is part of ShowEQ.
 *  http://www.sourceforge.net/projects/seq
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MESSAGEFILTERDIALOG_H_
#define _MESSAGEFILTERDIALOG_H_

#include <cstdint>

#include <QDialog>
#include <QLabel>

//----------------------------------------------------------------------
// forward declarations
class MessageFilter;
class MessageFilters;

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QGroupBox;
class QItemSelection;

//----------------------------------------------------------------------
// MessageFilterDialog
class MessageFilterDialog : public QDialog
{
  Q_OBJECT
 public:
  MessageFilterDialog(MessageFilters* filters, const QString& caption,
		      QWidget* parent = 0, const char* name = 0);
  ~MessageFilterDialog();

 public slots:
   void newFilter();
   void addFilter();
   void updateFilter();
   void deleteFilter();

 protected slots:
   void anyTextChanged(const QString& newText);
   void messageTypeSelectionChanged(const QItemSelection& selected,
           const QItemSelection& deselected);
   void existingFilterSelectionChanged(const QItemSelection& selected,
           const QItemSelection& deselected);
   void removedFilter(uint32_t mask, uint8_t filter);
   void addedFilter(uint32_t mask, uint8_t filterid, const MessageFilter& filter);

 protected:
   void clearFilter();
   void checkState();

  MessageFilters* m_filters;
  QListWidget* m_existingFilters;
  QPushButton* m_new;
  QGroupBox* m_filterGroup;
  QLineEdit* m_name;
  QLineEdit* m_pattern;
  QListWidget* m_messageTypes;
  QPushButton* m_add;
  QPushButton* m_update;
  QPushButton* m_delete;
  uint8_t m_currentFilterNum;
  const MessageFilter* m_currentFilter;
};

#endif // _MESSAGEFILTERDIALOG_H_
