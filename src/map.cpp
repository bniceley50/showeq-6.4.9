/*
 *  map.cpp
 *  Portions Copyright 2001-2007 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2001-2009, 2012, 2019 by the respective ShowEQ Developers
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

#define __STDC_LIMIT_MACROS
#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif


#include "map.h"
#include "mapicondialog.h"
#include "util.h"
#include "main.h"
#include "filtermgr.h"
#include "zonemgr.h"
#include "spawnmonitor.h"
#include "player.h"
#include "spawnshell.h"
#include "datalocationmgr.h"
#include "diagnosticmessages.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QFileDialog>
#include <QEvent>
#include <QPushButton>
#include <QToolButton>
#include <QLayout>
#include <QShortcut>
#include <QColorDialog>
#include <QFontDialog>
#include <QTimer>
#include <QStringList>
#include <QImage>
#include <QImageWriter>
#include <QMenu>
#include <QWidgetAction>
#include <QGridLayout>
#include <QCommonStyle>

#if 1 // ZBTEMP: Until we setup a better way to enter location name/color
#include <QInputDialog>
#endif

#include <QBoxLayout>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QPolygon>
#include <QFrame>
#include <QResizeEvent>
#include <QLabel>
#include <QHBoxLayout>
#include <QTextStream>
#include <QMouseEvent>

#pragma message("Once our minimum supported Qt version is greater than 5.14, this check can be removed and ENDL replaced with Qt::endl")
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
#define ENDL Qt::endl
#else
#define ENDL endl
#endif

//----------------------------------------------------------------------
// Macros
//#define DEBUG

//#define DEBUGMAP

//#define DEBUGMAPLOAD


//----------------------------------------------------------------------
// constants
const int panAmmt = 8;

//----------------------------------------------------------------------
// CLineDlg
CLineDlg::CLineDlg(QWidget *parent, QString name, MapMgr *mapMgr) 
  : QDialog(parent)
{
  setObjectName(name);
#ifdef DEBUGMAP
  qDebug ("CLineDlg()");
#endif /* DEBUGMAP */

  QFont labelFont;
  labelFont.setBold(true);

  QBoxLayout *topLayout = new QVBoxLayout(this);
  QBoxLayout *row2Layout = new QHBoxLayout();
  QBoxLayout *row1Layout = new QHBoxLayout();
  topLayout->addLayout(row2Layout);
  topLayout->addLayout(row1Layout);

  QLabel *colorLabel = new QLabel ("Color", this);
  colorLabel->setFont(labelFont);
  colorLabel->setFixedHeight(colorLabel->sizeHint().height());
  colorLabel->setFixedWidth(colorLabel->sizeHint().width()+10);
  colorLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  row1Layout->addWidget(colorLabel);

  m_LineColor = new QComboBox(this);
  m_LineColor->setEditable(false);
  m_LineColor->setObjectName("LineColor");
  m_LineColor->insertItem(m_LineColor->count(), "gray");
  m_LineColor->insertItem(m_LineColor->count(), "darkBlue");
  m_LineColor->insertItem(m_LineColor->count(), "darkGreen");
  m_LineColor->insertItem(m_LineColor->count(), "darkCyan");
  m_LineColor->insertItem(m_LineColor->count(), "darkRed");
  m_LineColor->insertItem(m_LineColor->count(), "darkMagenta");
  m_LineColor->insertItem(m_LineColor->count(), "darkGray");
  m_LineColor->insertItem(m_LineColor->count(), "white");
  m_LineColor->insertItem(m_LineColor->count(), "blue");
  m_LineColor->insertItem(m_LineColor->count(), "green");
  m_LineColor->insertItem(m_LineColor->count(), "cyan");
  m_LineColor->insertItem(m_LineColor->count(), "red");
  m_LineColor->insertItem(m_LineColor->count(), "magenta");
  m_LineColor->insertItem(m_LineColor->count(), "yellow");
  m_LineColor->insertItem(m_LineColor->count(), "white");

  m_LineColor->setFont(labelFont);
  m_LineColor->setFixedHeight(m_LineColor->sizeHint().height());
  m_LineColor->setFixedWidth(m_LineColor->sizeHint().width());
  row1Layout->addWidget(m_LineColor, 0, Qt::AlignLeft);

  m_ColorPreview = new QFrame(this);
  m_ColorPreview->setFrameStyle(QFrame::Box|QFrame::Raised);
  m_ColorPreview->setFixedWidth(50);
  m_ColorPreview->setFixedHeight(m_LineColor->sizeHint().height());
  m_ColorPreview->setPalette(QPalette(QColor(Qt::gray)));
  row1Layout->addWidget(m_ColorPreview);

  // Hook on when a color changes
  connect(m_LineColor, SIGNAL(activated(const QString &)), mapMgr, SLOT(setLineColor(const QString &)));
  connect(m_LineColor, SIGNAL(activated(const QString &)), SLOT(changeColor(const QString &)));

  QLabel *nameLabel = new QLabel ("Name", this);
  nameLabel->setFont(labelFont);
  nameLabel->setFixedHeight(nameLabel->sizeHint().height());
  nameLabel->setFixedWidth(nameLabel->sizeHint().width()+5);
  nameLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  row2Layout->addWidget(nameLabel);

  m_LineName  = new QLineEdit(this);
  m_LineName->setObjectName("LineName");
  m_LineName->setFont(labelFont);
  m_LineName->setFixedHeight(m_LineName->sizeHint().height());
  m_LineName->setFixedWidth(150);
  row2Layout->addWidget(m_LineName);

  // Hook on when the line name changes
  connect(m_LineName, SIGNAL(textChanged(const QString &)), mapMgr, SLOT(setLineName(const QString &)));

  QPushButton *ok = new QPushButton("OK", this);
  ok->setFixedWidth(30);
  ok->setFixedHeight(30);
  topLayout->addWidget(ok, 0, Qt::AlignCenter);

  // Hook on pressing the OK button
  connect(ok, SIGNAL(clicked()), SLOT(accept()));

  for (int i=0; i < m_LineColor->count(); i++)
  {
    if (m_LineColor->itemText(i) == mapMgr->curLineColor())
      m_LineColor->setCurrentIndex(i);
  }

  m_LineName->setText(mapMgr->curLineName());
}

void CLineDlg::changeColor(const QString &color)
{
#ifdef DEBUGMAP
  qDebug ("changeColor()");
#endif /* DEBUGMAP */
  m_ColorPreview->setPalette(QPalette(QColor(color)));
}

//----------------------------------------------------------------------
// MapLabel
MapLabel::MapLabel( Map* map )
  : QLabel(map,
       Qt::FramelessWindowHint | Qt::Tool | Qt::Window
       | Qt::Dialog | Qt::X11BypassWindowManagerHint)
{
  this->setObjectName("mapLabel");
  m_Map = map;
  setMargin( 1 );
  setIndent( 0 );
  setFrameStyle( QFrame::Plain | QFrame::Box );
  setLineWidth( 1 );
  setAlignment( Qt::AlignLeft | Qt::AlignTop );
  ensurePolished();
  setText( "" );
  adjustSize();
}

void MapLabel::popup(const QPoint& pos)
{
  // make sure the widgets size is current.
  adjustSize();

  // borrowed from QPopupMenu::popup()

  // get info about the widget and its environment
  QWidget *desktop = (QWidget*)QApplication::desktop();
  int sw = desktop->width();                  // screen width
  int sh = desktop->height();                 // screen height
  int sx = desktop->x();                      // screen pos
  int sy = desktop->y();
  int x  = pos.x();
  int y  = pos.y();
  int w  = width();
  int h  = height();

  // the complete widget must be visible, move it if necessary
  if ( x+w > sw )
    x = sw - w;
  if ( y+h > sh )
    y = sh - h;
  if ( x < sx )
    x = sx;
  if ( y < sy )
    y = sy;
  move( x, y );

  // show the widget
  show();
}

void MapLabel::mousePressEvent(QMouseEvent*)
{
  // hide if the user clicks on the label
  hide();
}

//----------------------------------------------------------------------
// MapMgr
MapMgr::MapMgr(const DataLocationMgr* dataLocMgr, 
           SpawnShell* spawnShell, Player* player, ZoneMgr* zoneMgr,
           QWidget* dialogParent, QObject* parent, const char* name)
  : QObject(parent),
    m_dataLocMgr(dataLocMgr),
    m_spawnShell(spawnShell),
    m_player(player),
    m_dialogParent(dialogParent)
{
  setObjectName(name);
  m_dlgLineProps = NULL;
  
  // get the preferences
  m_curLineColor = pSEQPrefs->getPrefString("DefaultLineColor", "MapMgr", "gray");
  m_curLineName = pSEQPrefs->getPrefString("DefaultLineName", "MapMgr", "line");
  m_curLocationColor = pSEQPrefs->getPrefString("DefaultLocationColor", "MapMgr", 
                        "white");

  // supply the MapMgr slots with signals from SpawnShell
  connect (m_spawnShell, SIGNAL(addItem(const Item*)),
       this, SLOT(addItem(const Item*)));
  connect (m_spawnShell, SIGNAL(delItem(const Item*)),
       this, SLOT(delItem(const Item*)));
  connect (m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
       this, SLOT(killSpawn(const Item*)));
  connect (m_spawnShell, SIGNAL(changeItem(const Item*, uint32_t)),
       this, SLOT(changeItem(const Item*, uint32_t)));
  connect(m_spawnShell, SIGNAL(clearItems()),
       this, SLOT(clearItems()));

  // supply the MapMgr slots with signals from ZoneMgr
  connect(zoneMgr, SIGNAL(zoneBegin(const QString&)),
      this, SLOT(zoneBegin(const QString&)));
  connect(zoneMgr, SIGNAL(zoneChanged(const QString&)),
      this, SLOT(zoneChanged(const QString&)));
  connect(zoneMgr, SIGNAL(zoneEnd(const QString&, const QString&)),
      this, SLOT(zoneEnd(const QString&, const QString&)));

  // if there is a short zone name already, try to load its map
  QString shortZoneName = zoneMgr->shortZoneName();
  if (!shortZoneName.isEmpty())
    loadZoneMap(shortZoneName);
}

MapMgr::~MapMgr()
{
}

uint16_t MapMgr::spawnAggroRange(const Spawn* spawn)
{
  uint16_t range = m_spawnAggroRange.value(spawn->id(), 0);
  return range;
}

void MapMgr::zoneBegin(const QString& shortZoneName)
{
#ifdef DEBUGMAP
  qDebug ("zoneBegin(%s)", 
     shortZoneName);
#endif /* DEBUGMAP */
  
  // clear the map data
  m_mapData.clear();
  
  // signal that the map has been unloaded
  emit mapUnloaded();
  
  // atttempt to load the new map
  loadZoneMap(shortZoneName);
}

void MapMgr::zoneChanged(const QString& shortZoneName)
{
#ifdef DEBUGMAP
  qDebug ("zoneChanged(%s)", 
     (const char*)shortZoneName);
#endif /* DEBUGMAP */

  // clear the map data
  m_mapData.clear();
  
  // signal that the map has been unloaded
  emit mapUnloaded();

  // atttempt to load the new map
  loadZoneMap(shortZoneName);
}

void MapMgr::zoneEnd(const QString& shortZoneName, const QString& longZoneName)
{
#ifdef DEBUGMAP
  qDebug ("zoneEnd(%s, %s)",
     longZoneName.toLatin1().data(), shortZoneName.toLatin1().data());
#endif /* DEBUGMAP */

  // atttempt to load the new map
  loadZoneMap(shortZoneName);
}

void MapMgr::loadZoneMap(const QString& shortZoneName)
{

  bool found = false;
  QString extension = "";

  QStringList mapFiles;

  // find maps
  QFileInfo mapFileInfo = m_dataLocMgr->findExistingFile("maps",
          shortZoneName + ".map");

  QFileInfo txtFileInfo = m_dataLocMgr->findExistingFile("maps",
          shortZoneName + ".txt");

  if (mapFileInfo.exists())
  {
      found = true;
      extension = ".map";
      mapFiles.append(mapFileInfo.absoluteFilePath());

  } else if (txtFileInfo.exists())
  {
      found = true;
      extension = ".txt";
      mapFiles.append(txtFileInfo.absoluteFilePath());
  }

  if (!found)
  {
    seqInfo("No Map found for zone '%s'!", shortZoneName.toLatin1().data());
    seqInfo("    Checked for all variants of '%s.map', '%s.txt', and '%s_1.txt'",
        shortZoneName.toLatin1().data(),
        shortZoneName.toLatin1().data(),
        shortZoneName.toLatin1().data());
    seqInfo("    in directories '%s' and '%s'!",
        m_dataLocMgr->userDataDir("maps").absolutePath().toLatin1().data(),
        m_dataLocMgr->pkgDataDir("maps").absolutePath().toLatin1().data());

    return;
  }

  // add other layers
  QFileInfo fileInfo;
  for (int i = 1; i < 10; ++i)
  {
      fileInfo = m_dataLocMgr->findExistingFile("maps",
              shortZoneName + "_" + QString::number(i) + extension);

      if (fileInfo.exists())
          mapFiles.append(fileInfo.absoluteFilePath());
  }

  loadFileMap(mapFiles);
}

void MapMgr::loadMap ()
{
#ifdef DEBUGMAP
  qDebug ("loadMap()");
#endif /* DEBUGMAP */

  QStringList fileNames;
  for (int i = 0; i < m_mapData.numLayers(); ++i)
  {
      fileNames.append(m_mapData.mapLayer(i)->fileName());
  }

  if (fileNames.isEmpty())
    fileNames.append(m_dataLocMgr->findExistingFile("maps", "").absoluteFilePath());

  // create a file dialog the defaults to the currently open map
  // it doesn't look like we can start with all the loaded files selected, 
  // so we pick the first one, which should be the base map
  fileNames = QFileDialog::getOpenFileNames(m_dialogParent, "Load Map", fileNames.first(),
          "All maps (*.map *.txt);;SEQ maps (*.map);;EQ maps (*.txt)");

  if (fileNames.isEmpty ())
    return;

  fileNames.sort();

  // load the map
  loadFileMap(fileNames, false, true);
}

void MapMgr::importMap ()
{
#ifdef DEBUGMAP
  qDebug ("importMap()");
#endif /* DEBUGMAP */

  QStringList fileNames;
  for (int i = 0; i < m_mapData.numLayers(); ++i)
  {
      fileNames.append(m_mapData.mapLayer(i)->fileName());
  }

  if (fileNames.isEmpty())
    fileNames.append(m_dataLocMgr->findExistingFile("maps", "").absoluteFilePath());

  // create a file dialog the defaults to the currently open map
  fileNames = QFileDialog::getOpenFileNames(m_dialogParent, "Import Map", fileNames.first(),
          "All maps (*.map *.txt);;SEQ maps (*.map);;EQ maps (*.txt)");

  if (fileNames.isEmpty ())
    return;

  fileNames.sort();

  // load the map
  loadFileMap(fileNames, true, true);
}


void MapMgr::loadFileMap (const QString& fileName, bool import, bool force) 
{
#ifdef DEBUGMAP
  qDebug ("loadFileMap()");
#endif /* DEBUGMAP */

  if (import)
    seqInfo("Attempting to import map: %s", fileName.toLatin1().data());
  else
    seqInfo("Attempting to load map: %s", fileName.toLatin1().data());


  // if not a forced load, and the same map is already loaded, do nothing
  if (!force)
  {
      for (int i = 0; i < m_mapData.numLayers(); ++i)
      {
          if (m_mapData.mapLayer(i)->mapLoaded() &&
                  m_mapData.mapLayer(i)->fileName() == fileName)
              return;
      }
  }


  // load the specified map
  if (!fileName.endsWith(".txt"))
    m_mapData.loadMap(fileName, import);
  else
    m_mapData.loadSOEMap(fileName, import);

  const ItemMap& itemMap = m_spawnShell->spawns();
  ItemConstIterator it(itemMap);
  const Item* item;
  uint16_t range;

  // iterate over the exixsting spawns to adjust the map size and find 
  // ones with aggro information
  while (it.hasNext())
  {
    it.next();
    // get the item from the list
    item = it.value();
    if (!item)
        break;

    // Adjust X and Y for spawns on map
    m_mapData.quickCheckPos(item->x(), item->y());

    if (m_mapData.isAggro(item->transformedName(), &range))
    {
      // create a range to insert into the dictionary

      // insert the spawns ID and aggro range into the dictionary.
      m_spawnAggroRange.insert(item->id(), range);
    }
  }

  // update the bounds
  m_mapData.updateBounds();

  // signal that the map has been loaded
  // note, the layers are populated in order, so the highest layer
  // number (0-indexed) will be the one we just loaded
  if (m_mapData.mapLayer(m_mapData.numLayers()-1)->mapLoaded())
    emit mapLoaded();
} // END loadFileMap


void MapMgr::loadFileMap (const QStringList& files, bool import, bool force)
{
    QStringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it)
    {
        // if we're loading multiple files, it doesn't sense to load and
        // immediately clear/replace each one.  So if import isn't specified,
        // we clear the current maps, load the first one, then import the
        // rest as layers
        if (!import && it == files.begin())
            loadFileMap(*it, false, force);
        else
            loadFileMap(*it, true, force);
    }
}


void MapMgr::saveMap ()
{
#ifdef DEBUGMAP
  qDebug ("saveMap()");
#endif /* DEBUGMAP */

  for (int i = 0; i < m_mapData.numLayers(); ++i)
  {
      QFileInfo fileInfo(m_mapData.mapLayer(i)->fileName());

      fileInfo = m_dataLocMgr->findWriteFile("maps", fileInfo.baseName() + ".map", 
                     false);

     m_mapData.saveMap(fileInfo.absoluteFilePath(), i);
  }
}

void MapMgr::saveSOEMap ()
{
#ifdef DEBUGMAP
  qDebug ("saveMap()");
#endif /* DEBUGMAP */

  for (int i = 0; i < m_mapData.numLayers(); ++i)
  {
      QFileInfo fileInfo(m_mapData.mapLayer(i)->fileName());

      fileInfo = m_dataLocMgr->findWriteFile("maps", fileInfo.baseName() + ".txt", 
                     false);

      m_mapData.saveSOEMap(fileInfo.absoluteFilePath(), i);
  }
}

void MapMgr::createNewLayer()
{
  m_mapData.createNewLayer();
  // signal that the map has been loaded
  // note, the layers are populated in order, so the highest layer
  // number (0-indexed) will be the one we just loaded
  if (m_mapData.mapLayer(m_mapData.numLayers()-1)->mapLoaded())
    emit mapLoaded();
}

void MapMgr::addItem(const Item* item)
{
  if ((item == NULL) || (item->type() != tSpawn))
    return;

  // make sure it fits on the map display
  m_mapData.checkPos(item->x(), item->y());

  uint16_t range;
  if (m_mapData.isAggro(item->transformedName(), &range))
  {
    // insert the spawns ID and aggro range into the dictionary.
    m_spawnAggroRange.insert(item->id(), range);
  }

  // signal that the map has changed
  emit mapUpdated();
}

void MapMgr::delItem(const Item* item)
{
  if (item == NULL)
    return;

  // remove from the spawn aggro dictionary
  if (item->type() == tSpawn)
    m_spawnAggroRange.remove(item->id());

  // signal that the map has changed
  emit mapUpdated();
}

void MapMgr::killSpawn(const Item* item)
{
  if ((item == NULL) || (item->type() != tSpawn))
    return;

  // make sure it fits on the map display
  m_mapData.checkPos(item->x(), item->y());

  // based on the principle of the dead can't aggro, let's remove it from
  // the spawn aggro dictionary (the undead are another matter... ;-)
  m_spawnAggroRange.remove(item->id());

  // signal that the map has changed
  emit mapUpdated();
}

void MapMgr::changeItem(const Item* item, uint32_t changeType)
{
  if (item == NULL)
    return;

  // only need to deal with position changes
  if (changeType & tSpawnChangedPosition)
  {
    // make sure it fits on the map display
    if ( m_mapData.checkPos(item->x(), item->y()))
      emit mapUpdated(); // signal if the map size has changed
  }
}

void MapMgr::clearItems()
{
  // clear the spawn aggro range info
  m_spawnAggroRange.clear();
}

void MapMgr::addLocation(QWidget* parent, const MapPoint& point)
{
  // ZBTEMP: Should create a real dialog to enter location info
  bool ok;
  QString name = QInputDialog::getText(parent, "Location Name",
                       "Please enter a location name",
                       QLineEdit::Normal,
                       "", &ok);

  // if the user clicked ok, and actually gave a name, add it
  if (ok && !name.isEmpty())
  {
      //m_curLocationColor doesn't get set anywhere, so for now, we'll
      //use whatever the current line color is.  Maybe we'll add selecting
      //a separate location color later. TODO
    m_mapData.addLocation(name, m_curLineColor, QPoint(point.x(), point.y()));
  }

  emit mapUpdated();

#ifdef DEBUGMAP
  seqDebug("addLocation(): Location x added at %d/%d", 
     point.x(), point.y()); 
#endif
}

void MapMgr::startLine(const MapPoint& point)
{
#ifdef DEBUGMAP
  qDebug ("startLine()");
#endif /* DEBUGMAP */
  // start the line
  m_mapData.startLine(m_curLineName, m_curLineColor, point);

  // signal that the map has been updated
  emit mapUpdated();
}

void MapMgr::addLinePoint(const MapPoint& point) 
{
#ifdef DEBUGMAP
  qDebug ("addLinePoint()");
#endif /* DEBUGMAP */
  // add a line point
  m_mapData.addLinePoint(point);

  // signal that the map has been updated
  emit mapUpdated();
}

void MapMgr::delLinePoint(void)
{
#ifdef DEBUGMAP
  qDebug ("delLinePoint()");
#endif /* DEBUGMAP */
  m_mapData.delLinePoint();

  // signal that the map has been updated
  emit mapUpdated();
} // END delLinePoint

void MapMgr::setLineName(const QString &name)
{
  // set the name of the current line
  m_mapData.setLineName(name);

  // save the name for future use
  m_curLineName = name;

  pSEQPrefs->setPrefString("DefaultLineName", "MapMgr", m_curLineName);

  // signal that the map has been updated
  emit mapUpdated();
}

void MapMgr::setLineColor(const QString &color)
{
  // set the color of the current line
  m_mapData.setLineColor(color);

  // save the line color for future use
  m_curLineColor = color;

  pSEQPrefs->setPrefString("DefaultLineColor", "MapMgr", m_curLineColor);

  // signal that the map has been updated
  emit mapUpdated();
}

void MapMgr::showLineDlg(QWidget* parent)
{
   // Creat the line properties dialog the first time it is needed
   if (m_dlgLineProps == NULL)
      m_dlgLineProps = new CLineDlg(parent, "LineDlg", this);

   m_dlgLineProps->show();
}

void MapMgr::scaleDownZ(int16_t factor)
{
  m_mapData.scaleDownZ(factor);

  // signal that the map has been updated
  emit mapUpdated();
}

void MapMgr::scaleUpZ(int16_t factor)
{
  m_mapData.scaleUpZ(factor);

  // signal that the map has been updated
  emit mapUpdated();
}

void MapMgr::setEditLayer(int layerNum)
{
    if (layerNum >= m_mapData.numLayers())
        return;

    m_mapData.setEditLayer(layerNum);

    emit editLayerChanged();
}

void MapMgr::savePrefs(void)
{
#if 0 // ZBTEMP: Migrate to place where ever this is set
  pSEQPrefs->setPrefString("DefaultLocationColor", "MapMgr", m_curLocationColor);
#endif
}

void MapMgr::dumpInfo(QTextStream& out)
{
  out << "[MapMgr]" << ENDL;
  out << "DefaultLineColor: " << m_curLineColor << ENDL;
  out << "DefaultLineName: " << m_curLineName << ENDL;
  out << "DefaultLocationColor: " << m_curLocationColor << ENDL;
  out << "ImageLoaded: " << m_mapData.imageLoaded() << ENDL;
  out << "ZoneShortName: " << m_mapData.zoneShortName() << ENDL;
  out << "ZoneLongName: " << m_mapData.zoneLongName() << ENDL;
  out << "boundingRect: top(" << m_mapData.boundingRect().top() 
      << ") bottom(" << m_mapData.boundingRect().bottom() 
      << ") left(" << m_mapData.boundingRect().left()
      << ") right(" << m_mapData.boundingRect().right() << ") " << ENDL;
  out << "size: width(" << m_mapData.size().width()
      << ") height(" << m_mapData.size().height() << ")" << ENDL;
  out << "ZoneZEM: " << m_mapData.zoneZEM() << ENDL;
  out << "numLayers: " << m_mapData.numLayers() << ENDL;
  for (int i = 0; i < m_mapData.numLayers(); ++i)
  {
      out << "Layer " << i << ":" << ENDL;
      out << "\tMapFileName: " << m_mapData.mapLayer(i)->fileName() << ENDL;
      out << "\tMapLoaded: " << m_mapData.mapLayer(i)->mapLoaded() << ENDL;
      out << "\tLLines: " << m_mapData.mapLayer(i)->lLines().count() << ENDL;
      out << "\tMLines: " << m_mapData.mapLayer(i)->mLines().count() << ENDL;
      out << "\tLocations: " << m_mapData.mapLayer(i)->locations().count() << ENDL;
  }
  out << "Aggros: " << m_mapData.aggros().count() << ENDL;
  out << ENDL;
}

//----------------------------------------------------------------------
// MapMenu
MapMenu::MapMenu(Map* map, QWidget* parent, const char* name)
  : QMenu(parent),
    m_map(map),
    m_mapIcons(map->mapIcons())
{
  this->setObjectName(name);
  QString preferenceName = m_map->preferenceName();

  // set the caption to be the preference name of the map
  setWindowTitle(preferenceName);

  QMenu* subMenu;
  QMenu* subSubMenu;

  subMenu = new QMenu("Follow");
  m_action_followMenu_Player = subMenu->addAction("Player");
  m_action_followMenu_Player->setCheckable(true);
  m_action_followMenu_Player->setData(tFollowPlayer);

  m_action_followMenu_Spawn = subMenu->addAction("Spawn");
  m_action_followMenu_Spawn->setCheckable(true);
  m_action_followMenu_Spawn->setData(tFollowSpawn);

  m_action_followMenu_None = subMenu->addAction("None");
  m_action_followMenu_None->setCheckable(true);
  m_action_followMenu_None->setData(tFollowNone);

  connect(subMenu, SIGNAL(triggered(QAction*)), this,
          SLOT(select_follow(QAction*)));
  addMenu(subMenu);

  subMenu = new QMenu("Edit", m_map);
  QKeySequence key;
  QShortcut *tmpShortcut = nullptr;

  /* Since the map menus also get inserted into Window menu, simply adding
   * a shortcut to the QAction gives "ambiguous shortcut" errors and causes
   * the shortcuts not to work due to both menus trying to handle the shortcut.
   *
   * So as a hacky work-around, we don't define the shortcut on the QAction,
   * but we do change the QAction's text to also show the shortcut, since
   * otherwise it's not shown.  Then we define the shortcut and attach it to
   * the map itself.
   *
   * This allows the map menu keyboard shortcuts to work if there is only one
   * map window open.
   *
   * If there are multiple maps open, then the menu entries will continue to
   * work, but the keyboard shortcuts will not, since Qt doesn't know which
   * window should handle the action.  It's probably possible to deal with this
   * by detecting which map has focus and giving it preference, but this
   * behavior doesn't appear to be the default.  FIXME
   *
   * - cn187
   */

  m_action_createNewLayer = subMenu->addAction(
          QString("Create New Layer\t"), m_map, SLOT(createNewLayer()));

  QMenu* layerMenu = new QMenu("Select Edit Layer");
  m_editLayerSpinBox = new QSpinBox(layerMenu);
  QWidgetAction* editLayerAction = new QWidgetAction(layerMenu);
  editLayerAction->setDefaultWidget(m_editLayerSpinBox);
  layerMenu->addAction(editLayerAction);
  subMenu->addMenu(layerMenu);

  m_editLayerSpinBox->setMinimum(0);
  m_editLayerSpinBox->setSingleStep(1);
  m_editLayerSpinBox->setMaximum(m_map->mapMgr()->mapData().numLayers()-1);
  m_editLayerSpinBox->setValue(m_map->mapMgr()->mapData().editLayer());

  connect(m_editLayerSpinBox, SIGNAL(valueChanged(int)),
          m_map->mapMgr(), SLOT(setEditLayer(int)));

  connect(m_map->mapMgr(), SIGNAL(mapLoaded(void)), this, SLOT(editLayerChanged(void)));
  connect(m_map->mapMgr(), SIGNAL(editLayerChanged(void)), this, SLOT(editLayerChanged(void)));

  key = pSEQPrefs->getPrefKey("AddLocationKey", preferenceName, "Ctrl+O");
  m_action_addLocation = subMenu->addAction(
          QString("Add Location...\t") + key.toString(), m_map, SLOT(addLocation()));
  tmpShortcut = new QShortcut(key, m_map, SLOT(addLocation()));

  key = pSEQPrefs->getPrefKey("StartLineKey", preferenceName, "Ctrl+L");
  m_action_startLine = subMenu->addAction(
          QString("Start Line\t") + key.toString(), m_map, SLOT(startLine()));
  tmpShortcut = new QShortcut(key, m_map, SLOT(startLine()));

  key = pSEQPrefs->getPrefKey("AddLinePointKey", preferenceName, "Ctrl+P");
  m_action_addLinePoint = subMenu->addAction(
          QString("Add Line Point\t") + key.toString(), m_map, SLOT(addLinePoint()));
  tmpShortcut = new QShortcut(key, m_map, SLOT(addLinePoint()));

  key = pSEQPrefs->getPrefKey("DelLinePointKey", preferenceName, "Ctrl+D");
  m_action_delLinePoint = subMenu->addAction(
          QString("Delete Line Point\t") + key.toString(), m_map, SLOT(delLinePoint()));
  tmpShortcut = new QShortcut(key, m_map, SLOT(delLinePoint()));

  m_action_showLineDlg = subMenu->addAction("Show Line Dialog...", m_map,
          SLOT(showLineDlg()));

  subSubMenu = new QMenu("Scale Map Z Coordinates", m_map);
  QAction* tmpAction;

  QActionGroup* zDownGroup= new QActionGroup(subSubMenu);

  tmpAction = subSubMenu->addAction("Down 8");
  tmpAction->setData(8);
  zDownGroup->addAction(tmpAction);

  tmpAction = subSubMenu->addAction("Down 10");
  tmpAction->setData(10);
  zDownGroup->addAction(tmpAction);

  connect(zDownGroup, SIGNAL(triggered(QAction*)), m_map, SLOT(scaleDownZ(QAction*)));

  QActionGroup* zUpGroup= new QActionGroup(subSubMenu);

  tmpAction = subSubMenu->addAction("Up 8");
  tmpAction->setData(8);
  zUpGroup->addAction(tmpAction);

  tmpAction = subSubMenu->addAction("Up 10");
  tmpAction->setData(10);
  zUpGroup->addAction(tmpAction);

  connect(zUpGroup, SIGNAL(triggered(QAction*)), m_map, SLOT(scaleUpZ(QAction*)));

  subMenu->addMenu(subSubMenu);
  addMenu(subMenu);

  subMenu = new QMenu("Map Line Display", m_map);

  /* NOTE: see the comments above for the Edit menu for an explanation of why
   * the shortcuts are like this */

  key = pSEQPrefs->getPrefKey("MapLineNormalKey", preferenceName, "Alt+1");
  m_action_mapLineStyle_Normal = subMenu->addAction(QString("Normal\t") + key.toString());
  m_action_mapLineStyle_Normal->setCheckable(true);
  m_action_mapLineStyle_Normal->setData(tMap_Normal);
  tmpShortcut = new QShortcut(key, m_map);
  connect(tmpShortcut, SIGNAL(activated()), m_action_mapLineStyle_Normal, SLOT(trigger()));

  key = pSEQPrefs->getPrefKey("MapLineDepthFilteredKey", preferenceName, "Alt+2");
  m_action_mapLineStyle_DepthFiltered = subMenu->addAction(
          QString("Depth Filtered\t") + key.toString());
  m_action_mapLineStyle_DepthFiltered->setCheckable(true);
  m_action_mapLineStyle_DepthFiltered->setData(tMap_DepthFiltered);
  tmpShortcut = new QShortcut(key, m_map);
  connect(tmpShortcut, SIGNAL(activated()), m_action_mapLineStyle_DepthFiltered, SLOT(trigger()));

  key = pSEQPrefs->getPrefKey("MapLineFadedFloorsKey", preferenceName, "Alt+3");
  m_action_mapLineStyle_FadedFloors = subMenu->addAction(
          QString("Faded Floors\t") + key.toString());
  m_action_mapLineStyle_FadedFloors->setCheckable(true);
  m_action_mapLineStyle_FadedFloors->setData(tMap_FadedFloors);
  tmpShortcut = new QShortcut(key, m_map);
  connect(tmpShortcut, SIGNAL(activated()), m_action_mapLineStyle_FadedFloors, SLOT(trigger()));

  connect(subMenu, SIGNAL(triggered(QAction*)), this,
          SLOT(select_mapLine(QAction*)));
  addMenu(subMenu);

  key = pSEQPrefs->getPrefKey("SpawnDepthFilteredKey", preferenceName, "Alt+5");
  m_action_spawnDepthFilter = addAction(QString("Spawn Depth Filter\t") + key.toString(),
          this, SLOT(toggle_spawnDepthFilter()));
  m_action_spawnDepthFilter->setCheckable(true);
  tmpShortcut = new QShortcut(key, m_map);
  connect(tmpShortcut, SIGNAL(activated()), m_action_spawnDepthFilter, SLOT(trigger()));

  subMenu = new QMenu("Show", m_map);

  m_action_tooltip = subMenu->addAction("Tooltips", this, SLOT(toggle_tooltip()));;
  m_action_tooltip->setCheckable(true);

  m_action_filtered = subMenu->addAction("Filtered", this, SLOT(toggle_filtered()));
  m_action_filtered->setCheckable(true);

  m_action_map = subMenu->addAction("Map Lines", this, SLOT(toggle_map()));
  m_action_map->setCheckable(true);

  m_action_velocity = subMenu->addAction("Velocity Lines", this,
          SLOT(toggle_velocity()));
  m_action_velocity->setCheckable(true);

  m_action_animate = subMenu->addAction("Animate Spawns", this,
          SLOT(toggle_animate()));
  m_action_animate->setCheckable(true);

  m_action_player = subMenu->addAction("Player", this, SLOT(toggle_player()));
  m_action_player->setCheckable(true);

  m_action_playerBackground = subMenu->addAction("Player Background", this,
          SLOT(toggle_playerBackground()));
  m_action_playerBackground->setCheckable(true);

  m_action_playerView = subMenu->addAction("Player View", this, SLOT(toggle_playerView()));
  m_action_playerView->setCheckable(true);

  m_action_gridLines = subMenu->addAction("Grid Lines", this, SLOT(toggle_gridLines()));;
  m_action_gridLines->setCheckable(true);

  m_action_gridTicks = subMenu->addAction("Grid Ticks", this, SLOT(toggle_gridTicks()));
  m_action_gridTicks->setCheckable(true);

  m_action_locations = subMenu->addAction("Locations", this, SLOT(toggle_locations()));
  m_action_locations->setCheckable(true);

  m_action_spawns = subMenu->addAction("Spawns", this, SLOT(toggle_spawns()));
  m_action_spawns->setCheckable(true);

  m_action_unknownSpawns = subMenu->addAction("Unknown Spawns", this,
          SLOT(toggle_unknownSpawns()));
  m_action_unknownSpawns->setCheckable(true);

  m_action_spawnPoints = subMenu->addAction("Spawn Points", this,
          SLOT(toggle_spawnPnts()));
  m_action_spawnPoints->setCheckable(true);

  m_action_drops = subMenu->addAction("Drops", this, SLOT(toggle_drops()));
  m_action_drops->setCheckable(true);

  m_action_doors = subMenu->addAction("Doors", this, SLOT(toggle_doors()));
  m_action_doors->setCheckable(true);

  m_action_spawnNames = subMenu->addAction("Spawn Names", this,
          SLOT(toggle_spawnNames()));
  m_action_spawnNames->setCheckable(true);

  m_action_highlightConsideredSpawns = subMenu->addAction("Highlight Considered Spawns",
          this, SLOT(toggle_highlightConsideredSpawns()));
  m_action_highlightConsideredSpawns->setCheckable(true);

  m_action_walkPath = subMenu->addAction("Selections Walk Path", this,
          SLOT(toggle_walkPath()));
  m_action_walkPath->setCheckable(true);

  m_action_npcWalkPaths = subMenu->addAction("NPC Walk Paths", this,
          SLOT(toggle_npcWalkPaths()));
  m_action_npcWalkPaths->setCheckable(true);

  m_action_mapImage = subMenu->addAction("Map Image", this,
          SLOT(toggle_mapImage()));
  m_action_mapImage->setCheckable(true);

  m_action_pvp = subMenu->addAction("PvP", this, SLOT(toggle_pvp()));
  m_action_pvp->setCheckable(true);

  m_action_deityPvP = subMenu->addAction("Deity PvP", this,
          SLOT(toggle_deityPvP()));
  m_action_deityPvP->setCheckable(true);

  m_action_racePvP = subMenu->addAction("Race PvP", this, SLOT(toggle_racePvP()));
  m_action_racePvP->setCheckable(true);

  m_action_zoneSafePoint = subMenu->addAction("Zone Safe Point", this,
          SLOT(toggle_zoneSafePoint()));
  m_action_zoneSafePoint->setCheckable(true);

  m_action_instanceLocation = subMenu->addAction("Instance Location Marker",
          this, SLOT(toggle_instanceLocationMarker()));
  m_action_instanceLocation->setCheckable(true);

#ifdef DEBUG
  m_action_debugInfo = subMenu->addAction("Debug Info", this,
          SLOT(toggle_debugInfo()));
  m_action_debugInfo->setCheckable(true);
#endif

  addMenu(subMenu);


  m_action_gridTickColor = addAction("Grid Tick Color...", this,
          SLOT(select_gridTickColor()));

  m_action_gridLineColor = addAction("Grid Line Color...", this,
          SLOT(select_gridLineColor()));

  m_action_backgroundColor = addAction("Background Color...", this,
          SLOT(select_backgroundColor()));

  m_action_font = addAction("Font...", this, SLOT(select_font()));

  addAction("Edit Map Icons...", m_map, SLOT(showMapIconDialog()));

  subMenu = new QMenu("Draw Size");
  m_drawSizeSpinBox = new QSpinBox(subMenu);
  m_drawSizeSpinBox->setMinimum(1);
  m_drawSizeSpinBox->setMaximum(6);
  m_drawSizeSpinBox->setSingleStep(1);
  m_drawSizeSpinBox->setValue(m_mapIcons->drawSize());
  connect(m_drawSizeSpinBox, SIGNAL(valueChanged(int)),
      m_mapIcons, SLOT(setDrawSize(int)));
  QWidgetAction* drawSizeSpinBoxAction = new QWidgetAction(subMenu);
  drawSizeSpinBoxAction->setDefaultWidget(m_drawSizeSpinBox);
  subMenu->addAction(drawSizeSpinBoxAction);
  m_action_drawSizeMenu = addMenu(subMenu);

  subMenu = new QMenu("Player FOV");
  QWidget* tmpHBox = new QWidget(subMenu);
  QHBoxLayout* tmpHBoxLayout = new QHBoxLayout(tmpHBox);
  tmpHBoxLayout->setContentsMargins(1, 1, 1, 1);
  m_fovSpinBoxLabel = new QLabel("Distance:", tmpHBox);
  m_fovSpinBox = new QSpinBox(tmpHBox);
  m_fovSpinBox->setObjectName("FOV");
  m_fovSpinBox->setMinimum(20);
  m_fovSpinBox->setMaximum(1200);
  m_fovSpinBox->setSingleStep(20);
  tmpHBoxLayout->addWidget(m_fovSpinBoxLabel);
  tmpHBoxLayout->addWidget(m_fovSpinBox);
  m_fovSpinBox->setValue(m_mapIcons->fovDistance());
  connect(m_fovSpinBox, SIGNAL(valueChanged(int)),
      m_mapIcons, SLOT(setFOVDistance(int)));
  QWidgetAction* fovSpinBoxAction = new QWidgetAction(tmpHBox);
  fovSpinBoxAction->setDefaultWidget(tmpHBox);

  subMenu->addAction(fovSpinBoxAction);
  m_action_FOVColor = subMenu->addAction("Color...",
                      this, SLOT(select_fovColor()));

  subSubMenu = new QMenu("FOV Style");

  m_action_FOVNoBrush = subSubMenu->addAction("No Background");
  m_action_FOVNoBrush->setCheckable(true);
  m_action_FOVNoBrush->setData(static_cast<int>(Qt::NoBrush));

  m_action_FOVSolidPattern = subSubMenu->addAction("Solid");
  m_action_FOVSolidPattern->setCheckable(true);
  m_action_FOVSolidPattern->setData(static_cast<int>(Qt::SolidPattern));

  m_action_FOVDense1Pattern = subSubMenu->addAction("94% fill");
  m_action_FOVDense1Pattern->setCheckable(true);
  m_action_FOVDense1Pattern->setData(static_cast<int>(Qt::Dense1Pattern));

  m_action_FOVDense2Pattern = subSubMenu->addAction("88% fill");
  m_action_FOVDense2Pattern->setCheckable(true);
  m_action_FOVDense2Pattern->setData(static_cast<int>(Qt::Dense2Pattern));

  m_action_FOVDense3Pattern  = subSubMenu->addAction("63% fill");
  m_action_FOVDense3Pattern ->setCheckable(true);
  m_action_FOVDense3Pattern ->setData(static_cast<int>(Qt::Dense3Pattern));

  m_action_FOVDense4Pattern = subSubMenu->addAction("50% fill");
  m_action_FOVDense4Pattern->setCheckable(true);
  m_action_FOVDense4Pattern->setData(static_cast<int>(Qt::Dense4Pattern));

  m_action_FOVDense5Pattern = subSubMenu->addAction("37% fill");
  m_action_FOVDense5Pattern->setCheckable(true);
  m_action_FOVDense5Pattern->setData(static_cast<int>(Qt::Dense5Pattern));

  m_action_FOVDense6Pattern = subSubMenu->addAction("12% fill");
  m_action_FOVDense6Pattern->setCheckable(true);
  m_action_FOVDense6Pattern->setData(static_cast<int>(Qt::Dense6Pattern));

  m_action_FOVDense7Pattern = subSubMenu->addAction("6% fill");
  m_action_FOVDense7Pattern->setCheckable(true);
  m_action_FOVDense7Pattern->setData(static_cast<int>(Qt::Dense7Pattern));

  m_action_FOVHorPattern = subSubMenu->addAction("Horizontal lines");
  m_action_FOVHorPattern->setCheckable(true);
  m_action_FOVHorPattern->setData(static_cast<int>(Qt::HorPattern));

  m_action_FOVVerPattern = subSubMenu->addAction("Vertical lines");
  m_action_FOVVerPattern->setCheckable(true);
  m_action_FOVVerPattern->setData(static_cast<int>(Qt::VerPattern));

  m_action_FOVCrossPattern = subSubMenu->addAction("Crossing lines");
  m_action_FOVCrossPattern->setCheckable(true);
  m_action_FOVCrossPattern->setData(static_cast<int>(Qt::CrossPattern));

  m_action_FOVBDiagPattern = subSubMenu->addAction("Diagonal lines (directed /)");
  m_action_FOVBDiagPattern->setCheckable(true);
  m_action_FOVBDiagPattern->setData(static_cast<int>(Qt::BDiagPattern));

  m_action_FOVFDiagPattern = subSubMenu->addAction("Diagonal lines (directed \\)");
  m_action_FOVFDiagPattern->setCheckable(true);
  m_action_FOVFDiagPattern->setData(static_cast<int>(Qt::FDiagPattern));

  m_action_FOVDiagCrossPattern = subSubMenu->addAction("Diagonal crossing lines");
  m_action_FOVDiagCrossPattern->setCheckable(true);
  m_action_FOVDiagCrossPattern->setData(static_cast<int>(Qt::DiagCrossPattern));

  connect(subSubMenu, SIGNAL(triggered(QAction*)), this,
          SLOT(select_fovStyle(QAction*)));
  subMenu->addMenu(subSubMenu);

  subSubMenu = new QMenu("FOV Mode");

  m_action_FOVDistanceBased = subSubMenu->addAction("Distance Based");
  m_action_FOVDistanceBased->setCheckable(true);
  m_action_FOVDistanceBased->setData(tFOVDistanceBased);

  m_action_FOVScaledClassic = subSubMenu->addAction("Scaled Classic");
  m_action_FOVScaledClassic->setCheckable(true);
  m_action_FOVScaledClassic->setData(tFOVScaledClassic);

  m_action_FOVClassic = subSubMenu->addAction("Classic");
  m_action_FOVClassic->setCheckable(true);
  m_action_FOVClassic->setData(tFOVClassic);

  connect(subSubMenu, SIGNAL(triggered(QAction*)), this,
          SLOT(select_fovMode(QAction*)));

  subMenu->addMenu(subSubMenu);
  m_action_FOV = addMenu(subMenu);

  subMenu = new QMenu("Default Zoom");
  m_zoomDefaultSpinBox = new QSpinBox(subMenu);
  m_zoomDefaultSpinBox->setMinimum(1);
  m_zoomDefaultSpinBox->setMaximum(32);
  m_zoomDefaultSpinBox->setSingleStep(1);
  m_zoomDefaultSpinBox->setValue(m_mapIcons->drawSize());
  connect(m_zoomDefaultSpinBox, SIGNAL(valueChanged(int)),
      m_map, SLOT(setZoomDefault(int)));
  QWidgetAction* zoomDefaultSpinBoxAction = new QWidgetAction(subMenu);
  zoomDefaultSpinBoxAction->setDefaultWidget(m_zoomDefaultSpinBox);
  subMenu->addAction(zoomDefaultSpinBoxAction);
  m_action_zoomDefaultMenu = addMenu(subMenu);

  m_action_cacheAlwaysRepaint = addAction("Always Repaint Map Cache", this,
          SLOT(toggle_cacheAlwaysRepaint()));
  m_action_cacheAlwaysRepaint->setCheckable(true);

  m_action_cacheChanges = addAction("Cache Changes", this,
          SLOT(toggle_cacheChanges()));
  m_action_cacheChanges->setCheckable(true);

  addAction("Save Map Image...", m_map, SLOT(saveMapImage()));

  connect(this, SIGNAL(aboutToShow()), this, SLOT(init_Menu()));
}

MapMenu::~MapMenu()
{
}

void MapMenu::init_Menu(void)
{
  FollowMode mode = m_map->followMode();
  m_action_followMenu_Player->setChecked(mode == tFollowPlayer);
  m_action_followMenu_Spawn->setChecked(mode == tFollowSpawn);
  m_action_followMenu_None->setChecked(mode == tFollowNone);
  
  const Item* selectedItem = m_map->selectedItem();
  m_action_followMenu_Spawn->setEnabled((selectedItem != NULL) &&
          (selectedItem->type() == tSpawn));

  MapLineStyle style = m_map->mapLineStyle();
  m_action_mapLineStyle_Normal->setChecked(style == tMap_Normal);
  m_action_mapLineStyle_DepthFiltered->setChecked(style == tMap_DepthFiltered);
  m_action_mapLineStyle_FadedFloors->setChecked(style == tMap_FadedFloors);
  m_action_spawnDepthFilter->setChecked(m_map->spawnDepthFilter());
  m_action_tooltip->setChecked(m_map->showTooltips());
  m_action_filtered->setChecked(m_map->showFiltered());
  m_action_map->setChecked(m_map->showLines());
  m_action_velocity->setChecked(m_map->showVelocityLines());
  m_action_animate->setChecked(m_map->animate());
  m_action_player->setChecked(m_map->showPlayer());
  m_action_playerBackground->setChecked(m_map->showPlayerBackground());
  m_action_playerView->setChecked(m_map->showPlayerView());
  m_action_gridLines->setChecked(m_map->showGridLines());
  m_action_gridTicks->setChecked(m_map->showGridTicks());
  m_action_locations->setChecked(m_map->showLocations());
  m_action_spawns->setChecked(m_map->showSpawns());
  m_action_spawnPoints->setChecked(m_map->showSpawnPoints());
  m_action_unknownSpawns->setChecked(m_map->showUnknownSpawns());
  m_action_drops->setChecked(m_map->showDrops());
  m_action_doors->setChecked(m_map->showDoors());
  m_action_spawnNames->setChecked(m_mapIcons->showSpawnNames());
  m_action_highlightConsideredSpawns->setChecked(
         m_map->highlightConsideredSpawns());
  m_action_walkPath->setChecked(m_map->walkPathShowSelect());
  m_action_npcWalkPaths->setChecked(m_mapIcons->showNPCWalkPaths());
  m_action_mapImage->setChecked(m_map->showBackgroundImage());
  m_action_pvp->setChecked(m_map->pvp());
  m_action_deityPvP->setChecked(m_map->deityPvP());
  m_action_racePvP->setChecked(m_map->racePvP());
  m_action_zoneSafePoint->setChecked(m_map->showZoneSafePoint());
  m_action_instanceLocation->setChecked(m_map->showInstanceLocationMarker());
#ifdef DEBUG
  m_action_debugInfo->setChecked(m_map->showDebugInfo());
#endif

  m_drawSizeSpinBox->setValue(m_mapIcons->drawSize());

  m_fovSpinBox->setValue(m_mapIcons->fovDistance());

  int fovStyle = m_map->fovStyle();
  m_action_FOVNoBrush->setChecked(fovStyle == Qt::NoBrush);
  m_action_FOVSolidPattern->setChecked(fovStyle == Qt::SolidPattern);
  m_action_FOVDense1Pattern->setChecked(fovStyle == Qt::Dense1Pattern);
  m_action_FOVDense2Pattern->setChecked(fovStyle == Qt::Dense2Pattern);
  m_action_FOVDense3Pattern->setChecked(fovStyle == Qt::Dense3Pattern);
  m_action_FOVDense4Pattern->setChecked(fovStyle == Qt::Dense4Pattern);
  m_action_FOVDense5Pattern->setChecked(fovStyle == Qt::Dense5Pattern);
  m_action_FOVDense6Pattern->setChecked(fovStyle == Qt::Dense6Pattern);
  m_action_FOVDense7Pattern->setChecked(fovStyle == Qt::Dense7Pattern);
  m_action_FOVHorPattern->setChecked(fovStyle == Qt::HorPattern);
  m_action_FOVVerPattern->setChecked(fovStyle == Qt::VerPattern);
  m_action_FOVCrossPattern->setChecked(fovStyle == Qt::CrossPattern);
  m_action_FOVBDiagPattern->setChecked(fovStyle == Qt::BDiagPattern);
  m_action_FOVFDiagPattern->setChecked(fovStyle == Qt::FDiagPattern);
  m_action_FOVDiagCrossPattern->setChecked(fovStyle == Qt::DiagCrossPattern);

  init_fovMenu();

  m_zoomDefaultSpinBox->setValue(m_map->zoomDefault());

  m_action_cacheAlwaysRepaint->setChecked(m_map->cacheAlwaysRepaint());
  m_action_cacheChanges->setChecked(m_map->cacheChanges());
}

void MapMenu::init_fovMenu(void)
{
  FOVMode fovMode = m_map->fovMode();

  // calculate new base FOV Distance
  int newFOVDistMin = 20;
  int newFOVDistMax = 1200;
  int newFOVDistInc = 20;
  if (fovMode != tFOVDistanceBased)
  {
    newFOVDistMin = 5;
    newFOVDistMax = 120;
    newFOVDistInc = 1;
    
    m_fovSpinBoxLabel->setText("Base Radius:");
  }
  else 
    m_fovSpinBoxLabel->setText("Distance:");

  int fovDistance = m_mapIcons->fovDistance();
  m_fovSpinBox->setRange(newFOVDistMin, newFOVDistMax);
  m_fovSpinBox->setSingleStep(newFOVDistInc);
  m_fovSpinBox->setValue(fovDistance);

  m_action_FOVDistanceBased->setChecked(fovMode == tFOVDistanceBased);
  m_action_FOVScaledClassic->setChecked(fovMode == tFOVScaledClassic);
  m_action_FOVClassic->setChecked(fovMode == tFOVClassic);
}

void MapMenu::editLayerChanged(void)
{
  m_editLayerSpinBox->setMinimum(0);
  m_editLayerSpinBox->setSingleStep(1);
  m_editLayerSpinBox->setMaximum(m_map->mapMgr()->mapData().numLayers()-1);
  m_editLayerSpinBox->setValue(m_map->mapMgr()->mapData().editLayer());
}

void MapMenu::select_follow(QAction* item)
{
  int mode = item->data().value<int>();
  // set the selected follow mode
  m_map->setFollowMode((FollowMode)mode);
}

void MapMenu::select_mapLine(QAction* item)
{
  int style = item->data().value<int>();
  m_map->setMapLineStyle((MapLineStyle)style);
}

void MapMenu::toggle_spawnDepthFilter()
{
  m_map->setSpawnDepthFilter(!m_map->spawnDepthFilter());
}

void MapMenu::toggle_tooltip()
{
  m_map->setShowTooltips(!m_map->showTooltips());
}

void MapMenu::toggle_filtered()
{
  m_map->setShowFiltered(!m_map->showFiltered());
}

void MapMenu::toggle_map()
{
  m_map->setShowLines(!m_map->showLines());
}

void MapMenu::toggle_pvp()
{
  m_map->setPvP(!m_map->pvp());
}

void MapMenu::toggle_deityPvP()
{
  m_map->setDeityPvP(!m_map->deityPvP());
}

void MapMenu::toggle_racePvP()
{
  m_map->setRacePvP(!m_map->racePvP());
}

void MapMenu::toggle_velocity()
{
  m_map->setShowVelocityLines(!m_map->showVelocityLines());
}

void MapMenu::toggle_animate()
{
  m_map->setAnimate(!m_map->animate());
}

void MapMenu::toggle_player()
{
  m_map->setShowPlayer(!m_map->showPlayer());
}

void MapMenu::toggle_playerBackground()
{
  m_map->setShowPlayerBackground(!m_map->showPlayerBackground());
}

void MapMenu::toggle_playerView()
{
  m_map->setShowPlayerView(!m_map->showPlayerView());
}

void MapMenu::toggle_gridLines()
{
  m_map->setShowGridLines(!m_map->showGridLines());
}

void MapMenu::toggle_gridTicks()
{
  m_map->setShowGridTicks(!m_map->showGridTicks());
}

void MapMenu::toggle_locations()
{
  m_map->setShowLocations(!m_map->showLocations());
}

void MapMenu::toggle_spawns()
{
  m_map->setShowSpawns(!m_map->showSpawns());
}

void MapMenu::toggle_spawnPnts()
{
  m_map->setShowSpawnPoints(!m_map->showSpawnPoints());
}

void MapMenu::toggle_unknownSpawns()
{
  m_map->setShowUnknownSpawns(!m_map->showUnknownSpawns());
}

void MapMenu::toggle_drops()
{
  m_map->setShowDrops(!m_map->showDrops());
}

void MapMenu::toggle_doors()
{
  m_map->setShowDoors(!m_map->showDoors());
}

void MapMenu::toggle_spawnNames()
{
  m_mapIcons->setShowSpawnNames(!m_mapIcons->showSpawnNames());
}

void MapMenu::toggle_highlightConsideredSpawns()
{
  m_map->setHighlightConsideredSpawns(!m_map->highlightConsideredSpawns());
}

void MapMenu::toggle_walkPath()
{
  m_map->setWalkPathShowSelect(!m_map->walkPathShowSelect());
}

void MapMenu::toggle_npcWalkPaths()
{
  m_mapIcons->setShowNPCWalkPaths(!m_mapIcons->showNPCWalkPaths());
}

void MapMenu::toggle_mapImage()
{
  m_map->setShowBackgroundImage(!m_map->showBackgroundImage());
}

void MapMenu::toggle_debugInfo()
{
#ifdef DEBUG
  m_map->setShowDebugInfo(!m_map->showDebugInfo());
#endif
}

void MapMenu::toggle_cacheAlwaysRepaint()
{
  m_map->setCacheAlwaysRepaint(!m_map->cacheAlwaysRepaint());
}

void MapMenu::toggle_cacheChanges()
{
  m_map->setCacheChanges(!m_map->cacheChanges());
}

void MapMenu::toggle_zoneSafePoint()
{
  m_map->setShowZoneSafePoint(!m_map->showZoneSafePoint());
}

void MapMenu::toggle_instanceLocationMarker()
{
  m_map->setShowInstanceLocationMarker(!m_map->showInstanceLocationMarker());
}

void MapMenu::select_gridTickColor()
{
  QString name = QString("ShowEQ - ") + m_map->preferenceName() 
    + " Grid Tick Color";
  QColor newColor = QColorDialog::getColor(m_map->gridTickColor(),
                       m_map, name);

  if (newColor.isValid())
    m_map->setGridTickColor(newColor);
}

void MapMenu::select_gridLineColor()
{
  QString name = QString("ShowEQ - ") + m_map->preferenceName() 
    + " Grid Line Color";
  QColor newColor = QColorDialog::getColor(m_map->gridLineColor(),
                       m_map, name);

  if (newColor.isValid())
    m_map->setGridLineColor(newColor);
}

void MapMenu::select_backgroundColor()
{
  QString name = QString("ShowEQ - ") + m_map->preferenceName() 
    + " Background Color";
  QColor newColor = QColorDialog::getColor(m_map->backgroundColor(),
                       m_map, name);

  if (newColor.isValid())
    m_map->setBackgroundColor(newColor);
}

void MapMenu::select_font()
{
  QString name = QString("ShowEQ - ") + m_map->preferenceName() 
    + " Font";
  bool ok = false;
  QFont newFont;
  newFont = QFontDialog::getFont(&ok, m_map->font(), m_map, name);

  if (ok)
    m_map->setFont(newFont);
}

void MapMenu::select_fovColor()
{
  QString name = QString("ShowEQ - ") + m_map->preferenceName() 
    + " Player FOV Color";
  QColor newColor = QColorDialog::getColor(m_map->fovColor(),
                       m_map, name);

  if (newColor.isValid())
    m_map->setFOVColor(newColor);
}

void MapMenu::select_fovStyle(QAction* item)
{
  int style = item->data().value<int>();
  m_map->setFOVStyle(style);
}

void MapMenu::select_fovMode(QAction* item)
{
  int mode = item->data().value<int>();
  FOVMode oldFOVMode = m_map->fovMode();
  FOVMode newFOVMode = (FOVMode)mode;

  if (oldFOVMode != newFOVMode)
  {
    // set the new FOV Mode
    uint16_t newFOVDistance = 0;
    if ((newFOVMode != tFOVDistanceBased) && 
    (oldFOVMode == tFOVDistanceBased))
      newFOVDistance = 40;
    else if (newFOVMode == tFOVDistanceBased)
      newFOVDistance = 200;

    m_map->setFOVMode(newFOVMode);

    if (newFOVDistance != 0)
      m_mapIcons->setFOVDistance(newFOVDistance);

    init_fovMenu();
  }
}

//----------------------------------------------------------------------
// Map
Map::Map(MapMgr* mapMgr, 
     Player* player, 
     SpawnShell* spawnshell, 
     ZoneMgr* zoneMgr,
     SpawnMonitor* spawnMonitor,
     const QString& preferenceName, 
     uint32_t runtimeFilterFlagMask,
     QWidget * parent, 
     const char *name)
  : QWidget (parent),
    m_preferenceName(preferenceName),
    m_param(mapMgr->mapData()),
    m_mapMgr(mapMgr),
    m_mapCache(mapMgr->mapData()),
    m_menu(NULL),
    m_mapIcons(0),
    m_mapIconDialog(0),
    m_runtimeFilterFlagMask(runtimeFilterFlagMask),
    m_player(player),
    m_spawnShell(spawnshell),
    m_zoneMgr(zoneMgr),
    m_spawnMonitor(spawnMonitor)
{

  setObjectName(name);
#ifdef DEBUGMAP
  qDebug ("Map()");
#endif /* DEBUGMAP */

  // save the name used for preferences 
  QString prefString = Map::preferenceName();
  QString tmpPrefString;
  QString tmpDefault;
  QString tmp;

  // create the map icons object
  m_mapIcons = new MapIcons(player, preferenceName + "Icons", 
                this, "mapicons");

  connect(m_mapIcons, SIGNAL(changed()),
      this, SLOT(reAdjustAndRefreshMap()));

  // load the map icon information
  m_mapIcons->load();

  // setup filter check ordering
  m_filterCheckOrdering[0] = 
    pSEQPrefs->getPrefInt("Filter0", prefString, FILTERED_FILTER);
  m_filterCheckOrdering[1] = 
    pSEQPrefs->getPrefInt("Filter1", prefString, TRACER_FILTER);
  m_filterCheckOrdering[2] = 
    pSEQPrefs->getPrefInt("Filter2", prefString, LOCATE_FILTER);
  m_filterCheckOrdering[3] = 
    pSEQPrefs->getPrefInt("Filter3", prefString, HUNT_FILTER);
  m_filterCheckOrdering[4] = 
    pSEQPrefs->getPrefInt("Filter4", prefString, ALERT_FILTER);
  m_filterCheckOrdering[5] = 
    pSEQPrefs->getPrefInt("Filter5", prefString, CAUTION_FILTER);
  m_filterCheckOrdering[6] = 
    pSEQPrefs->getPrefInt("Filter6", prefString, DANGER_FILTER);

  tmpPrefString = "Caption";
  tmpDefault = QString("ShowEQ - ") + prefString;
  setWindowTitle(pSEQPrefs->getPrefString(tmpPrefString, prefString, tmpDefault));

  tmpPrefString = "CacheChanges";
  m_cacheChanges = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "AnimateSpawnMovement";
  m_animate = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);

  tmpPrefString = "VelocityLines";
  m_showVelocityLines = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "SpawnDepthFilter";
  m_spawnDepthFilter = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);

  tmpPrefString = "Framerate";
  m_frameRate = pSEQPrefs->getPrefInt(tmpPrefString, prefString, 5);

#ifdef DEBUG
  tmpPrefString = "ShowDebugInfo";
  m_showDebugInfo = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);
#endif

  tmpPrefString = "ShowPlayer";
  m_showPlayer = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "ShowPlayerBackground";
  m_showPlayerBackground = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "ShowPlayerView";
  m_showPlayerView = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "ShowDroppedItems";
  m_showDrops = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "ShowDoors";
  m_showDoors = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);

  tmpPrefString = "ShowSpawns";
  m_showSpawns = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "ShowSpawnPoints";
  m_showSpawnPoints = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "ShowUnknownSpawns";
  m_showUnknownSpawns = pSEQPrefs->getPrefBool(tmpPrefString, prefString, 
                           showeq_params->createUnknownSpawns);

  tmpPrefString = "HighlightConsideredSpawns";
  m_highlightConsideredSpawns = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "ShowTooltips";
  m_showTooltips = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "WalkPathShowSelect";
  m_walkpathshowselect = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);

  tmpPrefString = "ShowFiltered";
  m_showFiltered = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);


  tmpPrefString = "FOVMode";
  m_fovMode = (FOVMode)pSEQPrefs->getPrefInt(tmpPrefString, prefString, 
                         tFOVDistanceBased);

  tmpPrefString = "FOVStyle";
  m_fovStyle = pSEQPrefs->getPrefInt(tmpPrefString, prefString, Qt::Dense7Pattern);

  tmpPrefString = "FOVColor";
  m_fovColor = pSEQPrefs->getPrefColor(tmpPrefString, prefString, QColor("#505050"));

  // mainly for backwards compatibility
  tmpPrefString = "MapDepthFilter";
  if (pSEQPrefs->getPrefBool(tmpPrefString, prefString, false))
    m_param.setMapLineStyle(tMap_DepthFiltered);

  tmpPrefString = "FadingFloors";
  if (pSEQPrefs->getPrefBool(tmpPrefString, prefString, false))
    m_param.setMapLineStyle(tMap_FadedFloors);

  // the new setting overrides old settings
  tmpPrefString = "MapLineStyle";
  if (pSEQPrefs->isPreference(tmpPrefString, prefString))
    m_param.setMapLineStyle((MapLineStyle)pSEQPrefs->getPrefInt(tmpPrefString, prefString,
                                tMap_Normal));

  tmpPrefString = "ShowMapPoints";
  m_param.setShowLocations(pSEQPrefs->getPrefBool(tmpPrefString, prefString, true));

  tmpPrefString = "ShowMapLines";
  m_param.setShowLines(pSEQPrefs->getPrefBool(tmpPrefString, prefString, true));

  tmpPrefString = "ShowGridLines";
  m_param.setShowGridLines(pSEQPrefs->getPrefBool(tmpPrefString, prefString, true));

  tmpPrefString = "ShowGridTicks";
  m_param.setShowGridTicks(pSEQPrefs->getPrefBool(tmpPrefString, prefString, true));

  tmpPrefString = "ShowBackgroundImage";
  m_param.setShowBackgroundImage(pSEQPrefs->getPrefBool(tmpPrefString, prefString, true));

  tmpPrefString = "GridResolution";
  m_param.setGridResolution(pSEQPrefs->getPrefInt(tmpPrefString, prefString, 500));

  tmpPrefString = "Font";
  m_param.setFont(pSEQPrefs->getPrefFont(tmpPrefString, prefString));

  tmpPrefString = "GridTickColor";
  m_param.setGridTickColor(pSEQPrefs->getPrefColor(tmpPrefString, prefString, QColor("#E1C819")));

  tmpPrefString = "GridLineColor";
  m_param.setGridLineColor(pSEQPrefs->getPrefColor(tmpPrefString, prefString, QColor("#194819")));

  tmpPrefString = "BackgroundColor";
  m_param.setBackgroundColor(pSEQPrefs->getPrefColor(tmpPrefString, prefString, QColor("black")));

  tmpPrefString = "HeadRoom";
  m_param.setHeadRoom(pSEQPrefs->getPrefInt(tmpPrefString, prefString, 75));

  tmpPrefString = "FloorRoom";
  m_param.setFloorRoom(pSEQPrefs->getPrefInt(tmpPrefString, prefString, 75));

  tmpPrefString = "ZoomDefault";
  m_param.setZoomDefault(pSEQPrefs->getPrefInt(tmpPrefString, prefString, 1));

  tmpPrefString = "CacheAlwaysRepaint";
  m_mapCache.setAlwaysRepaint(pSEQPrefs->getPrefBool(tmpPrefString, prefString, false));

  tmpPrefString = "PvP";
  m_pvp = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);

  tmpPrefString = "DeityPvP";
  m_deityPvP = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);

  tmpPrefString = "RacePvP";
  m_racePvP = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);

  tmpPrefString = "ShowZoneSafePoint";
  m_showZoneSafePoint = pSEQPrefs->getPrefBool(tmpPrefString, prefString, true);

  tmpPrefString = "ShowInstanceLocationMarker";
  m_showInstanceLocationMarker = pSEQPrefs->getPrefBool(tmpPrefString, prefString, false);

  tmpPrefString = "MapLayerMask";
  uint32_t mask = pSEQPrefs->getPrefInt(tmpPrefString, prefString, 0xffffffff);
  for (int i = 0; i < 32; ++i)
    m_param.setLayerVisibility(i, mask & (1 << i));


  // Accelerators
  QShortcut *tmpShortcut = nullptr;
  QKeySequence key;
  key = pSEQPrefs->getPrefKey("ZoomInKey", prefString, "+");
  tmpShortcut = new QShortcut(key, this, SLOT(zoomIn()));

  key = pSEQPrefs->getPrefKey("ZoomOutKey", prefString, "-");
  tmpShortcut = new QShortcut(key, this, SLOT(zoomOut()));

  key = pSEQPrefs->getPrefKey("PanDownLeftKey", prefString, "Ctrl+1");
  tmpShortcut = new QShortcut(key, this, SLOT(panDownLeft()));

  key = pSEQPrefs->getPrefKey("PanDownKey", prefString, "Ctrl+2");
  tmpShortcut = new QShortcut(key, this, SLOT(panDown()));

  key = pSEQPrefs->getPrefKey("PanDownRightKey", prefString, "Ctrl+3");
  tmpShortcut = new QShortcut(key, this, SLOT(panDownRight()));

  key = pSEQPrefs->getPrefKey("PanLeftKey", prefString, "Ctrl+4");
  tmpShortcut = new QShortcut(key, this, SLOT(panLeft()));

  key = pSEQPrefs->getPrefKey("CenterSelectedKey", prefString, "Ctrl+5");
  tmpShortcut = new QShortcut(key, this, SLOT(viewTarget()));

  key = pSEQPrefs->getPrefKey("PanRightKey", prefString, "Ctrl+6");
  tmpShortcut = new QShortcut(key, this, SLOT(panRight()));

  key = pSEQPrefs->getPrefKey("PanUpLeftKey", prefString, "Ctrl+7");
  tmpShortcut = new QShortcut(key, this, SLOT(panUpLeft()));

  key = pSEQPrefs->getPrefKey("PanUpKey", prefString, "Ctrl+8");
  tmpShortcut = new QShortcut(key, this, SLOT(panUp()));

  key = pSEQPrefs->getPrefKey("PanUpRightKey", prefString, "Ctrl+9");
  tmpShortcut = new QShortcut(key, this, SLOT(panUpRight()));

  key = pSEQPrefs->getPrefKey("ViewLockKey", prefString, "Ctrl+0");
  tmpShortcut = new QShortcut(key, this, SLOT(viewLock()));

  m_followMode = tFollowPlayer;
  
  m_selectedItem = NULL;

  setMinimumSize(100, 100);

#ifdef DEBUG
  for (int i = 0; i < maxFrameTimes; i++)
    m_frameTimes[i] = 0;
  m_frameTimeIndex = 0;
  m_paintCount = 0;
  m_paintTimeSum = 0;
#endif

  // Setup m_param
  m_param.setScreenSize(size());
  
  // Setup offscreen image
  m_offscreen = QPixmap(m_param.screenLength());
  
  m_mapTip = new MapLabel( this );
  this->setMouseTracking( true );

  m_mapPanning = false;

  setMouseTracking(true);
  
  m_timer = new QTimer(this);
  connect(m_timer, SIGNAL(timeout()), 
      this, SLOT(refreshMap()));
  
  // supply the Map slots with signals from MapMgr
  connect(m_mapMgr, SIGNAL(mapLoaded()),
      this, SLOT(mapLoaded()));
  connect(m_mapMgr, SIGNAL(mapUnloaded()),
      this, SLOT(mapUnloaded()));
  connect(m_mapMgr, SIGNAL(mapUpdated()),
      this, SLOT(mapUpdated()));

  // supply the Map slots with signals from SpawnShell
  connect(m_spawnShell, SIGNAL(delItem(const Item*)),
      this, SLOT(delItem(const Item*)));
  connect(m_spawnShell, SIGNAL(clearItems()),
      this, SLOT(clearItems()));
  connect (m_spawnShell,SIGNAL(changeItem(const Item*, uint32_t)),
       this, SLOT(changeItem(const Item*, uint32_t)));

  m_timer->start(1000/m_frameRate);

#ifdef DEBUG
  if (m_showDebugInfo)
    m_time.start();
#endif
} // end Map() constructor

Map::~Map(void)
{
}

void Map::savePrefs(void)
{
  QString prefString = preferenceName();
  QString tmpPrefString;
  m_mapIcons->save();
}

MapMenu* Map::menu()
{
  // return the existing menu if it exists
  if (m_menu != NULL)
    return m_menu;

  // create the menu
  m_menu = new MapMenu(this, this, "map menu");

  // make sure to use the applications font
  m_menu->setFont(QFont());

  return m_menu;
}

QSize Map::sizeHint() const // preferred size
{
#ifdef DEBUGMAP
  qDebug ("sizeHint()");
#endif /* DEBUGMAP */
  
  return QSize(600, 600);
}

QSize Map::minimumSizeHint() const // minimum size
{
#ifdef DEBUGMAP
  qDebug ("minimumSizeHint()");
#endif /* DEBUGMAP */
  return QSize(300, 300);
}

QSizePolicy Map::sizePolicy() const // size policy
{
#ifdef DEBUGMAP
  qDebug ("sizePolicy()");
#endif /* DEBUGMAP */
  return QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void Map::mouseDoubleClickEvent(QMouseEvent * me)
{
#ifdef DEBUGMAP
  qDebug ("mouseDoubleClickEvent()");
#endif /* DEBUGMAP */
  if (me->button () == Qt::MidButton)
    viewTarget();
}

void Map::mouseReleaseEvent(QMouseEvent* me)
{
#ifdef DEBUGMAP
  qDebug ("mouseReleaseEvent()");
#endif /* DEBUGMAP */
  if (me->button() == Qt::MidButton)
    m_mapPanning = false;
}

void Map::mousePressEvent(QMouseEvent* me)
{
#ifdef DEBUGMAP
  qDebug ("mousePressEvent()");
#endif /* DEBUGMAP */
  if (me->button () == Qt::RightButton) 
  {
    // display the Map's menu
    menu()->popup(mapToGlobal(me->pos()));
#ifdef DEBUGMAP
    FILE *f;
    f=fopen("/tmp/coords","at");
    if(f) 
    {
      fprintf (f,"%f, %f\n",
           (m_param.screenCenterX() - me->x()) * m_param.ratioX(),
           (m_param.screenCenterY() - me->y()) * m_param.ratioY());
      fclose(f);
    }
#endif /* DEBUGMAP */
  }
  else if (me->button () == Qt::MidButton) 
  {
    m_mapPanning = true;
    m_mapPanX     = me->x ();
    m_mapPanY     = me->y ();
  } 
  else 
  {
    const Item* closestSpawn;
    uint32_t dist = 15;
    // find the nearest spawn within a reasonable range
    closestSpawn = closestSpawnToPoint(me->pos(), dist);

    // check for closest spawn point
    const SpawnPoint* closestSP = NULL;
    if (m_showSpawnPoints)
      closestSP = closestSpawnPointToPoint(me->pos(), dist);

    // only get a spawn point if the user clicked closer to it then a
    // the closest spawn
    if (closestSP != NULL)
    {
      m_spawnMonitor->setSelected(closestSP);

      return;
    }

    // make sure the user actually clicked vaguely near a spawn
    if (closestSpawn != NULL)
    {
      // note new selection
      m_selectedItem = closestSpawn;
      
      // notify others of new selection
      emit spawnSelected(m_selectedItem);
      
      // reAdjust to make sure it's focused around
      reAdjust();

      // repaint if necessary
      if(!m_cacheChanges)
        refreshMap ();
    }
  }
}


void Map::wheelEvent( QWheelEvent * ev)
{
    int deltaY = 0;

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    deltaY = ev->angleDelta().y();
#else
    deltaY = ev->delta();
#endif

    //y is verticle, x is horizontal
    //we only handle vertical scroll for zoom in/out.
    //ignore anything else so it can be handled by parent.
    if (deltaY > 0)
    {
        setZoom(zoom() + 1);
        ev->accept();
        return;
    }
    else if (deltaY < 0)
    {
        setZoom(zoom() - 1);
        ev->accept();
        return;
    }

    ev->ignore();
}

void Map::zoomIn()
{
#ifdef DEBUGMAP
   qDebug ("Map::zoomIn()");
#endif /* DEBUGMAP */
   if (m_player->id() != 1)
   {
     if (m_param.zoomIn())
     {
       emit zoomChanged(m_param.zoom());

       // requires ReAdjust
       reAdjust();

       if (!m_cacheChanges)
         refreshMap ();
     }
   }
}

void Map::zoomOut()
{
#ifdef DEBUGMAP
   qDebug ("Map::zoomOut()");
#endif /* DEBUGMAP */

   if (m_player->id() != 1)
   {
     if (m_param.zoomOut())
     {
       emit zoomChanged(m_param.zoom());

       // requires ReAdjust
       reAdjust();

       if(!m_cacheChanges)
         refreshMap ();
     }
   }    
}

void Map::panRight()
{
#ifdef DEBUGMAP
   qDebug ("Map::panRight()");
#endif /* DEBUGMAP */
   m_param.panX(-panAmmt);

   emit panXChanged(m_param.panOffsetX());
   
   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::panLeft()
{
#ifdef DEBUGMAP
   qDebug ("Map::panLeft()");
#endif /* DEBUGMAP */
   m_param.panX(panAmmt);

   emit panXChanged(m_param.panOffsetX());

   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}


void Map::panDown()
{
#ifdef DEBUGMAP
   qDebug ("Map::panDown()");
#endif /* DEBUGMAP */
   m_param.panY(-panAmmt);

   emit panYChanged(m_param.panOffsetY());

   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}


void Map::panUp()
{
#ifdef DEBUGMAP
   qDebug ("Map::panUp()");
#endif /* DEBUGMAP */
   m_param.panY(panAmmt);

   emit panYChanged(m_param.panOffsetY());

   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::panUpRight()
{
#ifdef DEBUGMAP
   qDebug ("Map::panUpRight()");
#endif /* DEBUGMAP */
   m_param.panXY(-panAmmt, panAmmt);

   emit panXChanged(m_param.panOffsetX());
   emit panYChanged(m_param.panOffsetY());

   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::panUpLeft()
{
#ifdef DEBUGMAP
   qDebug ("Map::panUpLeft()");
#endif /* DEBUGMAP */
   m_param.panXY(panAmmt, panAmmt);

   emit panXChanged(m_param.panOffsetX());
   emit panYChanged(m_param.panOffsetY());

   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::panDownRight()
{
#ifdef DEBUGMAP
   qDebug ("Map::panDownRight()");
#endif /* DEBUGMAP */
   m_param.panXY(-panAmmt, -panAmmt);

   emit panXChanged(m_param.panOffsetX());
   emit panYChanged(m_param.panOffsetY());

   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::panDownLeft()
{
#ifdef DEBUGMAP
   qDebug ("Map::panDownLeft()");
#endif /* DEBUGMAP */
   m_param.panXY(panAmmt, -panAmmt);

   emit panXChanged(m_param.panOffsetX());
   emit panYChanged(m_param.panOffsetY());

   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::increaseGridResolution (void)
{
  m_param.increaseGridResolution();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::decreaseGridResolution (void)
{
  m_param.decreaseGridResolution();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::viewTarget()
{
#ifdef DEBUGMAP
  qDebug ("Map::viewTarget()");
#endif /* DEBUGMAP */
  
  switch (m_followMode)
  {
  case tFollowSpawn:
  case tFollowPlayer:
    m_param.clearPan();
    emit panXChanged(m_param.panOffsetX());
    emit panYChanged(m_param.panOffsetY());
    break;
  case tFollowNone:
    m_param.clearPan();
    emit panXChanged(m_param.panOffsetX());
    emit panYChanged(m_param.panOffsetY());
    if (m_selectedItem != NULL)
      m_followMode = tFollowSpawn;
    else
      m_followMode = tFollowPlayer;
  };
  
  reAdjust();
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::viewLock()
{
#ifdef DEBUGMAP
   qDebug ("Map::viewLock()");
#endif /* DEBUGMAP */
   switch (m_followMode)
   {
   case tFollowNone:
     // next mode is focused on selection if there is one or player if not
     if (m_selectedItem != NULL)
       m_followMode = tFollowSpawn;
     else
       m_followMode = tFollowPlayer;
     m_param.clearPan();
     emit panXChanged(m_param.panOffsetX());
     emit panYChanged(m_param.panOffsetY());
     break;
   case tFollowSpawn:
     if (m_selectedItem != NULL)
     {
       // next mode is follow none
       m_followMode = tFollowNone;
       MapPoint location;
       if (m_selectedItem->type() == tSpawn)
       {
         const Spawn* spawn = (const Spawn*)m_selectedItem;
     
         spawn->approximatePosition(m_animate, QTime::currentTime(),
                      location);
       }
       else
         location.setPoint(*m_selectedItem);

       m_param.setPan(location.x(), location.y());
       emit panXChanged(m_param.panOffsetX());
       emit panYChanged(m_param.panOffsetY());
     }
     else
     {
       // next mode is follow player
       m_followMode = tFollowPlayer;
       m_param.clearPan();
       emit panXChanged(m_param.panOffsetX());
       emit panYChanged(m_param.panOffsetY());
     }
     break;
   case tFollowPlayer:
     if (m_selectedItem == NULL)
     {
       // next mode is follow none
       m_followMode = tFollowNone;

       // retrieve the approximate current player position
       MapPoint targetPoint;
       m_player->approximatePosition(m_animate, QTime::currentTime(),
                   targetPoint);
       
       // set the current pan to it's position to avoid jarring the user
       m_param.setPan(targetPoint.x(), targetPoint.y());
       emit panXChanged(m_param.panOffsetX());
       emit panYChanged(m_param.panOffsetY());
     }
     else
     {
       // next mode is follow spawn
       m_followMode = tFollowSpawn;
       m_param.clearPan();
       emit panXChanged(m_param.panOffsetX());
       emit panYChanged(m_param.panOffsetY());
     }
     break;
   }

  // this requires a reAdjust
   reAdjust();

   if(!m_cacheChanges)
     refreshMap ();
}

void Map::setFollowMode(FollowMode mode) 
{ 
  // if the mode is the same, then nothing to do
  if (m_followMode == mode)
    return;

  switch(mode)
  {
  case tFollowSpawn:
    // if no spawn is selected, ignore the new setting
    if (m_selectedItem == NULL)
      return;

    // clear any panning parameters
    m_param.clearPan();
  case tFollowPlayer:
    m_param.clearPan();
    break;
  case tFollowNone:
    if (m_followMode == tFollowPlayer)
    {
       // retrieve the approximate current player position
       MapPoint targetPoint;
       m_player->approximatePosition(m_animate, QTime::currentTime(),
                   targetPoint);
       
       // set the current pan to it's position to avoid jarring the user
       m_param.setPan(targetPoint.x(), targetPoint.y());
    }
    else if (m_followMode == tFollowSpawn)
    {
       m_followMode = tFollowNone;
       if (m_selectedItem)
       {
         MapPoint location;
         if (m_selectedItem->type() == tSpawn)
         {
           const Spawn* spawn = 
             (const Spawn*)m_selectedItem;
           spawn->approximatePosition(m_animate, QTime::currentTime(),
                          location);
         }
         else
           location.setPoint(*m_selectedItem);
         
         m_param.setPan(location.x(), location.y());
       }
       else
         m_param.clearPan();
    }
    else // in case someone adds a new mode and forgets us...
      m_param.clearPan();
  }

  emit panXChanged(m_param.panOffsetX());
  emit panYChanged(m_param.panOffsetY());

  m_followMode = mode; 
  
  // this requires a reAdjust
  reAdjust();
  
  if(!m_cacheChanges)
    refreshMap ();
}

//
// ShowFiltered
//
// Toggle viewing of filtered spawns in map - they will show as grey dots
//
void
Map::setShowFiltered(bool bView)
{
  m_showFiltered = bView;

  pSEQPrefs->setPrefBool("ShowFiltered", preferenceName(), m_showFiltered);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setFrameRate(int val) 
{ 
  // make sure the value is within range
  if ((val >= 1) && (val <= 60))
  {
    m_frameRate = val;

    QString tmpPrefString = "Framerate";
    pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(), m_frameRate);

    emit frameRateChanged(m_frameRate);

    if (m_timer->isActive())
      m_timer->setInterval(1000/m_frameRate);
  }
}

void Map::setFOVStyle(int val)
{
  if ((val < Qt::NoBrush) || (val > Qt::DiagCrossPattern))
    return;

  m_fovStyle = val;

  QString tmpPrefString = "FOVStyle";
  pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(), m_fovStyle);
}

void Map::setFOVMode(FOVMode mode)
{
  if ((mode < tFOVDistanceBased) || (mode > tFOVClassic))
    return;

  m_fovMode = mode;

  QString tmpPrefString = "FOVMode";
  pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(), m_fovMode );
}

void Map::setFOVColor(const QColor& color)
{
  m_fovColor = color;

  // set color preference
  pSEQPrefs->setPrefColor("FOVColor", preferenceName(), m_fovColor);
}

void Map::setShowMapLines(bool val) 
{ 
  m_showMapLines = val; 
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowPlayer(bool val) 
{ 
  m_showPlayer = val; 
  
  QString tmpPrefString = "ShowPlayer";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showPlayer);

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowPlayerBackground(bool val) 
{ 
  m_showPlayerBackground = val; 
  
  QString tmpPrefString = "ShowPlayerBackground";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showPlayerBackground);

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowPlayerView(bool val) 
{ 
  m_showPlayerView = val; 

  QString tmpPrefString = "ShowPlayerView";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showPlayerView);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowHeading(bool val) 
{ 
  m_showHeading = val; 
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowSpawns(bool val) 
{ 
  m_showSpawns = val; 

  QString tmpPrefString = "ShowSpawns";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showSpawns);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowSpawnPoints(bool val) 
{ 
  m_showSpawnPoints = val; 

  QString tmpPrefString = "ShowSpawnPoints";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showSpawnPoints);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowUnknownSpawns(bool val) 
{ 
  m_showUnknownSpawns = val; 

  QString tmpPrefString = "ShowUnknownSpawns";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showUnknownSpawns);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowDrops(bool val) 
{ 
  m_showDrops = val; 

  QString tmpPrefString = "ShowDroppedItems";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showDrops);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowDoors(bool val) 
{ 
  m_showDoors = val; 

  QString tmpPrefString = "ShowDoors";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showDoors);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowVelocityLines(bool val) 
{ 
  m_showVelocityLines = val; 
  
  QString tmpPrefString = "VelocityLines";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showVelocityLines);

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowDebugInfo(bool val) 
{ 
#ifdef DEBUG
  m_showDebugInfo = val; 
  
  QString tmpPrefString = "ShowDebugInfo";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showDebugInfo);

  if(!m_cacheChanges)
    refreshMap ();
#endif
}

void Map::setAnimate(bool val) 
{ 
  m_animate = val; 

  QString tmpPrefString = "AnimateSpawnMovement";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_animate);

  // this requires a reAdjust
  reAdjust();

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setCacheChanges(bool val) 
{ 
  m_cacheChanges = val; 

  QString tmpPrefString = "CacheChanges";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_cacheChanges);

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setSpawnDepthFilter(bool val)
{
  m_spawnDepthFilter = val; 
  
  QString tmpPrefString = "SpawnDepthFilter";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_spawnDepthFilter);

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setHighlightConsideredSpawns(bool val) 
{ 
  m_highlightConsideredSpawns = val; 

  QString tmpPrefString = "HighlightConsideredSpawns";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_highlightConsideredSpawns);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowTooltips(bool val) 
{ 
  m_showTooltips = val; 

  QString tmpPrefString = "ShowTooltips";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showTooltips);

  // make sure it's hidden if they hid it
  if (!m_showTooltips)
    m_mapTip->hide();
}

void Map::setWalkPathShowSelect(bool val) 
{ 
  m_walkpathshowselect = val; 

  QString tmpPrefString = "WalkPathShowSelect";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_walkpathshowselect);
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setPvP(bool val) 
{ 
  m_pvp = val; 

  pSEQPrefs->setPrefBool("PvP", preferenceName(), m_pvp);
  
  // Can only have one style of PvP on at once
  if (val && m_deityPvP)
  {
    m_deityPvP = false;
    pSEQPrefs->setPrefBool("DeityPvP", preferenceName(), m_deityPvP);
  }
  if (val && m_racePvP)
  {
    m_racePvP = false;
    pSEQPrefs->setPrefBool("RacePvP", preferenceName(), m_racePvP);
  }

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setDeityPvP(bool val) 
{ 
  m_deityPvP = val; 

  pSEQPrefs->setPrefBool("DeityPvP", preferenceName(), m_deityPvP);
  
  // Can only have one style of PvP on at once
  if (val && m_pvp)
  {
    m_pvp = false;
    pSEQPrefs->setPrefBool("PvP", preferenceName(), m_pvp);
  }
  if (val && m_racePvP)
  {
    m_racePvP = false;
    pSEQPrefs->setPrefBool("RacePvP", preferenceName(), m_racePvP);
  }

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setRacePvP(bool val) 
{ 
  m_racePvP = val; 
  
  pSEQPrefs->setPrefBool("RacePvP", preferenceName(), m_racePvP);

  if (val && m_pvp)
  {
    m_pvp = false;
    pSEQPrefs->setPrefBool("PvP", preferenceName(), m_pvp);
  }
  if (val && m_racePvP)
  {
    m_deityPvP = false;
    pSEQPrefs->setPrefBool("DeityPvP", preferenceName(), m_deityPvP);
  }

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setMapLineStyle(MapLineStyle style) 
{ 
  m_param.setMapLineStyle(style); 

  QString tmpPrefString = "MapLineStyle";
  pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(), m_param.mapLineStyle());
  
  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setZoom(int val) 
{ 
  if (m_player->id() != 1)
  {
    if (m_param.setZoom(val))
    {
      emit zoomChanged(m_param.zoom());

      // requires reAdjust
      reAdjust();
      
      if (!m_cacheChanges)
        refreshMap ();
    }
  }
}

void Map::setZoomDefault(int val) 
{ 
  if (val == m_param.zoomDefault())
    return;

  m_param.setZoomDefault(val);

  QString tmpPrefString = "ZoomDefault";
  pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(),  
            m_param.zoomDefault());

  emit zoomDefaultChanged(val);
}

void Map::setPanOffsetX(int val) 
{ 
  m_param.setPanX(val); 

  // this requires a reAdjust
  reAdjust();

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setPanOffsetY(int val) 
{ 
  m_param.setPanY(val); 

  // this requires a reAdjust
  reAdjust();

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setGridResolution(int val) 
{ 
  m_param.setGridResolution(val); 

  QString tmpPrefString = "GridResolution";
  pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(), m_param.gridResolution());

  reAdjust();

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setGridTickColor(const QColor& color) 
{ 
  m_param.setGridTickColor(color); 

  // set color preference
  pSEQPrefs->setPrefColor("GridTickColor", preferenceName(), m_param.gridTickColor());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setGridLineColor(const QColor& color) 
{ 
  m_param.setGridLineColor(color); 

  // set color preference
  pSEQPrefs->setPrefColor("GridLineColor", preferenceName(), m_param.gridLineColor());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setBackgroundColor(const QColor& color) 
{ 
  m_param.setBackgroundColor(color); 

  // set color preference
  pSEQPrefs->setPrefColor("BackgroundColor", preferenceName(), m_param.backgroundColor());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setFont(const QFont& font) 
{ 
  m_param.setFont(font); 

  QString tmpPrefString = "Font";
  pSEQPrefs->setPrefFont(tmpPrefString, preferenceName(), m_param.font());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setHeadRoom(int val) 
{ 
  m_param.setHeadRoom(val); 

  QString tmpPrefString = "HeadRoom";
  pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(), m_param.headRoom());

  emit headRoomChanged(m_param.headRoom());

  reAdjust();

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setFloorRoom(int val) 
{ 
  m_param.setFloorRoom(val); 

  QString tmpPrefString = "FloorRoom";
  pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(), m_param.floorRoom());

  emit floorRoomChanged(m_param.floorRoom());

  reAdjust();

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowBackgroundImage(bool val) 
{ 
  m_param.setShowBackgroundImage(val); 

  QString tmpPrefString = "ShowBackgroundImage";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_param.showBackgroundImage());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowLocations(bool val) 
{ 
  m_param.setShowLocations(val); 

  QString tmpPrefString = "ShowMapPoints";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_param.showLocations());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowLines(bool val) 
{ 
  m_param.setShowLines(val); 

  QString tmpPrefString = "ShowMapLines";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_param.showLines());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowGridLines(bool val) 
{ 
  m_param.setShowGridLines(val); 

  QString tmpPrefString = "ShowGridLines";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_param.showGridLines());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowGridTicks(bool val) 
{ 
  m_param.setShowGridTicks(val); 

  QString tmpPrefString = "ShowGridTicks";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_param.showGridTicks());

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setCacheAlwaysRepaint(bool val) 
{ 
  m_mapCache.setAlwaysRepaint(val); 

  QString tmpPrefString = "CacheAlwaysRepaint";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_mapCache.alwaysRepaint());
}

void Map::setShowZoneSafePoint(bool val)
{
  m_showZoneSafePoint = val;

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::setShowInstanceLocationMarker(bool val)
{
  m_showInstanceLocationMarker = val;

  QString tmpPrefString = "ShowInstanceLocationMarker";
  pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_showInstanceLocationMarker);

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::toggleMapLayerVisibility(QAction* layer)
{
  int layerNum = layer->data().value<int>();
  int layerChecked = layer->isChecked();

  m_param.setLayerVisibility(layerNum, layerChecked);

  QString tmpPrefString = "MapLayerMask";
  uint32_t mask = pSEQPrefs->getPrefInt(tmpPrefString, preferenceName(), 0xffffffff);
  if (layerChecked)
      mask |= (1 << layerNum);
  else
      mask &= ~(1 << layerNum);
  pSEQPrefs->setPrefInt(tmpPrefString, preferenceName(), mask);

  if(!m_cacheChanges)
    refreshMap();
}

void Map::dumpInfo(QTextStream& out)
{
  out << "[" << preferenceName() << "]" << ENDL;
  out << "Caption: " << windowTitle() << ENDL;
  out << "AnimateSpawnMovement: " << m_animate << ENDL;
  out << "VelocityLines: " << m_showVelocityLines << ENDL;
  out << "SpawnDepthFilter: " << m_showVelocityLines << ENDL;
  out << "FrameRate: " << m_frameRate << ENDL;
#ifdef DEBUG
  out << "ShowDebugInfo: " << m_showDebugInfo << ENDL;
#endif
  out << "ShowPlayer: " << m_showPlayer << ENDL;
  out << "ShowPlayerBackground: " << m_showPlayerBackground << ENDL;
  out << "ShowPlayerView: " << m_showPlayerView << ENDL;
  out << "ShowDroppedItems: " << m_showDrops << ENDL;
  out << "ShowDoors: " << m_showDoors << ENDL;
  out << "ShowSpawns: " << m_showSpawns << ENDL;
  out << "ShowFiltered: " << m_showFiltered << ENDL;
  out << "HighlightConsideredSpawns: " << m_highlightConsideredSpawns << ENDL;
  out << "ShowTooltips: " << m_showTooltips << ENDL;
  out << "WalkPathShowSelect: " << m_walkpathshowselect << ENDL;
  out << "FOVStyle: " << m_fovStyle << ENDL;
  out << "FOVMode: " << m_fovMode << ENDL;
  out << "FOVColor: " << m_fovColor.name() << ENDL;

  out << ENDL;
  out << "[" << preferenceName() << " Parameters]" << ENDL;
  out << "MapLineStyle: " << m_param.mapLineStyle() << ENDL;
  out << "ShowMapPoints: " << m_param.showLocations() << ENDL;
  out << "ShowMapLines: " << m_param.showLines() << ENDL;
  out << "ShowGridLines: " << m_param.showGridLines() << ENDL;
  out << "ShowGridTicks: " << m_param.showGridTicks() << ENDL;
  out << "ShowBackgroundImage: " << m_param.showBackgroundImage() << ENDL;
  out << "GridResolution: " << m_param.gridResolution() << ENDL; 
  out << "GridTickColor: " << m_param.gridTickColor().name() << ENDL;
  out << "GridLineColor: " << m_param.gridLineColor().name() << ENDL;
  out << "BackgroundColor: " << m_param.backgroundColor().name() << ENDL;
  out << "HeadRoom: " << m_param.headRoom() << ENDL;
  out << "FloorRoom: " << m_param.floorRoom() << ENDL;

  out << ENDL;
  m_mapIcons->dumpInfo(out);

  out << "[" << preferenceName() << " State]" << ENDL;
  out << "screenLength: width(" << m_param.screenLength().width()
      << ") height(" << m_param.screenLength().height() << ")" << ENDL;
  out << "screenBounds: top(" << m_param.screenBounds().top() 
      << ") bottom(" << m_param.screenBounds().bottom() 
      << ") left(" << m_param.screenBounds().left()
      << ") right(" << m_param.screenBounds().right() << ") " << ENDL;
  out << "screenCenter: x(" << m_param.screenCenter().x()
      << ") y(" << m_param.screenCenter().y() << ")" << ENDL;
  out << "zoomMapLength: width(" << m_param.zoomMapLength().width() 
      << ") height(" << m_param.zoomMapLength().height() << ")" << ENDL;
  out << "panOffsetX: " << m_param.panOffsetX() << ENDL;
  out << "panOffsetY: " << m_param.panOffsetY() << ENDL;
  out << "zoom: " << m_param.zoom() << ENDL;
  out << "ratio: " << m_param.ratio() << ENDL; 
  out << "ratioIFixPt: " << m_param.ratioIFixPt() 
      << " (q = " << MapParameters::qFormat << ")" << ENDL;
  out << "player: x(" << m_param.player().x() 
      << ") y(" << m_param.player().y() 
      << ") z(" << m_param.player().z() << ")" << ENDL;
  out << "playerOffset: x(" << m_param.playerOffset().x() 
      << ") y(" << m_param.playerOffset().y() << ")" << ENDL;
  out << "scaledFOVDistance: " << m_scaledFOVDistance << ENDL;
  out << "playerHeadRoom: " << m_param.playerHeadRoom() << ENDL;
  out << "playerFloorRoom: " << m_param.playerFloorRoom() << ENDL;
  out << "FollowMode: " << m_followMode << ENDL;
  out << "MapPanning: " << m_mapPanning << ENDL;
  out << "PvP: " << m_pvp << ENDL;
  out << "DeityPvP: " << m_deityPvP << ENDL;
  out << "RacePvP: " << m_racePvP << ENDL;
  out << "CacheAlwaysRepaint: " << m_mapCache.alwaysRepaint() << ENDL;
  out << ENDL;

#ifdef DEBUG
  out << "[" << preferenceName() << " Statistics]" << ENDL;
  if (m_showDebugInfo)
  {
    long totalTime = 0;
    float fps = 0.0;
    for (int i = 0; i < maxFrameTimes; i++)
      totalTime += m_frameTimes[i];
  
    fps = float(maxFrameTimes) / (totalTime / 1000.0);

    out << "Actual FPS: " << fps << ENDL;
  }
  out << "Paint Count: " << m_paintCount << ENDL;
  out << "Cache Paint Count: " << m_mapCache.paintCount() << ENDL;
  out << "Average Cache Paints per Map Paint: " <<
    double(m_mapCache.paintCount()) / double(m_paintCount) << ENDL;
  out << "Average Paint Time: " 
      << double(m_paintTimeSum) / double(m_paintCount) 
      << " milliseconds " << ENDL;
  out << ENDL;
#endif // DEBUG
}

void Map::showMapIconDialog()
{
  if (!m_mapIconDialog)
  {
    // first create the dialog
    m_mapIconDialog = new MapIconDialog(this);
    m_mapIconDialog->setObjectName(
            QString(windowTitle() + " Icon Dialog").toLatin1().data());

    // then pass it this objects map icons object
    m_mapIconDialog->setMapIcons(m_mapIcons);
  }

  // show the dialog
  m_mapIconDialog->show();
}

void Map::resizeEvent (QResizeEvent *qs)
{
#ifdef DEBUGMAP
   qDebug ("resizeEvent()");
#endif /* DEBUGMAP */
   m_param.setScreenSize(qs->size());

   m_offscreen = QPixmap(m_param.screenLength());

   reAdjust();

   QWidget::resizeEvent(qs);
}

void Map::reAdjustAndRefreshMap(void)
{
  // first,, readjust the map state
  reAdjust();

  // then, repaint the map
   repaint(mapRect());
}

void Map::refreshMap(void)
{
#ifdef DEBUGMAP
   qDebug ("refreshMap()");
#endif /* DEBUGMAP */
   repaint(mapRect());
}

void Map::reAdjust()
{
  switch (m_followMode)
  {
  case tFollowSpawn:
    // only follow spawns that exists and are spawns, all others are nonsense
    if ((m_selectedItem != NULL) && (m_selectedItem->type() == tSpawn))
    {
      // following spawn, get it's approximate location
      EQPoint location;
      ((const Spawn*)m_selectedItem)->approximatePosition(m_animate,
                                    QTime::currentTime(),
                                    location);

      // adjust around it's location
      m_param.reAdjust(&location);
      break;
    }

    // no more target, change target mode back to follow player
    m_followMode = tFollowPlayer;

    // fall thru to next case since it's the new mode
  case tFollowPlayer:
  { 
    // retrieve the approximate current player position
    MapPoint targetPoint;
    m_player->approximatePosition(m_animate, QTime::currentTime(), 
                  targetPoint);

    // adjust around players location
    m_param.reAdjust(&targetPoint);
  }
  break;
  case tFollowNone:
    m_param.reAdjust(NULL);
    break;
  }

  switch (m_fovMode)
  {
  case tFOVDistanceBased:
    // scaled FOV Distance (m_fovDistance * scale)
    m_scaledFOVDistance = fixPtMulII(m_param.ratioIFixPt(), 
                     MapParameters::qFormat,
                     m_mapIcons->fovDistance());
    break;
  case tFOVScaledClassic:
    m_scaledFOVDistance = m_mapIcons->fovDistance() * m_param.zoom();
    break;
  case tFOVClassic:
    m_scaledFOVDistance = m_mapIcons->fovDistance();
    break;
  }
}

void Map::createNewLayer()
{
    m_mapMgr->createNewLayer();
}

void Map::addLocation(void)
{
#ifdef DEBUGMAP
  qDebug ("addLocation()");
#endif /* DEBUGMAP */

  // get it's approximage location
  MapPoint point;
  m_player->approximatePosition(m_animate, QTime::currentTime(), point);

#ifdef DEBUGMAP
  seqDebug("addLocation() point(%d, %d, %d)", point.x(), point.y(), point.z());
#endif

  // add the location
  m_mapCache.forceRepaint();
  m_mapMgr->addLocation(this, point);
}

void Map::startLine (void)
{
#ifdef DEBUGMAP
  qDebug ("startLine()");
#endif /* DEBUGMAP */
  // get it's approximate position
  MapPoint point;
  m_player->approximatePosition(m_animate, QTime::currentTime(), point);

#ifdef DEBUGMAP
  seqDebug("startLine() point(%d, %d, %d)", point.x(), point.y(), point.z());
#endif

  // start the line using the player spawns position
  m_mapMgr->startLine(point);
}

void Map::addLinePoint() 
{
#ifdef DEBUGMAP
  qDebug ("addLinePoint()");
#endif /* DEBUGMAP */

  // get the player spawns approximate position
  MapPoint point;
  m_player->approximatePosition(m_animate, QTime::currentTime(), point);


#ifdef DEBUGMAP
  seqDebug("addLinePoint() point(%d, %d, %d)", point.x(), point.y(), point.z());
#endif

  // add it as the next line point
  m_mapCache.forceRepaint();
  m_mapMgr->addLinePoint(point);
}


void Map::delLinePoint(void)
{
#ifdef DEBUGMAP
  qDebug ("delLinePoint()");
#endif /* DEBUGMAP */

  m_mapCache.forceRepaint();
  m_mapMgr->delLinePoint();
} // END delLinePoint


void Map::showLineDlg(void)
{
  // show the line dialog
  m_mapMgr->showLineDlg(this);
}

void Map::scaleDownZ(QAction* item)
{
  m_mapCache.forceRepaint();
  m_mapMgr->scaleDownZ(item->data().value<int>());
}

void Map::scaleUpZ(QAction* item)
{
  m_mapCache.forceRepaint();
  m_mapMgr->scaleUpZ(item->data().value<int>());
}

void Map::addPathPoint() 
{

  // get the player spawns approximate position
  MapPoint point;
  m_player->approximatePosition(m_animate, QTime::currentTime(), point);

#ifdef DEBUGMAP
  FILE *f;
  f=fopen("/tmp/coords","at");
  if(f) {
    fprintf (f,"%f, %f\n",
             (double)point.x(),
             (double)point.y());
    fclose(f);
  }
#endif
}

QRect Map::mapRect () const
{
#ifdef DEBUGMAP
   seqDebug("mapRect()");
   static int rendercount = 0;
   rendercount++;
   seqDebug("%i, (0,0,%i,%i)",rendercount, width (), height ());
#endif /* DEBUGMAP */
   QRect r (0, 0, width (), height ());
   r.moveBottomLeft (rect ().bottomLeft ());
#ifdef DEBUGMAP
   seqDebug("hmm2");
   rendercount--;
#endif /* DEBUGMAP */
   return r;
}

//----------------------------------------------------------------------
void Map::paintMap (QPainter * p)
{
#ifdef DEBUGMAP
  qDebug ("paintMap()");
#endif /* DEBUGMAP */
  QPainter tmp;
  
  QTime drawTime;

  // get the current time
  drawTime.start();

  EQPoint playerPos;

  // retrieve the approximate current player position, and set the 
  // parameters player position to it.
  m_player->approximatePosition(m_animate, drawTime, playerPos);
  m_param.setPlayer(playerPos);
  
  // make sure the player stays visible
  if ((m_param.zoom() > 1) &&
      ((m_followMode == tFollowPlayer) &&
       (!inRect(m_param.screenBounds(), 
        playerPos.x(), playerPos.y()))))
    reAdjust();

  // if following a spawn, and there is a spawn, make sure it's visible.
  if ((m_followMode == tFollowSpawn) &&
      (m_param.zoom() > 1) &&
      (m_selectedItem != NULL) && 
      (m_selectedItem->type() == tSpawn))
  {
    EQPoint location;
    
    ((const Spawn*)m_selectedItem)->approximatePosition(m_animate, 
                            drawTime, 
                            location);
    
    if (!inRect(m_param.screenBounds(), playerPos.x(), playerPos.y()))
      reAdjust();
  }

  // copy the background
  const QPixmap& tmpPix = m_mapCache.getMapImage(m_param);
  m_offscreen = tmpPix.copy(0, 0, tmpPix.width(), tmpPix.height());


  //Now, if we're animating, allow player to walk off. Grr, centering issue.

  /* Begin painting */
  tmp.begin (&m_offscreen);
  tmp.setPen (Qt::NoPen);
  tmp.setFont (m_param.font());

  if (m_player->validPos() && !m_zoneMgr->isZoning() && m_player->id() != 0)
  {
    if (m_showPlayerBackground)
      paintPlayerBackground(m_param, tmp);
    
    if (m_showPlayerView && ! m_player->isCorpse())
      paintPlayerView(m_param, tmp);
    
    if (m_showPlayer)
      paintPlayer(m_param, tmp);
  }

  if (m_showDrops)
    paintDrops(m_param, tmp);

  if (m_showZoneSafePoint)
  {
    const Point3D<int16_t>& safePoint = m_zoneMgr->safePoint();
    m_mapIcons->paintIcon(m_param, tmp, 
              m_mapIcons->icon(tIconTypeZoneSafePoint),
              safePoint, QString("Safe Point"),
              QPoint(m_param.calcXOffsetI(safePoint.x()),
                 m_param.calcYOffsetI(safePoint.y())));
  }

  if (m_showDoors)
    paintDoors(m_param, tmp);

  if (m_showSpawnPoints)
    paintSpawnPoints(m_param, tmp);

  if (m_showSpawns)
    paintSpawns(m_param, tmp, drawTime);

  if(m_showInstanceLocationMarker && m_zoneMgr->dzID())
  {
     const Point3D<int16_t>& instancePoint = m_zoneMgr->dzPoint();
     m_mapIcons->paintIcon(m_param, tmp, m_mapIcons->icon(tIconTypeDynamicZoneLocation),
                           instancePoint, m_zoneMgr->dzLongName(), QPoint(m_param.calcXOffsetI(instancePoint.x()),
                                 m_param.calcYOffsetI(instancePoint.y())));
  }

  paintSelectedSpawnSpecials(m_param, tmp, drawTime);
  paintSelectedSpawnPointSpecials(m_param, tmp, drawTime);

#ifdef DEBUG
  // increment paint count
  m_paintCount++;

  // get paint time
  int paintTime = drawTime.elapsed();
  
  // add paint time to sum
  m_paintTimeSum += paintTime;

  //--------------------------------------------------
   if (m_showDebugInfo)
   {
     long totalTime = 0;
     float fps = 0.0;

     m_frameTimes[m_frameTimeIndex] = m_time.elapsed();
     m_frameTimeIndex = (m_frameTimeIndex + 1) % maxFrameTimes;

     for (int i = 0; i < maxFrameTimes; i++)
       totalTime += m_frameTimes[i];

     fps = float(maxFrameTimes) / (totalTime / 1000.0);

     // paint the debug info
     paintDebugInfo(m_param, tmp, fps, paintTime);

     // reset the time
     m_time.restart();
   }
#endif

   //--------------------------------------------------
   // finished painting
   tmp.end ();
   // draw to the widget
   p->drawImage(
           QRect(0, 0, m_offscreen.width(), m_offscreen.height()), //target rect
           m_offscreen.toImage(),
           QRect(0, 0, m_offscreen.width(), m_offscreen.height()), //source rect
           Qt::AutoColor);

}

void Map::paintPlayerBackground(MapParameters& param, QPainter& p)
{
  /* Paint player position background */
  p.setPen (m_fovColor);

  QBrush tmpBrush;
  tmpBrush.setColor(m_fovColor);
  tmpBrush.setStyle((Qt::BrushStyle)m_fovStyle);
  p.setBrush(tmpBrush);
  if(m_fovStyle == Qt::SolidPattern) p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  // sizeWH is 2 * centerOffset
  int sizeWH = m_scaledFOVDistance << 1; 

  // FOV Distance 
  p.drawEllipse (m_param.playerXOffset() - m_scaledFOVDistance, 
         m_param.playerYOffset() - m_scaledFOVDistance, 
         sizeWH, sizeWH);
  
  if(m_fovStyle == Qt::SolidPattern) 
    p.setCompositionMode(QPainter::CompositionMode_Source);
  
}

void Map::paintPlayerView(MapParameters& param, QPainter& p)
{
  /* Paint the player direction */
#ifdef DEBUGMAP
  seqDebug("Paint the player direction");
#endif
  
  int const player_circle_radius = 4;
  
  int16_t playerAngle = m_player->headingDegrees();
  if (playerAngle != -1)
  {
    double const pi = 3.14159265358979323846;
    double const radians_per_circle = pi * 2;
    double const angle = (360 - playerAngle - 180) / 360.0 * radians_per_circle;
    int start_offset_x = int(sin( angle - radians_per_circle * 0.25 ) * player_circle_radius);
    int start_offset_y = int(cos( angle - radians_per_circle * 0.25 ) * player_circle_radius);
    double const fov_angle = radians_per_circle * 0.25;
    double fox_angle_offset = fov_angle / 2;
    
    p.setPen(Qt::yellow); // color
    p.drawLine( m_param.playerXOffset(), m_param.playerYOffset(),
        m_param.playerXOffset() + int (sin( angle ) * m_scaledFOVDistance),
        m_param.playerYOffset() + int (cos( angle ) * m_scaledFOVDistance) );
    
    p.setPen(Qt::red); // color
    for ( int n = 2; n--; )
    {
      int const start_x = m_param.playerXOffset() + start_offset_x;
      int const start_y = m_param.playerYOffset() + start_offset_y;
      
      p.drawLine( start_x, start_y,
          start_x + int (sin( angle - fox_angle_offset ) * m_scaledFOVDistance),
          start_y + int (cos( angle - fox_angle_offset ) * m_scaledFOVDistance) );
      start_offset_x *= -1;
      start_offset_y *= -1;
      fox_angle_offset *= -1;
    }
  }
}

void Map::paintPlayer(MapParameters& param, QPainter& p)
{
#ifdef DEBUGMAP
    seqDebug("Paint player position");
#endif

    if (! m_player->isCorpse())
    {
        // White dot for non-corpse
        p.setPen(Qt::gray);
        p.setBrush(Qt::white);
        p.drawEllipse(m_param.playerXOffset() - 3, 
            m_param.playerYOffset() - 3, 6, 6);
    }
    else
    {
        // Corpse icon for a corpse.
        MapIcon mapIcon = m_mapIcons->icon(tIconTypeSpawnPlayerCorpse);

        p.setPen(mapIcon.highlightPen());
        p.setBrush(mapIcon.highlightBrush());

        m_mapIcons->paintItemIcon(param, p, mapIcon, m_player,
            param.playerOffset());
    }
}

void Map::paintDrops(MapParameters& param,
             QPainter& p)
{
#ifdef DEBUGMAP
  seqDebug("Paint the dropped items");
#endif
  const ItemMap& itemMap = m_spawnShell->drops();
  ItemConstIterator it(itemMap);
  const Item* item;
  const QRect& screenBounds = m_param.screenBounds();
  MapIcon mapIcon;
  uint32_t filterFlags;
  uint8_t flag;

  // all drops are the same color
  p.setPen(Qt::yellow);

  /* Paint the dropped items */
  while (it.hasNext())
  {
    it.next();
    // get the item from the list
    item = it.value();
    if (!item)
        break;

    filterFlags = item->filterFlags();
 
    // make sure drop is within bounds
    if (!inRect(screenBounds, item->x(), item->y()) ||
        (m_spawnDepthFilter &&
         ((item->z() > m_param.playerHeadRoom()) ||
          (item->z() < m_param.playerFloorRoom()))) || 
        (!m_showFiltered && (filterFlags & FILTER_FLAG_FILTERED)))
      continue;

    mapIcon = m_mapIcons->icon(tIconTypeDrop);
    
    // only bother checking for specific flags if any are set...
    if (filterFlags != 0)
    {
      for (int i = 0; i < SIZEOF_FILTERS; i++)
      {
        flag = m_filterCheckOrdering[i];
        if (filterFlags & (1 << flag))
          mapIcon.combine(m_mapIcons->icon(tIconTypeFilterFlagBase + flag));
      }
    }
    
    // check runtime filter flags
    if(item->runtimeFilterFlags() & m_runtimeFilterFlagMask)
      mapIcon.combine(m_mapIcons->icon(tIconTypeRuntimeFiltered));
    
    // paint the icon
    m_mapIcons->paintItemIcon(param, p, mapIcon, item,
                  QPoint(param.calcXOffsetI(item->x()),
                     param.calcYOffsetI(item->y())));
  }
}

void Map::paintDoors(MapParameters& param,
             QPainter& p)
{
#ifdef DEBUGMAP
  seqDebug("Paint the door items");
#endif
  const ItemMap& itemMap = m_spawnShell->doors();
  ItemConstIterator it(itemMap);
  const Door* item;
  const QRect& screenBounds = m_param.screenBounds();
  MapIcon mapIcon;
  uint32_t filterFlags;
  uint8_t flag;

  // doors only come in one color
  p.setPen(QColor (110, 60, 0));

  /* Paint the door items */
  while (it.hasNext())
  {
    it.next();
    // get the item from the list
    item = (const Door*)it.value();
    if (!item)
        break;

    filterFlags = item->filterFlags();

    // make sure doors are within bounds
    if (!inRect(screenBounds, item->x(), item->y()) ||
        (m_spawnDepthFilter &&
         ((item->z() > m_param.playerHeadRoom()) ||
          (item->z() < m_param.playerFloorRoom()))) || 
        (!m_showFiltered && (filterFlags & FILTER_FLAG_FILTERED)))
      continue;

    mapIcon = m_mapIcons->icon(tIconTypeDoor);

    // add zone door effects
    if (item->zonePoint() != 0xFFFFFFFF)
      mapIcon.combine(m_mapIcons->icon(tIconTypeZoneDoor));

    // only bother checking for specific flags if any are set...
    if (filterFlags != 0)
    {
      for (int i = 0; i < SIZEOF_FILTERS; i++)
      {
        flag = m_filterCheckOrdering[i];
        if (filterFlags & (1 << flag))
          mapIcon.combine(m_mapIcons->icon(tIconTypeFilterFlagBase + flag));
      }
    }

    // check runtime filter flags
    if(item->runtimeFilterFlags() & m_runtimeFilterFlagMask)
      mapIcon.combine(m_mapIcons->icon(tIconTypeRuntimeFiltered));

    // paint the icon
    m_mapIcons->paintItemIcon(param, p, mapIcon, item, 
                  QPoint(param.calcXOffsetI(item->x()),
                     param.calcYOffsetI(item->y())));
  }
}             

const QColor Map::raceTeamHighlightColor(const Spawn* spawn) const
{
  uint8_t playerLevel = m_player->level();
  int diff = spawn->level() - playerLevel;
  if (diff < -8)  //They are much easier than you.
    return Qt::green; 
  if (diff > 8)  //They are much harder than you.
    return Qt::darkRed; 

  if (diff < 0) 
    diff *= -1;
  
  // if we are within 8 levels of other player
  if (diff <= 8)
  {
    // they are in your range
    switch ( (spawn->level() - playerLevel) + 8)
    {
    // easy
    case 0:  // you are 8 above them
    case 1:  // you are 7 above them
      return Qt::green; 
      break;
    case 2:  // you are 6 above them
    case 3:  // you are 5 above them
      return Qt::darkGreen; 
      break;
      
    // moderate
    case 4:  // you are 4 above them
    case 5:  // you are 3 above them
      return Qt::blue; 
      break;
    case 6:  // you are 2 above them
    case 7:  // you are 1 above them
      return Qt::darkBlue; 
      break;
      
    // even
    case 8:  // you are even with them
      return Qt::white; 
      break;
        
    // difficult 
    case 9:  // you are 1 below them
    case 10:  // you are 2 below them
      return Qt::yellow; 
      break;
        
    // downright hard
    case 11:  // you are 3 below them
    case 12:  // you are 4 below them
      return Qt::magenta; 
      break;
    case 13:  // you are 5 below them
    case 14:  // you are 6 below them
      return Qt::red; 
      break;
    case 15:  // you are 7 below them
    case 16:  // you are 8 below them
      return Qt::darkRed; 
      break;
    }
  }
  
  return Qt::black;
}

const QColor Map::deityTeamHighlightColor(const Spawn* spawn) const
{
  uint8_t playerLevel = m_player->level();
  int diff = spawn->level() - playerLevel;
  if (diff < -5)  //They are much easier than you.
    return Qt::green; 
  if (diff > 5)  //They are much harder than you.
    return Qt::darkRed; 

  if (diff < 0) 
    diff *= -1;

  // if we are within 8 levels of other player
  if (diff <= 5)
  {
    // they are in your range
    switch ( (spawn->level() - playerLevel) + 5)
    {
    // easy
    case 0:  // you are 5 above them
    case 1:  // you are 4 above them
      return Qt::green; 
      break;
    case 2:  // you are 3 above them
      return Qt::darkGreen; 
      break;
      
    // moderate
    case 3:  // you are 2 above them
      return Qt::blue; 
      break;
    case 4:  // you are 1 above them
      return Qt::darkBlue; 
      break;
      
    // even
    case 5:  // you are even with them
      return Qt::white; 
      break;
      
    // difficult 
    case 6:  // you are 1 below them
      return Qt::yellow; 
      break;
      
      // downright hard
    case 7:  // you are 2 below them
    case 8:  // you are 3 below them
      return Qt::magenta; 
      break;
    case 9:  // you are 4 below them
      return Qt::red; 
      break;
    case 10:  // you are 5 below them
      return Qt::darkRed; 
      break;
    }
  }

  return Qt::black;
}

void Map::paintSpawns(MapParameters& param,
              QPainter& p,
              const QTime& drawTime)
{
#ifdef DEBUGMAP
  seqDebug("Paint the spawns");
#endif
  const ItemMap& itemMap = m_spawnShell->spawns();
  ItemConstIterator it(itemMap);
  const Item* item;
  QPolygon  atri(3);
  QString spawnNameText;
  QFontMetrics fm(param.font());
  EQPoint spawnOffset;
  EQPoint location;
  QPen tmpPen;
  uint8_t flag;
  int spawnOffsetXPos, spawnOffsetYPos;
  uint16_t range;
  int scaledRange;
  int sizeWH;
  uint32_t filterFlags;
  const QRect& screenBounds = m_param.screenBounds();
  MapIcon mapIcon;
  bool up2date = false;

  /* Paint the spawns */
  const Spawn* spawn;
  // iterate over all spawns in of the current type
  while (it.hasNext())
  {
    it.next();
    // get the item from the list
    item = it.value();
    if (!item)
        break;

#ifdef DEBUGMAP
    spawn = spawnType(item);
    
    if (spawn == NULL)
    {
      seqWarn("Got non Spawn from iterator over type tSpawn (Tyep:%d ID: %d)!",
          item->type(), item->id());
      continue;
    }
#else
    // since only things of type Spawn should be in the list, 
    // just do a quicky conversion
    spawn = (const Spawn*)item;
#endif

    filterFlags = item->filterFlags();

    if (((m_spawnDepthFilter &&
         ((item->z() > m_param.playerHeadRoom()) ||
          (item->z() < m_param.playerFloorRoom()))) || 
        (!m_showFiltered && (filterFlags & FILTER_FLAG_FILTERED)) ||
        (!m_showUnknownSpawns && spawn->isUnknown())) &&
        (item != m_selectedItem))
      continue;
 
    // get the approximate position of the spawn
    up2date = spawn->approximatePosition(m_animate, drawTime, location);
    
    // check that the spawn is within the screen bounds
    if (!inRect(screenBounds, location.x(), location.y()))
      continue; // not in bounds, next...
    
    // calculate the spawn's offset location
    spawnOffsetXPos = m_param.calcXOffsetI(location.x());
    spawnOffsetYPos = m_param.calcYOffsetI(location.y());
    QPoint point(spawnOffsetXPos, spawnOffsetYPos);
    
    
    //--------------------------------------------------
#ifdef DEBUGMAP
    printf("Draw velocities\n");
#endif
    /* Draw velocities */
    if (m_showVelocityLines &&
        (spawn->deltaX() || spawn->deltaY())) // only if has a delta
    {
      p.setPen (Qt::darkGray);
      p.drawLine (spawnOffsetXPos,
          spawnOffsetYPos,
          spawnOffsetXPos - spawn->deltaX(),
          spawnOffsetYPos - spawn->deltaY());
    }
    
    //
    // Misc decorations
    //
    
    //--------------------------------------------------
#ifdef DEBUGMAP
    printf("Draw corpse, team, and filter boxes\n");
#endif
    // handle regular NPC's first, since they are generally the most common
    if (spawn->isNPC())
    {
      mapIcon = m_mapIcons->icon(tIconTypeSpawnNPC);
      
      // retrieve the spawns aggro range
      range = m_mapMgr->spawnAggroRange(spawn);
      
      // if aggro range is known (non-zero), draw the aggro range circle
      if (range != 0)
      {
        scaledRange = fixPtMulII(m_param.ratioIFixPt(), 
                     MapParameters::qFormat, range);
        sizeWH = scaledRange << 1;
        
        p.setBrush(Qt::NoBrush);
        p.setPen(Qt::red); 
        
        p.drawEllipse(spawnOffsetXPos - scaledRange, 
                  spawnOffsetYPos - scaledRange, 
                  sizeWH, 
                  sizeWH);
      }
    }
    else if (spawn->isOtherPlayer())
    {
//      if (!up2date)
//       mapIcon = m_mapIcons->icon(tIconTypeSpawnPlayerOld);
//      else
        mapIcon = m_mapIcons->icon(tIconTypeSpawnPlayer);
    }
    else if (spawn->NPC() == SPAWN_NPC_CORPSE) // x for NPC corpse
      mapIcon = m_mapIcons->icon(tIconTypeSpawnNPCCorpse);
    else if (spawn->NPC() == SPAWN_PC_CORPSE) // x for PC corpse
      mapIcon = m_mapIcons->icon(tIconTypeSpawnPlayerCorpse);
    else if (spawn->isUnknown())
      mapIcon = m_mapIcons->icon(tIconTypeSpawnUnknown);

    if (spawn->isNotUpdated())
    {
      if(spawn->isNPC())
        mapIcon = m_mapIcons->icon(tIconTypeSpawnNPCNoUpdate);
      else if (spawn->isOtherPlayer())
        mapIcon = m_mapIcons->icon(tIconTypeSpawnPlayerNoUpdate);
    }

    // if the spawn was considered, note it.
    if (m_highlightConsideredSpawns && spawn->considered())
      mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnConsidered));
    
     // only bother checking for specific flags if any are set...
    if (filterFlags != 0)
    {
      for (int i = 0; i < SIZEOF_FILTERS; i++)
      {
        flag = m_filterCheckOrdering[i];
        if (filterFlags & (1 << flag))
          mapIcon.combine(m_mapIcons->icon(tIconTypeFilterFlagBase + flag));
      }
    }
    
    // check runtime filter flags
    if(spawn->runtimeFilterFlags() & m_runtimeFilterFlagMask)
      mapIcon.combine(m_mapIcons->icon(tIconTypeRuntimeFiltered));
     
    // if PvP is not enabled, don't try to do it, 
    // paint the current spawn and continue to the next
    if (! (m_racePvP || m_pvp || m_deityPvP))
    {
      m_mapIcons->paintSpawnIcon(param, p, mapIcon, spawn, location, point);
      continue;
    }
    
    //--------------------------------------------------
#ifdef DEBUGMAP
    printf("PvP handling\n");
#endif
    
    const Spawn* owner;
    
    if (spawn->petOwnerID() != 0)
      owner = spawnType(m_spawnShell->findID(tSpawn, spawn->petOwnerID()));
    else 
      owner = NULL;
    
    if (m_pvp)
    {
      // New Combined Zek rules can only PvP +/- 4 levels from you. So
      // if spawn isn't you...
      if (spawn->isOtherPlayer())
      {
        int levelDiff = m_player->level() - spawn->level();
        
        if (abs(levelDiff) < 5)
        {
          // Gank away!
          mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnPlayerPvPEnabled));

          QPen p2(mapIcon.highlightPen());
          
          if (levelDiff < 0)
          {
            p2.setColor(Qt::yellow);
          }
          else if (levelDiff > 0)
          {
            p2.setColor(Qt::blue);
          }
          else
          {
            p2.setColor(Qt::white);
          }
          mapIcon.setHighlightPen(p2);
        }
      }
      else
      {
        // Pet owned by someone who is pvp?
        if (owner != NULL)
        {
          int levelDiff = m_player->level() - owner->level();

          if (abs(levelDiff) < 5)
          {
            // Gank away! Add color circle.
            mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnPetPvPEnabled));

            QPen p2(mapIcon.highlightPen());
            
            if (levelDiff < 0)
            {
              p2.setColor(Qt::yellow);
            }
            else if (levelDiff > 0)
            {
              p2.setColor(Qt::blue);
            }
            else
            {
              p2.setColor(Qt::white);
            }
            mapIcon.setHighlightPen(p2);
          }
        }
      }
    }
    else if (m_racePvP)
    {
      // if spawn is another pc, on a different team, and within 8 levels
      // highlight it flashing
      if (spawn->isOtherPlayer())
      {
        mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnPlayerTeamBase -1 +
                     spawn->raceTeam()));
     
        // if not the same team as us
        if (!m_player->isSameRaceTeam(spawn))
        {
          mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnPlayerTeamOtherRace));
          QPen p2(mapIcon.highlightPen());
          p2.setColor(raceTeamHighlightColor(spawn));
          mapIcon.setHighlightPen(p2);
        }
      } // if decorate pvp
      // circle around pvp pets
      else if ((owner != NULL) && !m_player->isSameRaceTeam(owner))
        mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnPlayerTeamOtherRacePet));
    } // end racePvp
    else if (m_deityPvP)
    {
      if (spawn->isOtherPlayer())
      {
        mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnPlayerTeamBase -1 + 
                         spawn->deityTeam()));
     
        // if not the same team as us
        if (!m_player->isSameDeityTeam(spawn))
        {
          mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnPlayerTeamOtherDeity));
          QPen p2(mapIcon.highlightPen());
          p2.setColor(deityTeamHighlightColor(spawn));
          mapIcon.setHighlightPen(p2);
        }
      } // if decorate pvp
     // circle around deity pvp pets
      else if ((owner != NULL) && !m_player->isSameDeityTeam(owner))
        mapIcon.combine(m_mapIcons->icon(tIconTypeSpawnPlayerTeamOtherDeityPet));
    } // end if deityPvP
    
    // paint the spawn icon
    m_mapIcons->paintSpawnIcon(param, p, mapIcon, spawn, location, point);
  } // end for spawns

  //----------------------------------------------------------------------
#ifdef DEBUGMAP
  seqDebug("Done drawing spawns");
#endif
}

void Map::paintSelectedSpawnSpecials(MapParameters& param, QPainter& p,
                     const QTime& drawTime)
{
  if (m_selectedItem == NULL)
    return;

#ifdef DEBUGMAP
  seqDebug("Draw the line of the selected spawn");
#endif
  EQPoint location;

  if (m_selectedItem->type() == tSpawn)
  {
    ((const Spawn*)m_selectedItem)->approximatePosition(m_animate, 
                            drawTime, 
                            location);
    m_mapIcons->paintSpawnIcon(param, p, m_mapIcons->icon(tIconTypeItemSelected), 
                  (Spawn*)m_selectedItem, location, 
                  QPoint(m_param.calcXOffsetI(location.x()), 
                     m_param.calcYOffsetI(location.y())));
  }
  else if (m_selectedItem->type() == tPlayer)
  {
    m_mapIcons->paintSpawnIcon(param, p, m_mapIcons->icon(tIconTypeItemSelected), 
                  (Spawn*)m_selectedItem, *m_selectedItem, 
                  QPoint(m_param.calcXOffsetI(m_selectedItem->x()), 
                     m_param.calcYOffsetI(m_selectedItem->y())));
  }
  else
    m_mapIcons->paintItemIcon(param, p, m_mapIcons->icon(tIconTypeItemSelected), 
                  m_selectedItem, 
                  QPoint(param.calcXOffsetI(m_selectedItem->x()),
                     param.calcYOffsetI(m_selectedItem->y())));
}

void Map::paintSelectedSpawnPointSpecials(MapParameters& param, 
                      QPainter& p,
                      const QTime& drawTime)
{
  const SpawnPoint* sp;

  // blue flashing square around selected spawn point
  sp = m_spawnMonitor->selected();

  // if no spawn point selected, or not showing a line to selected 
  // spawn point and not flashing, just return
  if (sp == NULL)
    return;

  m_mapIcons->paintSpawnPointIcon(m_param, p, 
                 m_mapIcons->icon(tIconTypeSpawnPointSelected), sp,
                 QPoint(param.calcXOffsetI(sp->x()),
                    param.calcYOffsetI(sp->y())));
}

void Map::paintSpawnPoints( MapParameters& param, QPainter& p )
{
  const QRect& screenBounds = m_param.screenBounds();
  
  // get the spawn point count
  long count = m_spawnMonitor->spawnPoints().count();
  
  if (!count )
    return;
  
  // get an iterator over the list of spawn points
  QHashIterator<QString, SpawnPoint*> it(m_spawnMonitor->spawnPoints());
  const SpawnPoint* sp;

  const MapIcon& mapIcon = m_mapIcons->icon(tIconTypeSpawnPoint);

  // iterate over the list of spawn points
  while (it.hasNext())
  {
    it.next();
    sp = it.value();

    // make sure spawn point is within bounds
    if (!inRect(screenBounds, sp->x(), sp->y()) ||
        (m_spawnDepthFilter &&
         ((sp->z() > m_param.playerHeadRoom()) ||
          (sp->z() < m_param.playerFloorRoom()))))
      continue;

    m_mapIcons->paintSpawnPointIcon(m_param, p, mapIcon, sp,
                   QPoint(param.calcXOffsetI(sp->x()),
                      param.calcYOffsetI(sp->y())));
  }
}


void Map::paintDebugInfo(MapParameters& param, 
             QPainter& p, 
             float fps, 
             int drawTime)
{
  // show coords of upper left corner and lower right corner
  p.setPen( Qt::yellow );
  QString ts;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  ts = QString::asprintf( "(%d, %d)", 
          (int)(param.screenCenterX() * param.ratioX()), 
          (int)(param.screenCenterY() * param.ratioY()));
#else
  ts.sprintf( "(%d, %d)", 
          (int)(param.screenCenterX() * param.ratioX()), 
          (int)(param.screenCenterY() * param.ratioY()));
#endif
  p.drawText( 10, 8, ts );
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  ts = QString::asprintf( "(%d, %d)",
          (int)((param.screenCenterX() - param.screenLength().width()) * 
            param.ratioX()),
          (int)((param.screenCenterY() - param.screenLength().height()) *
            param.ratioY()));
#else
  ts.sprintf( "(%d, %d)",
          (int)((param.screenCenterX() - param.screenLength().width()) * 
            param.ratioX()),
          (int)((param.screenCenterY() - param.screenLength().height()) *
            param.ratioY()));
#endif
  p.drawText( width() - 90, height() - 14, ts );
  
  // show frame times
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  ts = QString::asprintf( "%2.0ffps/%dms", fps, drawTime);
#else
  ts.sprintf( "%2.0ffps/%dms", fps, drawTime);
#endif
  p.drawText( this->width() - 60, 8, ts );
}

void Map::paintEvent (QPaintEvent * e)
{
#ifdef DEBUGMAP
   qDebug ("paintEvent()");
#endif /* DEBUGMAP */
   QRect updateR = e->rect ();
   QPainter p;
   p.begin (this);
   if (updateR.intersects (mapRect ()))
     paintMap (&p);
   p.end ();
}

void Map::mouseMoveEvent( QMouseEvent* event )
{
  // We're moving the map around, only try to move if we are in zoom mode
  // Also, since the mouse is more sensitive, pan a little slower.
  if (m_mapPanning && m_param.zoom() > 1) 
  {
    const QPoint    curpoint    = event->pos();
    
    if (curpoint.x() > m_mapPanX)
      m_param.panX(-3 * panAmmt);
    else if (curpoint.x() < m_mapPanX)
      m_param.panX(3 * panAmmt);
    
    if (curpoint.y() > m_mapPanY)
      m_param.panY(-3 * panAmmt);
    else if (curpoint.y() < m_mapPanY)
      m_param.panY(3 * panAmmt);
    
    m_mapPanX = curpoint.x();
    m_mapPanY = curpoint.y();
    
    emit panXChanged(m_param.panOffsetX());
    emit panYChanged(m_param.panOffsetY());
    
    reAdjust();

    if(!m_cacheChanges)
      refreshMap();
  }
  
  emit mouseLocation(m_param.invertXOffset(event->x()),
             m_param.invertYOffset(event->y()));
  
  if ( !m_showTooltips)
  {
    m_mapTip->hide();
    return;
  }
  
  if ( m_mapPanning && (m_param.zoom() > 1))
    return;
  
  uint32_t dist = 5;
  // check for closest spawn
  const Item* item = closestSpawnToPoint(event->pos(), dist);

  // check for closest spawn point
  const SpawnPoint* sp = NULL;
  if (m_showSpawnPoints)
    sp = closestSpawnPointToPoint(event->pos(), dist);

  // spawn point was closer, display it's info
  if (sp != NULL)
  {
    QString remaining;
    if ( sp->diffTime() == 0 || sp->deathTime() == 0 )
      remaining = "\277 ?";  
    else
    {
      long secs = sp->secsLeft();
    
      if ( secs > 0 )
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    remaining = QString::asprintf( "%2ld:%02ld", secs / 60, secs % 60  );
#else
    remaining.sprintf( "%2ld:%02ld", secs / 60, secs % 60  );
#endif
      else
    remaining = "now"; 
    }

    // construct and set the spawned string
    QString spawned;
    QDateTime dateTime;
    dateTime.setTime_t(sp->spawnTime());
    QDate createDate = dateTime.date();

    // spawn time
    if ( createDate != QDate::currentDate() )
      spawned = createDate.shortDayName( createDate.dayOfWeek() ) + " ";
    
    spawned += dateTime.time().toString();
    
    QString string;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    string = QString::asprintf("SpawnPoint: %s\n"
           "%.3s/Z: %5d/%5d/%5d\n"
           "Last: %s\n"
           "Spawned: %s\t Remaining: %s\t Count: %d",
           sp->name().toLatin1().data(),
           showeq_params->retarded_coords ? "Y/X" : "X/Y",
           showeq_params->retarded_coords ? sp->y() : sp->x(),
           showeq_params->retarded_coords ? sp->x() : sp->y(),
           sp->z(),
           sp->last().toLatin1().data(),
           spawned.toLatin1().data(),
           remaining.toLatin1().data(),
           sp->count());
#else
    string.sprintf("SpawnPoint: %s\n"
           "%.3s/Z: %5d/%5d/%5d\n"
           "Last: %s\n"
           "Spawned: %s\t Remaining: %s\t Count: %d",
           sp->name().toLatin1().data(),
           showeq_params->retarded_coords ? "Y/X" : "X/Y",
           showeq_params->retarded_coords ? sp->y() : sp->x(),
           showeq_params->retarded_coords ? sp->x() : sp->y(),
           sp->z(),
           sp->last().toLatin1().data(),
           spawned.toLatin1().data(),
           remaining.toLatin1().data(),
           sp->count());
#endif


    m_mapTip->setText( string  );
    QPoint popPoint = mapToGlobal(event->pos());
    m_mapTip->popup(QPoint(popPoint.x() + 15, popPoint.y() + 15));
  }
  else if (item != NULL)
  {
    QString string;

    const Spawn* spawn = 0;
    const Door* door = 0;
    if ((item->type() == tSpawn) || (item->type() == tPlayer))
      spawn = (const Spawn*)item;
    else if (item->type() == tDoors)
      door = (const Door*)item;

    if (spawn)
    {
      QString guild;
      if (!spawn->guildTag().isEmpty())
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
          guild = QString::asprintf("<%s>", spawn->guildTag().toLatin1().data());
#else
          guild.sprintf("<%s>", spawn->guildTag().toLatin1().data());
#endif
      else if (spawn->guildID())
          guild = QString::number(spawn->guildID());
      else
          guild = " ";

      QString hp;

      if (spawn->HP() <= 0)
        hp = "<= 0";
          else
        hp = QString::number(spawn->HP());

      QString lastName = spawn->lastName();
      if (!lastName.isEmpty())
      {
        // if the spawn isn't a player enclose the name in parenthesis
        if (!spawn->isPlayer())
        {
          lastName.prepend("(");
          lastName.append(") ");
        }
        else
          lastName.append(" ");
      }
      else
        lastName = "";

#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
      string = QString::asprintf("%s %s%s\n"
             "Level: %2d\tHP: %s\t %.3s/Z: %5d/%5d/%5d\n"
             "Race: %s\t Class: %s",
             spawn->transformedName().toUtf8().data(),
             lastName.toUtf8().data(),
             guild.toLatin1().data(),
             spawn->level(), hp.toLatin1().data(),
             showeq_params->retarded_coords ? "Y/X" : "X/Y",
             showeq_params->retarded_coords ? spawn->y() : spawn->x(),
             showeq_params->retarded_coords ? spawn->x() : spawn->y(),
             item->z(),
             spawn->raceString().toLatin1().data(),
             spawn->classString().toLatin1().data());
#else
      string.sprintf("%s %s%s\n"
             "Level: %2d\tHP: %s\t %.3s/Z: %5d/%5d/%5d\n"
             "Race: %s\t Class: %s",
             spawn->transformedName().toUtf8().data(),
             lastName.toUtf8().data(),
             guild.toLatin1().data(),
             spawn->level(), hp.toLatin1().data(),
             showeq_params->retarded_coords ? "Y/X" : "X/Y",
             showeq_params->retarded_coords ? spawn->y() : spawn->x(),
             showeq_params->retarded_coords ? spawn->x() : spawn->y(),
             item->z(),
             spawn->raceString().toLatin1().data(),
             spawn->classString().toLatin1().data());
#endif
      if (m_deityPvP)
        string += " Deity: " + spawn->deityName();

      if (spawn->isNPC())
        string += "\tType: " + spawn->typeString();
      else
           string += "\tGender: " + spawn->genderName();

      string += "\nEquipment: " + spawn->info();
    }
    else
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
      string = QString::asprintf("%s\n"
             "%.3s/Z: %5d/%5d/%5d\n"
             "Race: %s\t Class: %s",
             item->transformedName().toUtf8().data(),
             showeq_params->retarded_coords ? "Y/X" : "X/Y",
             showeq_params->retarded_coords ? item->y() : item->x(),
             showeq_params->retarded_coords ? item->x() : item->y(),
             item->z(),
             item->raceString().toLatin1().data(),
             item->classString().toLatin1().data());
#else
      string.sprintf("%s\n"
             "%.3s/Z: %5d/%5d/%5d\n"
             "Race: %s\t Class: %s",
             item->transformedName().toUtf8().data(),
             showeq_params->retarded_coords ? "Y/X" : "X/Y",
             showeq_params->retarded_coords ? item->y() : item->x(),
             showeq_params->retarded_coords ? item->x() : item->y(),
             item->z(),
             item->raceString().toLatin1().data(),
             item->classString().toLatin1().data());
#endif


      if ((door) && (door->zonePoint() != 0xFFFFFFFF))
      {
        const zonePointStruct* zp = m_zoneMgr->zonePoint(door->zonePoint());
        if (zp)
        {
          QString doorInfo("\nDestination Zone: %1 (%2/%3/%4 - %5)");
          if (showeq_params->retarded_coords)
            string += doorInfo.arg(m_zoneMgr->zoneNameFromID(zp->zoneId))
              .arg(zp->y).arg(zp->x).arg(zp->z).arg(zp->heading);
          else
            string += doorInfo.arg(m_zoneMgr->zoneNameFromID(zp->zoneId))
              .arg(zp->x).arg(zp->y).arg(zp->z).arg(zp->heading);
        }
      }
    }

    m_mapTip->setText( string  );
    QPoint popPoint = mapToGlobal(event->pos());
    m_mapTip->popup(QPoint(popPoint.x() + 15, popPoint.y() + 15));
  }
  else
    m_mapTip->hide();
} 

void Map::selectSpawn(const Item* item)
{
  /* seqDebug("%s", item->ID()); */
  m_selectedItem = item;
  
  // if following the selected spawn, call reAdjust to focus on the new one
  if (m_followMode == tFollowSpawn)
    reAdjust();

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::delItem(const Item* item)
{
  if (item == NULL)
    return;

  // if this is the selected spawn, clear the selected spawn
  if (item == m_selectedItem)
  {
    m_selectedItem = NULL;

    // if was following the selected spawn, call reAdjust to fix things
    if (m_followMode == tFollowSpawn)
      reAdjust();
  }

  if(!m_cacheChanges)
    refreshMap ();
}

void Map::clearItems()
{
  // clear the selected spawn since there are no more spawns
  m_selectedItem = NULL;

  // if was following the selected spawn, call reAdjust to fix things
  if (m_followMode == tFollowSpawn)
    reAdjust();

  // refresh the map
  refreshMap();
}


void Map::changeItem(const Item* item, uint32_t changeType)
{
  if (item == NULL)
    return;

  // only need to deal with position changes
  if (changeType & tSpawnChangedPosition)
  {
    if (m_followMode == tFollowSpawn) 
    {
      // follow mode is follow spawn, check if this is the selected spawn
      // and if so, reAdjust around it's position.
      if (item == m_selectedItem)
        reAdjust();
    }
    else if (m_followMode == tFollowPlayer)
    {
      // follow mode is follow player, check if this is the player spawn
      // and if so, reAdjust around it's position.
      if (item == (const Item*)m_player)
        reAdjust();
    }
  }
}

const Item* Map::closestSpawnToPoint(const QPoint& pt, 
                     uint32_t& closestDistance) const
{
  const Item* closestItem = NULL;

  uint32_t distance;
  EQPoint location;
  EQPoint testPoint;

  const Item* item;
  spawnItemType itemTypes[] = { tSpawn, tDrop, tDoors, tPlayer };
  const bool* showType[] = { &m_showSpawns, &m_showDrops, 
                 &m_showDoors, &m_showPlayer };
  
  for (uint8_t i = 0; i < (sizeof(itemTypes) / sizeof(spawnItemType)); i++)
  {
    if (!*showType[i])
      continue;

    const ItemMap& itemMap = m_spawnShell->getConstMap(itemTypes[i]);
    ItemConstIterator it(itemMap);

    // iterate over all spawns in of the current type
    while (it.hasNext())
    {
      it.next();
      // get the item from the list
      item = it.value();
      if (!item)
          break;

      if (m_spawnDepthFilter &&
          ((item->z() > m_param.playerHeadRoom()) ||
           (item->z() < m_param.playerFloorRoom())))
        continue;

      if (!m_showFiltered && (item->filterFlags() & FILTER_FLAG_FILTERED))
        continue;

      if ((item->type() == tSpawn) || (item->type() == tPlayer))
      {
        if (!m_showUnknownSpawns && ((const Spawn*)item)->isUnknown())
          continue;

        ((const Spawn*)item)->approximatePosition(m_animate, 
                              QTime::currentTime(), 
                              location);

        testPoint.setPoint(m_param.calcXOffsetI(location.x()), 
                   m_param.calcYOffsetI(location.y()), 0);
      }
      else
        testPoint.setPoint(m_param.calcXOffsetI(item->x()), 
                   m_param.calcYOffsetI(item->y()), 0);

      distance = testPoint.calcDist2DInt(pt);

      if (distance < closestDistance)
      {
        closestDistance = distance;
        closestItem = item;
      }
    }
  }

  return closestItem;
}

const SpawnPoint* Map::closestSpawnPointToPoint(const QPoint& pt, 
                        uint32_t& closestDistance) const
{
  const SpawnPoint* closestSP = NULL;

  uint32_t distance;
  EQPoint testPoint;

  QHashIterator<QString, SpawnPoint*> it(m_spawnMonitor->spawnPoints());
  SpawnPoint* sp;

  while (it.hasNext())
  {
    it.next();
    sp = it.value();

    if (m_spawnDepthFilter &&
        ((sp->z() > m_param.playerHeadRoom()) ||
         (sp->z() < m_param.playerFloorRoom())))
      continue;

    testPoint.setPoint(m_param.calcXOffsetI(sp->x()), 
               m_param.calcYOffsetI(sp->y()), 0);

    distance = testPoint.calcDist2DInt(pt);

    if (distance < closestDistance)
    {
      closestDistance = distance;
      closestSP = sp;
    }
  }

  return closestSP;
}

void Map::mapUnloaded(void)
{
#ifdef DEBUGMAP
  qDebug ("mapUnloaded()");
#endif /* DEBUGMAP */

  m_selectedItem = NULL;
  
  m_param.reset();

  emit zoomChanged(m_param.zoom());
  emit panXChanged(m_param.panOffsetX());
  emit panYChanged(m_param.panOffsetY());

  // stop the map update timer
  //  m_timer->stop();

  m_offscreen = QPixmap(m_param.screenLength());

  // force a map refresh
  refreshMap();
  
#ifdef DEBUG
  if (m_showDebugInfo)
    m_time.restart();
#endif
}

void Map::mapLoaded(void)
{
#ifdef DEBUGMAP
  qDebug ("mapLoaded()");
#endif /* DEBUGMAP */

  reAdjust();
  
  if (!m_cacheChanges)
    refreshMap();

  // start the map update timer if necessary
  if (!m_timer->isActive())
    m_timer->start(1000/m_frameRate);

#ifdef DEBUG
  if (m_showDebugInfo)
    m_time.restart();
#endif
}

void Map::mapUpdated(void)
{
  reAdjust();
  
  if(!m_cacheChanges)
    refreshMap();
}


void Map::saveMapImage(void)
{
  QList<QByteArray> formats(QImageWriter::supportedImageFormats());
  QString filters;
  QList<QByteArray>::iterator it;
  for (it = formats.begin(); it != formats.end(); ++it)
      filters += QString(*it) + QString(" (*.") + QString(*it) + ")\n";

  QFileDialog fileDlg(this, "Save Map Image Filename", QString(), filters);

  if (fileDlg.exec() != QDialog::Accepted)
    return;

  QString filter = fileDlg.selectedNameFilter();
  QStringList files = fileDlg.selectedFiles();
  QString filename;
  if (!files.isEmpty())
      filename = files[0];

  if (!filename.isEmpty())
    m_offscreen.save(filename.toLatin1().data(),
             filter.left(filter.indexOf(' ')).toLatin1().data());
}

//----------------------------------------------------------------------
// MapFrame
MapFrame::MapFrame(FilterMgr* filterMgr,
           MapMgr* mapMgr,
           Player* player, 
           SpawnShell* spawnshell,
           ZoneMgr* zoneMgr,
           SpawnMonitor* spawnMonitor,
           const QString& prefName, 
           const QString& defCaption,
           const char* mapName,
           QWidget* parent, const char* name)
  : SEQWindow(prefName + "Frame", defCaption, parent, name),
    m_mapPreferenceName(prefName)
{
  m_filterMgr = filterMgr;
  
  QString prefString = MapFrame::preferenceName();
  QString tmpPrefString;

  QLabel* tmpLabel;

  QWidget* mainWidget = new QWidget();
  setWidget(mainWidget);

  // setup the vertical box
  m_vertical = new QVBoxLayout(mainWidget);
  m_vertical->setContentsMargins(0, 0, 0, 0);

  // setup the top control window
  m_topControlBox = new QWidget(this);
  QHBoxLayout* topControlBoxLayout = new QHBoxLayout(m_topControlBox);
  topControlBoxLayout->setSpacing(1);
  topControlBoxLayout->setMargin(0);
  m_topControlBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  tmpPrefString = "ShowTopControlBox";
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 1))
    m_topControlBox->hide();
  m_vertical->addWidget(m_topControlBox);

  // setup runtime filter
  m_filterMgr->registerRuntimeFilter(m_mapPreferenceName, 
                     m_runtimeFilterFlag,
                     m_runtimeFilterFlagMask);

  // Create map
  m_map = new Map(mapMgr, player, spawnshell, zoneMgr, spawnMonitor,
          m_mapPreferenceName, m_runtimeFilterFlagMask, 
          this, mapName);
  m_vertical->addWidget(m_map);

  // setup bottom control window
  m_bottomControlBox = new QWidget(this);
  QHBoxLayout* bottomControlBoxLayout = new QHBoxLayout(m_bottomControlBox);
  bottomControlBoxLayout->setSpacing(1);
  bottomControlBoxLayout->setMargin(0);
  m_bottomControlBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  tmpPrefString = "ShowBottomControlBox";
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 1))
    m_bottomControlBox->hide();
  m_vertical->addWidget(m_bottomControlBox);


  // setup Zoom control
  m_zoomBox = new QWidget(m_topControlBox);
  m_zoomBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* zoomBoxLayout = new QHBoxLayout(m_zoomBox);
  zoomBoxLayout->setSpacing(1);
  zoomBoxLayout->setMargin(0);
  tmpLabel = new QLabel(m_zoomBox);
  tmpLabel->setText("Zoom:");
  zoomBoxLayout->addWidget(tmpLabel);
  m_zoom = new QSpinBox(m_zoomBox);
  m_zoom->setMinimum(1);
  m_zoom->setMaximum(32);
  m_zoom->setSingleStep(1);
  m_zoom->setWrapping(true);
  m_zoom->setSuffix("x");
  m_zoom->setValue(m_map->zoom());
  zoomBoxLayout->addWidget(m_zoom);
  tmpLabel->setBuddy(m_zoom);
  tmpPrefString = "ShowZoom";
  //minimum width should be the sum of all the minimum widths of the components
  //minimum height should be the minimum height of the tallest component
  m_zoomBox->setMinimumSize(tmpLabel->minimumSizeHint().width() + m_zoom->minimumSizeHint().width(),
          qMax(tmpLabel->minimumSizeHint().height(), m_zoom->minimumSizeHint().height()));
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 1))
    m_zoomBox->hide();
  connect(m_zoom, SIGNAL(valueChanged(int)),
      m_map, SLOT(setZoom(int)));
  connect(m_map, SIGNAL(zoomChanged(int)),
      m_zoom, SLOT(setValue(int)));
  topControlBoxLayout->addWidget(m_zoomBox);

  // setup Player Location display
  m_playerLocationBox = new QWidget(m_topControlBox);
  m_playerLocationBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* playerLocationBoxLayout = new QHBoxLayout(m_playerLocationBox);
  playerLocationBoxLayout->setSpacing(1);
  playerLocationBoxLayout->setMargin(0);
  tmpLabel = new QLabel(m_playerLocationBox);
  tmpLabel->setText("You:");
  playerLocationBoxLayout->addWidget(tmpLabel);
  m_playerLocation = new QLabel(m_playerLocationBox);
  m_playerLocation->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_playerLocation->setText("0      0      0      ");
  m_playerLocation->setMinimumWidth(90);
  playerLocationBoxLayout->addWidget(m_playerLocation);
  tmpLabel->setBuddy(m_playerLocation);
  tmpPrefString = "ShowPlayerLocation";
  //minimum width should be the sum of all the minimum widths of the components
  //minimum height should be the minimum height of the tallest component
  m_playerLocationBox->setMinimumSize(tmpLabel->minimumSizeHint().width() + m_playerLocation->minimumSizeHint().width(),
          qMax(tmpLabel->minimumSizeHint().height(), m_playerLocation->minimumSizeHint().height()));
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, false))
    m_playerLocationBox->hide();
  connect (player, SIGNAL(posChanged(int16_t,int16_t,int16_t,
                    int16_t,int16_t,int16_t,int32_t)), 
       this, SLOT(setPlayer(int16_t,int16_t,int16_t,
                int16_t,int16_t,int16_t,int32_t)));
  topControlBoxLayout->addWidget(m_playerLocationBox);

  // setup Mouse Location display
  m_mouseLocationBox = new QWidget(m_topControlBox);
  m_mouseLocationBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* mouseLocationBoxLayout = new QHBoxLayout(m_mouseLocationBox);
  mouseLocationBoxLayout->setSpacing(1);
  mouseLocationBoxLayout->setMargin(0);
  tmpLabel = new QLabel(m_mouseLocationBox);
  tmpLabel->setText("Cursor:");
  mouseLocationBoxLayout->addWidget(tmpLabel);
  m_mouseLocation = new QLabel(m_mouseLocationBox);
  m_mouseLocation->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_mouseLocation->setText("0      0      ");
  m_mouseLocation->setMinimumWidth(90);
  mouseLocationBoxLayout->addWidget(m_mouseLocation);
  tmpLabel->setBuddy(m_mouseLocationBox);
  tmpPrefString = "ShowMouseLocation";
  //minimum width should be the sum of all the minimum widths of the components
  //minimum height should be the minimum height of the tallest component
  m_mouseLocationBox->setMinimumSize(tmpLabel->minimumSizeHint().width() +
          m_mouseLocation->minimumWidth(),
          qMax(tmpLabel->minimumSizeHint().height(), m_mouseLocation->minimumSizeHint().height()));
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 1))
    m_mouseLocationBox->hide();
  connect (m_map, SIGNAL(mouseLocation(int16_t, int16_t)), 
       this, SLOT(mouseLocation(int16_t, int16_t)));
  topControlBoxLayout->addWidget(m_mouseLocationBox);

  // setup Filter
  m_filterBox = new QWidget(m_topControlBox);
  m_filterBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  QHBoxLayout* filterBoxLayout = new QHBoxLayout(m_filterBox);
  filterBoxLayout->setSpacing(1);
  filterBoxLayout->setMargin(0);
  tmpLabel = new QLabel(m_filterBox);
  tmpLabel->setText("Find:");
  filterBoxLayout->addWidget(tmpLabel);
  m_filter = new MapFilterLineEdit(m_filterBox);
  filterBoxLayout->addWidget(m_filter);
  tmpLabel->setBuddy(m_filter);
  tmpPrefString = "ShowFilter";
  //minimum width should be the sum of all the minimum widths of the components
  //minimum height should be the minimum height of the tallest component
  m_filterBox->setMinimumSize(tmpLabel->minimumSizeHint().width() + m_filter->minimumSizeHint().width(),
          qMax(tmpLabel->minimumSizeHint().height(), m_filter->minimumSizeHint().height()));
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 1))
    m_filterBox->hide();
#ifdef MAPFRAME_IMMEDIATE_REGEX
  connect (m_filter, SIGNAL(textChanged (const QString &)), 
       this, SLOT(setregexp(const QString &)));
#else
  connect (m_filter, SIGNAL(returnPressed()),
       this, SLOT(filterConfirmed()));
#endif
  topControlBoxLayout->addWidget(m_filterBox);

  // setup Layers control
  m_layersBox = new QWidget(m_bottomControlBox);
  m_layersBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* layersBoxLayout = new QHBoxLayout(m_layersBox);
  layersBoxLayout->setSpacing(1);
  layersBoxLayout->setMargin(0);

  loadLayerButtons();

  tmpPrefString = "ShowLayersControl";
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 1))
    m_layersBox->hide();

  bottomControlBoxLayout->addWidget(m_layersBox);

  connect(mapMgr, SIGNAL(mapLoaded()), this, SLOT(mapLoaded()));

  // setup Frame Rate control
  m_frameRateBox = new QWidget(m_bottomControlBox);
  m_frameRateBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* frameRateBoxLayout = new QHBoxLayout(m_frameRateBox);
  frameRateBoxLayout->setSpacing(1);
  frameRateBoxLayout->setMargin(0);
  tmpLabel = new QLabel(m_frameRateBox);
  tmpLabel->setText("Frame Rate:");
  frameRateBoxLayout->addWidget(tmpLabel);
  m_frameRate = new QSpinBox(m_frameRateBox);
  m_frameRate->setMinimum(1);
  m_frameRate->setMaximum(60);
  m_frameRate->setSingleStep(1);
  m_frameRate->setWrapping(true);
  m_frameRate->setSuffix(" fps");
  m_frameRate->setValue(m_map->frameRate());
  frameRateBoxLayout->addWidget(m_frameRate);
  tmpLabel->setBuddy(m_frameRate);
  tmpPrefString = "ShowFrameRate";
  //minimum width should be the sum of all the minimum widths of the components
  //minimum height should be the minimum height of the tallest component
  m_frameRateBox->setMinimumSize(tmpLabel->minimumSizeHint().width() + m_frameRate->minimumSizeHint().width(),
          qMax(tmpLabel->minimumSizeHint().height(), m_frameRate->minimumSizeHint().height()));
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 1))
    m_frameRateBox->hide();
  m_frameRate->setValue(m_map->frameRate());
  connect(m_frameRate, SIGNAL(valueChanged(int)),
      m_map, SLOT(setFrameRate(int)));
  connect(m_map, SIGNAL(frameRateChanged(int)),
      m_frameRate, SLOT(setValue(int)));
  bottomControlBoxLayout->addWidget(m_frameRateBox);

  // setup Pan Controls
  m_panBox = new QWidget(m_bottomControlBox);
  m_panBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* panBoxLayout = new QHBoxLayout(m_panBox);
  panBoxLayout->setSpacing(1);
  panBoxLayout->setMargin(0);
  tmpLabel = new QLabel(m_panBox);
  tmpLabel->setText("Pan X:");
  QSize panXLabelMinSize = tmpLabel->minimumSizeHint();
  panBoxLayout->addWidget(tmpLabel);
  m_panX = new QSpinBox(m_panBox);
  m_panX->setMinimum(-8192);
  m_panX->setMaximum(8192);
  m_panX->setSingleStep(16);
  m_panX->setValue(m_map->panOffsetX());
  panBoxLayout->addWidget(m_panX);
  tmpLabel = new QLabel(m_panBox);
  tmpLabel->setText("Y:");
  QSize panYLabelMinSize = tmpLabel->minimumSizeHint();
  panBoxLayout->addWidget(tmpLabel);
  m_panY = new QSpinBox(m_panBox);
  m_panY->setMinimum(-8192);
  m_panY->setMaximum(8192);
  m_panY->setSingleStep(16);
  m_panY->setValue(m_map->panOffsetY());
  panBoxLayout->addWidget(m_panY);
  tmpPrefString = "ShowPanControls";
  //minimum width should be the sum of all the minimum widths of the components
  //minimum height should be the minimum height of the tallest component
  m_panBox->setMinimumSize(
          //width
          panXLabelMinSize.width() + m_panX->minimumSizeHint().width() +
          panYLabelMinSize.width() + m_panY->minimumSizeHint().width(),
          //height
          qMax(panYLabelMinSize.height(),
              qMax(m_panY->minimumSizeHint().height(),
                  qMax(panXLabelMinSize.height(),
                      m_panX->minimumSizeHint().height()))));
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 1))
    m_panBox->hide();
  connect(m_panX, SIGNAL(valueChanged(int)),
      m_map, SLOT(setPanOffsetX(int)));
  connect(m_panY, SIGNAL(valueChanged(int)),
      m_map, SLOT(setPanOffsetY(int)));
  connect(m_map, SIGNAL(panXChanged(int)),
      m_panX, SLOT(setValue(int)));
  connect(m_map, SIGNAL(panYChanged(int)),
      m_panY, SLOT(setValue(int)));
  bottomControlBoxLayout->addWidget(m_panBox);

  m_depthControlBox = new QWidget(m_bottomControlBox);
  m_depthControlBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* depthControlBoxLayout = new QHBoxLayout(m_depthControlBox);
  depthControlBoxLayout->setSpacing(1);
  depthControlBoxLayout->setMargin(0);
  tmpLabel = new QLabel(m_depthControlBox);
  tmpLabel->setText("Head:");
  QSize depthHeadLabelMinSize = tmpLabel->minimumSizeHint();
  depthControlBoxLayout->addWidget(tmpLabel);
  m_head = new QSpinBox(m_depthControlBox);
  m_head->setMinimum(5);
  m_head->setMaximum(3000);
  m_head->setSingleStep(10);
  m_head->setValue(m_map->headRoom());
  depthControlBoxLayout->addWidget(m_head);
  tmpLabel = new QLabel(m_depthControlBox);
  tmpLabel->setText("Floor:");
  QSize depthFloorLabelMinSize = tmpLabel->minimumSizeHint();
  depthControlBoxLayout->addWidget(tmpLabel);
  m_floor = new QSpinBox(m_depthControlBox);
  m_floor->setMinimum(5);
  m_floor->setMaximum(3000);
  m_floor->setSingleStep(10);
  m_floor->setValue(m_map->floorRoom());
  depthControlBoxLayout->addWidget(m_floor);
  tmpPrefString = "ShowDepthFilterControls";
  //minimum width should be the sum of all the minimum widths of the components
  //minimum height should be the minimum height of the tallest component
  m_depthControlBox->setMinimumSize(
          //width
          depthHeadLabelMinSize.width() +
          m_panX->minimumSizeHint().width() +
          depthFloorLabelMinSize.width() +
          m_floor->minimumSizeHint().width(),
          //height
          qMax(depthHeadLabelMinSize.height(),
              qMax(m_panX->minimumSizeHint().height(),
                  qMax(depthFloorLabelMinSize.height(),
                      m_floor->minimumSizeHint().height()))));
  if (!pSEQPrefs->getPrefBool(tmpPrefString, prefString, 
                  (m_map->mapLineStyle() == tMap_DepthFiltered)))
    m_depthControlBox->hide();
  connect(m_head, SIGNAL(valueChanged(int)),
      m_map, SLOT(setHeadRoom(int)));
  connect(m_map, SIGNAL(headRoomChanged(int)),
      m_head, SLOT(setValue(int)));
  connect(m_floor, SIGNAL(valueChanged(int)),
      m_map, SLOT(setFloorRoom(int)));
  connect(m_map, SIGNAL(floorRoomChanged(int)),
      m_floor, SLOT(setValue(int)));
  bottomControlBoxLayout->addWidget(m_depthControlBox);

  // add our own menu items to the maps menu
  QMenu* mapMenu = m_map->menu();

  // insert a seperator to seperate our stuff from the rest
  mapMenu->addSeparator();

  m_action_topControl = mapMenu->addAction("Show Top Controls", this,
          SLOT(toggle_top_controls()));
  m_action_topControl->setCheckable(true);

  m_action_bottomControl = mapMenu->addAction("Show Bottom Controls", this,
                       SLOT(toggle_bottom_controls()));
  m_action_bottomControl->setCheckable(true);

  mapMenu->addAction("Status Font...", this, SLOT(set_font()));

  // insert a seperator to seperate main controls from sub-menus
  mapMenu->addSeparator();

  QMenu* subMenu = new QMenu("Top Controls");

  m_action_zoom = subMenu->addAction("Show Zoom Controls", this,
          SLOT(toggle_zoom()));
  m_action_zoom->setCheckable(true);

  m_action_playerLocation = subMenu->addAction("Show Player Location", this,
          SLOT(toggle_playerLocation()));
  m_action_playerLocation->setCheckable(true);

  m_action_mouseLocation = subMenu->addAction("Show Mouse Location", this,
          SLOT(toggle_mouseLocation()));
  m_action_mouseLocation->setCheckable(true);

  m_action_filter = subMenu->addAction("Show Find", this, SLOT(toggle_filter()));
  m_action_filter->setCheckable(true);

  m_action_topControl_Options = mapMenu->addMenu(subMenu);

  subMenu = new QMenu("Bottom Controls");

  m_action_layers = subMenu->addAction("Show Layer Controls", this,
          SLOT(toggle_layers()));
  m_action_layers->setCheckable(true);

  m_action_frameRate = subMenu->addAction("Show Frame Rate", this,
          SLOT(toggle_frameRate()));
  m_action_frameRate->setCheckable(true);

  m_action_pan = subMenu->addAction("Show Pan", this, SLOT(toggle_pan()));
  m_action_pan->setCheckable(true);

  m_action_depthControlRoom = subMenu->addAction("Show Depth Filter Controls",
                       this, SLOT(toggle_depthControls()));
  m_action_depthControlRoom->setCheckable(true);

  m_action_bottomControl_Options = mapMenu->addMenu(subMenu);

  // setup signal to initialize menu items when the map is about to be displayeed
  connect(mapMenu, SIGNAL(aboutToShow()), this, SLOT(init_Menu()));
}

MapFrame::~MapFrame()
{
}

QMenu* MapFrame::menu()
{
  return m_map->menu();
}

void MapFrame::filterConfirmed()
{
  setregexp(m_filter->text());
}

void MapFrame::setregexp(const QString &str)
{
  if (m_filterMgr == NULL)
    return;

  // quick check to see if this is the same as the last filter
  if (str == m_lastFilter)
    return;
    
  //seqDebug("New Filter: %s", (const char*)str);

  bool needCommit = false;

  if (!m_lastFilter.isEmpty())
  {
    m_filterMgr->runtimeFilterRemFilter(m_runtimeFilterFlag,
                    m_lastFilter);
    needCommit = true;
  }

  m_lastFilter = str;

  if(str.isEmpty()) 
    regexpok(0);
  else
  {
    bool valid = m_filterMgr->runtimeFilterAddFilter(m_runtimeFilterFlag, str);
    
    needCommit = true;
    
    if (valid) 
      regexpok(1);
    else 
      regexpok(2);
  }

  if (needCommit)
    m_filterMgr->runtimeFilterCommit(m_runtimeFilterFlag);
}

void MapFrame::regexpok(int ok) 
{
  static int ook=0;
  if(ok == ook)
    return;

  ook=ok;

  switch(ok)
  {
  case 0: // no text at all
    m_filter->setPalette( QPalette( QColor(200,200,200) ) );
    break;
  case 1: // Ok
    m_filter->setPalette( QPalette( QColor(0,0,255) ) );
    break;
  case 2:  // Bad
  default:
    m_filter->setPalette( QPalette( QColor(255,0,0) ) );
    break;
  } 
}

void MapFrame::mouseLocation(int16_t x, int16_t y)
{
  QString cursorPos;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  cursorPos = QString::asprintf(" %+5hd, %+5hd", y, x);
#else
  cursorPos.sprintf(" %+5hd, %+5hd", y, x);
#endif
  m_mouseLocation->setText(cursorPos);
}

void MapFrame::setPlayer(int16_t x, int16_t y, int16_t z, 
             int16_t Dx, int16_t Dy, int16_t Dz, int32_t degrees)
{
  QString playerPos;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  playerPos = QString::asprintf(" %+5hd, %+5hd, %+5hd", y, x, z);
#else
  playerPos.sprintf(" %+5hd, %+5hd, %+5hd", y, x, z);
#endif
  m_playerLocation->setText(playerPos);
}

void MapFrame::savePrefs(void)
{
  SEQWindow::savePrefs();

  // make the map belonging to this frame save it's preferences
  if (m_map)
    m_map->savePrefs();
}

void MapFrame::mapLoaded()
{
    loadLayerButtons();
}


void MapFrame::loadLayerButtons()
{

  //delete existing buttons
  QLayout* layersBoxLayout = m_layersBox->layout();
  QLayoutItem* item;
  while ((item = layersBoxLayout->takeAt(0)))
  {
    if (item)
    {
      delete item->widget();
      delete item;
    }
  }

  int numLayers = m_map->mapMgr()->mapData().numLayers();

  QLabel* tmpLabel = new QLabel(m_layersBox);
  if (numLayers == 0)
    tmpLabel->setText("Layers: None");
  else
    tmpLabel->setText("Layers:");

  layersBoxLayout->addWidget(tmpLabel);

  QToolButton* tmpButton = NULL;
  int w = tmpLabel->minimumSizeHint().width();

  QAction* tmpAction = NULL;


  for (int i = 0; i < numLayers; ++i)
  {
      QString label;
      if (i == 0)
          label = "Base";
      else
          label = QString::number(i);

      tmpAction = new QAction(tmpButton);
      tmpAction->setText(label);
      tmpAction->setCheckable(true);
      tmpAction->setData(i);
      tmpAction->setChecked(m_map->isLayerVisible(i));

      tmpButton = new QToolButton();
      tmpButton->setDefaultAction(tmpAction);
      tmpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tmpButton->setMinimumWidth(fontMetrics().width(label));

      layersBoxLayout->addWidget(tmpButton);
      w += fontMetrics().width(label);

      connect(tmpButton, SIGNAL(triggered(QAction*)),
             m_map, SLOT(toggleMapLayerVisibility(QAction*)));
  }

  //minimum width should be the sum of all the minimum widths of the components
  //minimum height should be the minimum height of the tallest component
  m_layersBox->setMinimumSize(tmpLabel->minimumSizeHint().width() + w,
          qMax(tmpLabel->minimumSizeHint().height(),
              (tmpButton) ? tmpButton->minimumSizeHint().height():0));



}

void MapFrame::dumpInfo(QTextStream& out)
{
  // first dump information about the map frame
  out << "[" << preferenceName() << "]" << ENDL;
  out << "Caption: " << windowTitle() << ENDL;
  out << "ShowStatusBox: " << m_topControlBox->isVisible() << ENDL;
  out << "ShowZoom: " << m_zoomBox->isVisible() << ENDL;
  out << "ShowPlayerLocation: " << m_playerLocationBox->isVisible() << ENDL;
  out << "ShowMouseLocation: " << m_mouseLocationBox->isVisible() << ENDL;
  out << "ShowFilter: " << m_filterBox->isVisible() << ENDL;
  out << "ShowControlBox: " << m_bottomControlBox->isVisible() << ENDL;
  out << "ShowFrameRate: " << m_frameRateBox->isVisible() << ENDL;
  out << "ShowLayersControl: " << m_layersBox->isVisible() << ENDL;
  out << "ShowPanControls: " << m_panBox->isVisible() << ENDL; 
  out << "ShowDepthFilterControls: " << m_depthControlBox->isVisible() << ENDL;
  out << "CurrentFilter: '" << m_lastFilter << "'" << ENDL;
  out << "RuntimeFilterFlag: " << m_runtimeFilterFlag << ENDL;
  out << "RuntimeFilterFlagMask: " << m_runtimeFilterFlagMask << ENDL;
  out << ENDL;

  // dump information about the map
  if (m_map)
    m_map->dumpInfo(out);
}

void MapFrame::init_Menu(void)
{
  QMenu* mapMenu = m_map->menu();
  m_action_topControl->setChecked(m_topControlBox->isVisible());
  if (m_topControlBox->isVisible())
  {
    m_action_zoom->setChecked(m_zoomBox->isVisible());
    m_action_playerLocation->setChecked(m_playerLocationBox->isVisible());
    m_action_mouseLocation->setChecked(m_mouseLocation->isVisible());
    m_action_filter->setChecked(m_filterBox->isVisible());
  }

  m_action_bottomControl->setChecked(m_bottomControlBox->isVisible());
  if (m_bottomControlBox->isVisible())
  {
    m_action_layers->setChecked(m_layersBox->isVisible());
    m_action_frameRate->setChecked(m_frameRateBox->isVisible());
    m_action_pan->setChecked(m_panBox->isVisible());
    m_action_depthControlRoom->setChecked(m_depthControlBox->isVisible());
  }
}

void MapFrame::toggle_top_controls()
{
  if (m_topControlBox->isVisible())
    m_topControlBox->hide();
  else
    m_topControlBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowStatusBox";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_topControlBox->isVisible());
  }
}

void MapFrame::toggle_bottom_controls()
{
  if (m_bottomControlBox->isVisible())
    m_bottomControlBox->hide();
  else
    m_bottomControlBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowControlBox";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_bottomControlBox->isVisible());
  }
}

void MapFrame::set_font()
{
  QString name = windowTitle() + " Font";
  bool ok = false;

  // setup a default new status font
  QFont newFont;
  newFont.setPointSize(8);

  // get new status font
  newFont = QFontDialog::getFont(&ok, 
                 font(),
                 this, name);

  // if the user clicked ok and selected a valid font, set it
  if (ok)
    setWindowFont(newFont);
}

void MapFrame::toggle_zoom()
{
  if (m_zoomBox->isVisible())
    m_zoomBox->hide();
  else
    m_zoomBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowZoom";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_zoomBox->isVisible());
  }
}

void MapFrame::toggle_playerLocation()
{
  if (m_playerLocationBox->isVisible())
    m_playerLocationBox->hide();
  else
    m_playerLocationBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowPlayerLocation";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_playerLocationBox->isVisible());
  }
}

void MapFrame::toggle_mouseLocation()
{
  if (m_mouseLocationBox->isVisible())
    m_mouseLocationBox->hide();
  else
    m_mouseLocationBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowMouseLocation";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_mouseLocationBox->isVisible());
  }
}

void MapFrame::toggle_filter()
{
  if (m_filterBox->isVisible())
    m_filterBox->hide();
  else
    m_filterBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowFilter";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_filterBox->isVisible());
  }
}

void MapFrame::toggle_frameRate()
{
  if (m_frameRateBox->isVisible())
    m_frameRateBox->hide();
  else
    m_frameRateBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowFrameRate";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_frameRateBox->isVisible());
  }
}

void MapFrame::toggle_layers()
{
  if (m_layersBox->isVisible())
    m_layersBox->hide();
  else
    m_layersBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowLayersControl";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_layersBox->isVisible());
  }
}

void MapFrame::toggle_pan()
{
  if (m_panBox->isVisible())
    m_panBox->hide();
  else
    m_panBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowPanControls";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_panBox->isVisible());
  }
}

void MapFrame::toggle_depthControls()
{
  if (m_depthControlBox->isVisible())
    m_depthControlBox->hide();
  else
    m_depthControlBox->show();

  QString tmpPrefString = "SaveControls";
  if (pSEQPrefs->getPrefBool(tmpPrefString, preferenceName(), true))
  {
    tmpPrefString = "ShowDepthFilterControls";
    pSEQPrefs->setPrefBool(tmpPrefString, preferenceName(), m_depthControlBox->isVisible()); 
 }
}

MapColorDialog::MapColorDialog(QWidget* parent) : QDialog(parent)
{
    #define X(a,b) m_color_base_table[b] = a;
    SEQMAP_COLOR_TABLE
    #undef X

    setWindowTitle("Map Colors");

    loadUserColors();

    QVBoxLayout* vbox = new QVBoxLayout(this);

    QGridLayout* gridLayout = new QGridLayout();
    vbox->addLayout(gridLayout);

    int col = 0;
    int row = 0;
    for (int i = 0; i < SEQMAP_NUM_COLORS; ++i)
    {
        row = floor(i / 8);
        col = i % 8;

        m_color_pb[i] = new QPushButton(QString::number(i), this);
        m_color_pb[i]->setStyle(new QCommonStyle());
        m_color_pb[i]->setPalette(QPalette(QColor(m_color_user_table[i])));
        m_color_pb[i]->setProperty("colorIndex", i);
        connect(m_color_pb[i], SIGNAL(clicked()), this, SLOT(selectColor()));

        gridLayout->addWidget(m_color_pb[i], row, col);
    }

    QHBoxLayout* hbox = new QHBoxLayout();
    vbox->addItem(new QSpacerItem(20, 25));
    vbox->addLayout(hbox);

    QPushButton* resetButton = new QPushButton("Reset");
    resetButton->setDefault(false);
    resetButton->setAutoDefault(false);
    hbox->addWidget(resetButton);
    connect(resetButton, SIGNAL(clicked()), this, SLOT(resetDialog()));

    QPushButton* defaultsButton = new QPushButton("Load Defaults");
    defaultsButton->setDefault(false);
    defaultsButton->setAutoDefault(false);
    hbox->addWidget(defaultsButton);
    connect(defaultsButton, SIGNAL(clicked()), this, SLOT(loadDefaults()));

    hbox->addItem(new QSpacerItem(20, 1));

    QPushButton* okButton = new QPushButton("Ok");
    okButton->setDefault(false);
    okButton->setAutoDefault(false);
    hbox->addWidget(okButton);
    connect(okButton, SIGNAL(clicked()), this, SLOT(acceptDialog()));

    QPushButton* cancelButton = new QPushButton("Cancel");
    okButton->setDefault(false);
    okButton->setAutoDefault(false);
    hbox->addWidget(cancelButton);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    setLayout(vbox);

    resetDialog();
}

MapColorDialog::~MapColorDialog()
{ }

void MapColorDialog::resetDialog()
{
    loadUserColors();
    for (int i=0; i<SEQMAP_NUM_COLORS; ++i)
        m_color_pb[i]->setPalette(QPalette(QColor(m_color_user_table[i])));
}

void MapColorDialog::acceptDialog()
{
    updateUserColors();
    done(QDialog::Accepted);
}

void MapColorDialog::loadDefaults()
{
    for (int i=0; i<SEQMAP_NUM_COLORS; ++i)
    {
        m_color_user_table[i] = m_color_base_table[i];
        m_color_pb[i]->setPalette(QPalette(QColor(m_color_base_table[i])));
    }
}

void MapColorDialog::loadUserColors()
{
    for (int i=0; i<SEQMAP_NUM_COLORS; ++i)
        m_color_user_table[i] = pSEQPrefs->getPrefString("MapColor" + QString::number(i),
                "MapColors", m_color_base_table[i]);
}

void MapColorDialog::updateUserColors()
{
    for (int i=0; i<SEQMAP_NUM_COLORS; ++i)
        pSEQPrefs->setPrefString("MapColor" + QString::number(i), "MapColors", m_color_user_table[i]);
}



void MapColorDialog::selectColor()
{
    QPushButton* pb = qobject_cast<QPushButton*>(sender());
    if (!pb) return;

    QColor newColor = QColorDialog::getColor(pb->palette().color(backgroundRole()), this);
    if (newColor.isValid())
    {
        int i = pb->property("colorIndex").toInt();
        m_color_user_table[i] = newColor.name();
        pb->setPalette(QPalette(QColor(m_color_user_table[i])));
    }

}

#ifndef QMAKEBUILD
#include "map.moc"
#endif

