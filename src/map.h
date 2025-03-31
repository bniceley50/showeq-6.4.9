/*
 *  map.h
 *  Portions Copyright 2001-2007 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2001-2009, 2019 by the respective ShowEQ Developers
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

#ifndef _EQMAP_H_
#define _EQMAP_H_

#ifdef __FreeBSD__
#include <limits.h>
#define UINT32_MAX UINT_MAX
#endif

#include <QWidget>
#include <QPixmap>
#include <QSize>
#include <QMap>
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QToolTip>
#include <QRegExp>
#include <QHash>
#include <QTextStream>
#include <QDateTime>
#include <QPen>
#include <QBrush>
#include <QPushButton>

// includes required for MapMenu
#include <QMenu>

// includes required for MapFrame
#include <QLayout>
#include <QSpinBox>
#include <QList>

#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QVBoxLayout>
#include <QFrame>
#include <QPaintEvent>

#include <ctime>
#include <sys/time.h>
#include <unistd.h>

#include "filtermgr.h"
#include "mapcore.h"
#include "seqwindow.h"
#include "spawn.h"
#include "mapicon.h"
#include "mapcolors.h"

//----------------------------------------------------------------------
// forward declarations
class FilterMgr;
class ZoneMgr;
class SpawnMonitor;
class SpawnPoint;
class Player;
class SpawnShell;
class Item;
class Spawn;
class CLineDlg;
class MapLabel;
class MapMgr;
class Map;
class MapFilterLineEdit;
class MapFrame;
class MapIconDialog;
class DataLocationMgr;

//----------------------------------------------------------------------
// enumerated types
enum FollowMode
{ 
  tFollowPlayer = 0,
  tFollowSpawn = 1,
  tFollowNone = 2,
};

enum FOVMode
{
  tFOVDistanceBased = 0,
  tFOVScaledClassic = 1,
  tFOVClassic = 2
};



//----------------------------------------------------------------------
// constants
const int maxFrameTimes = 40;


//----------------------------------------------------------------------
// CLineDlg
class CLineDlg : public QDialog
{
   Q_OBJECT
 public:
   CLineDlg(QWidget* parent, QString name, MapMgr* mapMgr);

   QComboBox *m_LineColor;
   QLineEdit *m_LineName;
   QFrame    *m_ColorPreview;
 public slots:
   void changeColor(const QString &);
};

//----------------------------------------------------------------------
// MapLabel
class MapLabel: public QLabel
{
 public:
  MapLabel( Map* map );
  void popup(const QPoint& pos);

 protected:
  virtual void mousePressEvent(QMouseEvent*);

  Map*	m_Map;
};

//----------------------------------------------------------------------
// MapMgr
class MapMgr : public QObject
{
   Q_OBJECT

 public:
   MapMgr(const DataLocationMgr* dataLocMgr, 
	  SpawnShell* spawnShell, Player* player, ZoneMgr* zoneMgr,
	  QWidget* dialogParent, 
	  QObject* parent = 0, const char* name = "mapmgr");
   virtual ~MapMgr();
   
   uint16_t spawnAggroRange(const Spawn* spawn);
   const MapData& mapData() { return  m_mapData; }

  const QString& curLineColor() { return m_curLineColor; }
  const QString& curLineName() { return m_curLineName; }

  uint8_t getZEM(void) { return m_mapData.zoneZEM(); }
  void setZEM(uint8_t newZEM) { m_mapData.setZoneZEM(newZEM); }

 public slots:
  // Zone Handling
  void zoneBegin(const QString& shortZoneName);
  void zoneChanged(const QString& shortZoneName);
  void zoneEnd(const QString& shortZoneName, const QString& longZoneName);
  void loadZoneMap(const QString& shortZoneName);

   // Map Handling
  void loadMap(void);
  void importMap(void);
  void loadFileMap(const QString& fileName, 
		   bool import = false, bool force = false);
  void loadFileMap(const QStringList& files,
          bool import = false, bool force = false);
  void saveMap(void);
  void saveSOEMap(void);
  void createNewLayer();

  // Spawn Handling
  void addItem(const Item* item);
  void delItem(const Item* item);
  void killSpawn(const Item* item);
  void changeItem(const Item* item, uint32_t changeType);
  void clearItems(void);

  // Map Editing
  void addLocation(QWidget* parent, const MapPoint& point);
  void startLine(const MapPoint& point);
  void addLinePoint(const MapPoint& point);
  void delLinePoint(void);

  // Map Editing control
  void setLineName(const QString &);
  void setLineColor(const QString &);
  void showLineDlg(QWidget* parent);
  void scaleDownZ(int16_t);
  void scaleUpZ(int16_t);

  void setEditLayer(int layerNum);

  // Preference handling
  void savePrefs(void);

  // dump debug info
  void dumpInfo(QTextStream& out);

 signals:
  void mapLoaded(void);
  void mapUnloaded(void);
  void mapUpdated(void);
  void editLayerChanged(void);

 private:
  const DataLocationMgr* m_dataLocMgr;
  SpawnShell* m_spawnShell;
  Player* m_player;
  QWidget* m_dialogParent;
  CLineDlg *m_dlgLineProps;
  MapData m_mapData;
  QHash<int, uint16_t> m_spawnAggroRange;

  QString m_curLineColor;
  QString m_curLineName;
  QString m_curLocationColor;
};

//----------------------------------------------------------------------
// MapMenu
class MapMenu : public QMenu
{
  Q_OBJECT

 public:
  MapMenu(Map* map, QWidget* parent = 0, const char* name = 0);
  virtual ~MapMenu();

 protected slots:
  void init_Menu(void);
  void init_fovMenu(void);

  void editLayerChanged(void);

  void select_follow(QAction* item);
  void select_mapLine(QAction* item);

  void toggle_spawnDepthFilter();
  void toggle_tooltip();
  void toggle_filtered();
  void toggle_map();
  void toggle_velocity();
  void toggle_animate();
  void toggle_player();
  void toggle_playerBackground();
  void toggle_playerView();
  void toggle_gridLines();
  void toggle_gridTicks();
  void toggle_locations();
  void toggle_spawns();
  void toggle_spawnPnts();
  void toggle_unknownSpawns();
  void toggle_drops();
  void toggle_doors();
  void toggle_highlightConsideredSpawns();
  void toggle_spawnNames();
  void toggle_mapImage();
  void toggle_pvp();
  void toggle_deityPvP();
  void toggle_racePvP();
  void toggle_walkPath();
  void toggle_npcWalkPaths();
  void toggle_debugInfo();
  void toggle_cacheAlwaysRepaint();
  void toggle_cacheChanges();
  void toggle_zoneSafePoint();
  void toggle_instanceLocationMarker();
  void select_gridTickColor();
  void select_gridLineColor();
  void select_backgroundColor();
  void select_font();
  void select_fovColor();
  void select_fovStyle(QAction* item);
  void select_fovMode(QAction* item);

 protected:
  // pointer to the Map this menu controls
  Map* m_map;
  MapIcons* m_mapIcons;

  QLabel* m_fovSpinBoxLabel;
  QSpinBox* m_fovSpinBox;
  QSpinBox* m_drawSizeSpinBox;
  QSpinBox* m_zoomDefaultSpinBox;
  QSpinBox* m_editLayerSpinBox;
  QAction* m_action_followMenu_Player;
  QAction* m_action_followMenu_Spawn;
  QAction* m_action_followMenu_None;
  QAction* m_action_createNewLayer;
  QAction* m_action_addLocation;
  QAction* m_action_startLine;
  QAction* m_action_addLinePoint;
  QAction* m_action_delLinePoint;
  QAction* m_action_showLineDlg;
  QAction* m_action_mapLineStyle_Normal;
  QAction* m_action_mapLineStyle_DepthFiltered;
  QAction* m_action_mapLineStyle_FadedFloors;
  QAction* m_action_spawnDepthFilter;
  QAction* m_action_tooltip;
  QAction* m_action_filtered;
  QAction* m_action_map;
  QAction* m_action_velocity;
  QAction* m_action_animate;
  QAction* m_action_player;
  QAction* m_action_playerBackground;
  QAction* m_action_playerView;
  QAction* m_action_gridLines;
  QAction* m_action_gridTicks;
  QAction* m_action_locations;
  QAction* m_action_spawns;
  QAction* m_action_spawnPoints;
  QAction* m_action_unknownSpawns;
  QAction* m_action_drops;
  QAction* m_action_doors;
  QAction* m_action_spawnNames;
  QAction* m_action_highlightConsideredSpawns;
  QAction* m_action_walkPath;
  QAction* m_action_npcWalkPaths;
  QAction* m_action_mapImage;
  QAction* m_action_deityPvP;
  QAction* m_action_pvp;
  QAction* m_action_racePvP;
  QAction* m_action_zoneSafePoint;
  QAction* m_action_instanceLocation;
#ifdef DEBUG
  QAction* m_action_debugInfo;
#endif
  QAction* m_action_gridTickColor;
  QAction* m_action_gridLineColor;
  QAction* m_action_backgroundColor;
  QAction* m_action_font;
  QAction* m_action_drawSizeMenu;
  QAction* m_action_FOV;
  QAction* m_action_FOVColor;
  QAction* m_action_FOVNoBrush;
  QAction* m_action_FOVSolidPattern;
  QAction* m_action_FOVDense1Pattern;
  QAction* m_action_FOVDense2Pattern;
  QAction* m_action_FOVDense3Pattern;
  QAction* m_action_FOVDense4Pattern;
  QAction* m_action_FOVDense5Pattern;
  QAction* m_action_FOVDense6Pattern;
  QAction* m_action_FOVDense7Pattern;
  QAction* m_action_FOVHorPattern;
  QAction* m_action_FOVVerPattern;
  QAction* m_action_FOVCrossPattern;
  QAction* m_action_FOVBDiagPattern;
  QAction* m_action_FOVFDiagPattern;
  QAction* m_action_FOVDiagCrossPattern;
  QAction* m_action_FOVDistanceBased;
  QAction* m_action_FOVScaledClassic;
  QAction* m_action_FOVClassic;
  QAction* m_action_zoomDefaultMenu;
  QAction* m_action_cacheAlwaysRepaint;
  QAction* m_action_cacheChanges;
};

//----------------------------------------------------------------------
// Map
class Map :public QWidget
{
  Q_OBJECT

 public:
  Map (MapMgr* m_mapMgr,
       Player* player, 
       SpawnShell* spawnshell, 
       ZoneMgr* zoneMgr,
       SpawnMonitor* spawnMonitor,
       const QString& preferenceName, uint32_t runtimeFilterFlagMask,
       QWidget * parent = 0, const char *name = "map");
  virtual ~Map(void);
  
  QSize sizeHint() const; // preferred size
  QSize minimumSizeHint() const; // minimum size
  QSizePolicy sizePolicy() const; // size policy
  QRect getRect()         { return rect(); }
  MapMgr* mapMgr() const { return m_mapMgr; }
  MapIcons* mapIcons() const { return m_mapIcons; }

  unsigned char getZEM (void);
  void          setZEM (unsigned char newZEM);
  
  // old methods
  
  MapMenu* menu();
  
  // get methods
  const QString& preferenceName() { return m_preferenceName; }
  const Item* selectedItem() { return m_selectedItem; }
  FollowMode followMode() const { return m_followMode; }
  int frameRate() const { return m_frameRate; }
  int fovStyle() const { return m_fovStyle; }
  const QColor& fovColor() const { return m_fovColor; }
  FOVMode fovMode() const { return m_fovMode; }
  bool showPlayer() const { return m_showPlayer; }
  bool showPlayerBackground() const { return m_showPlayerBackground; }
  bool showPlayerView() const { return m_showPlayerView; }
  bool showHeading() const { return m_showHeading; }
  bool showSpawns() const { return m_showSpawns; }
  bool showSpawnPoints() const { return m_showSpawnPoints; }
  bool showUnknownSpawns() const { return m_showUnknownSpawns; }
  bool showDrops() const { return m_showDrops; }
  bool showDoors() const { return m_showDoors; }
  bool showFiltered() const { return m_showFiltered; }
  bool showVelocityLines() const { return m_showVelocityLines; }
#ifdef DEBUG
  bool showDebugInfo() const { return m_showDebugInfo; }
#endif
  bool cacheChanges() const { return m_cacheChanges; }
  bool animate() const { return m_animate; }
  bool spawnDepthFilter() const { return m_spawnDepthFilter; }
  bool highlightConsideredSpawns() const { return m_highlightConsideredSpawns; }
  bool showTooltips() const { return m_showTooltips; }
  bool walkPathShowSelect() const { return m_walkpathshowselect; }
  bool pvp() const { return m_pvp; }
  bool deityPvP() const { return m_deityPvP; }
  bool racePvP() const { return m_racePvP; }
  bool showZoneSafePoint() const { return m_showZoneSafePoint; }
  bool showInstanceLocationMarker() const { return m_showInstanceLocationMarker; }

  MapLineStyle mapLineStyle() { return m_param.mapLineStyle(); }
  int zoom() const { return m_param.zoom(); }
  int zoomDefault() const { return m_param.zoomDefault(); }
  int panOffsetX() const { return m_param.panOffsetX(); }
  int panOffsetY() const { return m_param.panOffsetY(); }
  int gridResolution() const { return m_param.gridResolution(); }
  const QColor& gridTickColor() const { return m_param.gridTickColor(); }
  const QColor& gridLineColor() const { return m_param.gridLineColor(); }
  const QColor& backgroundColor() const { return m_param.backgroundColor(); }
  const QFont& font() const { return m_param.font(); }
  int headRoom() const { return m_param.headRoom(); }
  int floorRoom() const { return m_param.floorRoom(); }
  
  bool showBackgroundImage() const { return m_param.showBackgroundImage(); }
  bool showLocations() const { return m_param.showLocations(); }
  bool showLines() const { return m_param.showLines(); }
  bool showGridLines() const { return m_param.showGridLines(); }
  bool showGridTicks() const { return m_param.showGridTicks(); }
  bool cacheAlwaysRepaint() const { return m_mapCache.alwaysRepaint(); }

  bool isLayerVisible(uint8_t layerNum) const { return m_param.isLayerVisible(layerNum); }

 public slots:   
  void savePrefs(void);
  void saveMapImage(void);
  
  void selectSpawn(const Item* item);

  // SpawnShell handling
  void delItem(const Item* item);
  void changeItem(const Item* item, uint32_t changeType);
  void clearItems(void);
  
  // MapMgr handling
  void mapLoaded(void);
  void mapUnloaded(void);
  void mapUpdated(void);

  void toggleMapLayerVisibility(QAction* layer);

  // map editing
  void createNewLayer();
  void addLocation();
  void startLine();
  void addLinePoint();
  void delLinePoint(void);
  void addPathPoint();
  void showLineDlg(void);
  void scaleDownZ(QAction* item);
  void scaleUpZ(QAction* item);
  
  // assorted
  void zoomIn();
  void zoomOut();
  void increaseGridResolution	(void);
  void decreaseGridResolution	(void);
  void panRight();
  void panLeft();
  void panDown();
  void panUp();
  void panUpRight();
  void panUpLeft();
  void panDownRight();
  void panDownLeft();
  void viewTarget();
  void viewLock();

  void reAdjustAndRefreshMap(void);
  void reAdjust (void);
  void refreshMap(void);
  
  // set methods
  void setFollowMode(FollowMode mode);
  void setShowFiltered(bool val);
  void setFrameRate(int val);
  void setFOVStyle(int val);
  void setFOVColor(const QColor& color);
  void setFOVMode(FOVMode mode);
  void setShowMapLines(bool val);
  void setShowPlayer(bool val);
  void setShowPlayerBackground(bool val);
  void setShowPlayerView(bool val);
  void setShowHeading(bool val);
  void setShowSpawns(bool val);
  void setShowSpawnPoints(bool val);
  void setShowUnknownSpawns(bool val);
  void setShowDrops(bool val);
  void setShowDoors(bool val);
  void setShowVelocityLines(bool val);
  void setShowDebugInfo(bool val);
  void setCacheChanges(bool val);
  void setAnimate(bool val);
  void setSpawnDepthFilter(bool val);
  void setHighlightConsideredSpawns(bool val);
  void setShowTooltips(bool val);
  void setWalkPathShowSelect(bool val);
  void setPvP(bool val);
  void setDeityPvP(bool val);
  void setRacePvP(bool val);
  
  void setMapLineStyle(MapLineStyle style);
  void setZoom(int val);
  void setZoomDefault(int val);
  void setPanOffsetX(int val);
  void setPanOffsetY(int val);
  void setGridResolution(int val);
  void setGridTickColor(const QColor& color);
  void setGridLineColor(const QColor& color);
  void setBackgroundColor(const QColor& color);
  void setFont(const QFont& font);
  void setHeadRoom(int val);
  void setFloorRoom(int val);
  
  void setShowBackgroundImage(bool val);
  void setShowLocations(bool val);
  void setShowLines(bool val);
  void setShowGridLines(bool val);
  void setShowGridTicks(bool val);
  void setCacheAlwaysRepaint(bool val);
  void setShowZoneSafePoint(bool val);
  void setShowInstanceLocationMarker(bool val);

  // dump debug info
  void dumpInfo(QTextStream& out);

  void showMapIconDialog();

 signals: 
  void mouseLocation(int16_t x, int16_t y);
  void spawnSelected(const Item* item);
  void zoomChanged(int zoom);
  void zoomDefaultChanged(int zoom);
  void frameRateChanged(int frameRate);
  void panXChanged(int x);
  void panYChanged(int y);
  void headRoomChanged(int headRoom);
  void floorRoomChanged(int floorRoom);

protected:
   const Item* closestSpawnToPoint(const QPoint& pt,
				   uint32_t& closestDistance) const;
   const SpawnPoint* closestSpawnPointToPoint(const QPoint& pt,
					      uint32_t& closestDistance) const;
   void paintEvent (QPaintEvent *);
   void mousePressEvent (QMouseEvent *);
   void mouseMoveEvent( QMouseEvent* );
   void mouseReleaseEvent( QMouseEvent *);
   void mouseDoubleClickEvent( QMouseEvent *);
   void wheelEvent( QWheelEvent *);
   void resizeEvent (QResizeEvent *);

   void paintMap (QPainter *);
   void paintPlayerBackground(MapParameters& param, QPainter& p);
   void paintPlayerView(MapParameters& param, QPainter& p);
   void paintPlayer(MapParameters& param, QPainter& p);
   void paintDrops(MapParameters& param, QPainter& p);
   void paintDoors(MapParameters& param, QPainter& p);
   void paintSelectedSpawnSpecials(MapParameters& param, QPainter& p,
				   const QTime& drawTime);
   void paintSelectedSpawnPointSpecials(MapParameters& param, QPainter& p,
					const QTime& drawTime);
   const QColor raceTeamHighlightColor(const Spawn* spawn) const;
   const QColor deityTeamHighlightColor(const Spawn* spawn) const;
   void paintSpawns(MapParameters& param, QPainter& p, 
		    const QTime& drawTime);
   void paintSpawnPoints(MapParameters& param, QPainter& p);
   void paintDebugInfo(MapParameters& param, 
		       QPainter& tmp, 
		       float fps, 
		       int drawTime);
   QRect mapRect () const;

private:   
   QString m_preferenceName;
   MapParameters m_param;
   MapMgr* m_mapMgr;
   MapCache m_mapCache;
   MapMenu* m_menu;
   MapIcons* m_mapIcons;
   MapIconDialog* m_mapIconDialog;
   uint8_t m_filterCheckOrdering[SIZEOF_FILTERS];
   uint32_t m_runtimeFilterFlagMask;
   QTimer* m_timer;

#ifdef DEBUG
   // debug timing info
   QTime m_time;
   int m_frameTimes[maxFrameTimes];
   int m_frameTimeIndex;
   int m_paintCount;
   int m_paintTimeSum;
#endif

   // mouse based panning state info
   bool m_mapPanning;
   int m_mapPanX;
   int m_mapPanY;

   QPixmap m_offscreen;

   const Item* m_selectedItem;
   Player* m_player;
   SpawnShell* m_spawnShell;
   ZoneMgr* m_zoneMgr;
   SpawnMonitor* m_spawnMonitor;
   MapLabel* m_mapTip;

   FollowMode m_followMode;

   int m_frameRate;
   unsigned int m_scaledFOVDistance;
   int m_fovStyle;
   FOVMode m_fovMode;
   QColor m_fovColor;
   bool m_showMapLines;
   bool m_showMapLocations;
   bool m_showPlayer;
   bool m_showPlayerBackground;
   bool m_showPlayerView;
   bool m_showHeading;
   bool m_showDrops;
   bool m_showDoors;
   bool m_showSpawns;
   bool m_showSpawnPoints;
   bool m_showUnknownSpawns;
   bool m_showSpawnNames;
   bool m_showFiltered;
   bool m_showVelocityLines;
#ifdef DEBUG
   bool m_showDebugInfo;
#endif
   bool m_cacheChanges;
   bool m_animate;
   bool m_spawnDepthFilter;
   bool m_highlightConsideredSpawns;
   bool m_showTooltips;
   bool m_walkpathshowselect;
   bool m_pvp;
   bool m_deityPvP;
   bool m_racePvP;
   bool m_showZoneSafePoint;
   bool m_showInstanceLocationMarker;
};

//----------------------------------------------------------------------
// MapFilterLineEdit
class MapFilterLineEdit : public QLineEdit
{
 public:
  MapFilterLineEdit(QWidget* parent = 0, const char* name = 0)
    : QLineEdit(parent) {}
  ~MapFilterLineEdit() {} ;

 protected:
  virtual void leaveEvent(QEvent* ev) { emit returnPressed(); }
};

//----------------------------------------------------------------------
// MapFrame
class MapFrame : public SEQWindow
{
   Q_OBJECT

 public:
   MapFrame(FilterMgr* filterMgr,
	    MapMgr* mapMgr,
	    Player* player, 
	    SpawnShell* spawnshell,
	    ZoneMgr* zoneMgr,
	    SpawnMonitor* spawnMonitor,
	    const QString& preferenceName = "Map", 
	    const QString& caption = "Map",
	    const char* mapName = "map",
	    QWidget* parent = 0, const char* name = "mapframe");
   virtual ~MapFrame();

   virtual QMenu* menu();

   Map* map() { return m_map; }
   const QString& mapPreferenceName() { return m_mapPreferenceName; }

   void loadLayerButtons(void);

 public slots:
   void regexpok(int ok);
   void setregexp(const QString&);
   void filterConfirmed();
   void mouseLocation(int16_t x, int16_t y);
   void setPlayer(int16_t x, int16_t y, int16_t z, 
		  int16_t Dx, int16_t Dy, int16_t Dz, int32_t degrees);
   virtual void savePrefs(void);
   void mapLoaded(void);

  // dump debug info
  void dumpInfo(QTextStream& out);

 protected slots:
   void init_Menu(void);

   void toggle_top_controls();
   void toggle_bottom_controls();
   void toggle_zoom();
   void toggle_playerLocation();
   void toggle_mouseLocation();
   void toggle_filter();
   void toggle_frameRate();
   void toggle_layers();
   void toggle_pan();
   void toggle_depthControls();
   void set_font();

 private:

   // pointer to the Map that this frame contains/controls
   Map* m_map;

   FilterMgr* m_filterMgr;
   QString m_lastFilter;
   uint32_t m_runtimeFilterFlagMask;
   uint8_t m_runtimeFilterFlag;

   QString m_mapPreferenceName;

   QVBoxLayout* m_vertical;
   QWidget* m_topControlBox;
   QWidget* m_zoomBox;
   QSpinBox* m_zoom;
   QWidget* m_playerLocationBox;
   QLabel* m_playerLocation;
   QWidget* m_mouseLocationBox;
   QLabel* m_mouseLocation;
   QWidget* m_filterBox;
   MapFilterLineEdit* m_filter;

   QWidget* m_bottomControlBox;
   QWidget* m_layersBox;
   QWidget* m_frameRateBox;
   QSpinBox* m_frameRate;
   QWidget* m_panBox;
   QSpinBox* m_panX;
   QSpinBox* m_panY;
   QWidget* m_depthControlBox;
   QSpinBox* m_head;
   QSpinBox* m_floor;
   QList<QWidget*> m_statusWidgets;

   QAction* m_action_topControl;
   QAction* m_action_bottomControl;
   QAction* m_action_zoom;
   QAction* m_action_playerLocation;
   QAction* m_action_mouseLocation;
   QAction* m_action_filter;
   QAction* m_action_topControl_Options;
   QAction* m_action_frameRate;
   QAction* m_action_layers;
   QAction* m_action_pan;
   QAction* m_action_depthControlRoom;
   QAction* m_action_bottomControl_Options;
};

class MapColorDialog : public QDialog
{
    Q_OBJECT

    public:
        MapColorDialog(QWidget* parent=0);
        ~MapColorDialog();

    private slots:
        void selectColor();
        void resetDialog();
        void acceptDialog();
        void loadDefaults();

    private:
        void loadUserColors();
        void updateUserColors();

        QString m_color_base_table[SEQMAP_NUM_COLORS];
        QString m_color_user_table[SEQMAP_NUM_COLORS];
        QPushButton* m_color_pb[SEQMAP_NUM_COLORS];

};

#endif // _EQMAP_H_

