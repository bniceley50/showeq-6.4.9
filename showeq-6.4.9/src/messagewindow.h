/*
 *  messagewindow.h
 *  Copyright 2003-2005, 2019 by the respective ShowEQ Developers
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

#ifndef _MESSAGEWINDOW_H_
#define _MESSAGEWINDOW_H_

#include "seqwindow.h"

#include <cstdint>

#include <QTextEdit>
#include <QRegExp>
#include <QDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QMenu>
#include <QKeyEvent>
#include <QEvent>

//----------------------------------------------------------------------
// forward declarations
class MessageEntry;
class MessageFilter;
class MessageFilters;
class Messages;
class MessageFilterDialog;

class QMenu;
class QLineEdit;
class QCheckBox;
class QLabel;

//----------------------------------------------------------------------
// MessageBrowser
class MessageBrowser : public QTextEdit
{
  Q_OBJECT
 public:
  MessageBrowser(QWidget* parent = 0, const char* name = 0);

 signals:
  void rightClickedMouse(QMouseEvent* e);
  void refreshRequest();
  void findRequest();
  void lockRequest();

 protected:
  virtual void keyPressEvent(QKeyEvent* e);
  virtual bool eventFilter(QObject *o, QEvent *e);
};

//----------------------------------------------------------------------
// MessageFindDialog
class MessageFindDialog : public QDialog
{
  Q_OBJECT
 public:
  MessageFindDialog(MessageBrowser*, const QString& caption,
		    QWidget* parent = 0, const char* name = 0);

 public slots:
  void find();
  void close();

 protected slots:
  void textChanged(const QString& newText);

 protected:
  MessageBrowser* m_messageWindow;
  QLineEdit* m_findText;
  QCheckBox* m_matchCase;
  QCheckBox* m_wholeWords;
  QCheckBox* m_findBackwards;
  QPushButton* m_find;
  int m_lastParagraph;
  int m_lastIndex;
};

//----------------------------------------------------------------------
// MessageTypeStyle
class MessageTypeStyle
{
 public:
  MessageTypeStyle();
  MessageTypeStyle(const MessageTypeStyle& style);
  ~MessageTypeStyle();
  
  MessageTypeStyle& operator=(const MessageTypeStyle& style);

  const QColor& color() const { return m_color; }
  void setColor(const QColor& color) { m_color = color; }
  const QColor& bgColor() const { return m_bgColor; }
  void setBGColor(const QColor& color) { m_bgColor = color; }
  const QFont& font() const { return m_font; }
  void setFont(const QFont& font) { m_font = font; }
  bool useDefaultFont() const { return m_useDefaultFont; }
  void setUseDefaultFont(bool val) { m_useDefaultFont = val; }
  
  void load(const QString& preferenceName, const QString& typeName);
  void save(const QString& preferenceName, const QString& typeName) const;
 protected:
  QColor m_color;
  QColor m_bgColor;
  QFont m_font;
  bool m_useDefaultFont;
};

//----------------------------------------------------------------------
// MessageTypeStyleDialog
class MessageTypeStyleDialog : public QDialog
{
  Q_OBJECT
 public:
  MessageTypeStyleDialog(MessageTypeStyle& style, 
			 const QColor& color, const QColor& bgColor,
			 const QString& caption,
			 QWidget* parent = 0, const char* name = 0);
  ~MessageTypeStyleDialog();
  const MessageTypeStyle& style() { return m_style; }

 public slots:
   void selectColor();
   void selectBGColor();
   void selectFont();

 protected slots:
  void useDefaultFontToggled(bool on);

 protected:
  MessageTypeStyle m_style;
  const QColor& m_defaultColor;
  const QColor& m_defaultBGColor;
  QPushButton* m_color;
  QPushButton* m_bgColor;
  QCheckBox* m_useDefaultFont;
  QPushButton* m_font;
  QLabel* m_example;
};

//----------------------------------------------------------------------
// MessageWindow
class MessageWindow : public SEQWindow
{
  Q_OBJECT
 public:
  MessageWindow(Messages* messages, MessageFilters* filters,
		const QString& prefName = "MessageWindow",
		const QString& caption = "Message Window",
		QWidget* parent = 0, const char* name = 0);
  ~MessageWindow();

  virtual QMenu* menu();
  
 public slots:
  void newMessage(const MessageEntry& message);
  void refreshMessages(void);
  void findDialog(void);
  void messageFilterDialog(void);
  void saveText(void);

 protected slots:
  void toggleTypeFilter(QAction*);
  void disableAllTypeFilters();
  void enableAllTypeFilters();
  void toggleShowUserFilter(QAction*);
  void disableAllShowUserFilters();
  void enableAllShowUserFilters();
  void toggleHideUserFilter(QAction*);
  void disableAllHideUserFilters();
  void enableAllHideUserFilters();
  void toggleLockedText();
  void toggleDisplayType(bool);
  void toggleDisplayTime(bool);
  void toggleEQDisplayTime(bool);
  void toggleUseTypeStyles(bool);
  void toggleWrapText(bool);
  void setTypeStyle(QAction*);
  void setColor();
  void setBGColor();
  void setFont();
  void setCaption();
  virtual void restoreFont();
  void removedFilter(uint32_t mask, uint8_t filter);
  void addedFilter(uint32_t mask, uint8_t filterid, const MessageFilter& filter);

 protected:
  void addMessage(const MessageEntry& message);
  void addColorMessage(const MessageEntry& message);

  Messages* m_messages;
  MessageFilters* m_messageFilters;
  MessageBrowser* m_messageWindow;
  QMenu* m_menu;
  QMenu* m_typeFilterMenu;
  QMenu* m_showUserFilterMenu;
  QMenu* m_hideUserFilterMenu;
  QAction* m_action_lockText;
  MessageFindDialog* m_findDialog;
  MessageFilterDialog* m_filterDialog;
  uint64_t m_enabledTypes;
  uint32_t m_enabledShowUserFilters;
  uint32_t m_enabledHideUserFilters;
  QColor m_defaultColor;
  QColor m_defaultBGColor;
  QString m_dateTimeFormat;
  QString m_eqDateTimeFormat;
  MessageTypeStyle* m_typeStyles;
  QRegExp m_itemPattern;
  bool m_lockedText;
  bool m_displayType;
  bool m_displayDateTime;
  bool m_displayEQDateTime;
  bool m_useTypeStyles;
  bool m_wrapText;
};

#endif // _MESSAGEWINDOW_H_

