/*
 *  mapicon.cpp
 *  Portions Copyright 2003-2007 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2005-2009, 2019 by the respective ShowEQ Developers
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

#include "mapicon.h"
#include "mapcore.h"
#include "spawn.h"
#include "spawnmonitor.h"
#include "player.h"
#include "main.h"

#include <QPainter>
#include <QPoint>
#include <QTimer>
#include <QTextStream>
#include <QPolygon>

#pragma message("Once our minimum supported Qt version is greater than 5.14, this check can be removed and ENDL replaced with Qt::endl")
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
#define ENDL Qt::endl
#else
#define ENDL endl
#endif

//----------------------------------------------------------------------
// constants
static const QString iconSizeNames[] = 
{
  "None",
  "Tiny",
  "Small",
  "Medium",
  "Large",
  "X Large",
  "XX Large",
};

static const QString iconStyleNames[] = 
{
  "None",
  "Circle",
  "Square",
  "Plus",
  "X",
  "Up Triangle",
  "Right Triangle",
  "Down Triangle",
  "Left Triangle",
  "Star",
  "Diamond",
};

static const QString iconTypePrefBaseNames[] = 
{
  "Unknown",
  "Drop",
  "Door",
  "ZoneDoor",
  "SpawnNPC",
  "SpawnNPCCorpse",
  "SpawnPlayer",
  "SpawnPlayerCorpse",
  "SpawnUnknown",
  "SpawnConsidered",
  "SpawnPlayerPvPEnabled",
  "SpawnPetPvPEnabled",
  "SpawnPlayerTeam1",
  "SpawnPlayerTeam2",
  "SpawnPlayerTeam3",
  "SpawnPlayerTeam4",
  "SpawnPlayerTeam5",
  "SpawnPlayerTeamOtherDeity",
  "SpawnPlayerTeamOtherRace",
  "SpawnPlayerTeamOtherDeityPet",
  "SpawnPlayerTeamOtherRacePet",
  "SpawnPlayerOld",
  "SpawnItemSelected",
  "FilterFlagHunt",
  "FilterFlagCaution",
  "FilterFlagDanger",
  "FilterFlagLocate",
  "FilterFlagAlert",
  "FilterFlagFiltered",
  "FilterFlagTracer",
  "RuntimeFiltered",
  "SpawnPoint",
  "SpawnPointSelected",
  "ZoneSafePoint",
  "SpawnPlayerNoUpdate",
  "SpawnNPCNoUpdate",
  "InstanceLocation",
};

static const QString iconTypeNames[] = 
{
  "Unknown",
  "Drop",
  "Door",
  "Zone Door",
  "Spawn NPC",
  "Spawn NPC Corpse",
  "Spawn Player",
  "Spawn Player Corpse",
  "Spawn Unknown",
  "Spawn Considered",
  "Spawn PvP Enabled Player",
  "Spawn PvP Enabled Pet",
  "Spawn Player Team 1",
  "Spawn Player Team 2",
  "Spawn Player Team 3",
  "Spawn Player Team 4",
  "Spawn Player Team 5",
  "Spawn Player Team Other Deity",
  "Spawn Player Team Other Race",
  "Spawn Player Team Other Deity Pet",
  "Spawn Player Team Other Race Pet",
  "Spawn Player Old",
  "Selected Spawn Item",
  "Hunt Filter",
  "Caution Filter",
  "Danger Filter",
  "Locate Filter",
  "Alert Filter",
  "Filtered",
  "Tracer Filter",
  "Runtime Filtered",
  "Spawn Point",
  "Selected Spawn Point",
  "Zone Safe Point",
  "Spawn Player No Update",
  "Spawn NPC No Update",
  "Instance Location",
};

//----------------------------------------------------------------------
// MapIcon
MapIcon::IconImageFunction MapIcon::s_iconImageFunctions[] = 
  {
    &MapIcon::paintNone,
    &MapIcon::paintCircle,
    &MapIcon::paintSquare,
    &MapIcon::paintPlus,
    &MapIcon::paintX,
    &MapIcon::paintUpTriangle,
    &MapIcon::paintRightTriangle,
    &MapIcon::paintDownTriangle,
    &MapIcon::paintLeftTriangle,
    &MapIcon::paintStar,
    &MapIcon::paintDiamond,
  };

//----------------------------------------------------------------------
// MapIcon
MapIcon::MapIcon()
  : m_line1Distance(0),
    m_line2Distance(0),
    m_imageStyle(tIconStyleNone),
    m_imageSize(tIconSizeNone),
    m_highlightStyle(tIconStyleNone),
    m_highlightSize(tIconSizeNone),
    m_image(false),
    m_imageUseSpawnColorPen(false),
    m_imageUseSpawnColorBrush(false),
    m_imageFlash(false),
    m_highlight(false),
    m_highlightUseSpawnColorPen(false),
    m_highlightUseSpawnColorBrush(false),
    m_highlightFlash(false),
    m_showLine0(false),
    m_useWalkPathPen(false),
    m_showWalkPath(false),
    m_showName(false)
{
}

MapIcon::~MapIcon()
{
}

MapIcon& MapIcon::operator=(const MapIcon& mapIcon)
{
  m_imageBrush = mapIcon.m_imageBrush;
  m_highlightBrush = mapIcon.m_highlightBrush;
  m_imagePen = mapIcon.m_imagePen;
  m_highlightPen = mapIcon.m_highlightPen;
  m_line0Pen = mapIcon.m_line0Pen;
  m_line1Pen = mapIcon.m_line1Pen;
  m_line2Pen = mapIcon.m_line2Pen;
  m_walkPathPen = mapIcon.m_walkPathPen;
  m_line1Distance = mapIcon.m_line1Distance;
  m_line2Distance = mapIcon.m_line2Distance;
  m_imageStyle = mapIcon.m_imageStyle;
  m_imageSize = mapIcon.m_imageSize;
  m_highlightStyle = mapIcon.m_highlightStyle;
  m_highlightSize = mapIcon.m_highlightSize;
  m_image = mapIcon.m_image;
  m_imageUseSpawnColorPen = mapIcon.m_imageUseSpawnColorPen;
  m_imageUseSpawnColorBrush = mapIcon.m_imageUseSpawnColorBrush;
  m_imageFlash = mapIcon.m_imageFlash;
  m_highlight = mapIcon.m_highlight;
  m_highlightUseSpawnColorPen = mapIcon.m_highlightUseSpawnColorPen;
  m_highlightUseSpawnColorBrush = mapIcon.m_highlightUseSpawnColorPen;
  m_highlightFlash = mapIcon.m_highlightFlash;
  m_showLine0 = mapIcon.m_showLine0;
  m_useWalkPathPen = mapIcon.m_useWalkPathPen;
  m_showWalkPath = mapIcon.m_showWalkPath;
  m_showName = mapIcon.m_showName;

  return *this;
}

void MapIcon::combine(const MapIcon& mapIcon)
{
  // try our best to generate the combined result of the two MapIcons

  // use image information
  if (mapIcon.m_image)
  {
    m_image = mapIcon.m_image;
    if (mapIcon.m_imageStyle != tIconStyleNone)
      m_imageStyle = mapIcon.m_imageStyle;
    if (mapIcon.m_imageSize != tIconSizeNone)
      m_imageSize = mapIcon.m_imageSize;
    m_imageBrush = mapIcon.m_imageBrush;
    m_imagePen = mapIcon.m_imagePen;
    m_imageUseSpawnColorPen = mapIcon.m_imageUseSpawnColorPen;
    m_imageUseSpawnColorBrush = mapIcon.m_imageUseSpawnColorBrush;
    if (mapIcon.m_imageFlash)
      m_imageFlash = mapIcon.m_imageFlash;
  }

  // use highlight information
  if (mapIcon.m_highlight)
  {
    m_highlight = mapIcon.m_highlight;
    if (mapIcon.m_highlightStyle != tIconStyleNone)
      m_highlightStyle = mapIcon.m_highlightStyle;
    if (mapIcon.m_highlightSize != tIconSizeNone)
      m_highlightSize = mapIcon.m_highlightSize;
    m_highlightBrush = mapIcon.m_highlightBrush;
    m_highlightPen = mapIcon.m_highlightPen;
    m_highlightUseSpawnColorPen = mapIcon.m_highlightUseSpawnColorPen;
    m_highlightUseSpawnColorBrush = mapIcon.m_highlightUseSpawnColorBrush;
    if (mapIcon.m_highlightFlash)
      m_highlightFlash = mapIcon.m_highlightFlash;
  }

  // use walk path pen info iff set
  if (mapIcon.m_useWalkPathPen)
  {
    m_useWalkPathPen = mapIcon.m_useWalkPathPen;
    m_walkPathPen = mapIcon.m_walkPathPen;
  }

  // use showWalkPath info iff set
  if (mapIcon.m_showWalkPath)
    m_showWalkPath = mapIcon.m_showWalkPath;

  // use showLine0 info iff set
  if (mapIcon.m_showLine0)
  {
    m_showLine0 = mapIcon.m_showLine0;
    m_line0Pen = mapIcon.m_line0Pen;
  }
  
  // use line1 info iff set and larger then current setting
  if ((mapIcon.m_line1Distance) && (m_line1Distance < mapIcon.m_line1Distance))
  {
    m_line1Distance = mapIcon.m_line1Distance;
    m_line1Pen = mapIcon.m_line1Pen;
  }

  // use line2 info iff set and larger then current setting
  if ((mapIcon.m_line2Distance) && (m_line2Distance < mapIcon.m_line2Distance))
  {
    m_line2Distance = mapIcon.m_line2Distance;
    m_line2Pen = mapIcon.m_line2Pen;
  }

  // use showName info iff set
  if (mapIcon.m_showName)
    m_showName = mapIcon.m_showName;
}

void MapIcon::load(const QString& prefBase, const QString& section)
{
  // Initialize the image related members
  m_imageBrush = pSEQPrefs->getPrefBrush(prefBase + "ImageBrush", section, m_imageBrush);
  m_imagePen = pSEQPrefs->getPrefPen(prefBase + "ImagePen", section, m_imagePen);
  m_imageStyle = (MapIconStyle)pSEQPrefs->getPrefInt(prefBase + "ImageStyle", 
						     section, m_imageStyle);
  m_imageSize = (MapIconSize)pSEQPrefs->getPrefInt(prefBase + "ImageSize", 
						   section, m_imageSize);
  m_image = pSEQPrefs->getPrefBool(prefBase + "UseImage", section, m_image);
  m_imageUseSpawnColorPen =
    pSEQPrefs->getPrefBool(prefBase + "ImageUseSpawnColorPen", section, 
			   m_imageUseSpawnColorPen);
  m_imageUseSpawnColorBrush =
    pSEQPrefs->getPrefBool(prefBase + "ImageUseSpawnColorBrush", section,
			   m_imageUseSpawnColorBrush);
  m_imageFlash =
    pSEQPrefs->getPrefBool(prefBase + "ImageFlash", section, m_imageFlash);

  // Initialize the Highlight related members
  m_highlightBrush = 
    pSEQPrefs->getPrefBrush(prefBase + "HighlightBrush", section, m_highlightBrush);
  m_highlightPen = pSEQPrefs->getPrefPen(prefBase + "HighlightPen", section, m_highlightPen);
  m_highlightStyle = 
    (MapIconStyle)pSEQPrefs->getPrefInt(prefBase + "HighlightStyle", section,
					m_highlightStyle);
  m_highlightSize =
    (MapIconSize)pSEQPrefs->getPrefInt(prefBase + "HighlightSize", section,
				       m_highlightSize);
  m_highlight = 
    pSEQPrefs->getPrefBool(prefBase + "UseHighlight", section, m_highlight);
  m_highlightUseSpawnColorPen =
    pSEQPrefs->getPrefBool(prefBase + "HighlightUseSpawnColorPen", section, 
			   m_highlightUseSpawnColorPen);
  m_highlightUseSpawnColorBrush =
    pSEQPrefs->getPrefBool(prefBase + "HighlightUseSpawnColorBrush", section,
			   m_highlightUseSpawnColorBrush);
  m_highlightFlash =
    pSEQPrefs->getPrefBool(prefBase + "HighlightFlash", section, 
			   m_highlightFlash);

  // Initialize the line stuff
  m_line0Pen = pSEQPrefs->getPrefPen(prefBase + "Line0Pen", section, m_line0Pen);
  m_showLine0 = pSEQPrefs->getPrefBool(prefBase + "ShowLine0", section,
				       m_showLine0);
  m_line1Pen = pSEQPrefs->getPrefPen(prefBase + "Line1Pen", section, m_line1Pen);
  m_line1Distance = pSEQPrefs->getPrefInt(prefBase + "Line1Distance", section, 
					  m_line1Distance);
  m_line2Pen = pSEQPrefs->getPrefPen(prefBase + "Line2Pen", section, m_line2Pen);
  m_line2Distance = pSEQPrefs->getPrefInt(prefBase + "Line2Distance", section,
					  m_line2Distance);

  // Initialize the Walk Path related member variables
  m_walkPathPen = pSEQPrefs->getPrefPen(prefBase + "WalkPathPen", section, m_walkPathPen);
  m_useWalkPathPen = pSEQPrefs->getPrefBool(prefBase + "UseWalkPathPen", 
					    section, m_useWalkPathPen);
  m_showWalkPath = pSEQPrefs->getPrefBool(prefBase + "ShowWalkPath", section,
					  m_showWalkPath);

  // Initialize whatever's left
  m_showName = pSEQPrefs->getPrefBool(prefBase + "ShowName", section,
				      m_showName);
}


void MapIcon::save(const QString& prefBase, const QString& section)
{
  // Save the image related members
  pSEQPrefs->setPrefBrush(prefBase + "ImageBrush", section, m_imageBrush);
  pSEQPrefs->setPrefPen(prefBase + "ImagePen", section, m_imagePen);
  pSEQPrefs->setPrefInt(prefBase + "ImageStyle", section, m_imageStyle);
  pSEQPrefs->setPrefInt(prefBase + "ImageSize", section, m_imageSize);
  pSEQPrefs->setPrefBool(prefBase + "UseImage", section, m_image);
  pSEQPrefs->setPrefBool(prefBase + "ImageUseSpawnColorPen", section, 
			 m_imageUseSpawnColorPen);
  pSEQPrefs->setPrefBool(prefBase + "ImageUseSpawnColorBrush", section,
			 m_imageUseSpawnColorBrush);
  pSEQPrefs->setPrefBool(prefBase + "ImageFlash", section, m_imageFlash);

  // Save the Highlight related members
  pSEQPrefs->setPrefBrush(prefBase + "HighlightBrush", section, 
			  m_highlightBrush);
  pSEQPrefs->setPrefPen(prefBase + "HighlightPen", section, m_highlightPen);
  pSEQPrefs->setPrefInt(prefBase + "HighlightStyle", section, 
			m_highlightStyle);
  pSEQPrefs->setPrefInt(prefBase + "HighlightSize", section, m_highlightSize);
  pSEQPrefs->setPrefBool(prefBase + "UseHighlight", section, m_highlight);
  pSEQPrefs->setPrefBool(prefBase + "HighlightUseSpawnColorPen", section, 
			 m_highlightUseSpawnColorPen);
  pSEQPrefs->setPrefBool(prefBase + "HighlightUseSpawnColorBrush", section,
			 m_highlightUseSpawnColorBrush);
  pSEQPrefs->setPrefBool(prefBase + "HighlightFlash", section, 
			 m_highlightFlash);

  // Save the line stuff
  pSEQPrefs->setPrefPen(prefBase + "Line0Pen", section, m_line0Pen);
  pSEQPrefs->setPrefBool(prefBase + "ShowLine0", section, m_showLine0);
  pSEQPrefs->setPrefPen(prefBase + "Line1Pen", section, m_line1Pen);
  pSEQPrefs->setPrefInt(prefBase + "Line1Distance", section, m_line1Distance);
  pSEQPrefs->setPrefPen(prefBase + "Line2Pen", section, m_line2Pen);
  pSEQPrefs->setPrefInt(prefBase + "Line2Distance", section, m_line2Distance);

  // Save the Walk Path related member variables
  pSEQPrefs->setPrefPen(prefBase + "WalkPathPen", section, m_walkPathPen);
  pSEQPrefs->setPrefBool(prefBase + "UseWalkPathPen", section, 
			 m_useWalkPathPen);
  pSEQPrefs->setPrefBool(prefBase + "ShowWalkPath", section, m_showWalkPath);

  // Save whatever's left
  pSEQPrefs->setPrefBool(prefBase + "ShowName", section, m_showName);
}

void MapIcon::setImage(const QBrush& brush, const QPen& pen, 
		       MapIconStyle style, MapIconSize size, bool use, 
		       bool useSpawnColorPen, bool useSpawnColorBrush, 
		       bool flash)
{
  // set all the image characteristics
  m_imageBrush = brush;
  m_imagePen = pen;
  m_imageStyle = style;
  m_imageSize = size;
  m_image = use;
  m_imageUseSpawnColorPen = useSpawnColorPen;
  m_imageUseSpawnColorBrush = useSpawnColorBrush;
  m_imageFlash = flash;
}

void MapIcon::setHighlight(const QBrush& brush, const QPen& pen, 
			   MapIconStyle style, MapIconSize size, bool use, 
			   bool useSpawnColorPen, bool useSpawnColorBrush, 
			   bool flash)
{
  // set all the highlight characteristics
  m_highlightBrush = brush;
  m_highlightPen = pen;
  m_highlightStyle = style;
  m_highlightSize = size;
  m_highlight = use;
  m_highlightUseSpawnColorPen = useSpawnColorPen;
  m_highlightUseSpawnColorBrush = useSpawnColorBrush;
  m_highlightFlash = flash;
}

void MapIcon::setLine0(bool show, const QPen& pen)
{
  // set the line 0 characteristics
  m_showLine0 = show;
  m_line0Pen = pen;
}

void MapIcon::setLine1(uint32_t distance, const QPen& pen)
{
  // set the line 1 characteristics
  m_line1Distance = distance;
  m_line1Pen = pen;
}

void MapIcon::setLine2(uint32_t distance, const QPen& pen)
{
  // set the line 2 characteristics
  m_line2Distance = distance;
  m_line2Pen = pen;
}

  // convenience static methods
const QString& MapIcon::iconSizeName(MapIconSize size)
{
  if ((size > tIconSizeNone) && (size <= tIconSizeMax))
    return iconSizeNames[size];

  return iconSizeNames[tIconSizeNone];
}

const QString& MapIcon::iconStyleName(MapIconStyle style)
{
  if ((style > tIconStyleNone) && (style <= tIconStyleMax)) 
    return iconStyleNames[style];
  
  return iconStyleNames[tIconStyleNone];
}

// static paint methods
void MapIcon::paintNone(QPainter&p, const QPoint& point, 
			int size, int sizeWH)
{
}

void MapIcon::paintCircle(QPainter&p, const QPoint& point, 
			  int size, int sizeWH)
{
  p.drawEllipse(point.x() - size, point.y() - size, sizeWH, sizeWH);
}

void MapIcon::paintSquare(QPainter&p, const QPoint& point, 
			  int size, int sizeWH)
{
  p.drawRect(point.x() - size, point.y() - size, sizeWH, sizeWH);
}

void MapIcon::paintPlus(QPainter&p, const QPoint& point, int size, int sizeWH)
{
    p.drawLine(point.x(), point.y() - size, point.x(), point.y() + size );
    p.drawLine(point.x() - size, point.y(), point.x() + size, point.y() );
}

void MapIcon::paintX(QPainter&p, const QPoint& point, int size, int sizeWH)
{
    p.drawLine(point.x() - size, point.y() - size,
	       point.x() + size, point.y() + size);
    p.drawLine(point.x() - size, point.y() + size,
	       point.x() + size, point.y() - size);
}

void MapIcon::paintUpTriangle(QPainter&p, const QPoint& point, 
			      int size, int sizeWH)
{
  QPolygon atri(3);
  atri.setPoint(0, point.x(), point.y() - size);
  atri.setPoint(1, point.x() + size, point.y() + size);
  atri.setPoint(2, point.x() - size, point.y() + size);
  p.drawPolygon(atri);
}

void MapIcon::paintRightTriangle(QPainter&p, const QPoint& point,
				 int size, int sizeWH)
{
  QPolygon atri(3);
  atri.setPoint(0, point.x() + size, point.y());
  atri.setPoint(1, point.x() - size,  point.y() + size);
  atri.setPoint(2, point.x() - size,  point.y() - size);
  p.drawPolygon(atri);
}

void MapIcon::paintDownTriangle(QPainter&p, const QPoint& point, 
				int size, int sizeWH)
{
  QPolygon atri(3);
  atri.setPoint(0, point.x(), point.y() + size);
  atri.setPoint(1, point.x() + size, point.y() - size);
  atri.setPoint(2, point.x() - size, point.y() - size);
  p.drawPolygon(atri);
}

void MapIcon::paintLeftTriangle(QPainter&p, const QPoint& point, 
				int size, int sizeWH)
{
  QPolygon atri(3);
  atri.setPoint(0, point.x() - size, point.y());
  atri.setPoint(1, point.x() + size, point.y() + size);
  atri.setPoint(2, point.x() + size, point.y() - size);
  p.drawPolygon(atri);
}

void MapIcon::paintStar(QPainter&p, const QPoint& point, int size, int sizeWH)
{
  p.drawLine(point.x(), point.y() - size, point.x(), point.y() + size);
  p.drawLine(point.x() - size, point.y(), point.x() + size, point.y());
  p.drawLine(point.x() - size, point.y() - size,
	     point.x() + size, point.y() + size);
  p.drawLine(point.x() - size, point.y() + size,
	     point.x() + size, point.y() - size);
}

void MapIcon::paintDiamond(QPainter&p, const QPoint& point, 
			   int size, int sizeWH)
{
  QPolygon diamond(4);
  diamond.setPoint(0, point.x(), point.y() +  size);
  diamond.setPoint(1, point.x() + size, point.y());
  diamond.setPoint(2, point.x(), point.y() - size);
  diamond.setPoint(3, point.x() - size, point.y());
  p.drawPolygon(diamond);
}

//----------------------------------------------------------------------
// MapIcons
MapIcons::MapIcons(Player* player, const QString& preferenceName,
		   QObject* parent, const char* name)
  : QObject(parent),
    m_player(player),
    m_preferenceName(preferenceName),
    m_flash(false)
{
  setObjectName(name);
  // Setup the map icons with default icon type characteristics
  Qt::PenCapStyle cap = Qt::SquareCap;
  Qt::PenJoinStyle join = Qt::BevelJoin;

  m_mapIcons[tIconTypeUnknown]
    .setImage(QBrush(), QPen(Qt::gray, 1, Qt::SolidLine, cap, join),
	      tIconStyleCircle, tIconSizeSmall,
	      true, false, true, false);
  m_mapIcons[tIconTypeDrop]
    .setImage(QBrush(), QPen(Qt::yellow, 1, Qt::SolidLine, cap, join),
	      tIconStyleX, tIconSizeRegular,
	      true, false, false, false);
  m_mapIcons[tIconTypeDoor]
    .setImage(QBrush(Qt::NoBrush), QPen(QColor(110, 60, 0)),
	      tIconStyleSquare, tIconSizeTiny,
	      true, false, false, false);
  m_mapIcons[tIconTypeZoneDoor]
    .setImage(QBrush(QColor(110,26,104)), QPen(QColor(110, 26, 104)),
	      tIconStyleDiamond, tIconSizeSmall,
	      true, false, false, false);
  m_mapIcons[tIconTypeSpawnNPC]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::black, 0, Qt::SolidLine, cap, join), 
	      tIconStyleCircle, tIconSizeRegular,
	      true, false, true, false);
  m_mapIcons[tIconTypeSpawnNPCCorpse]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::cyan, 1, Qt::SolidLine, cap, join),
	      tIconStylePlus, tIconSizeRegular,
	      true, false, false, false);
  m_mapIcons[tIconTypeSpawnPlayer]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join),
	      tIconStyleSquare, tIconSizeRegular,
	      true, false, true, false);
  m_mapIcons[tIconTypeSpawnPlayerCorpse]
    .setImage(QBrush(), QPen(Qt::yellow, 2, Qt::SolidLine, cap, join), 
	      tIconStyleSquare, tIconSizeRegular,
	      true, false, false, false);
  m_mapIcons[tIconTypeSpawnUnknown]
    .setImage(QBrush(Qt::gray), QPen(Qt::NoBrush, 1, Qt::SolidLine, cap, join), 
	      tIconStyleCircle, tIconSizeRegular,
	      true, false, false, false);
  m_mapIcons[tIconTypeSpawnConsidered]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::red, 1, Qt::SolidLine, cap, join),
		  tIconStyleSquare, tIconSizeLarge,
		  true, false, false, false);
  m_mapIcons[tIconTypeSpawnPlayerPvPEnabled]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::red, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeXLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeSpawnPlayerPvPEnabled]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join), 
	      tIconStyleUpTriangle, tIconSizeRegular,
	      true, false, true, false);
  m_mapIcons[tIconTypeSpawnPetPvPEnabled]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::red, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeXLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeSpawnPlayerTeam1]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join), 
	      tIconStyleUpTriangle, tIconSizeRegular,
	      true, false, true, false);
  m_mapIcons[tIconTypeSpawnPlayerTeam2]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join), 
	      tIconStyleRightTriangle, tIconSizeRegular,
	      true, false, true, false);
  m_mapIcons[tIconTypeSpawnPlayerTeam3]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join), 
	      tIconStyleDownTriangle, tIconSizeRegular,
	      true, false, true, false);
  m_mapIcons[tIconTypeSpawnPlayerTeam4]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join), 
	      tIconStyleLeftTriangle, tIconSizeRegular,
	      true, false, true, false);
  m_mapIcons[tIconTypeSpawnPlayerTeam5]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join), 
	      tIconStyleSquare, tIconSizeRegular,
	      true, false, true, false);
  m_mapIcons[tIconTypeSpawnPlayerTeamOtherRace]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::gray, 1, Qt::SolidLine, cap, join),
		  tIconStyleSquare, tIconSizeXLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeSpawnPlayerTeamOtherDeity]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::SolidPattern, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeXLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeSpawnPlayerTeamOtherRacePet]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::SolidPattern, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeXLarge,
		  true, true, false, true);
  m_mapIcons[tIconTypeSpawnPlayerTeamOtherDeityPet]
    .setImage(QBrush(Qt::NoBrush), QPen(Qt::SolidPattern, 1, Qt::SolidLine, cap, join),
	      tIconStyleCircle, tIconSizeXLarge,
	      true, true, false, true);
  m_mapIcons[tIconTypeSpawnPlayerOld]
    .setImage(QBrush(), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join), 
	      tIconStylePlus, tIconSizeRegular,
	      true, false, false, false);
  m_mapIcons[tIconTypeItemSelected]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeXXLarge,
		  true, false, false, false);
  m_mapIcons[tIconTypeItemSelected].setLine0(true, QPen(Qt::magenta));
  m_mapIcons[tIconTypeItemSelected].setShowWalkPath(true);
  m_mapIcons[tIconTypeFilterFlagHunt]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::gray, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeFilterFlagCaution]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::yellow, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeFilterFlagCaution]
    .setLine1(500, QPen(Qt::yellow, 1, Qt::SolidLine, cap, join));
  m_mapIcons[tIconTypeFilterFlagDanger]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::red, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeFilterFlagDanger]
    .setLine1(500, QPen(Qt::red, 1, Qt::SolidLine, cap, join));
  m_mapIcons[tIconTypeFilterFlagDanger]
    .setLine2(1000, QPen(Qt::yellow, 1, Qt::SolidLine, cap, join));
  m_mapIcons[tIconTypeFilterFlagLocate]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::white, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeFilterFlagLocate]
    .setLine0(true, QPen(Qt::white, 1, Qt::SolidLine, cap, join));
  m_mapIcons[tIconTypeFilterFlagAlert]
    .setHighlight(QBrush(Qt::NoBrush), QPen(),
		  tIconStyleCircle, tIconSizeLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeFilterFlagFiltered]
    .setImage(QBrush(Qt::Dense2Pattern), QPen(Qt::gray, 0, Qt::SolidLine, cap, join), 
	          tIconStyleCircle, tIconSizeSmall,
	          true, false, true, false);
  m_mapIcons[tIconTypeFilterFlagTracer]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::yellow, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeLarge,
		  true, false, false, false);
  m_mapIcons[tIconTypeRuntimeFiltered]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::white, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeLarge,
		  true, false, false, true);
  m_mapIcons[tIconTypeSpawnPoint]
    .setImage(QBrush(Qt::SolidPattern), QPen(Qt::darkGray, 1, Qt::SolidLine, cap, join),
	          tIconStylePlus, tIconSizeRegular,
	          true, true, false, false);
  m_mapIcons[tIconTypeSpawnPointSelected]
    .setHighlight(QBrush(Qt::NoBrush), QPen(Qt::blue, 1, Qt::SolidLine, cap, join),
		  tIconStyleCircle, tIconSizeTiny,
		  true, false, false, false);
  m_mapIcons[tIconTypeSpawnPointSelected]
    .setLine0(true, QPen(Qt::blue));
  m_mapIcons[tIconTypeZoneSafePoint]
    .setImage(QBrush(), QPen(Qt::green, 1, Qt::SolidLine, cap, join),
	          tIconStyleX, tIconSizeSmall,
	          true, false, false, false);
  m_mapIcons[tIconTypeZoneSafePoint].setShowName(true);
  m_mapIcons[tIconTypeSpawnPlayerNoUpdate]
      .setImage(QBrush(Qt::gray), QPen(Qt::darkGray, 1, Qt::SolidLine, cap, join),
                  tIconStyleSquare, tIconSizeRegular,
                  true, false, false, false);
  m_mapIcons[tIconTypeSpawnNPCNoUpdate]
      .setImage(QBrush(Qt::gray), QPen(Qt::NoBrush, 1, Qt::SolidLine, cap, join), 
                  tIconStyleCircle, tIconSizeRegular,
                  true, false, false, false);
  m_mapIcons[tIconTypeDynamicZoneLocation]
      .setImage(QBrush(Qt::magenta), QPen(Qt::magenta, 1, Qt::SolidLine, cap, join), 
                tIconStyleX, tIconSizeLarge,
                  true, false, false, false);
  m_mapIcons[tIconTypeDynamicZoneLocation].setShowName(true);

  // setup icon size maps
  m_mapIconSizes[tIconSizeNone] = &m_markerNSize; // none should never be drawn
  m_mapIconSizesWH[tIconSizeNone] = &m_markerNSizeWH; // but just in case...
  m_mapIconSizes[tIconSizeTiny] = &m_markerNSize;
  m_mapIconSizesWH[tIconSizeTiny] = &m_markerNSizeWH;
  m_mapIconSizes[tIconSizeSmall] = &m_marker0Size;
  m_mapIconSizesWH[tIconSizeSmall] = &m_marker0SizeWH;
  m_mapIconSizes[tIconSizeRegular] = &m_drawSize;
  m_mapIconSizesWH[tIconSizeRegular] = &m_drawSizeWH;
  m_mapIconSizes[tIconSizeLarge] = &m_marker1Size;
  m_mapIconSizesWH[tIconSizeLarge] = &m_marker1SizeWH;
  m_mapIconSizes[tIconSizeXLarge] = &m_marker2Size;
  m_mapIconSizesWH[tIconSizeXLarge] = &m_marker2SizeWH;
  m_mapIconSizes[tIconSizeXXLarge] = &m_marker2Size;
  m_mapIconSizesWH[tIconSizeXXLarge] = &m_marker2SizeWH;

  // setup the flash timer
  m_flashTimer = new QTimer(this);
  connect(m_flashTimer, SIGNAL(timeout()), this, SLOT(flashTick()));
  m_flashTimer->start(200);
}

MapIcons::~MapIcons()
{
}

void MapIcons::load()
{
  // load map icon preferences
  for (int k = 0; k <= tIconTypeMax; k++)
    m_mapIcons[k].load(iconTypePrefBaseNames[k], preferenceName());

  // load the other preferences
  m_showNPCWalkPaths = pSEQPrefs->getPrefBool("ShowNPCWalkPaths", 
					      preferenceName(), false);
  m_showSpawnNames = pSEQPrefs->getPrefBool("ShowSpawnNames", preferenceName(),
					    false);
  m_fovDistance = pSEQPrefs->getPrefInt("FOVDistance", preferenceName(), 
					200);

  int val = pSEQPrefs->getPrefInt("DrawSize", preferenceName(), 3);

  // calculate the icon sizes
  m_drawSize = val; 
  m_drawSizeWH = val << 1; // 2 x size
  m_marker1Size = val + 1;
  m_marker1SizeWH = m_marker1Size << 1; // 2 x size
  m_marker2Size = val + 2;
  m_marker2SizeWH = m_marker2Size << 1; // 2 x size
  m_marker3Size = val + 3;
  m_marker3SizeWH = m_marker2Size << 1; // 2 x size
  if (val > 1)
    m_marker0Size = val - 1;
  else 
    m_marker0Size = val;
  m_marker0SizeWH = m_marker0Size << 1; // 2 x size
  if (val > 2)
    m_markerNSize = val - 2;
  else
    m_markerNSize = 1;
  m_markerNSizeWH = m_markerNSize << 1; // 2 x size
}

void MapIcons::save()
{
  // save map icons
  for (int k = 0; k <= tIconTypeMax; k++)
    m_mapIcons[k].save(iconTypePrefBaseNames[k], preferenceName());
}

void MapIcons::dumpInfo(QTextStream& out)
{
  out << "[" << preferenceName() << " MapIcons]" << ENDL;
  out << "ShowSpawnNames: " << m_showSpawnNames << ENDL;
  out << "FOVDistance: " << m_fovDistance << ENDL;
  out << "DrawSize: " << m_drawSize << ENDL;
  out << ENDL;
}

const QString& MapIcons::iconTypeName(MapIconType type)
{
  if ((type > tIconTypeUnknown) && (type <= tIconTypeMax))
    return iconTypeNames[type];

  return iconTypeNames[tIconTypeUnknown];
}

void MapIcons::setDrawSize(int val)
{
  // validate the input sizes
  if ((val < 1) || (val > 6))
    return;

  // store and calculate the new sizes
  m_drawSize = val; 
  m_drawSizeWH = val << 1; // 2 x size
  m_marker1Size = val + 1;
  m_marker1SizeWH = m_marker1Size << 1; // 2 x size
  m_marker2Size = val + 2;
  m_marker2SizeWH = m_marker2Size << 1; // 2 x size
  m_marker3Size = val + 3;
  m_marker3SizeWH = m_marker2Size << 1; // 2 x size
  if (val > 1)
    m_marker0Size = val - 1;
  else 
    m_marker0Size = val;
  m_marker0SizeWH = m_marker0Size << 1; // 2 x size
  if (val > 2)
    m_markerNSize = val - 2;
  else
    m_markerNSize = 1;
  m_markerNSizeWH = m_markerNSize << 1; // 2 x size

  // set the preference
  pSEQPrefs->setPrefInt("DrawSize", preferenceName(), m_drawSize);

  // signal that the settings have changed
  emit changed();
}

void MapIcons::setShowNPCWalkPaths(bool val) 
{ 
  // set the value
  m_showNPCWalkPaths = val; 

  // save the preference
  pSEQPrefs->setPrefBool("ShowNPCWalkPaths", preferenceName(), 
			 m_showNPCWalkPaths);

  // signal that the settings have changed
  emit changed();
}


void MapIcons::setShowSpawnNames(bool val) 
{ 
  // set the value
  m_showSpawnNames = val; 

  // save the preference
  pSEQPrefs->setPrefBool("ShowSpawnNames", preferenceName(), m_showSpawnNames);

  // signal that the settings have changed
  emit changed();
}

void MapIcons::setFOVDistance(int val) 
{ 
  // validate the input
  if ((val < 1) || (val > 1200))
    return;

  // set the value
  m_fovDistance = val; 

  // save the preference
  pSEQPrefs->setPrefInt("FOVDistance", preferenceName(), m_fovDistance);

  // signal that the settings have changed
  emit changed();
}

void MapIcons::setIcon(int iconType, const MapIcon& icon)
{
  // if a valid map icon was passed in, use it
  if ((iconType <= tIconTypeMax) && (iconType >= tIconTypeUnknown))
    m_mapIcons[iconType] = icon;

  // signal that the settings have changed
  emit changed();
}


void MapIcons::paintIcon(MapParameters& param, 
			 QPainter& p, 
			 const MapIcon& mapIcon,
			 const Point3D<int16_t>& item, 
			 const QString& itemName,
			 const QPoint& point)
{
  // Draw Line
  if (mapIcon.showLine0())
  {
    p.setPen(mapIcon.line0Pen());
    p.drawLine(param.playerXOffset(), 
	       param.playerYOffset(),
	       point.x(), point.y());
  }

  // Calculate distance and draw distance related lines
  uint32_t distance = UINT32_MAX;
  if (mapIcon.line1Distance() || mapIcon.line2Distance())
  {
    if (!showeq_params->fast_machine)
      distance = item.calcDist2DInt(param.player());
    else
      distance = (int)item.calcDist(param.player());

    if (mapIcon.line1Distance() > distance)
    {
      p.setPen(mapIcon.line1Pen());
      p.drawLine(param.playerXOffset(), 
		 param.playerYOffset(),
		 point.x(), point.y());
    }

    if (mapIcon.line2Distance() > distance)
    {
      p.setPen(mapIcon.line2Pen());
      p.drawLine(param.playerXOffset(), 
		 param.playerYOffset(),
		 point.x(), point.y());
    }
  }

  // Draw Item Name
  if (mapIcon.showName())
  {
    QFontMetrics fm(param.font());
    int width = fm.width(itemName);
    p.setPen(Qt::gray);
    p.drawText(point.x() - (width / 2),
	       point.y() + fm.height() + 1, itemName);
  }

  // Draw Icon Image
  if (mapIcon.image() && 
      (!mapIcon.imageFlash() || m_flash) &&
      (mapIcon.imageStyle() != tIconStyleNone))
  {
    p.setPen(mapIcon.imagePen());
    p.setBrush(mapIcon.imageBrush());

    mapIcon.paintIconImage(mapIcon.imageStyle(), p, point, 
			   *m_mapIconSizes[mapIcon.imageSize()],
			   *m_mapIconSizesWH[mapIcon.imageSize()]);
  }

  // Draw Highlight
  if (mapIcon.highlight() && 
      (!mapIcon.highlightFlash() || m_flash) &&
      (mapIcon.highlightStyle() != tIconStyleNone))
  {
    p.setPen(mapIcon.highlightPen());
    p.setBrush(mapIcon.highlightBrush());

    mapIcon.paintIconImage(mapIcon.highlightStyle(), p, point, 
			   *m_mapIconSizes[mapIcon.highlightSize()],
			   *m_mapIconSizesWH[mapIcon.highlightSize()]);
  }
}

void MapIcons::paintItemIcon(MapParameters& param, 
			     QPainter& p, 
			     const MapIcon& mapIcon,
			     const Item* item, 
			     const QPoint& point)
{
  paintIcon(param, p, mapIcon, *item, item->name(), point);
}

void MapIcons::paintSpawnIcon(MapParameters& param, 
			      QPainter& p, 
			      const MapIcon& mapIcon,
			      const Spawn* spawn, 
			      const EQPoint& location,
			      const QPoint& point)
{
  // ------------------------
  // Draw Walk Path
  if (mapIcon.showWalkPath() ||
      (m_showNPCWalkPaths && spawn->isNPC()))
  {
    const SpawnTrackList& trackList = spawn->trackList();
    SpawnTrackListIterator trackIt(spawn->trackList());
    int cnt = trackList.count ();
    
    if (cnt >= 2) {	// only make a line if there is more than one point
	const SpawnTrackPoint* trackPoint = trackIt.next();
	if (trackPoint)
	{
	  if (!mapIcon.useWalkPathPen())
	    p.setPen(Qt::blue);
	  else
	    p.setPen(mapIcon.walkPathPen());

	  int16_t x_1, y_1, x_2, y_2;

	  x_1 = trackPoint->x();
	  y_1 = trackPoint->y();

	  while (trackIt.hasNext())
	  {
	      trackPoint = trackIt.next();
	      if (!trackPoint)
		  break;

	      x_2 = trackPoint->x();
	      y_2 = trackPoint->y();

	      p.drawLine (param.calcXOffsetI(x_1),
			  param.calcYOffsetI(y_1),
			  param.calcXOffsetI(x_2),
			  param.calcYOffsetI(y_2));
	      x_1 = x_2;
	      y_1 = y_2;
	  }

	  p.drawLine (param.calcXOffsetI(x_2), param.calcYOffsetI(y_2), point.x(), point.y());
	}
    }
  }

  // Draw Line
  if (mapIcon.showLine0())
  {
    p.setPen(mapIcon.line0Pen());
    p.drawLine(param.playerXOffset(), 
	       param.playerYOffset(),
	       point.x(), point.y());
  }

  // calculate distance and draw distance related lines
  uint32_t distance = UINT32_MAX;
  if (mapIcon.line1Distance() || mapIcon.line2Distance() || 
      m_showSpawnNames)
  {
    if (!showeq_params->fast_machine)
      distance = location.calcDist2DInt(param.player());
    else
      distance = (int)location.calcDist(param.player());
    
    if (mapIcon.line1Distance() > distance)
    {
      p.setPen(mapIcon.line1Pen());
      p.drawLine(param.playerXOffset(), 
		 param.playerYOffset(),
		 point.x(), point.y());
    }

    if (mapIcon.line2Distance() > distance)
    {
      p.setPen(mapIcon.line2Pen());
      p.drawLine(param.playerXOffset(), 
		 param.playerYOffset(),
		 point.x(), point.y());
    }
  }

  // Draw Spawn Names
  if (mapIcon.showName() || 
      (m_showSpawnNames && (distance < m_fovDistance)))
  {
    QString spawnNameText;

#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    spawnNameText = QString::asprintf("%2d: %s",
            spawn->level(),
            spawn->name().toLatin1().data());
#else
    spawnNameText.sprintf("%2d: %s",
            spawn->level(),
            spawn->name().toLatin1().data());
#endif

    QFontMetrics fm(param.font());
    int width = fm.width(spawnNameText);
    p.setPen(Qt::gray);
    p.drawText(point.x() - (width / 2),
	       point.y() + fm.height() + 1, spawnNameText);
  }
  
  // Draw the Icon
  if (mapIcon.image() && 
      (!mapIcon.imageFlash() || m_flash) &&
      (mapIcon.imageStyle() != tIconStyleNone))
  {
    if (mapIcon.imageUseSpawnColorPen())
    {
      QPen pen = mapIcon.imagePen();
      pen.setColor(pickSpawnColor(spawn));
      p.setPen(pen);
    }
    else
      p.setPen(mapIcon.imagePen());

    if (mapIcon.imageUseSpawnColorBrush())
    {
      QBrush brush = mapIcon.imageBrush();
      brush.setColor(pickSpawnColor(spawn));
      p.setBrush(brush);
    }
    else
      p.setBrush(mapIcon.imageBrush());

    mapIcon.paintIconImage(mapIcon.imageStyle(), p, point, 
			   *m_mapIconSizes[mapIcon.imageSize()],
			   *m_mapIconSizesWH[mapIcon.imageSize()]);
  }

  // Draw the highlight
  if (mapIcon.highlight() && 
      (!mapIcon.highlightFlash() || m_flash) &&
      (mapIcon.highlightStyle() != tIconStyleNone))
  {
    if (mapIcon.highlightUseSpawnColorPen())
    {
      QPen pen = mapIcon.highlightPen();
      pen.setColor(pickSpawnColor(spawn));
      p.setPen(pen);
    }
    else
      p.setPen(mapIcon.highlightPen());

    if (mapIcon.highlightUseSpawnColorBrush())
    {
      QBrush brush = mapIcon.highlightBrush();
      brush.setColor(pickSpawnColor(spawn));
      p.setBrush(brush);
    }
    else
      p.setBrush(mapIcon.highlightBrush());

    mapIcon.paintIconImage(mapIcon.highlightStyle(), p, point, 
			   *m_mapIconSizes[mapIcon.highlightSize()],
			   *m_mapIconSizesWH[mapIcon.highlightSize()]);
  }
}

void MapIcons::paintSpawnPointIcon(MapParameters& param, 
				   QPainter& p, 
				   const MapIcon& mapIcon,
				   const SpawnPoint* sp, 
				   const QPoint& point)
{
  // Draw Line
  if (mapIcon.showLine0())
  {
    p.setPen(mapIcon.line0Pen());
    p.drawLine(param.playerXOffset(), 
	       param.playerYOffset(),
	       point.x(), point.y());
  }

  // calculate distance and draw distance related lines
  uint32_t distance = UINT32_MAX;
  if (mapIcon.line1Distance() || mapIcon.line2Distance())
  {
    if (!showeq_params->fast_machine)
      distance = sp->calcDist2DInt(param.player());
    else
      distance = (int)sp->calcDist(param.player());
    
    if (mapIcon.line1Distance() > distance)
    {
      p.setPen(mapIcon.line1Pen());
      p.drawLine(param.playerXOffset(), 
		 param.playerYOffset(),
		 point.x(), point.y());
    }

    if (mapIcon.line2Distance() > distance)
    {
      p.setPen(mapIcon.line2Pen());
      p.drawLine(param.playerXOffset(), 
		 param.playerYOffset(),
		 point.x(), point.y());
    }
  }

  // Draw Spawn Names
  if (mapIcon.showName())
  {
    QString spawnNameText;

#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    spawnNameText = QString::asprintf("sp:%s %s (%d)",
            sp->name().toLatin1().data(),
            sp->last().toLatin1().data(),
            sp->count());
#else
    spawnNameText.sprintf("sp:%s %s (%d)",
            sp->name().toLatin1().data(),
            sp->last().toLatin1().data(),
            sp->count());
#endif

    QFontMetrics fm(param.font());
    int width = fm.width(spawnNameText);
    p.setPen(Qt::gray);
    p.drawText(point.x() - (width / 2),
	       point.y() + fm.height() + 1, spawnNameText);
  }
  
  // Draw the Icon
  if (mapIcon.image() && 
      (!mapIcon.imageFlash() || m_flash) &&
      (mapIcon.imageStyle() != tIconStyleNone))
  {
    if (mapIcon.imageUseSpawnColorPen())
    {
      QPen pen = mapIcon.imagePen();
      pen.setColor(pickSpawnPointColor(sp, pen.color()));
      p.setPen(pen);
    }
    else
      p.setPen(mapIcon.imagePen());

    if (mapIcon.imageUseSpawnColorBrush())
    {
      QBrush brush = mapIcon.imageBrush();
      brush.setColor(pickSpawnPointColor(sp, brush.color()));
      p.setBrush(brush);
    }
    else
      p.setBrush(mapIcon.imageBrush());

    mapIcon.paintIconImage(mapIcon.imageStyle(), p, point, 
			   *m_mapIconSizes[mapIcon.imageSize()],
			   *m_mapIconSizesWH[mapIcon.imageSize()]);
  }

  // Draw the highlight
  if (mapIcon.highlight() && 
      (!mapIcon.highlightFlash() || m_flash) &&
      (mapIcon.highlightStyle() != tIconStyleNone))
  {
    if (mapIcon.highlightUseSpawnColorPen())
    {
      QPen pen = mapIcon.highlightPen();
      pen.setColor(pickSpawnPointColor(sp, pen.color()));
      p.setPen(pen);
    }
    else
      p.setPen(mapIcon.highlightPen());

    if (mapIcon.highlightUseSpawnColorBrush())
    {
      QBrush brush = mapIcon.highlightBrush();
      brush.setColor(pickSpawnPointColor(sp, brush.color()));
      p.setBrush(brush);
    }
    else
      p.setBrush(mapIcon.highlightBrush());

    mapIcon.paintIconImage(mapIcon.highlightStyle(), p, point, 
			   *m_mapIconSizes[mapIcon.highlightSize()],
			   *m_mapIconSizesWH[mapIcon.highlightSize()]);
  }
}

void MapIcons::flashTick()
{
  m_flash = !m_flash;
}

QColor MapIcons::pickSpawnPointColor(const SpawnPoint* sp, 
				     const QColor& defColor)
{
  // if no time to go on, just use the default
  if ((sp->diffTime() == 0) || (sp->deathTime() == 0))
    return defColor;

  QColor color = defColor;

  // calculate the pen color
  unsigned char age = sp->age();
  
  if ( age == 255 )
    return Qt::darkRed;

  if ( age > 220 )
  {
    if (m_flash)
      return Qt::red;
  }
  else
    color = QColor(age, age, 0);

  return color;
}

QColor MapIcons::pickSpawnColor(const Spawn* spawn)
{
  switch (spawn->typeflag())
  {
  case 65:
    return Qt::magenta;
  case 66:
  case 67:
  case 100:
    return Qt::darkMagenta;
  default:
    break;
  }

  return m_player->pickConColor(spawn->level());
}

#ifndef QMAKEBUILD
#include "mapicon.moc"
#endif

