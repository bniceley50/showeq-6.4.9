/*
 *  xmlpreferences.h
 *  Copyright 2002-2003 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *  Copyright 2002-2005, 2019 by the respective ShowEQ Developers
 *
 *  Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net)
 *  for use under the terms of the GNU General Public License,
 *  incorporated herein by reference.
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

#ifndef __XMLPREFERENCE_H__
#define __XMLPREFERENCE_H__

#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif

#include <QObject>
#include <QString>
#include <QVariant>
#include <QHash>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QSizePolicy>
#include <QCursor>
#include <QStringList>
#include <QKeySequence>

// XMLPreferences is a generic class that implements a persistant config
// file in the XML format specified in seqpref.dtd.
//
//  The following public members are available for use
//    save()             - Saves all modified cache values to file
//    load()             - Loads values from file into cache
//    revert()           - Reloads values from file into cache (looses mods)
//
//  Note:  Both 'Set' members allow a 'persistent' flag to be passed in
//         which is defaulted to TRUE.  Setting this to FALSE makes that 
//         preference not get saved upon a save.  Any later Set with a 
//         TRUE value overrides this.  This is usefull for command line args 
//         which should override config file prefs but not overwrite them  

typedef QHash<QString, QVariant*> PreferenceDict;
typedef QHash<QString, PreferenceDict*> PrefSectionDict;

typedef QHash<QString, QString*> CommentDict;
typedef QHash<QString, CommentDict*> CommentSectionDict;

// PreferenceFile
// A File comprised of NameValuePair Items
// This is an XML text file implementation in the format:
//

class XMLPreferences 
: public QObject
{
  Q_OBJECT

 public:
  // Persistence controls how/whether a preference is persisted and/or
  // which pool of values are retrieved
  enum Persistence
  {
    Runtime  = 0x01, // Runtime settings - not persisted
    User     = 0x02, // User settings - to/from user preference file
    Defaults = 0x04, // Default settings - to/from default preference file
    Any      = Runtime | User | Defaults, // used for get methods to get any pref
    All      = Any // can be used for set methods to set all stores
  };

  XMLPreferences(const QString& defaultsFileName, const QString& inFileName);
  ~XMLPreferences();
  
  // retrieves the comment associated with the preference
  QString getPrefComment(const QString& inName, const QString& inSection);

  // checks if a section exists
  bool isSection(const QString& inSection, Persistence pers = Any);

  // checks if a preference exists
  bool isPreference(const QString& inName, const QString& inSection,
		    Persistence pers = Any);
  
  // getPref{} methods retrieve the current value of a preference
  QString getPrefString(const QString& inName, const QString& inSection, 
			const QString& outDefault = "",
			Persistence pers = Any);
  int getPrefInt(const QString& inName, const QString& inSection, 
		 int def = -1, Persistence pers = Any);
  uint getPrefUInt(const QString& inName, const QString& inSection, 
		   uint def = 0, Persistence pers = Any);
  int64_t getPrefInt64(const QString& inName, const QString& inSection, 
		       int64_t def = -1, Persistence pers = Any);
  uint64_t getPrefUInt64(const QString& inName, const QString& inSection, 
			 uint64_t def = 0, Persistence pers = Any);
  double getPrefDouble(const QString& inName, const QString& inSection, 
		       double def = -0.0, Persistence pers = Any);
  bool getPrefBool(const QString& inName, const QString& inSection, 
		   bool def = false, Persistence pers = Any);
  QColor getPrefColor(const QString& inName, const QString& inSection, 
		      const QColor& def = QColor(), Persistence pers = Any);
  QPen getPrefPen(const QString& inName, const QString& inSection, 
		  const QPen& def = QPen(), Persistence pers = Any);
  QBrush getPrefBrush(const QString& inName, const QString& inSection, 
		      const QBrush& def = QBrush(), Persistence pers = Any);
  QPoint getPrefPoint(const QString& inName, const QString& inSection, 
		      const QPoint& def = QPoint(), Persistence pers = Any);
  QRect getPrefRect(const QString& inName, const QString& inSection, 
		    const QRect& def = QRect(), Persistence pers = Any);
  QSize getPrefSize(const QString& inName, const QString& inSection, 
		    const QSize& def = QSize(), Persistence pers = Any);
  QKeySequence getPrefKey(const QString& inName, const QString& inSection,
		 const QString& def, Persistence pers = Any);
  QKeySequence getPrefKey(const QString& inName, const QString& inSection,
		 const QKeySequence& def, Persistence pers = Any);
  QFont getPrefFont(const QString& inName, const QString& inSection, 
		    const QFont& def = QFont(), Persistence pers = Any);
  QSizePolicy getPrefSizePolicy(const QString& inName, 
				const QString& inSection, 
				const QSizePolicy& def = QSizePolicy(), 
				Persistence pers = Any);
  QCursor getPrefCursor(const QString& inName, const QString& inSection, 
			const QCursor& def = QCursor(), 
			Persistence pers = Any);
  QStringList getPrefStringList(const QString& inName, 
				const QString& inSection, 
				const QStringList& def = QStringList(), 
				Persistence pers = Any);
  QVariant getPrefVariant(const QString& inName, const QString& inSection, 
			  const QVariant& outDefault = QVariant(),
			  Persistence pers = Any);

  // setPref{} methods set the current value of a preference
  void setPrefString(const QString& inName, const QString& inSection, 
		     const QString& inValue = "", 
		     Persistence pers = User);
  void setPrefInt(const QString& inName, const QString& inSection, 
		  int inValue, Persistence pers = User);
  void setPrefUInt(const QString& inName, const QString& inSection, 
		   uint inValue, Persistence pers = User);
  void setPrefInt64(const QString& inName, const QString& inSection, 
		    int64_t inValue, Persistence pers = User);
  void setPrefUInt64(const QString& inName, const QString& inSection, 
		     uint64_t inValue, Persistence pers = User);
  void setPrefDouble(const QString& inName, const QString& inSection, 
		     double inValue, Persistence pers = User);
  void setPrefBool(const QString& inName, const QString& inSection, 
		   bool inValue, Persistence pers = User);
  void setPrefColor(const QString& inName, const QString& inSection, 
		    const QColor& inValue, Persistence pers = User);
  void setPrefPen(const QString& inName, const QString& inSection, 
		    const QPen& inValue, Persistence pers = User);
  void setPrefBrush(const QString& inName, const QString& inSection, 
		    const QBrush& inValue, Persistence pers = User);
  void setPrefPoint(const QString& inName, const QString& inSection, 
		    const QPoint& inValue, Persistence pers = User);
  void setPrefRect(const QString& inName, const QString& inSection, 
		   const QRect& inValue, Persistence pers = User);
  void setPrefSize(const QString& inName, const QString& inSection, 
		   const QSize& inValue, Persistence pers = User);
  void setPrefKey(const QString& inName, const QString& inSection, 
		  int inKey, Persistence pers = User);
  void setPrefFont(const QString& inName, const QString& inSection, 
		   const QFont& inValue, Persistence pers = User);
  void setPrefSizePolicy(const QString& inName, const QString& inSection, 
			 const QSizePolicy& inValue, Persistence pers = User);
  void setPrefCursor(const QString& inName, const QString& inSection, 
		     const QCursor& inValue, Persistence pers = User);
  void setPrefStringList(const QString& inName, const QString& inSection, 
			 const QStringList& inValue, Persistence pers = User);
  void setPrefVariant(const QString& inName, const QString& inSection, 
		      const QVariant& inValue, Persistence pers);
  
 public slots:
  void save(); // Saves all modified values to the appropriate file
  void load(); // Loads values from files into the appropriate caches
  void revert(); // Reloads values from files into the appropriate caches
 
 protected:
  // loads a preference dictionary from the specified filename
  void loadPreferences(const QString& filename, PrefSectionDict& dict);
  
  // saves a preference dictionary to the specified filename
  void savePreferences(const QString& filename, PrefSectionDict& dict);

  // convenience routines for getting/setting preferences
  QVariant* getPref(const QString& inName, const QString& inSection,
		    Persistence pers = Any);
  void setPref(const QString& inName, const QString& inSection, 
	       const QVariant& inValue, Persistence pers);
  void setPref(PrefSectionDict& dict,
	       const QString& inName, const QString& inSection, 
	       const QVariant& inValue);

 private:

 QString m_defaultsFilename;
 QString m_filename;
 uint8_t m_modified;
 
 // stores the non-persistant user preferences
 PrefSectionDict m_runtimeSections;
 
 // stores the user preferences
 PrefSectionDict m_userSections;

 // stores the system defaults
 PrefSectionDict m_defaultsSections;

 // stores the property comment information
 CommentSectionDict m_commentSections;

 // template document used to create saves to previously non-existant files
 QString m_templateDoc;
};

#endif // __XMLPREFERENCE_H__

