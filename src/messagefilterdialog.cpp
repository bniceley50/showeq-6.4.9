/*
 *  messagefilterdialog.cpp
 *  Copyright 2003-2007, 2019 by the respective ShowEQ Developers
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

#include "messagefilterdialog.h"
#include "messagefilter.h"
#include "message.h"

#include <cstdint>
#include <cstdio>

#include <QString>
#include <QRegExp>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFormLayout>

//----------------------------------------------------------------------
// MessageFilterListBoxText
class MessageFilterListBoxText : public QListWidgetItem
{
public:
  MessageFilterListBoxText(QListWidget * listbox,
			   const QString & text = "", 
			   uint32_t data = 0);

  virtual ~MessageFilterListBoxText();

  uint32_t data() { return m_data; }
  void setData(uint32_t data) { m_data = data; }

protected:
  uint32_t m_data;
};

MessageFilterListBoxText::MessageFilterListBoxText(QListWidget * listbox,
						   const QString & text, 
						   uint32_t data)
  : QListWidgetItem(text, listbox),
    m_data(data)
{
}


MessageFilterListBoxText::~MessageFilterListBoxText()
{
}

//----------------------------------------------------------------------
// MessageFilterDialog
MessageFilterDialog::MessageFilterDialog(MessageFilters* filters, 
					 const QString& caption,
					 QWidget* parent,
					 const char* name)
  : QDialog(parent, Qt::Dialog),
    m_filters(filters),
    m_currentFilterNum(0xFF),
    m_currentFilter(0)
{
  setObjectName(name);
  // set the caption
  setWindowTitle(caption);

  // don't support resizing the dialog
  setSizeGripEnabled(false);

  // connect to the MessageFilter signals
  connect(m_filters, SIGNAL(removed(uint32_t, uint8_t)),
	  this, SLOT(removedFilter(uint32_t, uint8_t)));
  connect(m_filters, SIGNAL(added(uint32_t, uint8_t, 
				  const MessageFilter&)),
	  this, SLOT(addedFilter(uint32_t, uint8_t, const MessageFilter&)));

  // setup the dialog
  QVBoxLayout* outerLayout = new QVBoxLayout(this);
  QHBoxLayout* columnLayout = new QHBoxLayout();
  QVBoxLayout* column1Layout = new QVBoxLayout();
  outerLayout->addLayout(columnLayout, 1);
  columnLayout->addLayout(column1Layout, 1);

  // layout 1st column
  QLabel* label = new QLabel("&Existing Filters", this);
  column1Layout->addWidget(label, 1, Qt::AlignCenter);

  m_existingFilters = new QListWidget(this);
  column1Layout->addWidget(m_existingFilters, 10);
  label->setBuddy(m_existingFilters);
  m_existingFilters->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(m_existingFilters->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this,
          SLOT(existingFilterSelectionChanged(const QItemSelection&, const QItemSelection&)));

  m_new = new QPushButton("Ne&w", this);
  column1Layout->addWidget(m_new, 1, Qt::AlignCenter);
  connect(m_new, SIGNAL(clicked()),
	  this, SLOT(newFilter()));

  m_filterGroup = new QGroupBox("New &Filter", this);
  columnLayout->addWidget(m_filterGroup, 5);

  QVBoxLayout *filterGBoxLayout = new QVBoxLayout(m_filterGroup);
  QHBoxLayout *filterButtonLayout = new QHBoxLayout();
  QFormLayout* newFilterLayout = new QFormLayout();

  m_name = new QLineEdit(m_filterGroup);
  m_name->setObjectName("name");
  newFilterLayout->addRow("&Name", m_name);
  connect(m_name, SIGNAL(textChanged(const QString&)),
	  this, SLOT(anyTextChanged(const QString&)));

  m_pattern = new QLineEdit(m_filterGroup);
  m_pattern->setObjectName("pattern");
  newFilterLayout->addRow("&Pattern", m_pattern);
  connect(m_pattern, SIGNAL(textChanged(const QString&)),
	  this, SLOT(anyTextChanged(const QString&)));

  m_messageTypes = new QListWidget(m_filterGroup);
  newFilterLayout->addRow("&Message Types", m_messageTypes);
  m_messageTypes->setSelectionMode(QAbstractItemView::MultiSelection);
  connect(m_messageTypes->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this,
          SLOT(messageTypeSelectionChanged(const QItemSelection&, const QItemSelection&)));

  m_delete = new QPushButton("&Delete", m_filterGroup);
  m_delete->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  filterButtonLayout->addWidget(m_delete);
  m_delete->setEnabled(false);
  connect(m_delete, SIGNAL(clicked()),
	  this, SLOT(deleteFilter()));

  m_update = new QPushButton("&Update", m_filterGroup);
  m_update->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  filterButtonLayout->addWidget(m_update);
  m_update->setEnabled(false);
  connect(m_update, SIGNAL(clicked()),
	  this, SLOT(updateFilter()));

  m_add = new QPushButton("&Add", m_filterGroup);
  m_add->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  filterButtonLayout->addWidget(m_add);
  m_add->setEnabled(false);
  connect(m_add, SIGNAL(clicked()),
	  this, SLOT(addFilter()));

  filterButtonLayout->setAlignment(Qt::AlignCenter);
  filterButtonLayout->insertStretch(0, 1);
  filterButtonLayout->insertStretch(2, 1);
  filterButtonLayout->insertStretch(4, 1);
  filterButtonLayout->insertStretch(6, 1);

  newFilterLayout->addRow(filterButtonLayout);

  filterGBoxLayout->addLayout(newFilterLayout, 2);

  QPushButton* close = new QPushButton("&Close", this);
  outerLayout->addWidget(close, 1, Qt::AlignCenter);
  connect(close, SIGNAL(clicked()),
	  this, SLOT(accept()));

  // fill in message types
  QString typeName;
  for (int i = MT_Guild; i <= MT_Max; i++)
  {
    typeName = MessageEntry::messageTypeString((MessageType)i);
    if (!typeName.isEmpty())
      (void)new MessageFilterListBoxText(m_messageTypes, typeName, i);
  }

  // fill in existing messages
  const MessageFilter* filter;
  for (int i = 0; i < maxMessageFilters; i++)
  {
    filter = m_filters->filter(i);
    if (filter)
      (void)new MessageFilterListBoxText(m_existingFilters, filter->name(), i);
  }
}

MessageFilterDialog::~MessageFilterDialog()
{
}

void MessageFilterDialog::newFilter()
{
  // clear any selection
  m_existingFilters->clearSelection();

  // clear the filter display
  clearFilter();

  // check the current state and set UI up accordingly
  checkState();
}

void MessageFilterDialog::addFilter()
{
  uint32_t type;
  uint64_t types = 0;

  // iterate over the message types
  int numTypeRows = m_messageTypes->count();
  for (int row = 0; row < numTypeRows; ++row)
  {
      MessageFilterListBoxText* item = (MessageFilterListBoxText*) m_messageTypes->item(row);

      if (item->isSelected())
      {
          // get the message type of the selected item
          type = item->data();

          // add its flag to the types
          types |= (uint64_t(1) << type);
      }
  }

  // create a message filter object
  MessageFilter newFilter(m_name->text(), types, QRegExp(m_pattern->text()));

  // if this isn't a valid filter, don't create it
  if (!newFilter.valid())
    return;

  // add the new filter
  m_currentFilterNum = m_filters->addFilter(newFilter);

  // if it is a valid filter, make the new filter the current selection
  if (m_currentFilterNum != 0xFF)
  {
    // retrieve the current item
    m_currentFilter = m_filters->filter(m_currentFilterNum);

    // iterate over the existing filters
    int numFiltRows = m_existingFilters->count();
    for (int row = 0; row < numFiltRows; ++row)
    {
      MessageFilterListBoxText* item = (MessageFilterListBoxText*) m_existingFilters->item(row);
      // find the current filter
      if (item->data() == m_currentFilterNum)
      {
          // make the current filter the selected filter
          item->setSelected(true);
          m_existingFilters->setCurrentRow(row);
          break;
      }
    }
  }
  else // clear the current filter
  {
    // clear the current filter
    m_currentFilter = 0;
    clearFilter();
  }
  
  // setup the current dialog state
  checkState();
}

void MessageFilterDialog::updateFilter()
{

  //Note, this used to happen in the opposite order: delete then add.
  //But in Qt4, removing the old filter triggers the list selection update,
  //which causes the previous/next item to be selected, losing the updated
  //values and instead duplicating the adjacent filter.
  //
  //Doing it this way avoids that problem, but as a result it causes the
  //updated filter to jump to a different position in the list.
  //
  //TODO to properly fix this, either we need to do one or both of:
  // 1) implement a proper "update" that update the existing filter and
  //    list item without removing the old one
  // 2) Change the filter list to sort by something other than the filter index
  //    Name might be a good choice here.
  //

  const MessageFilter* oldFilter = m_currentFilter;

  // add in a new filter
  addFilter();

  // delete the old filter
  if (oldFilter)
    m_filters->remFilter(*oldFilter);

}

void MessageFilterDialog::deleteFilter()
{
  // remove the current filter (if any are selected)
  if (m_currentFilter)
    m_filters->remFilter(*m_currentFilter);

  // clear any selection
  m_existingFilters->clearSelection();

  // clear the filter display
  clearFilter();

  // check the current state and set UI up accordingly
  checkState();
}

void MessageFilterDialog::anyTextChanged(const QString& newText)
{
  // check the state whenever any text changes
  checkState();
}

void MessageFilterDialog::messageTypeSelectionChanged(
        const QItemSelection& selected, const QItemSelection& deselected)
{
  // check the state whenever the message type selection changed
  checkState();
}

void MessageFilterDialog::existingFilterSelectionChanged(
        const QItemSelection& selected, const QItemSelection& deselected)
{

  //
  if (selected.count())
  {
      // the filter list only allows selecting one item at a time, so
      // we can simply grab the first (and only) selection
      int row = selected.indexes().first().row();

      MessageFilterListBoxText* item = (MessageFilterListBoxText*)m_existingFilters->item(row);

      // get the current filter number from the listbox item
      m_currentFilterNum = item->data();

      // get the specified filter
      m_currentFilter = m_filters->filter(m_currentFilterNum);

      // set the GroupBox's label
      m_filterGroup->setTitle(m_currentFilter->name() + " &Filter");

      // setup all the filter values
      m_name->setText(m_currentFilter->name());
      m_pattern->setText(m_currentFilter->regexp().pattern());

      // select all the message types
      uint64_t messageTypes = m_currentFilter->types();
      uint32_t messageType;
      int numRows = m_messageTypes->count();
      for (int row = 0; row < numRows; ++row)
      {
          MessageFilterListBoxText* item = (MessageFilterListBoxText*) m_messageTypes->item(row);

          messageType = item->data();
          item->setSelected(((uint64_t(1) << messageType) & messageTypes) != 0);
      }

  }
  else
    clearFilter();

  // check the current state
  checkState();
}

void MessageFilterDialog::removedFilter(uint32_t mask, uint8_t filter)
{
  // iterate over all the existing filters
  int numRows = m_existingFilters->count();
  for (int row = 0; row < numRows; ++row)
  {
      MessageFilterListBoxText* item = (MessageFilterListBoxText*)m_existingFilters->item(row);

      // check if this is the removed filter
      if (item->data() == filter)
      {
          // delete the removed filter's list box item
          delete m_existingFilters->takeItem(row);

          // nothing more to do
          break;
      }
  }
}

void MessageFilterDialog::addedFilter(uint32_t mask, uint8_t filterid, 
				      const MessageFilter& filter)
{
  if (m_existingFilters->count() == 0)
  {
      // add the new message filter 
      new MessageFilterListBoxText(m_existingFilters, filter.name(), filterid);

      return;
  }

  // iterate over all the existing filters
  int numFiltRows = m_existingFilters->count();
  for (int row = 0; row < numFiltRows; ++row)
  {
    MessageFilterListBoxText* item = (MessageFilterListBoxText*)m_existingFilters->item(row);
    // check if this is the removed filter
    if (item->data() > filterid)
    {
      // add a new message filter at the appropriate location
      //   NOTE: This maintains list order during an item update
      m_existingFilters->insertItem(row,
              new MessageFilterListBoxText(nullptr, filter.name(), filterid));
      break;
    }
    else
    {
      //if we're at the end of the list and still haven't inserted, we need
      //to do it, otherwise, it won't get added to the list.
      if (row + 1 == numFiltRows)
      {
        m_existingFilters->addItem(
                new MessageFilterListBoxText(nullptr, filter.name(), filterid));
        break;
      }
    }
  }
}

void MessageFilterDialog::clearFilter()
{
  // set filter information to default state
  m_currentFilterNum = 0xFF;
  m_currentFilter = 0;
  m_filterGroup->setTitle("New &Filter");
  m_name->setText("");
  m_pattern->setText("");
  m_messageTypes->clearSelection();
}

void MessageFilterDialog::checkState()
{
  bool update = false;
  bool add = false;

  // the state check varies depending on if their is a current filter or not
  if (m_currentFilter)
  {
    uint32_t type;
    uint64_t types = 0;

    // buttons should only be enabled for valid message filter content
    if (!m_name->text().isEmpty() &&
	!m_pattern->text().isEmpty() &&
	QRegExp(m_pattern->text()).isValid())
    {
      // iterate over all the message types
      int numRows = m_messageTypes->count();
      for (int row = 0; row < numRows; ++row)
      {
          MessageFilterListBoxText* item = (MessageFilterListBoxText*)m_messageTypes->item(row);
          // is the current item selected
          if (item->isSelected())
          {
              // get the items message type
              type = item->data();

              // add the message type into the message types
              types |= (uint64_t(1) << type);

              // found a selected item, fields are valid for update
              update = true;
          }
      }

      // only enable add if the filter is different from its predecessor
      if ((m_name->text() != m_currentFilter->name()) ||
              (m_pattern->text() != m_currentFilter->regexp().pattern()) ||
              (types != m_currentFilter->types()))
          add = true;

    }
  }
  else
  {
    // buttons should only be enabled for valid message filter content
    if (!m_name->text().isEmpty() &&
	!m_pattern->text().isEmpty())
    {
      // iterate over all the message types
      int numRows = m_messageTypes->count();
      for (int row = 0; row < numRows; ++row)
      {
          MessageFilterListBoxText* item = (MessageFilterListBoxText*)m_messageTypes->item(row);
          // if the item isn't selected, try the next item
          if (!item->isSelected())
              continue;

          // found a selected item, fields are valid for add
          add = true;
          break;
      }
    }
  }

  // set the button states according to the results from above
  m_add->setEnabled(add);
  m_update->setEnabled(update);

  // only enable delete if editing an existing filter
  m_delete->setEnabled(m_currentFilter != 0);
}

#ifndef QMAKEBUILD
#include "messagefilterdialog.moc"
#endif
