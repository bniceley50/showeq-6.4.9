/*
 *  interface.h
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

#ifndef EQINT_H
#define EQINT_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QMenu>
#include <QMainWindow>
#include <QSplitter>
#include <QList>
#include <QTimer>
#include <QMessageBox>
#include <QSpinBox>
#include <QHash>
#include <QTextStream>
#include "everquest.h"
#include "spawnlist.h"
#include "spawnshell.h"
#include "packetlog.h"
#include "message.h"
#include "messagefilter.h"
#include "map.h"

//--------------------------------------------------
// forward declarations
class Player;
class MapMgr;
class SpawnListWindow;
class SpawnListWindow2;
class SpellListWindow;
class SkillListWindow;
class StatListWindow;
class SpawnPointWindow;
class EQPacket;
class ZoneMgr;
class FilterMgr;
class CategoryMgr;
class SpawnShell;
class SpellShell;
class GroupMgr;
class SpawnMonitor;
class SpawnLog;
class FilteredSpawnLog;
class FilterNotifications;
class Item;
class CompassFrame;
class MapFrame;
class ExperienceWindow;
class CombatWindow;
class NetDiag;
class GuildMgr;
class Spells;
class DateTimeMgr;
class PacketLog;
class PacketStreamLog;
class UnknownPacketLog;
class OPCodeMonitorPacketLog;
class DataLocationMgr;
class EQStr;
class MessageFilters;
class Messages;
class MessageShell;
class MessageWindow;
class Terminal;
class MessageFilterDialog;
class GuildShell;
class GuildListWindow;
class BazaarLog;

//--------------------------------------------------
// typedefs
typedef QList<QAction*> MenuActionList;

//--------------------------------------------------
// constants
// maximum number of maps
const int maxNumMaps = 5; 

// This is the base number where the map dock options appear in the
// Docked menu
const int mapDockBase = 11; 

// This is the base number where the map caption options appear in the
// Window caption menu
const int mapCaptionBase = 11; 

// maximum number of message windows
const int maxNumMessageWindows = 10;

// This is the base number where the message window dock options appear
// in the Docked menu
const int messageWindowDockBase = 16;

//--------------------------------------------------
// EQInterface
/*!
  \brief QMainWindow from Hell!  Also known as ShowEQ's main window.
*/
class EQInterface:public QMainWindow
{
   Q_OBJECT

 public:
   EQInterface(DataLocationMgr* dlm, 
	       QWidget * parent = 0, const char *name = 0);
   ~EQInterface();

   QFont appFont;

 public slots:
   void stsMessage(const QString &, int timeout = 0);
   void numSpawns(int);
   void setExp(uint32_t totalExp, uint32_t totalTick,
	       uint32_t minExpLevel, uint32_t maxExpLevel, 
	       uint32_t tickExpLevel);
   void newExp(uint32_t newExp, uint32_t totalExp, 
	       uint32_t totalTick,
	       uint32_t minExpLevel, uint32_t maxExpLevel, 
	       uint32_t tickExpLevel);
   void setAltExp(uint32_t totalExp,
		  uint32_t maxExp, uint32_t tickExp, uint32_t aapoints);
   void newAltExp(uint32_t newExp, uint32_t totalExp, uint32_t totalTick, 
		  uint32_t maxExp, uint32_t tickExp, uint32_t aapoints);
   void levelChanged(uint8_t level);
   void newSpeed(double);
   void numPacket(int, int);
   void resetPacket(int, int);
   void attack2Hand1(const uint8_t*);
   void action2Message(const uint8_t *);
   void combatKillSpawn(const uint8_t*);
   void updatedDateTime(const QDateTime&);
   void syncDateTime(const QDateTime&);
   void clientTarget(const uint8_t* cts);

   void zoneBegin(const QString& shortZoneName);
   void zoneEnd(const QString& shortZoneName, const QString& longZoneName);
   void zoneChanged(const QString& shortZoneName);

   void spawnSelected(const Item* item);
   void spawnConsidered(const Item* item);
   void addItem(const Item* item);
   void delItem(const Item* item);
   void killSpawn(const Item* item);
   void changeItem(const Item* item);

   void updateSelectedSpawnStatus(const Item* item);

   void savePrefs(void);

   void addCategory(void);
   void reloadCategories(void);
   void rebuildSpawnList();
   void selectNext(void);
   void selectPrev(void);
   void saveSelectedSpawnPath(void);
   void saveSpawnPaths(void);
   void saveSpawnPath(QTextStream& out, const Item* item);
   void toggle_log_AllPackets();
   void toggle_log_WorldData();
   void toggle_log_ZoneData();
   void toggle_opt_BazaarData();
   void toggle_log_UnknownData();
   void toggle_log_RawData();
   void listSpawns(void);
   void listDrops(void);
   void listMapInfo(void);
   void listInterfaceInfo(void);
   void listGroup(void);
   void listGuild(void);
   void dumpSpawns(void);
   void dumpDrops(void);
   void dumpMapInfo(void);
   void dumpGuildInfo(void);
   void dumpSpellBook(void);
   void dumpGroup(void);
   void dumpGuild(void);
   void launch_editor_filters(void);
   void launch_filterlistwindow_filters(void);
   void launch_editor_zoneFilters(void);
   void launch_filterlistwindow_zoneFilters(void);
   void toggleAutoDetectPlayerSettings(bool enable);
   void SetDefaultCharacterClass(QAction*);
   void SetDefaultCharacterRace(QAction*);
   void SetDefaultCharacterLevel(int id);
   void toggle_view_StatWin(QAction* stat);
   void toggle_view_SkillWin(QAction* skill);
   void toggle_view_SpawnListCol(QAction* col);
   void toggle_view_DockedWin(QAction* win);
   void toggle_view_DockableWin(QAction* win);
   void toggle_log_Filter_ZoneData_Client();
   void toggle_log_Filter_ZoneData_Server();
   
   void selectTheme(QAction*);
   void toggle_opcode_monitoring ();
   void set_opcode_monitored_list (void);
   void toggle_opcode_view();
   void toggle_opcode_log();
   void select_opcode_file(void);
   void toggle_net_session_tracking(bool enable);
   void toggle_net_real_time_thread(bool realtime);
   void set_net_monitor_next_client();
   void set_net_client_IP_address();
   void set_net_client_MAC_address();
   void set_net_device();
   void set_net_capture_snap_len(int len);
   void set_net_capture_buffer_size(int size);
   void set_net_arq_giveup(int giveup);
   virtual void setCaption(const QString&);
   void restoreStatusFont();
   void showMessageFilterDialog(void);

 signals:
   void guildList2text(QString);
   void loadFileMap();
   void selectSpawn(const Item* item);
   void saveAllPrefs(void);
   void newZoneName (const QString &); 
   void spellMessage(QString&);
   void restoreFonts();

   void styleChanged();

   // Decoder signals
   void theKey(uint64_t);
   void backfillPlayer(charProfileStruct *);

   void combatSignal(int, int, int, int, int, QString, QString);

 private slots:
   void toggle_opt_Fast();
   void toggle_view_UnknownData();
   void toggle_view_ChannelMsgs(QAction* msgwin);
   void toggle_view_ExpWindow();
   void toggle_view_CombatWindow();
   void toggle_opt_ConSelect();
   void toggle_opt_TarSelect();
   void toggle_opt_TarDeselect();
   void toggle_opt_KeepSelectedVisible();
   void toggle_opt_LogSpawns();
   void toggle_opt_PvPTeams();
   void toggle_opt_PvPDeity();
   void toggle_opt_CreateUnknownSpawns(bool enable);
   void toggle_opt_WalkPathRecord(bool enable);
   void set_opt_WalkPathLength(int);
   void toggle_opt_RetardedCoords(bool enable);
   void toggle_opt_SystimeSpawntime(bool enable);
   void select_opt_conColorBase(QAction* con);
   void select_opt_mapColors();
   void toggle_view_SpawnList();
   void toggle_view_SpawnList2();
   void toggle_view_SpawnPointList();
   void toggle_view_SpellList();
   void toggle_view_PlayerStats();
   void toggle_view_Compass();
   void toggle_view_PlayerSkills();
   void toggle_view_Map(QAction* map);
   void toggle_view_NetDiag();
   void toggle_view_GuildList();
   void resetMaxMana();
   void select_filter_file();
   void toggle_filter_Case(bool cs);
   void toggle_filter_AlertInfo(bool enable);
   void toggle_filter_UseSystemBeep(bool enable);
   void toggle_filter_UseCommands(bool enable);
   void toggle_filter_Log(QAction*);
   void set_filter_AudioCommand(QAction* type);
   void toggle_view_menubar();
   void toggle_view_statusbar();
   void set_main_WindowCaption(QAction*);
   void set_main_WindowFont(QAction*);
   void set_main_Font();
   void select_main_FormatFile();
   void select_main_SpellsFile();
   void toggle_main_statusbar_Window(QAction*);
   void set_main_statusbar_Font();
   void toggle_main_SavePosition(bool enable);
   void toggle_main_UseWindowPos(bool enable);
   void toggle_opt_save_PlayerState(bool enable);
   void toggle_opt_save_ZoneState(bool enable);
   void toggle_opt_save_Spawns(bool enable);
   void set_opt_save_SpawnFrequency(int frequency);
   void set_opt_save_BaseFilename();
   void opt_clearChannelMsgs();
   void init_view_menu();
   void toggle_opt_UseUpdateRadius();

   void toggleTypeFilter(QAction* type);
   void disableAllTypeFilters();
   void enableAllTypeFilters();
   void toggleShowUserFilter(QAction* filter);
   void disableAllShowUserFilters();
   void enableAllShowUserFilters();
   void toggleHideUserFilter(QAction* filter);
   void disableAllHideUserFilters();
   void enableAllHideUserFilters();
   void addUserFilterMenuEntry(uint32_t mask, uint8_t filterid, const MessageFilter& filter);
   void removeUserFilterMenuEntry(uint32_t mask, uint8_t filterid);
   void toggleDisplayType(bool enable);
   void toggleDisplayTime(bool enable);
   void toggleEQDisplayTime(bool enable);
   void toggleUseColor(bool enable);

 protected:
   bool getMonitorOpCodeList(const QString& title, QString& opcodeList);
   QString setTheme(QString name);
   int setTheme(int id);
   void loadFormatStrings();
   void showMap(int mapNum);
   void showMessageWindow(int winNum);
   void showSpawnList(void);
   void showSpawnList2(void);
   void showSpawnPointList(void);
   void showStatList(void);
   void showSkillList(void);
   void showSpellList(void);
   void showCompass(void);
   void showNetDiag(void);
   void showGuildList(void);
   void createFilteredSpawnLog(void);
   void createSpawnLog(void);
   void createGlobalLog(void);
   void createWorldLog(void);
   void createZoneLog(void);
   void createBazaarLog(void);
   void createUnknownZoneLog(void);
   void createOPCodeMonitorLog(const QString&);
   void insertWindowMenu(SEQWindow* window);
   void removeWindowMenu(SEQWindow* window);
   void setDockEnabled(QDockWidget* dw, bool enable);
   QStringList enumerateDevices();
   QString promptForNetDevice();

   virtual void contextMenuEvent(QContextMenuEvent* event) override;

   void createFileMenu();
   void createViewMenu();
   void createOptionsMenu();
   void createNetworkMenu();
   void createCharacterMenu();
   void createFiltersMenu();
   void createInterfaceMenu();
   void createWindowMenu();
   void createDebugMenu();
   void createStatusBar();

 public:
   Player* m_player;
   MapMgr* mapMgr(void) { return m_mapMgr; }

 private:
   DataLocationMgr* m_dataLocationMgr;
   MapMgr* m_mapMgr;
   SpawnListWindow* m_spawnList;
   SpawnListWindow2* m_spawnList2;
   SpellListWindow* m_spellList;
   SkillListWindow* m_skillList;
   StatListWindow* m_statList;
   SpawnPointWindow* m_spawnPointList;
   EQPacket* m_packet;
   ZoneMgr* m_zoneMgr;
   FilterMgr* m_filterMgr;
   CategoryMgr* m_categoryMgr;
   SpawnShell* m_spawnShell;
   Spells* m_spells;
   SpellShell* m_spellShell;
   GroupMgr* m_groupMgr;
   SpawnMonitor* m_spawnMonitor;
   GuildMgr* m_guildmgr; 
   GuildShell* m_guildShell;
   DateTimeMgr* m_dateTimeMgr;
   EQStr* m_eqStrings;
   MessageFilters* m_messageFilters;
   Messages* m_messages;
   MessageShell* m_messageShell;
   Terminal* m_terminal;
   FilteredSpawnLog* m_filteredSpawnLog;
   FilterNotifications* m_filterNotifications;
   SpawnLog *m_spawnLogger;

   PacketLog* m_globalLog;
   PacketStreamLog* m_worldLog;
   PacketStreamLog* m_zoneLog;
   BazaarLog* m_bazaarLog;
   UnknownPacketLog* m_unknownZoneLog;
   OPCodeMonitorPacketLog* m_opcodeMonitorLog;

   const Item* m_selectedSpawn;

   QMenu* m_netMenu;
   QMenu* m_decoderMenu;
   QMenu* m_statWinMenu;
   QMenu* m_skillWinMenu;
   QMenu* m_spawnListMenu;
   QMenu* m_dockedWinMenu;
   QMenu* m_dockableWinMenu;
   QMenu* m_windowCaptionMenu;
   QMenu* m_charMenu;
   QMenu* m_charLevelMenu;
   QSpinBox* m_levelSpinBox;
   QMenu* m_charClassMenu;
   QMenu* m_charRaceMenu;
   QMenu* m_terminalMenu;
   QMenu* m_terminalTypeFilterMenu;
   QMenu* m_terminalShowUserFilterMenu;
   QMenu* m_terminalHideUserFilterMenu;
   QMenu* m_windowMenu;
   QHash<void*, QAction*> m_windowsMenus;
   QMenu* m_filterZoneDataMenu;

   CompassFrame* m_compass;
   MessageWindow* m_messageWindow[maxNumMessageWindows];
   MapFrame*  m_map[maxNumMaps];
   ExperienceWindow* m_expWindow;
   CombatWindow* m_combatWindow;
   NetDiag* m_netDiag;
   MessageFilterDialog* m_messageFilterDialog;
   GuildListWindow* m_guildListWindow;

   QStringList m_deviceList;


   QLabel* m_stsbarSpawns;
   QLabel* m_stsbarStatus;
   QLabel* m_stsbarZone;
   QLabel* m_stsbarID;
   QLabel* m_stsbarExp;
   QLabel* m_stsbarExpAA;
   QLabel* m_stsbarPkt;
   QLabel* m_stsbarEQTime;
   QLabel* m_stsbarSpeed;
   // ZEM code
   QLabel* m_stsbarZEM;

   QString ipstr[5];
   QString macstr[5];

   QHash<QString, QString> m_formattedMessageStrings;

   QAction* m_action_character_Class[PLAYER_CLASSES];
   QAction* m_action_character_Race[PLAYER_RACES];
   QAction* m_action_log_AllPackets;
   QAction* m_action_log_WorldData;
   QAction* m_action_log_ZoneData;
   QAction* m_action_log_UnknownData;
   QAction* m_action_log_RawData;
   int m_id_log_Items;
   int m_id_log_ItemPackets;
   QAction* m_action_opt_BazaarData;
   QAction* m_action_opt_OptionsDlg;
   QAction* m_action_opt_Fast;
   QAction* m_action_opt_ResetMana;
   QAction* m_action_view_UnknownData;
   QAction* m_action_view_ExpWindow;
   QAction* m_action_view_CombatWindow;
   QAction* m_action_view_SpawnList;
   QAction* m_action_view_SpawnList2;
   QAction* m_action_view_SpawnPointList;
   QAction* m_action_view_PlayerStats;
   QAction* m_action_view_PlayerSkills;
   QAction* m_action_view_Compass;
   QAction* m_action_view_Map[maxNumMaps];
   QAction* m_action_view_MessageWindow[maxNumMessageWindows];
   QAction* m_action_view_NetDiag;
   QAction* m_action_view_GuildListWindow;
   QAction* m_action_view_SpellList;
   QAction* m_action_view_PlayerStats_Options;
   QAction* m_action_view_PlayerStats_Stats[LIST_MAXLIST];
   QAction* m_action_view_PlayerSkills_Options;
   QAction* m_action_view_PlayerSkills_Languages;
   QAction* m_action_view_SpawnList_Options;
   QAction* m_action_view_SpawnList_Cols[tSpawnColMaxCols];
   QAction* m_action_opt_ConSelect;
   QAction* m_action_opt_TarSelect;
   QAction* m_action_opt_TarDeselect;
   QAction* m_action_opt_KeepSelectedVisible;
   QAction* m_action_opt_LogSpawns;
   QAction* m_action_opt_PvPTeams;
   QAction* m_action_opt_PvPDeity;
   QAction* m_action_net_sessiontrack;
   QAction* m_action_opcode_monitor;
   QAction* m_action_opcode_log;
   QAction* m_action_opcode_view;
   int  m_packetStartTime;
   int  m_initialcount;
   QAction* m_action_opt_UseUpdateRadius;
   QAction* m_action_log_Filter_ZoneData_Client;
   QAction* m_action_log_Filter_ZoneData_Server;
   QAction* m_action_term_MessageTypeFilters[MT_Max+1];
   QAction* m_action_term_ShowUserFilters[maxMessageFilters+1];
   QAction* m_action_term_HideUserFilters[maxMessageFilters+1];

   MenuActionList ActionList_StyleMenu;

   QStringList m_StringList;
   QDialog *dialogbox;

   MapColorDialog* m_mapColorDialog;

   bool m_isSkillListDocked;
   bool m_isStatListDocked;
   bool m_isMapDocked[maxNumMaps];
   bool m_isMessageWindowDocked[maxNumMessageWindows];
   bool m_isSpawnListDocked;
   bool m_isSpawnList2Docked;
   bool m_isSpawnPointListDocked;
   bool m_isSpellListDocked;
   bool m_isCompassDocked;

   bool m_selectOnConsider;
   bool m_selectOnTarget;
   bool m_deselectOnUntarget;
   bool m_useUpdateRadius;
};

#endif // EQINT_H

