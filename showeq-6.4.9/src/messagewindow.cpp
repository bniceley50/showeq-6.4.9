/*
 *  messagewindow.cpp
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
#include "messagewindow.h"
#include "messagefilter.h"
#include "messages.h"
#include "main.h"

#include <QMenu>
#include <QInputDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QRegExp>
#include <QLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QGridLayout>
#include <QFormLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QEvent>

#pragma message("Once our minimum supported Qt version is greater than 5.14, this check can be removed and ENDL replaced with Qt::endl")
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
#define ENDL Qt::endl
#else
#define ENDL endl
#endif

//---------------------------------------------------------------------- 
// MessageBrowser
MessageBrowser::MessageBrowser(QWidget* parent, const char* name)
  : QTextEdit(parent)
{
    setObjectName(name);
}

bool MessageBrowser::eventFilter(QObject *o, QEvent *e)
{
#if 0 // ZBTEMP
  if (e->type() == QEvent::KeyPress)
  {
    QKeyEvent* k = (QKeyEvent*)e;
    fprintf(stderr, "MessageBrowser::eventFilter(KeyEvent, %x)\n", k->key());
  }
#endif // ZBTEPM
  if (e->type() != QEvent::MouseButtonPress)
    return QTextEdit::eventFilter(o, e);

  QMouseEvent* m = (QMouseEvent*)e;

  if (m->button() == Qt::RightButton)
  {
    emit rightClickedMouse(m);

    return true;
  }

  return QTextEdit::eventFilter(o, e);
}

void MessageBrowser::keyPressEvent(QKeyEvent* e)
{
  //fprintf(stderr, "MessageBrowser::keyPressEvent(%x)\n", e->key());
  switch (e->key())
  {
  case Qt::Key_R:
    if (e->modifiers() == Qt::ControlModifier)
    {
      emit refreshRequest();
      return;
    }
  case Qt::Key_F:
    if (e->modifiers() == Qt::ControlModifier)
    {
      emit findRequest();
      return;
    }
  case Qt::Key_L:
    if (e->modifiers() == Qt::ControlModifier)
    {
      emit lockRequest();
      return;
    }
  };

  QTextEdit::keyPressEvent(e);
}

//----------------------------------------------------------------------
// MessageFindDialog
MessageFindDialog::MessageFindDialog(MessageBrowser* messageWindow, 
				     const QString& caption,
				     QWidget* parent, const char* name)
  : QDialog(parent),
    m_messageWindow(messageWindow),
    m_lastParagraph(0),
    m_lastIndex(0)
{
  setObjectName(name);
  setWindowTitle(caption);

  // setup the GUI
  QGridLayout* grid = new QGridLayout(this);

  // sets margin around the grid
  grid->setMargin(5);

  m_findText = new QLineEdit(this);
  m_findText->setReadOnly(false);
  connect(m_findText, SIGNAL(textChanged(const QString&)),
          this, SLOT(textChanged(const QString&)));
  grid->addWidget(m_findText, 0, 1, 1, 3);
  QLabel* label = new QLabel("Find &Text:", this);
  label->setBuddy(m_findText);
  grid->addWidget(label, 0, 0);
  m_matchCase = new QCheckBox("&Match case", this);
  grid->addWidget(m_matchCase, 1, 1);
  m_wholeWords = new QCheckBox("&Whole Words", this);
  grid->addWidget(m_wholeWords, 2, 1);
  m_findBackwards = new QCheckBox("Find &Backwards", this);
  grid->addWidget(m_findBackwards, 3, 1);


  m_find = new QPushButton("&Find", this);
  grid->addWidget(m_find, 4, 1);
  m_find->setEnabled(false);
  connect(m_find, SIGNAL(clicked()), this, SLOT(find()));
  QPushButton* close = new QPushButton("&Close", this);
  grid->addWidget(close, 4, 3);
  connect(close, SIGNAL(clicked()), this, SLOT(close()));

  // turn off resizing
  setSizeGripEnabled(false);
}

void MessageFindDialog::find()
{
  // perform a find in the message window, starting at the current position
  // using the settings from the checkboxes.
  QTextDocument::FindFlags options;
  if (m_matchCase->isChecked()) options |= QTextDocument::FindCaseSensitively;
  if (m_wholeWords->isChecked()) options |= QTextDocument::FindWholeWords;
  if (m_findBackwards->isChecked()) options |= QTextDocument::FindBackward;
  m_messageWindow->find(m_findText->text(), options);
}

void MessageFindDialog::close()
{
  // close the dialog
  QDialog::close();
}

void MessageFindDialog::textChanged(const QString& newText)
{
  // enable the find button iff there is text to search with
  m_find->setEnabled(!newText.isEmpty());
}

//----------------------------------------------------------------------
// MessageTypeStyle
MessageTypeStyle::MessageTypeStyle()
  : m_useDefaultFont(true)
{
}

MessageTypeStyle::MessageTypeStyle(const MessageTypeStyle& style)
  : m_color(style.color()),
    m_bgColor(style.bgColor()),
    m_font(style.font()),
    m_useDefaultFont(style.useDefaultFont())
{
}

MessageTypeStyle::~MessageTypeStyle()
{
}

MessageTypeStyle& MessageTypeStyle::operator=(const MessageTypeStyle& style)
{
  m_color = style.color();
  m_bgColor = style.bgColor();
  m_font = style.font();
  m_useDefaultFont = style.useDefaultFont();

  return *this;
}

void MessageTypeStyle::load(const QString& preferenceName, 
			    const QString& typeName)
{
  // load the preferences
  m_color = pSEQPrefs->getPrefColor(typeName + "Color", preferenceName,
				    m_color);
  m_bgColor = pSEQPrefs->getPrefColor(typeName + "BGColor", preferenceName,
				      m_bgColor);
  m_useDefaultFont = pSEQPrefs->getPrefBool(typeName + "UseDefaultFont",
					    preferenceName,
					    true);
  m_font = pSEQPrefs->getPrefFont(typeName + "Font", preferenceName,
				  QFont());
}

void MessageTypeStyle::save(const QString& preferenceName, 
			    const QString& typeName) const
{
  // save the preferences
  pSEQPrefs->setPrefColor(typeName + "Color", preferenceName, m_color);
  pSEQPrefs->setPrefColor(typeName + "BGColor", preferenceName, m_bgColor);
  pSEQPrefs->setPrefBool(typeName + "UseDefaultFont", preferenceName,
			 m_useDefaultFont);
  pSEQPrefs->setPrefFont(typeName + "Font", preferenceName, m_font);
}

//----------------------------------------------------------------------
// MessageTypeStyleDialog
MessageTypeStyleDialog::MessageTypeStyleDialog(MessageTypeStyle& style, 
					       const QColor& color, 
					       const QColor& bgColor,
					       const QString& caption, 
					       QWidget* parent, 
					       const char* name)
  :QDialog(parent),
   m_style(style),
   m_defaultColor(color),
   m_defaultBGColor(bgColor)
{
  setObjectName(name);
  setWindowTitle(caption);
  setModal(true);

  // setup the GUI
  QFormLayout* grid = new QFormLayout(this);

  // sets margin around the grid
  grid->setMargin(10);

  m_color = new QPushButton("...", this);
  m_color->setObjectName("color");
  if (m_style.color().isValid())
  {
    QPalette p = m_color->palette();
    p.setColor(m_color->backgroundRole(), m_style.color());
    m_color->setPalette(p);
  }
  else
  {
    QPalette p = m_color->palette();
    p.setColor(m_color->backgroundRole(), m_defaultColor);
    m_color->setPalette(p);
  }
  connect(m_color, SIGNAL(clicked()),
	  this, SLOT(selectColor()));

  grid->addRow("&Color", m_color);

  m_bgColor = new QPushButton("...", this);
  m_bgColor->setObjectName("backgroundcolor");
  if (m_style.bgColor().isValid())
  {
    QPalette p = m_bgColor->palette();
    p.setColor(m_bgColor->backgroundRole(), m_style.bgColor());
    m_bgColor->setPalette(p);
  }
  else
  {
    QPalette p = m_bgColor->palette();
    p.setColor(m_bgColor->backgroundRole(), m_defaultBGColor);
    m_bgColor->setPalette(p);
  }
  connect(m_bgColor, SIGNAL(clicked()),
	  this, SLOT(selectBGColor()));

  grid->addRow("&Background Color", m_bgColor);

  m_useDefaultFont = new QCheckBox("Use &Default Font", this);
  m_useDefaultFont->setObjectName("usedefaultfont");
  m_useDefaultFont->setChecked(m_style.useDefaultFont());
  connect(m_useDefaultFont, SIGNAL(toggled(bool)),
	  this, SLOT(useDefaultFontToggled(bool)));

  m_font = new QPushButton("&Font", this);
  m_font->setObjectName("font");
  m_font->setEnabled(!m_style.useDefaultFont());
  connect(m_font, SIGNAL(clicked()),
	  this, SLOT(selectFont()));

  grid->addRow(m_useDefaultFont, m_font);

  QGroupBox* exampleBox = new QGroupBox("Example", this);
  exampleBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  QVBoxLayout * exampleBoxLayout = new QVBoxLayout(exampleBox);

  m_example = new QLabel(caption, exampleBox);
  m_example->setObjectName("example");
  m_example->setFrameShape(QFrame::Box);
  m_example->setFrameShadow(QFrame::Sunken);
  m_example->setAutoFillBackground(true);
  if (m_style.color().isValid())
  {
    QPalette p = m_example->palette();
    p.setColor(m_example->foregroundRole(), m_style.color());
    m_example->setPalette(p);
  }
  else
  {
    QPalette p = m_example->palette();
    p.setColor(m_example->foregroundRole(), m_defaultColor);
    m_example->setPalette(p);
  }
  if (m_style.bgColor().isValid())
  {
    QPalette p = m_example->palette();
    p.setColor(m_example->backgroundRole(), m_style.bgColor());
    m_example->setPalette(p);
  }
  else
  {
    QPalette p = m_example->palette();
    p.setColor(m_example->backgroundRole(), m_defaultBGColor);
    m_example->setPalette(p);
  }
  if (m_style.useDefaultFont())
    m_example->setFont(parent->font());
  else
    m_example->setFont(m_style.font());

  exampleBoxLayout->addWidget(m_example);
  exampleBoxLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Preferred, QSizePolicy::Expanding));

  grid->addRow(exampleBox);

  grid->setSpacing(5);

  QHBoxLayout* layout = new QHBoxLayout();
  grid->addRow(layout);
  layout->addStretch();
  QPushButton* ok = new QPushButton("OK", this);
  connect(ok, SIGNAL(clicked()),
	  this, SLOT(accept()));
  layout->addWidget(ok);

  layout->addStretch();
  QPushButton* cancel = new QPushButton("Cancel", this);
  connect(cancel, SIGNAL(clicked()),
	  this, SLOT(reject()));
  cancel->setDefault(true);
  layout->addWidget(cancel);
  layout->addStretch();

}

MessageTypeStyleDialog::~MessageTypeStyleDialog()
{
}

void MessageTypeStyleDialog::useDefaultFontToggled(bool on)
{
  m_style.setUseDefaultFont(on);

  m_font->setEnabled(!on);

  if (on)
    m_example->setFont(parentWidget()->font());
  else
    m_example->setFont(m_style.font());
}

void MessageTypeStyleDialog::selectColor()
{
  QColor color = QColorDialog::getColor(m_style.color(), this,
          windowTitle() + " Color");

  if (color.isValid())
  {
    m_style.setColor(color);

    QPalette p_bg = m_color->palette();
    p_bg.setColor(m_color->backgroundRole(), m_style.color());
    m_color->setPalette(p_bg);

    QPalette p_ex = m_example->palette();
    p_ex.setColor(m_example->foregroundRole(), m_style.color());
    m_example->setPalette(p_ex);
  }
}

void MessageTypeStyleDialog::selectBGColor()
{
  QColor color = QColorDialog::getColor(m_style.bgColor(), this,
          windowTitle() + " Background Color");

  if (color.isValid())
  {
    m_style.setBGColor(color);

    QPalette p_bg = m_bgColor->palette();
    p_bg.setColor(m_bgColor->backgroundRole(), m_style.bgColor());
    m_bgColor->setPalette(p_bg);

    QPalette p_ex = m_example->palette();
    p_ex.setColor(m_example->backgroundRole(), m_style.bgColor());
    m_example->setPalette(p_ex);
  }
}

void MessageTypeStyleDialog::selectFont()
{
  QFont newFont;
  bool ok = false;
  // get a new font
  newFont = QFontDialog::getFont(&ok, m_style.font(),
          this, windowTitle() + " Font");


  // if the user entered a font and clicked ok, set the windows font
  if (ok)
  {
    // set the styles font
    m_style.setFont(newFont);
    
    // set the example
    m_example->setFont(m_style.font());
  }
}

//----------------------------------------------------------------------
// MessageWindow
MessageWindow::MessageWindow(Messages* messages, MessageFilters* filters,
			     const QString& prefName,
			     const QString& caption,
			     QWidget* parent, const char* name)
  : SEQWindow(prefName, caption, parent, name),
    m_messages(messages),
    m_messageFilters(filters),
    m_messageWindow(0),
    m_menu(0),
    m_typeFilterMenu(0),
    m_findDialog(0),
    m_filterDialog(0),
    m_enabledTypes(0xFFFFFFFFFFFFFFFFULL),
    m_enabledShowUserFilters(0),
    m_enabledHideUserFilters(0),
    m_defaultColor(Qt::black),
    m_defaultBGColor(Qt::white),
    m_dateTimeFormat("hh:mm"),
    m_eqDateTimeFormat("ddd M/d/yyyy h:mm"),
    m_typeStyles(0),
    m_itemPattern("\022\\d(.{5}).{39}([^\022]+)\022"),
    m_lockedText(false),
    m_displayType(true),
    m_displayDateTime(false),
    m_displayEQDateTime(false),
    m_useTypeStyles(true),
    m_wrapText(true)
{
  m_enabledTypes = pSEQPrefs->getPrefUInt64("EnabledTypes", preferenceName(), 
					    m_enabledTypes);
  m_enabledShowUserFilters = pSEQPrefs->getPrefUInt("EnabledShowUserFilters",
						preferenceName(), 
						m_enabledShowUserFilters);
  m_enabledHideUserFilters = pSEQPrefs->getPrefUInt("EnabledHideUserFilters",
						preferenceName(), 
						m_enabledHideUserFilters);
  m_defaultColor = pSEQPrefs->getPrefColor("DefaultColor", preferenceName(),
					   m_defaultColor);
  m_defaultBGColor = pSEQPrefs->getPrefColor("DefaultBGColor", 
					     preferenceName(), 
					     m_defaultBGColor);
  m_dateTimeFormat = pSEQPrefs->getPrefString("DateTimeFormat",
					      preferenceName(), 
					      m_dateTimeFormat);
  m_eqDateTimeFormat = pSEQPrefs->getPrefString("EQDateTimeForamt",
						preferenceName(),
						m_eqDateTimeFormat);
  m_displayType = pSEQPrefs->getPrefBool("DisplayType", preferenceName(),
					 m_displayType);
  m_displayDateTime = pSEQPrefs->getPrefBool("DisplayDateTime",
					     preferenceName(), 
					     m_displayDateTime);
  m_displayEQDateTime = pSEQPrefs->getPrefBool("DisplayEQDateTime",
					       preferenceName(),
					       m_displayEQDateTime);
  m_useTypeStyles = pSEQPrefs->getPrefBool("UseTypeStyles", preferenceName(),
				      m_useTypeStyles);
  m_wrapText = pSEQPrefs->getPrefBool("WrapText", preferenceName(),
				      m_wrapText);

  // allocate the array of type styles
  m_typeStyles = new MessageTypeStyle[MT_Max+1];

  // create the window for text display
  m_messageWindow = new MessageBrowser(this, "messageText");

  // make the message window the main widget of the SEQWindow
  setWidget(m_messageWindow);
  
  // set the message window frame style
  m_messageWindow->setFrameStyle(QFrame::Panel | QFrame::Sunken);

  // set the current font
  m_messageWindow->setCurrentFont(font());

  // set the colors
  QPalette p = m_messageWindow->palette();
  p.setColor(QPalette::Base, m_defaultBGColor);
  p.setColor(QPalette::Text, m_defaultColor);
  m_messageWindow->setPalette(p);

  // make sure history isn't kept
  m_messageWindow->setUndoRedoEnabled(false);

  // set it to read only
  m_messageWindow->setReadOnly(true);

  // set the word wrap
  m_messageWindow->setLineWrapMode(m_wrapText ?
          QTextEdit::WidgetWidth : QTextEdit::NoWrap);

  // set the wrap policy to break at space
  m_messageWindow->setWordWrapMode(QTextOption::WordWrap);

  // connect to the Messages signal(s)
  connect(m_messages, SIGNAL(newMessage(const MessageEntry&)),
	  this, SLOT(newMessage(const MessageEntry&)));
  connect(m_messages, SIGNAL(cleared(void)),
	  this, SLOT(refreshMessages(void)));

  // connect to the message filters signals
  connect(m_messageFilters, SIGNAL(removed(uint32_t, uint8_t)),
	  this, SLOT(removedFilter(uint32_t, uint8_t)));
  connect(m_messageFilters, SIGNAL(added(uint32_t, uint8_t, 
					 const MessageFilter&)),
	  this, SLOT(addedFilter(uint32_t, uint8_t, const MessageFilter&)));

  // connect up our own signals
  connect(m_messageWindow, SIGNAL(rightClickedMouse(QMouseEvent*)),
	  this, SLOT(mousePressEvent(QMouseEvent*)));
  connect(m_messageWindow, SIGNAL(refreshRequest()),
	  this, SLOT(refreshMessages()));
  connect(m_messageWindow, SIGNAL(findRequest()),
	  this, SLOT(findDialog()));
  connect(m_messageWindow, SIGNAL(lockRequest()),
	  this, SLOT(toggleLockedText()));

  m_menu = new QMenu;
  QMenu* typeStyleMenu = new QMenu("Type St&yles");

  m_typeFilterMenu = new QMenu("Message &Type Filter - Show");
  m_menu->addMenu(m_typeFilterMenu);

  QAction* tmpAction;

  tmpAction = m_typeFilterMenu->addAction("&Enable All", this, SLOT(enableAllTypeFilters()));
  tmpAction->setData(-1);
  tmpAction = m_typeFilterMenu->addAction("&Disable All", this, SLOT(disableAllTypeFilters()));
  tmpAction->setData(-1);

  m_typeFilterMenu->addSeparator();

  QString typeName;
  // iterate over the message types, filling in various menus and getting 
  // font color preferences
  for (int i = MT_Guild; i <= MT_Max; i++)
  {
    typeName = MessageEntry::messageTypeString((MessageType)i);
    if (!typeName.isEmpty())
    {
      tmpAction = m_typeFilterMenu->addAction(typeName);
      tmpAction->setData(i);
      tmpAction->setCheckable(true);
      tmpAction->setChecked(((uint64_t(1) << i) & m_enabledTypes) != 0);

      tmpAction = typeStyleMenu->addAction(typeName + "...");
      tmpAction->setData(i);

      m_typeStyles[i].load(preferenceName(), typeName);
    }
  }

  connect(m_typeFilterMenu, SIGNAL(triggered(QAction*)),
          this, SLOT(toggleTypeFilter(QAction*)));
  connect(typeStyleMenu, SIGNAL(triggered(QAction*)),
          this, SLOT(setTypeStyle(QAction*)));

  m_showUserFilterMenu = new QMenu("User Message Filter - &Show");
  m_menu->addMenu(m_showUserFilterMenu);

  tmpAction = m_showUserFilterMenu->addAction("&Enable All", this,
                                             SLOT(enableAllShowUserFilters()));
  tmpAction->setData(-1);
  tmpAction = m_showUserFilterMenu->addAction("&Disable All", this,
                                              SLOT(disableAllShowUserFilters()));
  tmpAction->setData(-1);
  m_showUserFilterMenu->addSeparator();


  m_hideUserFilterMenu = new QMenu("User Message Filter - &Hide");
  m_menu->addMenu(m_hideUserFilterMenu);

  tmpAction = m_hideUserFilterMenu->addAction("&Enable All", this,
                                              SLOT(enableAllHideUserFilters()));
  tmpAction->setData(-1);
  tmpAction = m_hideUserFilterMenu->addAction("&Disable All", this,
                                              SLOT(disableAllHideUserFilters()));
  tmpAction->setData(-1);
  m_hideUserFilterMenu->addSeparator();

  const MessageFilter* filter;
  for(int i = 0; i < maxMessageFilters; i++)
  {
    filter = m_messageFilters->filter(i);
    if (filter)
    {
      tmpAction = m_showUserFilterMenu->addAction(filter->name());
      tmpAction->setData(i);
      tmpAction->setCheckable(true);
      tmpAction->setChecked((1 << i) & m_enabledShowUserFilters);

      tmpAction = m_hideUserFilterMenu->addAction(filter->name());
      tmpAction->setData(i);
      tmpAction->setCheckable(true);
      tmpAction->setChecked((1 << i) & m_enabledHideUserFilters);
    }
  }

  connect(m_showUserFilterMenu, SIGNAL(triggered(QAction*)),
          this, SLOT(toggleShowUserFilter(QAction*)));
  connect(m_hideUserFilterMenu, SIGNAL(triggered(QAction*)),
          this, SLOT(toggleHideUserFilter(QAction*)));

  m_menu->addAction("Edit User &Message Filters...", this,
                    SLOT(messageFilterDialog()));

  m_menu->addSeparator();
  m_menu->addAction("&Find...", this, SLOT(findDialog()), Qt::CTRL+Qt::Key_F);
  m_menu->addSeparator();

  m_action_lockText = m_menu->addAction("&Lock Text", this,
          SLOT(toggleLockedText()), Qt::CTRL+Qt::Key_L);
  m_action_lockText->setCheckable(true);
  m_action_lockText->setChecked(m_lockedText);
  m_menu->addAction("Refresh Messages...", this,
          SLOT(refreshMessages()), Qt::CTRL+Qt::Key_R);
  m_menu->addAction("Save Message Text...", this, SLOT(saveText()),
          Qt::CTRL+Qt::Key_S);
  m_menu->addSeparator();

  tmpAction = m_menu->addAction("&Display Type", this,
          SLOT(toggleDisplayType(bool)));
  tmpAction->setCheckable(true);
  tmpAction->setChecked(m_displayType);

  tmpAction = m_menu->addAction("Display T&ime/Date", this,
          SLOT(toggleDisplayTime(bool)));
  tmpAction->setCheckable(true);
  tmpAction->setChecked(m_displayDateTime);

  tmpAction = m_menu->addAction("Display &EQ Date/Time", this,
          SLOT(toggleEQDisplayTime(bool)));
  tmpAction->setCheckable(true);
  tmpAction->setChecked(m_displayEQDateTime);

  tmpAction = m_menu->addAction("&Use Type Styles", this,
          SLOT(toggleUseTypeStyles(bool)));
  tmpAction->setCheckable(true);
  tmpAction->setChecked(m_useTypeStyles);

  tmpAction = m_menu->addAction("&Wrap Text", this, SLOT(toggleWrapText(bool)));
  tmpAction->setCheckable(true);
  tmpAction->setChecked(m_wrapText);

  m_menu->addAction("Fo&nt...", this, SLOT(setFont()));
  m_menu->addAction("&Caption...", this, SLOT(setCaption()));
  m_menu->addAction("Text Colo&r...", this, SLOT(setColor()));
  m_menu->addAction("Text Back&ground Color...", this, SLOT(setBGColor()));

  m_menu->addMenu(typeStyleMenu);

  refreshMessages();
}

MessageWindow::~MessageWindow()
{
  delete [] m_typeStyles;
}

QMenu* MessageWindow::menu()
{
  return m_menu;
}

void MessageWindow::addMessage(const MessageEntry& message)
{
  MessageType type = message.type();

  // if the message type isn't enabled, nothing to do
  if ((((m_enabledTypes & ( uint64_t(1) << type)) == 0) &&
       ((m_enabledShowUserFilters & message.filterFlags()) == 0)) ||
      ((m_enabledHideUserFilters & message.filterFlags()) != 0))
    return;
  
  QString text;

  // if displaying the type, add it
  if (m_displayType)
    text = MessageEntry::messageTypeString(message.type()) + ": ";
    
  // if displaying the message date/time append it
  if (m_displayDateTime)
    text += message.dateTime().toString(m_dateTimeFormat) + " - ";

  // if displaying the messages eq date/time, append it
  if (m_displayEQDateTime && (message.eqDateTime().isValid()))
    text += message.eqDateTime().toString(m_eqDateTimeFormat) + " - ";

  // append the actual message text
  text += message.text();

  text.replace(m_itemPattern, "\\2 (#\\1)");

  // now append the message text to the buffer
  m_messageWindow->append(text);
}

void MessageWindow::addColorMessage(const MessageEntry& message)
{
  MessageType type = message.type();

  // if the message type isn't enabled, nothing to do
  if ((((m_enabledTypes & ( uint64_t(1) << type)) == 0) &&
       ((m_enabledShowUserFilters & message.filterFlags()) == 0)) ||
      ((m_enabledHideUserFilters & message.filterFlags()) != 0))
    return;

  // if the message has a specific color, then use it
  if (message.color() != ME_InvalidColor)
    m_messageWindow->setTextColor(QColor(message.color()));
  else if (m_typeStyles[type].color().isValid()) // or use the types color
    m_messageWindow->setTextColor(m_typeStyles[type].color());
  else // otherwise use the default color
    m_messageWindow->setTextColor(m_defaultColor);

  if (m_typeStyles[type].useDefaultFont())
    m_messageWindow->setCurrentFont(font());
  else
    m_messageWindow->setCurrentFont(m_typeStyles[type].font());

  QString text;

  // if displaying the type, add it
  if (m_displayType)
    text = MessageEntry::messageTypeString(type) + ": ";
    
  // if displaying the message date/time append it
  if (m_displayDateTime)
    text += message.dateTime().toString(m_dateTimeFormat) + " - ";

  // if displaying the messages eq date/time, append it
  if (m_displayEQDateTime && (message.eqDateTime().isValid()))
    text += message.eqDateTime().toString(m_eqDateTimeFormat) + " - ";

  // append the actual message text
  text += message.text();

  text.replace(m_itemPattern, "\\2 (#\\1)");

  //Set the fg/bg colors
  if (m_typeStyles[type].bgColor().isValid() &&
          m_typeStyles[type].color().isValid())
  {
      QTextCharFormat format = m_messageWindow->currentCharFormat();
      format.setForeground(m_typeStyles[type].color());
      format.setBackground(m_typeStyles[type].bgColor());
      m_messageWindow->setCurrentCharFormat(format);
  }

  // now append the message text to the buffer
  m_messageWindow->append(text);

  //Reset the fg/bg colors
  QTextCharFormat format = m_messageWindow->currentCharFormat();
  format.setForeground(m_defaultColor);
  format.setBackground(m_defaultBGColor);
  m_messageWindow->setCurrentCharFormat(format);

}

void MessageWindow::newMessage(const MessageEntry& message)
{
  // if text is locked, nothing to do
  if (m_lockedText)
    return;

  if (m_useTypeStyles)
    addColorMessage(message);
  else
    addMessage(message);
}

void MessageWindow::refreshMessages(void)
{
  // get the list of messages
  const MessageList& messages = m_messages->messageList();
 
  // set the IBeam Cursor for easier text selection
  setCursor(Qt::WaitCursor);
  m_messageWindow->setCursor(Qt::WaitCursor);

  // clear the document
  m_messageWindow->clear();

  // turn off updates while doing this mass update
  m_messageWindow->setUpdatesEnabled(false);
  setUpdatesEnabled(false);

  m_messageWindow->append(" ");

  // set the cursor to the beginning of the document
  m_messageWindow->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);

  // move the cursor to the end of the document
  m_messageWindow->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);

  // iterate over the message list and add the messages
  MessageList::const_iterator it;
  int i;
  if (m_useTypeStyles)
    for (i = 0, it = messages.begin(); it != messages.end(); ++it, ++i)
      addColorMessage(*it); // append the message with color
  else 
    for (i = 0, it = messages.begin(); it != messages.end(); ++it, ++i)
      addMessage(*it); // append the message plain
    
  // turn updates back on 
  m_messageWindow->setUpdatesEnabled(true);
  setUpdatesEnabled(true);

  // repain window now that updates have been re-enabled
  repaint();

  // set the IBeam Cursor for easier text selection
  unsetCursor();
  m_messageWindow->unsetCursor();

  // move the cursor to the end of the document
  m_messageWindow->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);

  // move the cursor to the end of the document
  m_messageWindow->ensureCursorVisible();
}

void MessageWindow::findDialog(void)
{
  // create the find dialog, if necessary
  if (!m_findDialog)
    m_findDialog = new MessageFindDialog(m_messageWindow, windowTitle() + " Find",
            this, "messagefinddialog");

  // show the find dialog
  m_findDialog->show();
}

void MessageWindow::messageFilterDialog(void)
{
  // create the filter dialog, if necessary
  if (!m_filterDialog)
    m_filterDialog = new MessageFilterDialog(m_messageFilters,
            windowTitle() + " Message Filters",
            this, "messagefilterdialog");

  // show the message filter dialog
  m_filterDialog->show();
}

void MessageWindow::saveText(void)
{
  QString fileName =
    QFileDialog::getSaveFileName(this, "ShowEQ - Message Text File", QString(), "*.txt");

  if (fileName.isEmpty())
    return;

  QFile file( fileName ); // Write the text to a file
  if ( file.open( QIODevice::WriteOnly ) ) 
  {
    QTextStream stream( &file );

    stream << m_messageWindow->toPlainText() << ENDL;
  }
}

void MessageWindow::toggleTypeFilter(QAction* action)
{

  int id = action->data().value<int>();

  //Enable/Disable All also invokes this method (due to connecting via the
  //menu rather than the individual items), and this causes issues
  //with MT_Guild since it's type 0.  So we've given Enable/Disable all their
  //own bogus type value
  if (id == -1) return;

  // toggle whether the message type is shown or not
  if (((uint64_t(1) << id) & m_enabledTypes) != 0)
    m_enabledTypes &= ~(uint64_t(1) << id);
  else
    m_enabledTypes |= (uint64_t(1) << id);

  // save the new setting
  pSEQPrefs->setPrefUInt64("EnabledTypes", preferenceName(), m_enabledTypes);

}

void MessageWindow::disableAllTypeFilters()
{
  // set and save all message types disabled
  m_enabledTypes = 0;
  pSEQPrefs->setPrefUInt64("EnabledTypes", preferenceName(), m_enabledTypes);


  // uncheck all the menu items
  foreach (QAction* action, m_typeFilterMenu->actions())
  {
      if (action->text().contains("&Enable All") ||
              action->text().contains("&Disable All") ||
              action->isSeparator())
          continue;

      action->setChecked(false);
  }
}

void MessageWindow::enableAllTypeFilters()
{
  // set and save all message types enabled
  m_enabledTypes = 0xFFFFFFFFFFFFFFFFULL;
  pSEQPrefs->setPrefUInt64("EnabledTypes", preferenceName(), m_enabledTypes);


  // check all the menu items
  foreach (QAction* action, m_typeFilterMenu->actions())
  {
      if (action->text().contains("&Enable All") ||
              action->text().contains("&Disable All") ||
              action->isSeparator())
          continue;

      action->setChecked(true);
  }
}

void MessageWindow::toggleShowUserFilter(QAction* action)
{
  int id = action->data().value<int>();

  //Enable/Disable All also invokes this method (due to connecting via the
  //menu rather than the individual items), and this causes issues
  //with MT_Guild since it's type 0.  So we've given Enable/Disable all their
  //own bogus type value
  if (id == -1) return;

  // toggle whether the filter is enabled/disabled
  if (((1 << id) & m_enabledShowUserFilters) != 0)
    m_enabledShowUserFilters &= ~(1 << id);
  else
    m_enabledShowUserFilters |= (1 << id);

  // save the new setting
  pSEQPrefs->setPrefUInt("EnabledShowUserFilters", preferenceName(),
          m_enabledShowUserFilters);

}

void MessageWindow::disableAllShowUserFilters()
{
  // set and save all filters disabled setting
  m_enabledShowUserFilters = 0;
  pSEQPrefs->setPrefUInt("EnabledShowUserFilters", preferenceName(),
          m_enabledShowUserFilters);

  // uncheck all the menu items
  foreach (QAction* action, m_showUserFilterMenu->actions())
  {
      if (action->text().contains("&Enable All") ||
              action->text().contains("&Disable All") ||
              action->isSeparator())
          continue;

      action->setChecked(false);
  }
}

void MessageWindow::enableAllShowUserFilters()
{
  // set and save all filters enabled flag
  m_enabledShowUserFilters = 0xFFFFFFFF;
  pSEQPrefs->setPrefUInt("EnabledShowUserFilters", preferenceName(),
          m_enabledShowUserFilters);

  // check all the menu items
  foreach (QAction* action, m_showUserFilterMenu->actions())
  {
      if (action->text().contains("&Enable All") ||
              action->text().contains("&Disable All") ||
              action->isSeparator())
          continue;

      action->setChecked(true);
  }
}

void MessageWindow::toggleHideUserFilter(QAction* action)
{
  int id = action->data().value<int>();

  //Enable/Disable All also invokes this method (due to connecting via the
  //menu rather than the individual items), and this causes issues
  //with MT_Guild since it's type 0.  So we've given Enable/Disable all their
  //own bogus type value
  if (id == -1) return;

  // toggle whether the filter is enabled/disabled
  if (((1 << id) & m_enabledHideUserFilters) != 0)
    m_enabledHideUserFilters &= ~(1 << id);
  else
    m_enabledHideUserFilters |= (1 << id);

  // save the new setting
  pSEQPrefs->setPrefUInt("EnabledHideUserFilters", preferenceName(),
          m_enabledHideUserFilters);
}

void MessageWindow::disableAllHideUserFilters()
{
  // set and save all filters disabled setting
  m_enabledHideUserFilters = 0;
  pSEQPrefs->setPrefUInt("EnabledHideUserFilters", preferenceName(),
          m_enabledHideUserFilters);

  // uncheck all the menu items
  foreach (QAction* action, m_hideUserFilterMenu->actions())
  {
      if (action->text().contains("&Enable All") ||
              action->text().contains("&Disable All") ||
              action->isSeparator())
          continue;

      action->setChecked(false);
  }
}

void MessageWindow::enableAllHideUserFilters()
{
  // set and save all filters enabled flag
  m_enabledHideUserFilters = 0xFFFFFFFF;
  pSEQPrefs->setPrefUInt("EnabledHideUserFilters", preferenceName(),
          m_enabledHideUserFilters);

  // check all the menu items
  foreach (QAction* action, m_hideUserFilterMenu->actions())
  {
      if (action->text().contains("&Enable All") ||
              action->text().contains("&Disable All") ||
              action->isSeparator())
          continue;

      action->setChecked(true);
  }
}

void MessageWindow::toggleLockedText()
{
//  m_lockedText = enable;
  m_lockedText = !m_lockedText;

  // if the text had been locked, refresh the messages
  if (!m_lockedText)
    refreshMessages();
}

void MessageWindow::toggleDisplayType(bool enable)
{
  m_displayType = enable;

  pSEQPrefs->setPrefBool("DisplayType", preferenceName(), m_displayType);
}

void MessageWindow::toggleDisplayTime(bool enable)
{
  m_displayDateTime = enable;

  pSEQPrefs->setPrefBool("DisplayDateTime", preferenceName(),
          m_displayDateTime);
}

void MessageWindow::toggleEQDisplayTime(bool enable)
{
  m_displayEQDateTime = enable;

  pSEQPrefs->setPrefBool("DisplayEQDateTime", preferenceName(),
          m_displayEQDateTime);
}

void MessageWindow::toggleUseTypeStyles(bool enable)
{
  m_useTypeStyles = enable;

  pSEQPrefs->setPrefBool("UseTypeStyles", preferenceName(), m_useTypeStyles);
}

void MessageWindow::toggleWrapText(bool enable)
{
  m_wrapText = enable;

  pSEQPrefs->setPrefBool("WrapText", preferenceName(), m_wrapText);

  // set the wrap policy according to the setting
  m_messageWindow->setLineWrapMode(m_wrapText ?
          QTextEdit::WidgetWidth : QTextEdit::NoWrap);
}

void MessageWindow::setTypeStyle(QAction* action)
{

  int id = action->data().value<int>();

  // Create the dialog object
  QString typeName = MessageEntry::messageTypeString((MessageType)id);
  QString styleCaption = windowTitle() + " - " + typeName + " Style";
  MessageTypeStyleDialog dialog(m_typeStyles[id],
          m_defaultColor, m_defaultBGColor,
          styleCaption.toLatin1().data(),
          this, styleCaption.toLatin1().data());

  // popup the modal dialog
  int result = dialog.exec();

  // if the user accepted the changes, then apply them to the style
  if (result == QDialog::Accepted)
  {
    // apply the style
    m_typeStyles[id] = dialog.style();

    // save the updates
    m_typeStyles[id].save(preferenceName(), typeName);
  }
}

void MessageWindow::setColor()
{
  QString clrCaption = windowTitle() + " Default Text Color";

  // get a new color
  QColor color = QColorDialog::getColor(m_defaultColor, this, clrCaption);

  // if the user clicked ok, use/save the preference
  if (color.isValid())
  {
    m_defaultColor = color;

    pSEQPrefs->setPrefColor("DefaultColor", preferenceName(), 
			    m_defaultColor);
  }
}

void MessageWindow::setBGColor()
{
  QString clrCaption = windowTitle() + " Default Text Background Color";

  // get a new background color
  QColor color = QColorDialog::getColor(m_defaultBGColor, this, clrCaption);

  // if the user clicked ok, use/save the preference
  if (color.isValid())
  {
    m_defaultBGColor = color;
    QPalette p = m_messageWindow->palette();
    p.setColor(QPalette::Base, m_defaultBGColor);
    m_messageWindow->setPalette(p);

    pSEQPrefs->setPrefColor("DefaultBGColor", preferenceName(), 
			    m_defaultBGColor);
  }
}

void MessageWindow::setFont()
{
  QFont newFont;
  bool ok = false;

  // get a new font
  newFont = QFontDialog::getFont(&ok, m_messageWindow->currentFont(),
          this, windowTitle() + " Font");


  // if the user entered a font and clicked ok, set the windows font
  if (ok)
    setWindowFont(newFont);
}

void MessageWindow::setCaption()
{
  bool ok = false;

  QString captionText =
    QInputDialog::getText(this, "ShowEQ Message Window Caption",
            "Enter caption for this Message Window:",
            QLineEdit::Normal, windowTitle(),
            &ok);

  // if the user entered a caption and clicked ok, set the windows caption
  if (ok)
    SEQWindow::setCaption(captionText);
}

void MessageWindow::restoreFont()
{
  // restore the font
  SEQWindow::restoreFont();
  
  // set the message windows font to match
  if (m_messageWindow)
    m_messageWindow->setCurrentFont(font());
}

void MessageWindow::removedFilter(uint32_t mask, uint8_t filter)
{
  // remove the show user filter menu item
  foreach(QAction* action, m_showUserFilterMenu->actions())
  {
      if (action->data().value<int>() == filter)
      {
          m_showUserFilterMenu->removeAction(action);
          break;
      }
  }

  // remove the hide user filter menu item
  foreach(QAction* action, m_hideUserFilterMenu->actions())
  {
      if (action->data().value<int>() == filter)
      {
          m_hideUserFilterMenu->removeAction(action);
          break;
      }
  }


  // if all filters are enabled, don't unselect it
  if (m_enabledShowUserFilters != 0xFFFFFFFF)
  {
    // remove the filter from the enabled filters list
    m_enabledShowUserFilters &= ~mask;

    // update the preference
    pSEQPrefs->setPrefUInt("EnabledShowUserFilters", preferenceName(), 
			   m_enabledShowUserFilters);
  }
  
  // if all filters are enabled, don't unselect it
  if (m_enabledHideUserFilters != 0xFFFFFFFF)
  {
    // remove the filter from the enabled filters list
    m_enabledHideUserFilters &= ~mask;

    // update the preference
    pSEQPrefs->setPrefUInt("EnabledHideUserFilters", preferenceName(), 
			   m_enabledHideUserFilters);
  }
}

void MessageWindow::addedFilter(uint32_t mask, uint8_t filterid, 
				const MessageFilter& filter)
{
  QAction* action;
  // insert a user filter menu item for the new filter
  action = m_showUserFilterMenu->addAction(filter.name());
  action->setCheckable(true);
  action->setData(filterid);

  // insert a user filter menu item for the new filter
  action = m_hideUserFilterMenu->addAction(filter.name());
  action->setCheckable(true);
  action->setData(filterid);
}

#ifndef QMAKEBUILD
#include "messagewindow.moc"
#endif

