/*
 *  interface.cpp
 *  Copyright 2000-2019 by the respective ShowEQ Developers
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

#include "interface.h"
#include "util.h"
#include "main.h"
#include "editor.h"
#include "filterlistwindow.h"
#include "packet.h"
#include "zonemgr.h"
#include "compassframe.h"
#include "map.h"
#include "experiencelog.h"
#include "combatlog.h"
#include "filtermgr.h"
#include "spellshell.h"
#include "spawnlist.h"
#include "spelllist.h"
#include "player.h"
#include "skilllist.h"
#include "statlist.h"
#include "group.h"
#include "netdiag.h"
#include "spawnmonitor.h"
#include "spawnpointlist.h"
#include "spawnlist2.h"
#include "logger.h"
#include "spawnlog.h"
#include "packetlog.h"
#include "bazaarlog.h"
#include "category.h"
#include "guild.h"
#include "guildshell.h"
#include "guildlist.h"
#include "spells.h"
#include "datetimemgr.h"
#include "datalocationmgr.h"
#include "eqstr.h"
#include "messagefilter.h"
#include "messages.h"
#include "messageshell.h"
#include "messagewindow.h"
#include "terminal.h"
#include "filteredspawnlog.h"
#include "messagefilterdialog.h"
#include "diagnosticmessages.h"
#include "filternotifications.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

#include <QFont>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QStatusBar>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QShortcut>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QMenu>
#include <QWidgetAction>
#include <QDesktopWidget>
#include <QStyleFactory>

#pragma message("Once our minimum supported Qt version is greater than 5.14, this check can be removed and ENDL replaced with Qt::endl")
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
#define ENDL Qt::endl
#else
#define ENDL endl
#endif

// this define is used to diagnose the order with which zone packets are rcvd
#define ZONE_ORDER_DIAG

const char* player_classes[] = {"Warrior", "Cleric", "Paladin",
    "Ranger", "Shadow Knight", "Druid", "Monk", "Bard", "Rogue", "Shaman",
    "Necromancer", "Wizard", "Magician", "Enchanter", "Beastlord", "Berserker"
};

const char* player_races[] = {"Human", "Barbarian", "Erudite",
    "Wood elf", "High Elf", "Dark Elf", "Half Elf", "Dwarf", "Troll", "Ogre",
    "Halfling", "Gnome", "Iksar", "Vah Shir", "Froglok" };


/* The main interface widget */
EQInterface::EQInterface(DataLocationMgr* dlm,
        QWidget * parent, const char *name)
  : QMainWindow (parent),
    m_player(0),
    m_dataLocationMgr(dlm),
    m_mapMgr(0),
    m_spawnList(0),
    m_spawnList2(0),
    m_spellList(0),
    m_skillList(0),
    m_statList(0),
    m_spawnPointList(0),
    m_packet(0),
    m_zoneMgr(0),
    m_filterMgr(0),
    m_categoryMgr(0),
    m_spawnShell(0),
    m_spellShell(0),
    m_groupMgr(0),
    m_spawnMonitor(0),
    m_guildmgr(0),
    m_guildShell(0),
    m_dateTimeMgr(0),
    m_eqStrings(0),
    m_messageFilters(0),
    m_messages(0),
    m_messageShell(0),
    m_terminal(0),
    m_filteredSpawnLog(0),
    m_filterNotifications(0),
    m_spawnLogger(0),
    m_globalLog(0),
    m_worldLog(0),
    m_zoneLog(0),
    m_bazaarLog(0),
    m_unknownZoneLog(0),
    m_opcodeMonitorLog(0),
    m_selectedSpawn(0),
    m_windowsMenus(),
    m_compass(0),
    m_expWindow(0),
    m_combatWindow(0),
    m_netDiag(0),
    m_messageFilterDialog(0),
    m_guildListWindow(0),
    m_deviceList(enumerateNetworkDevices()),
    m_mapColorDialog(new MapColorDialog())
{
  setObjectName(name);
  setWindowFlags(Qt::Window);

  setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks);

  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHeightForWidth(false);
  setSizePolicy(sizePolicy);

  for (int l = 0; l < maxNumMaps; l++)
    m_map[l] = 0;

  for (int l = 0; l < maxNumMessageWindows; l++)
    m_messageWindow[l] = 0;

  QString tempStr;
  QString section = "Interface";

  m_selectOnConsider = pSEQPrefs->getPrefBool("SelectOnCon", section, false);
  m_selectOnTarget = pSEQPrefs->getPrefBool("SelectOnTarget", section, false);
  m_deselectOnUntarget = pSEQPrefs->getPrefBool("DeselectOnUntarget", section, false);

   // set the applications default font
   if (pSEQPrefs->isPreference("Font", section))
   {
     QFont appFont = pSEQPrefs->getPrefFont("Font", section, qApp->font());
     qApp->setFont( appFont );
   }

   // initialize packet count
   m_initialcount = 0;
   m_packetStartTime = 0;

   // Create the date/time manager
   m_dateTimeMgr = new DateTimeMgr(this, "datetimemgr");

   // Create Message Filters object
   m_messageFilters = new MessageFilters(this, "messagefilters");

   // Create Messages storage
   m_messages = new Messages(m_dateTimeMgr, m_messageFilters, 
			     this, "messages");

   // Create the terminal object
   m_terminal = new Terminal(m_messages, this, "terminal");

   QString fileName, fileName2;
   QFileInfo fileInfo, fileInfo2;

   // Create the packet object
   section = "Network";
   QString vpsection = "VPacket";

   fileName = pSEQPrefs->getPrefString("WorldOPCodes", section, 
				       "worldopcodes.xml");

   fileInfo = m_dataLocationMgr->findExistingFile(".", fileName);

   fileName2 = pSEQPrefs->getPrefString("ZoneOPCodes", section, 
					"zoneopcodes.xml");

   fileInfo2 = m_dataLocationMgr->findExistingFile(".", fileName2);

   QString net_device = pSEQPrefs->getPrefString("Device", section, "eth0");
   if (!m_deviceList.contains(net_device))
   {
       QString selected = promptForNetDevice();

       if (!selected.isEmpty())
       {
           // set it as the device to monitor next session
           pSEQPrefs->setPrefString("Device", section, selected);
           net_device = selected;
       }
   }

   m_packet = new EQPacket(fileInfo.absoluteFilePath(),
			   fileInfo2.absoluteFilePath(),
			   pSEQPrefs->getPrefInt("ArqSeqGiveUp", section, 512),
			   net_device,
			   pSEQPrefs->getPrefString("IP", section,
						    AUTOMATIC_CLIENT_IP),
			   pSEQPrefs->getPrefString("MAC", section, "0"),
			   pSEQPrefs->getPrefBool("RealTimeThread", section,
						  false),
               pSEQPrefs->getPrefInt("CaptureSnapLen", section, 1),
               pSEQPrefs->getPrefInt("CaptureBufferSize", section, 2),
			   pSEQPrefs->getPrefBool("SessionTracking", 
						  section, false),
			   pSEQPrefs->getPrefBool("Record", vpsection, false),
			   pSEQPrefs->getPrefInt("Playback", vpsection,
						  PLAYBACK_OFF),
			   pSEQPrefs->getPrefInt("PlaybackRate", vpsection, 
						 false),
       			   this, "packet");
   
  ipstr[0] = m_packet->ip();	//Retrieves last IP used in previous session 
  for( int i = 1; i < 5; i++) 
  ipstr[i] = "0.0.0.0";

  macstr[0] = m_packet->mac();	//Retrieves last MAC used in previous session
  for( int i = 1; i < 5; i++)  
  macstr[i] = "00:00:00:00:00:00";

   section = "Interface";

   // Create the Spells object
   fileName = pSEQPrefs->getPrefString("SpellsFile", section,
					       "spells_us.txt");

   fileInfo = m_dataLocationMgr->findExistingFile(".", fileName);
   
   m_spells = new Spells(fileInfo.absoluteFilePath());
   
   // Create the EQStr storage
   m_eqStrings = new EQStr();

   // Create the Zone Manager
   m_zoneMgr = new ZoneMgr(this, "zonemgr");

   // Create GuildMgr object
   fileName = pSEQPrefs->getPrefString("GuildsFile", "Interface",
				       "guilds2.dat");
   
   fileInfo = m_dataLocationMgr->findWriteFile("tmp", fileName);

   m_guildmgr = new GuildMgr(fileInfo.absoluteFilePath(), 
			     this, "guildmgr");

   // Create our player object
   m_player = new Player(this, m_zoneMgr, m_guildmgr);

   // Create the filter manager
   m_filterMgr = new FilterMgr(m_dataLocationMgr,
			       pSEQPrefs->getPrefString("FilterFile", 
							section, "global.xml"),
			       pSEQPrefs->getPrefBool("IsCaseSensitive", 
						      section, false));

   // if there is a short zone name already, try to load its filters
   QString shortZoneName = m_zoneMgr->shortZoneName();
   if (!shortZoneName.isEmpty())
     m_filterMgr->loadZone(shortZoneName);
   
   m_guildShell = new GuildShell(m_zoneMgr, this, "GuildShell");

   // Create the spawn shell
   m_spawnShell = new SpawnShell(*m_filterMgr, m_zoneMgr, m_player, m_guildmgr);

   // Create the Category manager
   m_categoryMgr = new CategoryMgr();

   // Create the map manager
   m_mapMgr = new MapMgr(m_dataLocationMgr, m_spawnShell, m_player, m_zoneMgr,
			 this);

   // Create the spell shell
   m_spellShell = new SpellShell(m_player, m_spawnShell, m_spells);

   // Create the Spawn Monitor
   m_spawnMonitor = new SpawnMonitor(m_dataLocationMgr, 
				     m_zoneMgr, m_spawnShell);

   // Create the Group Manager
   m_groupMgr = new GroupMgr(m_spawnShell, m_player, this, "groupmgr");

   // Create the message shell
   m_messageShell = new MessageShell(m_messages, m_eqStrings, m_spells, 
				     m_zoneMgr, m_spawnShell, m_player,
				     this, "messageshell");

   // Create the Filter Notifications object
   m_filterNotifications = new FilterNotifications(this, "filternotifications");
   
   // Create log objects as necessary
   if (pSEQPrefs->getPrefBool("LogAllPackets", "PacketLogging", false))
     createGlobalLog();

   if (pSEQPrefs->getPrefBool("LogZonePackets", "PacketLogging", false))
     createZoneLog();

   if (pSEQPrefs->getPrefBool("LogBazaarPackets", "PacketLogging", false))
     createBazaarLog();

   if (pSEQPrefs->getPrefBool("LogWorldPackets", "PacketLogging", false))
     createWorldLog();

   if (pSEQPrefs->getPrefBool("LogUnknownZonePackets", "PacketLogging", false))
     createUnknownZoneLog();

   section = "OpCodeMonitoring";
   if (pSEQPrefs->getPrefBool("Enable", section, false))
     createOPCodeMonitorLog(pSEQPrefs->getPrefString("OpCodeList", section, ""));

   // create the filtered spawn log object if any filters are to be logged
   uint32_t filters = pSEQPrefs->getPrefInt("Log", "Filters", 0);
   if (filters)
   {
     // create the filtered spawn log object
     createFilteredSpawnLog();

     // set the filters to log
     m_filteredSpawnLog->setFilters(filters);
   }

   // if the user wants spawns logged, create the spawn logger
   if (pSEQPrefs->getPrefBool("LogSpawns", "Misc", false))
     createSpawnLog();

   section = "Interface";

   // create window menu
   m_windowMenu = new QMenu("&Window");

   // Initialize the experience window;
   m_expWindow = new ExperienceWindow(m_dataLocationMgr, m_player, m_groupMgr,
           m_zoneMgr, this);
   setDockEnabled(m_expWindow,
           pSEQPrefs->getPrefBool("DockableExperienceWindow", section, false));
   Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock",
           m_expWindow->preferenceName(), Qt::LeftDockWidgetArea);
   addDockWidget(edge, m_expWindow);
   m_expWindow->setFloating(true);

   if (pSEQPrefs->getPrefBool("ShowExpWindow", section, false))
   {
     m_expWindow->show();
     insertWindowMenu(m_expWindow);
   }
   else
   {
     m_expWindow->hide();
     removeWindowMenu(m_expWindow);
   }



   // Initialize the combat window
   m_combatWindow = new CombatWindow(m_player, this);
   setDockEnabled(m_combatWindow,
           pSEQPrefs->getPrefBool("DockableCombatWindow", section, false));
   edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock",
           m_combatWindow->preferenceName(), Qt::LeftDockWidgetArea);
   addDockWidget(edge, m_combatWindow);
   m_combatWindow->setFloating(true);

   if (pSEQPrefs->getPrefBool("ShowCombatWindow", section, false))
   {
     m_combatWindow->show();
     insertWindowMenu(m_combatWindow);
   }
   else
   {
     m_combatWindow->hide();
     removeWindowMenu(m_combatWindow);
   }

/////////////////
// Main widgets
   // Make a VBox to use as central widget
   // Create/display the Map(s)
   for (int i = 0; i < maxNumMaps; i++)
   {
     QString tmpPrefSuffix = "";
     if (i > 0)
       tmpPrefSuffix = QString::number(i + 1);
     
     // construct the preference name
     QString tmpPrefName = QString("DockedMap") + tmpPrefSuffix;

     // retrieve if the map should be docked
     m_isMapDocked[i] = pSEQPrefs->getPrefBool(tmpPrefName, section, (i == 0));

     // construct the preference name
     tmpPrefName = QString("ShowMap") + tmpPrefSuffix;

     // and as appropriate, craete the map
     if (pSEQPrefs->getPrefBool(tmpPrefName, section, (i == 0)))
       showMap(i);
   }

   // Create/display the MessageWindow(s)
   for (int i = 0; i < maxNumMessageWindows; i++)
   {
     QString tmpPrefSuffix = "";
     if (i > 0)
       tmpPrefSuffix = QString::number(i + 1);
     
     // construct the preference name
     QString tmpPrefName = QString("DockedMessageWindow") + tmpPrefSuffix;

     // retrieve if the message window should be docked
     m_isMessageWindowDocked[i] = 
       pSEQPrefs->getPrefBool(tmpPrefName, section, false);

     // construct the preference name
     tmpPrefName = QString("ShowMessageWindow") + tmpPrefSuffix;

     // and as appropriate, craete the message window
     if (pSEQPrefs->getPrefBool(tmpPrefName, section, false))
       showMessageWindow(i);
   }

   // should the compass be docked if it's created
   m_isCompassDocked = pSEQPrefs->getPrefBool("DockedCompass", section, true);

   //
   // Create the Player Skills listview (ONLY CREATED WHEN NEEDED FLOYD!!!!)
   //
   m_isSkillListDocked = pSEQPrefs->getPrefBool("DockedPlayerSkills", section, true);
   if (pSEQPrefs->getPrefBool("ShowPlayerSkills", section, true))
     showSkillList();

   //
   // Create the Player Status listview (ONLY CREATED WHEN NEEDED FLOYD!!!!)
   //
   m_isStatListDocked = pSEQPrefs->getPrefBool("DockedPlayerStats", section, true);
   if (pSEQPrefs->getPrefBool("ShowPlayerStats", section, true))
     showStatList();

   //
   // Create the compass as required
   //
   if (pSEQPrefs->getPrefBool("ShowCompass", section, false))
       showCompass();

   //
   // Create the spells listview as required (dynamic object)
   //
   m_isSpellListDocked = pSEQPrefs->getPrefBool("DockedSpellList", section, true);
   if (pSEQPrefs->getPrefBool("ShowSpellList", section, false))
       showSpellList();


   //
   // Create the Spawn List listview (always exists, just hidden if not specified)
   //
   m_isSpawnListDocked = pSEQPrefs->getPrefBool("DockedSpawnList", section, true);
   if (pSEQPrefs->getPrefBool("ShowSpawnList", section, false))
     showSpawnList();

   //
   // Create the Spawn List2 listview (always exists, just hidden if not specified)
   //
   m_isSpawnList2Docked = pSEQPrefs->getPrefBool("DockedSpawnList2", section, true);
   if (pSEQPrefs->getPrefBool("ShowSpawnList2", section, true))
     showSpawnList2();

   //
   // Create the Spawn List listview (always exists, just hidden if not specified)
   //
   m_isSpawnPointListDocked = pSEQPrefs->getPrefBool("DockedSpawnPointList", section, true);
   if (pSEQPrefs->getPrefBool("ShowSpawnPointList", section, false))
     showSpawnPointList();

   //
   // Create the Net Statistics window as required
   // 
   if (pSEQPrefs->getPrefBool("ShowNetStats", section, false))
     showNetDiag();

   //
   // Create the Guild member List window as required
   if (pSEQPrefs->getPrefBool("ShowGuildList", section, false))
     showGuildList();

/////////////////////
// QMenuBar

   // The first call to menuBar() makes it exist
   menuBar()->addSeparator();

   createFileMenu();
   createViewMenu();
   createOptionsMenu();
   createNetworkMenu();
   createCharacterMenu();
   createFiltersMenu();
   createInterfaceMenu();
   createWindowMenu();
   createDebugMenu();

////////////////////
// QStatusBar creation
   createStatusBar();

/////////////////
// interface connections
   // connect EQInterface slots to its own signals
   connect(this, SIGNAL(restoreFonts(void)),
	   this, SLOT(restoreStatusFont(void)));

   // connect MapMgr slots to interface signals
   connect(this, SIGNAL(saveAllPrefs(void)),
	   m_mapMgr, SLOT(savePrefs(void)));

   // connect CategoryMgr slots to interface signals
   connect(this, SIGNAL(saveAllPrefs(void)),
	   m_categoryMgr, SLOT(savePrefs(void)));

   if (m_zoneMgr)
   {
     m_packet->connect2("OP_ZoneEntry", SP_Zone, DIR_Client,
			"ClientZoneEntryStruct", SZC_Match,
			m_zoneMgr, SLOT(zoneEntryClient(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_PlayerProfile", SP_Zone, DIR_Server,
			"uint8_t", SZC_None,
			m_zoneMgr, SLOT(zonePlayer(const uint8_t*, size_t)));
     m_packet->connect2("OP_ZoneChange", SP_Zone, DIR_Client|DIR_Server,
			"zoneChangeStruct", SZC_Match,
			m_zoneMgr, SLOT(zoneChange(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_NewZone", SP_Zone, DIR_Server,
			"uint8_t", SZC_None,
			m_zoneMgr, SLOT(zoneNew(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_SendZonePoints", SP_Zone, DIR_Server,
			"zonePointsStruct", SZC_None,
			m_zoneMgr, SLOT(zonePoints(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_DzSwitchInfo", SP_Zone, DIR_Server,
                        "dzSwitchInfo", SZC_None,
                        m_zoneMgr, SLOT(dynamicZonePoints(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_DzInfo", SP_Zone, DIR_Server,
                        "dzInfo", SZC_Match,
                        m_zoneMgr, SLOT(dynamicZoneInfo(const uint8_t*, size_t, uint8_t)));
   }

   if (m_groupMgr != 0)
   {
     connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)),
                        m_groupMgr, SLOT(player(const charProfileStruct*)));
     m_packet->connect2("OP_GroupUpdate", SP_Zone, DIR_Server,
                        "uint8_t", SZC_None,
                        m_groupMgr, SLOT(groupUpdate(const uint8_t*, size_t)));
     m_packet->connect2("OP_GroupFollow", SP_Zone, DIR_Server,
                        "groupFollowStruct", SZC_Match,
                        m_groupMgr, SLOT(addGroupMember(const uint8_t*)));
     m_packet->connect2("OP_GroupDisband", SP_Zone, DIR_Server,
                        "groupDisbandStruct", SZC_Match,
                        m_groupMgr, SLOT(removeGroupMember(const uint8_t*)));
     m_packet->connect2("OP_GroupDisband2", SP_Zone, DIR_Server,
                        "groupDisbandStruct", SZC_Match,
                        m_groupMgr, SLOT(removeGroupMember(const uint8_t*)));
     // connect GroupMgr slots to SpawnShell signals
     connect(m_spawnShell, SIGNAL(addItem(const Item*)),
	     m_groupMgr, SLOT(addItem(const Item*)));
     // connect GroupMgr slots to SpawnShell signals
     connect(m_spawnShell, SIGNAL(delItem(const Item*)),
	     m_groupMgr, SLOT(delItem(const Item*)));
     // connect GroupMgr slots to SpawnShell signals
     connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
	     m_groupMgr, SLOT(killSpawn(const Item*)));
   }

   if (m_dateTimeMgr)
   {
     // connect DateTimeMgr slots to EQPacket signals
     m_packet->connect2("OP_TimeOfDay", SP_Zone, DIR_Server,
			"timeOfDayStruct", SZC_Match, 
			m_dateTimeMgr, SLOT(timeOfDay(const uint8_t*)));

     // connect interface slots to DateTimeMgr signals
     connect(m_dateTimeMgr, SIGNAL(updatedDateTime(const QDateTime&)),
	     this, SLOT(updatedDateTime(const QDateTime&)));

     connect(m_dateTimeMgr, SIGNAL(syncDateTime(const QDateTime&)),
	     this, SLOT(syncDateTime(const QDateTime&)));
   }

   if (m_filterMgr)
   {
     connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)),
	     m_filterMgr, SLOT(loadZone(const QString&)));
     connect(m_zoneMgr, SIGNAL(zoneEnd(const QString&, const QString&)),
	     m_filterMgr, SLOT(loadZone(const QString&)));
     connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)),
	     m_filterMgr, SLOT(loadZone(const QString&)));
   }

   if (m_guildmgr)
   {
       /*
     m_packet->connect2("OP_GuildList", SP_World, DIR_Server, 
			"worldGuildListStruct", SZC_None,
			m_guildmgr, 
			SLOT(worldGuildList(const uint8_t*, size_t)));
            */

     m_packet->connect2("OP_GuildsInZoneList", SP_Zone, DIR_Server,
             "guildsInZoneListStruct", SZC_None, m_guildmgr,
             SLOT(guildsInZoneList(const uint8_t*, size_t)));

     m_packet->connect2("OP_NewGuildInZone", SP_Zone, DIR_Server,
             "newGuildInZoneStruct", SZC_None, m_guildmgr,
             SLOT(newGuildInZone(const uint8_t*, size_t)));

     connect(this, SIGNAL(guildList2text(QString)),
	     m_guildmgr, SLOT(guildList2text(QString)));

   }

   if (m_guildShell)
   {
     m_packet->connect2("OP_GuildMemberList", SP_Zone, DIR_Server,
			"uint8_t", SZC_None,
			m_guildShell,
			SLOT(guildMemberList(const uint8_t*, size_t)));
     m_packet->connect2("OP_GuildMemberUpdate", SP_Zone, DIR_Server,
			"GuildMemberUpdate", SZC_Match,
			m_guildShell,
			SLOT(guildMemberUpdate(const uint8_t*, size_t)));
   }

   if (m_messageShell)
   {
     m_packet->connect2("OP_CommonMessage", SP_Zone, DIR_Client|DIR_Server,
			"channelMessageStruct", SZC_None,
			m_messageShell,
			SLOT(channelMessage(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_FormattedMessage", SP_Zone, DIR_Server,
			"formattedMessageStruct", SZC_None,
			m_messageShell,
			SLOT(formattedMessage(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_SimpleMessage", SP_Zone, DIR_Server,
			"simpleMessageStruct", SZC_Match,
			m_messageShell,
			SLOT(simpleMessage(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_SpecialMesg", SP_Zone, DIR_Server,
			"specialMessageStruct", SZC_None,
			m_messageShell,
			SLOT(specialMessage(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_GuildMOTD", SP_Zone, DIR_Server,
			"guildMOTDStruct", SZC_None,
			m_messageShell,
			SLOT(guildMOTD(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_RandomReq", SP_Zone, DIR_Client,
			"randomReqStruct", SZC_Match,
			m_messageShell, SLOT(randomRequest(const uint8_t*)));
     m_packet->connect2("OP_RandomReply", SP_Zone, DIR_Server,
			"randomStruct", SZC_Match,
			m_messageShell, SLOT(random(const uint8_t*)));
     m_packet->connect2("OP_ConsentResponse", SP_Zone, DIR_Server,
			"consentResponseStruct", SZC_Match,
			m_messageShell, SLOT(consent(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_DenyResponse", SP_Zone, DIR_Server,
			"consentResponseStruct", SZC_Match,
			m_messageShell, SLOT(consent(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_Emote", SP_Zone, DIR_Server|DIR_Client,
			"emoteTextStruct", SZC_None,
			m_messageShell, SLOT(emoteText(const uint8_t*)));
     m_packet->connect2("OP_InspectAnswer", SP_Zone, DIR_Server,
			"inspectDataStruct", SZC_Match,
			m_messageShell, SLOT(inspectData(const uint8_t*)));
     m_packet->connect2("OP_MoneyOnCorpse", SP_Zone, DIR_Server,
			"moneyOnCorpseStruct", SZC_Match,
			m_messageShell, SLOT(moneyOnCorpse(const uint8_t*)));
     m_packet->connect2("OP_Logout", SP_Zone, DIR_Server,
			"none", SZC_Match,
			m_messageShell, SLOT(logOut(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_NewZone", SP_Zone, DIR_Server,
			"uint8_t", SZC_None,
			m_messageShell, SLOT(zoneNew(const uint8_t*, size_t, uint8_t)));
     connect(m_zoneMgr, SIGNAL(zoneBegin(const ClientZoneEntryStruct*, size_t, uint8_t)),
	     m_messageShell, SLOT(zoneEntryClient(const ClientZoneEntryStruct*)));
     connect(m_zoneMgr, SIGNAL(zoneChanged(const zoneChangeStruct*, size_t, uint8_t)),
	     m_messageShell, SLOT(zoneChanged(const zoneChangeStruct*, size_t, uint8_t)));
     connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)),
	     m_messageShell, SLOT(zoneBegin(const QString&)));
     connect(m_zoneMgr, SIGNAL(zoneEnd(const QString&, const QString&)),
	     m_messageShell, SLOT(zoneEnd(const QString&, const QString&)));
     connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)),
	     m_messageShell, SLOT(zoneChanged(const QString&)));

     m_packet->connect2("OP_MOTD", SP_World, DIR_Server,
			"worldMOTDStruct", SZC_None,
			m_messageShell, SLOT(worldMOTD(const uint8_t*)));
     m_packet->connect2("OP_MemorizeSpell", SP_Zone, DIR_Server|DIR_Client,
			"memSpellStruct", SZC_Match,
			m_messageShell, SLOT(handleSpell(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_BeginCast", SP_Zone, DIR_Server|DIR_Client,
			"beginCastStruct", SZC_Match,
			m_messageShell, SLOT(beginCast(const uint8_t*)));
     m_packet->connect2("OP_BuffFadeMsg", SP_Zone, DIR_Server|DIR_Client,
			"spellFadedStruct", SZC_None,
			m_messageShell, SLOT(spellFaded(const uint8_t*)));
     m_packet->connect2("OP_CastSpell", SP_Zone, DIR_Server|DIR_Client,
			"startCastStruct", SZC_Match,
			m_messageShell, SLOT(startCast(const uint8_t*)));
     connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)),
        m_messageShell, SLOT(player(const charProfileStruct*)));
     m_packet->connect2("OP_SkillUpdate", SP_Zone, DIR_Server,
			"skillIncStruct", SZC_Match,
			m_messageShell, SLOT(increaseSkill(const uint8_t*)));
     m_packet->connect2("OP_LevelUpdate", SP_Zone, DIR_Server,
			"levelUpUpdateStruct", SZC_Match,
			m_messageShell, SLOT(updateLevel(const uint8_t*)));

     m_packet->connect2("OP_Consider", SP_Zone, DIR_Server,
			"considerStruct", SZC_Match,
			m_messageShell, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));

     connect(m_player, SIGNAL(setExp(uint32_t, uint32_t, uint32_t, uint32_t, 
				     uint32_t)),
	     m_messageShell, SLOT(setExp(uint32_t, uint32_t, uint32_t, 
					 uint32_t, uint32_t)));
     connect(m_player, SIGNAL(newExp(uint32_t, uint32_t, uint32_t, uint32_t, 
				     uint32_t, uint32_t)),
	     m_messageShell, SLOT(newExp(uint32_t, uint32_t, uint32_t,  
					 uint32_t, uint32_t, uint32_t)));
     connect(m_player, SIGNAL(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)),
	     m_messageShell, SLOT(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)));
     connect(m_player, SIGNAL(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t,
					uint32_t, uint32_t)),
	     m_messageShell, SLOT(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t,
					    uint32_t, uint32_t)));

     connect(m_spawnShell, SIGNAL(addItem(const Item*)),
	     m_messageShell, SLOT(addItem(const Item*)));
     connect(m_spawnShell, SIGNAL(delItem(const Item*)),
	     m_messageShell, SLOT(delItem(const Item*)));
     connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
	     m_messageShell, SLOT(killSpawn(const Item*)));

     connect(m_dateTimeMgr, SIGNAL(syncDateTime(const QDateTime&)),
	     m_messageShell, SLOT(syncDateTime(const QDateTime&)));

// 9/3/2008 - Removed.  Serialized packet now.
//      m_packet->connect2("OP_GroupUpdate", SP_Zone, DIR_Server,
// 			"groupUpdateStruct", SZC_None,
// 			m_messageShell, SLOT(groupUpdate(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_GroupInvite", SP_Zone, DIR_Client|DIR_Server,
			               "groupInviteStruct", SZC_None,
			               m_messageShell, SLOT(groupInvite(const uint8_t*, size_t, uint8_t)));
//      m_packet->connect2("OP_GroupInvite", SP_Zone, DIR_Server,
//                         "groupAltInviteStruct", SZC_Match,
//                         m_messageShell, SLOT(groupInvite(const uint8_t*)));
     m_packet->connect2("OP_GroupInvite2", SP_Zone, DIR_Client,
                        "groupInviteStruct", SZC_None,
                        m_messageShell, SLOT(groupInvite(const uint8_t*, size_t, uint8_t)));
     m_packet->connect2("OP_GroupFollow", SP_Zone, DIR_Server,
			"groupFollowStruct", SZC_Match,
                        m_messageShell, SLOT(groupFollow(const uint8_t*)));
     m_packet->connect2("OP_GroupFollow2", SP_Zone, DIR_Server,
                        "groupFollowStruct", SZC_Match,
                        m_messageShell, SLOT(groupFollow(const uint8_t*)));
     m_packet->connect2("OP_GroupDisband", SP_Zone, DIR_Server,
			"groupDisbandStruct", SZC_Match,
                        m_messageShell, SLOT(groupDisband(const uint8_t*)));
     m_packet->connect2("OP_GroupDisband2", SP_Zone, DIR_Server,
                        "groupDisbandStruct", SZC_Match,
                        m_messageShell, SLOT(groupDisband(const uint8_t*)));
     m_packet->connect2("OP_GroupCancelInvite", SP_Zone, DIR_Server|DIR_Client,
			"groupDeclineStruct", SZC_Match,
			m_messageShell, SLOT(groupDecline(const uint8_t*)));
     m_packet->connect2("OP_GroupLeader", SP_Zone, DIR_Server,
                        "groupLeaderChangeStruct", SZC_Match,
                        m_messageShell, SLOT(groupLeaderChange(const uint8_t*)));
   }

   if (m_filterNotifications)
   {
     connect(m_spawnShell, SIGNAL(addItem(const Item*)),
	     m_filterNotifications, SLOT(addItem(const Item*)));
     connect(m_spawnShell, SIGNAL(delItem(const Item*)),
	     m_filterNotifications, SLOT(delItem(const Item*)));
     connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
	     m_filterNotifications, SLOT(killSpawn(const Item*)));
     connect(m_spawnShell, SIGNAL(changeItem(const Item*, uint32_t)),
	     m_filterNotifications, 
	     SLOT(changeItem(const Item*, uint32_t)));
   }

   // connect interface slots to Packet signals
   m_packet->connect2("OP_TargetMouse", SP_Zone, DIR_Client|DIR_Server,
		      "clientTargetStruct", SZC_Match,
		      this, SLOT(clientTarget(const uint8_t*)));
#if 0 // ZBTEMP
   connect(m_packet, SIGNAL(attack2Hand1(const uint8_t*, size_t, uint8_t)), 
	   this, SLOT(attack2Hand1(const uint8_t*)));
#endif
   m_packet->connect2("OP_Action2", SP_Zone, DIR_Client|DIR_Server,
		      "action2Struct", SZC_Match,
		      this, SLOT(action2Message(const uint8_t*)));
   m_packet->connect2("OP_Death", SP_Zone, DIR_Server,
		      "newCorpseStruct", SZC_Match,
		      this, SLOT(combatKillSpawn(const uint8_t*)));
#if 0 // ZBTEMP
   connect(m_packet, SIGNAL(interruptSpellCast(const uint8_t*, size_t, uint8_t)),
	   this, SLOT(interruptSpellCast(const uint8_t*)));
   connect(m_packet, SIGNAL(moneyUpdate(const uint8_t*, size_t, uint8_t)),
	   this, SLOT(moneyUpdate(const uint8_t*)));
   connect(m_packet, SIGNAL(moneyThing(const uint8_t*, size_t, uint8_t)),
	   this, SLOT(moneyThing(const uint8_t*)));
#endif // ZBTEMP

   connect(m_packet, SIGNAL(toggle_session_tracking(bool)),
	   this, SLOT(toggle_net_session_tracking(bool)));
   
   // connect EQInterface slots to ZoneMgr signals
  connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)),
	  this, SLOT(zoneBegin(const QString&)));
  connect(m_zoneMgr, SIGNAL(zoneEnd(const QString&, const QString&)),
	  this, SLOT(zoneEnd(const QString&, const QString&)));
  connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)),
	  this, SLOT(zoneChanged(const QString&)));

   // connect the SpellShell slots to EQInterface signals
   connect(this, SIGNAL(spellMessage(QString&)),
	   m_spellShell, SLOT(spellMessage(QString&)));

   // connect EQInterface slots to SpawnShell signals
   connect(m_spawnShell, SIGNAL(addItem(const Item*)),
	   this, SLOT(addItem(const Item*)));
   connect(m_spawnShell, SIGNAL(delItem(const Item*)),
	   this, SLOT(delItem(const Item*)));
   connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
	   this, SLOT(killSpawn(const Item*)));
   connect(m_spawnShell, SIGNAL(changeItem(const Item*, uint32_t)),
	   this, SLOT(changeItem(const Item*)));
   connect(m_spawnShell, SIGNAL(spawnConsidered(const Item*)),
	   this, SLOT(spawnConsidered(const Item*)));

   // connect the SpawnShell slots to Packet signals
   m_packet->connect2("OP_GroundSpawn", SP_Zone, DIR_Server,
		      "makeDropStruct", SZC_None,
		      m_spawnShell, SLOT(newGroundItem(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_ClickObject", SP_Zone, DIR_Server,
		      "remDropStruct", SZC_Match,
		      m_spawnShell, SLOT(removeGroundItem(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_SpawnDoor", SP_Zone, DIR_Server,
		      "doorStruct", SZC_Modulus,
		      m_spawnShell, SLOT(newDoorSpawns(const uint8_t*, size_t, uint8_t)));
// OP_NewSpawn is deprecated in the client
//    m_packet->connect2("OP_NewSpawn", SP_Zone, DIR_Server,
// 		      "spawnStruct", SZC_Match,
// 		      m_spawnShell, SLOT(newSpawn(const uint8_t*)));
   m_packet->connect2("OP_ZoneEntry", SP_Zone, DIR_Server,
                      "uint8_t", SZC_None,
		      m_spawnShell, SLOT(zoneEntry(const uint8_t*, size_t)));
   m_packet->connect2("OP_MobUpdate", SP_Zone, DIR_Server|DIR_Client,
		      "spawnPositionUpdate", SZC_Match,
		      m_spawnShell, SLOT(updateSpawns(const uint8_t*)));
   m_packet->connect2("OP_WearChange", SP_Zone, DIR_Server|DIR_Client,
		      "SpawnUpdateStruct", SZC_Match,
		      m_spawnShell, SLOT(updateSpawnInfo(const uint8_t*)));
   m_packet->connect2("OP_HPUpdate", SP_Zone, DIR_Server|DIR_Client,
		      "hpNpcUpdateStruct", SZC_Match,
		      m_spawnShell, SLOT(updateNpcHP(const uint8_t*)));
   m_packet->connect2("OP_DeleteSpawn", SP_Zone, DIR_Server|DIR_Client,
                      "deleteSpawnStruct", SZC_Match,
                      m_spawnShell, SLOT(deleteSpawn(const uint8_t*)));
   m_packet->connect2("OP_SpawnRename", SP_Zone, DIR_Server,
		      "spawnRenameStruct", SZC_Match,
		      m_spawnShell, SLOT(renameSpawn(const uint8_t*)));
   m_packet->connect2("OP_Illusion", SP_Zone, DIR_Server|DIR_Client,
		      "spawnIllusionStruct", SZC_Match,
		      m_spawnShell, SLOT(illusionSpawn(const uint8_t*)));
   m_packet->connect2("OP_SpawnAppearance", SP_Zone, DIR_Server|DIR_Client,
		      "spawnAppearanceStruct", SZC_Match,
		      m_spawnShell, SLOT(updateSpawnAppearance(const uint8_t*)));
   m_packet->connect2("OP_Death", SP_Zone, DIR_Server,
		      "newCorpseStruct", SZC_Match,
		      m_spawnShell, SLOT(killSpawn(const uint8_t*)));
//    m_packet->connect2("OP_RespawnFromHover", SP_Zone, DIR_Server|DIR_Client,
// 		      "uint8_t", SZC_None,
//                       m_spawnShell, SLOT(respawnFromHover(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_Shroud", SP_Zone, DIR_Server,
                      "spawnShroudSelf", SZC_None,
                      m_spawnShell, SLOT(shroudSpawn(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_RemoveSpawn", SP_Zone, DIR_Server|DIR_Client,
                      "removeSpawnStruct", SZC_None,
                      m_spawnShell, SLOT(removeSpawn(const uint8_t*, size_t, uint8_t)));
#if 0 // ZBTEMP
   connect(m_packet, SIGNAL(spawnWearingUpdate(const uint8_t*, size_t, uint8_t)),
	   m_spawnShell, SLOT(spawnWearingUpdate(const uint8_t*)));
#endif
   m_packet->connect2("OP_Consider", SP_Zone, DIR_Server|DIR_Client,
		      "considerStruct", SZC_Match,
		      m_spawnShell, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_NpcMoveUpdate", SP_Zone, DIR_Server,
		      "uint8_t", SZC_None,
		      m_spawnShell, SLOT(npcMoveUpdate(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_ClientUpdate", SP_Zone, DIR_Server,
		      "playerSpawnPosStruct", SZC_Match,
		      m_spawnShell, SLOT(playerUpdate(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_CorpseLocResponse", SP_Zone, DIR_Server,
		      "corpseLocStruct", SZC_Match,
		      m_spawnShell, SLOT(corpseLoc(const uint8_t*)));
#if 0 // No longer used as of 5-22-2008
   m_packet->connect2("OP_ZoneSpawns", SP_Zone, DIR_Server,
		      "spawnStruct", SZC_None,
		      m_spawnShell, SLOT(zoneSpawns(const uint8_t*, size_t)));
#endif

   // connect the SpellShell slots to ZoneMgr signals
   connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)),
	   m_spellShell, SLOT(zoneChanged()));

   // connect the SpellShell slots to SpawnShell signals
   connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
	   m_spellShell, SLOT(killSpawn(const Item*)));

   // connect the SpellShell slots to Player signals
   connect(m_player, SIGNAL(newPlayer(void)),
	   m_spellShell, SLOT(clear()));
   connect(m_player, SIGNAL(buffLoad(const spellBuff *)), 
	   m_spellShell, SLOT(buffLoad(const spellBuff *)));

   // connect the SpellShell slots to EQPacket signals
   m_packet->connect2("OP_CastSpell", SP_Zone, DIR_Server|DIR_Client,
		      "startCastStruct", SZC_Match,
		      m_spellShell, SLOT(selfStartSpellCast(const uint8_t*)));
   m_packet->connect2("OP_Buff", SP_Zone, DIR_Server|DIR_Client,
		      "buffStruct", SZC_Match,
		      m_spellShell, SLOT(buff(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_Action", SP_Zone, DIR_Server|DIR_Client,
		      "actionStruct", SZC_Match,
		      m_spellShell, SLOT(action(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_Action", SP_Zone, DIR_Server|DIR_Client,
		      "actionAltStruct", SZC_Match,
		      m_spellShell, SLOT(action(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_SimpleMessage", SP_Zone, DIR_Server,
		      "simpleMessageStruct", SZC_Match,
		      m_spellShell,
		      SLOT(simpleMessage(const uint8_t*, size_t, uint8_t)));


   // connect Player slots to EQPacket signals
   connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)),
       m_player, SLOT(player(const charProfileStruct*)));
   m_packet->connect2("OP_SkillUpdate", SP_Zone, DIR_Server,
		      "skillIncStruct", SZC_Match,
		      m_player, SLOT(increaseSkill(const uint8_t*)));
   m_packet->connect2("OP_ManaChange", SP_Zone, DIR_Server,
		      "manaDecrementStruct", SZC_Match,
		      m_player, SLOT(manaChange(const uint8_t*)));
   m_packet->connect2("OP_ClientUpdate", SP_Zone, DIR_Server|DIR_Client,
		      "playerSelfPosStruct", SZC_Match,
		      m_player, SLOT(playerUpdateSelf(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_ExpUpdate", SP_Zone, DIR_Server,
		      "expUpdateStruct", SZC_Match,
		      m_player, SLOT(updateExp(const uint8_t*)));
   m_packet->connect2("OP_AAExpUpdate", SP_Zone, DIR_Server,
		      "altExpUpdateStruct", SZC_Match,
		      m_player, SLOT(updateAltExp(const uint8_t*)));
   m_packet->connect2("OP_LevelUpdate", SP_Zone, DIR_Server,
		      "levelUpUpdateStruct", SZC_Match,
		      m_player, SLOT(updateLevel(const uint8_t*)));
   m_packet->connect2("OP_HPUpdate", SP_Zone, DIR_Server|DIR_Client,
		      "hpNpcUpdateStruct", SZC_Match,
		      m_player, SLOT(updateNpcHP(const uint8_t*)));
   m_packet->connect2("OP_WearChange", SP_Zone, DIR_Server|DIR_Client,
		      "SpawnUpdateStruct", SZC_Match,
		      m_player, SLOT(updateSpawnInfo(const uint8_t*)));
   m_packet->connect2("OP_Stamina", SP_Zone, DIR_Server,
		      "staminaStruct", SZC_Match,
		      m_player, SLOT(updateStamina(const uint8_t*)));
   m_packet->connect2("OP_Consider", SP_Zone, DIR_Server|DIR_Client,
		      "considerStruct", SZC_Match,
		      m_player, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));
   m_packet->connect2("OP_SwapSpell", SP_Zone, DIR_Server,
		      "tradeSpellBookSlotsStruct", SZC_Match,
	   m_player, SLOT(tradeSpellBookSlots(const uint8_t*, size_t, uint8_t)));

   // interface statusbar slots
   connect (this, SIGNAL(newZoneName(const QString&)),
            m_stsbarZone, SLOT(setText(const QString&)));
   connect (m_packet, SIGNAL(stsMessage(const QString &, int)),
            this, SLOT(stsMessage(const QString &, int)));
   connect (m_spawnShell, SIGNAL(numSpawns(int)),
            this, SLOT(numSpawns(int)));
   connect (m_packet, SIGNAL(numPacket(int, int)),
            this, SLOT(numPacket(int, int)));
   connect (m_packet, SIGNAL(resetPacket(int, int)),
            this, SLOT(resetPacket(int, int)));
   connect (m_player, SIGNAL(newSpeed(double)),
            this, SLOT(newSpeed(double)));
   connect(m_player, SIGNAL(setExp(uint32_t, uint32_t, uint32_t, uint32_t, 
				   uint32_t)),
	   this, SLOT(setExp(uint32_t, uint32_t, uint32_t, 
			     uint32_t, uint32_t)));
   connect(m_player, SIGNAL(newExp(uint32_t, uint32_t, uint32_t, uint32_t, 
				   uint32_t, uint32_t)),
	   this, SLOT(newExp(uint32_t, uint32_t, uint32_t,  
			     uint32_t, uint32_t, uint32_t)));
   connect(m_player, SIGNAL(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)),
	   this, SLOT(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)));
   connect(m_player, SIGNAL(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t,
				      uint32_t, uint32_t)),
	   this, SLOT(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t,
				uint32_t, uint32_t)));
   connect(m_player, SIGNAL(levelChanged(uint8_t)),
	   this, SLOT(levelChanged(uint8_t)));


   if (m_expWindow != 0)
   {
     // connect ExperienceWindow slots to Player signals
     connect(m_player, SIGNAL(newPlayer(void)),
	     m_expWindow, SLOT(clear(void)));
     connect(m_player, SIGNAL(expGained(const QString &, int, long, QString )),
	     m_expWindow, SLOT(addExpRecord(const QString &, int, long,QString )));

     // connect ExperienceWindow slots to EQInterface signals
     connect(this, SIGNAL(restoreFonts(void)),
	     m_expWindow, SLOT(restoreFont(void)));
     connect(this, SIGNAL(saveAllPrefs(void)),
	     m_expWindow, SLOT(savePrefs(void)));
   }

   if (m_combatWindow != 0)
   {
     // connect CombatWindow slots to the signals
     connect(m_player, SIGNAL(newPlayer(void)),
	     m_combatWindow, SLOT(clear(void)));
     connect (this, SIGNAL(combatSignal(int, int, int, int, int, QString, QString)),
	      m_combatWindow, SLOT(addCombatRecord(int, int, int, int, int, QString, QString)));
     connect (m_spawnShell, SIGNAL(spawnConsidered(const Item*)),
	      m_combatWindow, SLOT(resetDPS()));
     connect(this, SIGNAL(restoreFonts(void)),
	     m_combatWindow, SLOT(restoreFont(void)));
     connect(this, SIGNAL(saveAllPrefs(void)),
	     m_combatWindow, SLOT(savePrefs(void)));
   }


   //
   // Geometry Configuration
   //


   // interface components

   // set mainwindow Geometry
   section = "Interface";

   // The way window/dock state was saved with Qt3 won't work with Qt4+, so
   // we use the Qt-provided functions instead.  Unforunately, that means the
   // size/position preferences saved in the Qt3 version won't be usable, and
   // the users will have to redo their window/dock size/placement.
   QString dockPrefsState = pSEQPrefs->getPrefString("DockingInfoState",
           section, QString());
   QString dockPrefsGeometry = pSEQPrefs->getPrefString("DockingInfoGeometry",
           section, QString());

   bool usePos = pSEQPrefs->getPrefBool("UseWindowPos", section, true);
   if (usePos)
   {
       if (!restoreGeometry(QByteArray::fromBase64(dockPrefsGeometry.toLatin1())))
           seqWarn("Could not restore dock/window geometry.  Rearrange windows "
                   "as desired and then re-save preferences");
       else
           // work around QTBUG-46620
           if (isMaximized())
               setGeometry(QApplication::desktop()->availableGeometry(this));

       if (!restoreState(QByteArray::fromBase64(dockPrefsState.toLatin1())))
           seqWarn("Could not restore dock/window state.  Rearrange windows as"
                   " desired and then re-save prefrences");
   }

   new QShortcut(Qt::CTRL|Qt::ALT|Qt::Key_S, this, SLOT(toggle_view_statusbar()));
   new QShortcut(Qt::CTRL|Qt::ALT|Qt::Key_T, this, SLOT(toggle_view_menubar()));

   // Set main window title
   // TODO: Add % replacement values and a signal to update, for ip address currently
   // TODO: being monitored.

   QMainWindow::setWindowTitle(pSEQPrefs->getPrefString("Caption", section,
               "ShowEQ - Main (ctrl+alt+t to toggle menubar)"));

   show();


   // load the format strings for display
   loadFormatStrings();

   /* Start the packet capturing */
   m_packet->start (10);
}// end constructor
////////////////////

EQInterface::~EQInterface()
{
  if (m_netDiag != 0)
    delete m_netDiag;

  if (m_spawnPointList != 0)
    delete m_spawnPointList;

  if (m_statList != 0)
    delete m_statList;

  if (m_guildListWindow != 0)
    delete m_guildListWindow;

  if (m_skillList != 0)
    delete m_skillList;

  if (m_spellList != 0)
    delete m_spellList;

  if (m_spawnList2 != 0)
    delete m_spawnList2;

  if (m_spawnList != 0)
    delete m_spawnList;

  for (int i = 0; i < maxNumMaps; i++)
    if (m_map[i] != 0)
      delete m_map[i];

  for (int i = 0; i < maxNumMessageWindows; i++)
    if (m_messageWindow[i] != 0)
      delete m_messageWindow[i];

  if (m_combatWindow != 0)
    delete m_combatWindow;

  if (m_expWindow != 0)
    delete m_expWindow;

  if (m_spawnLogger != 0)
    delete m_spawnLogger;

  if (m_spawnMonitor != 0)
    delete m_spawnMonitor;

  if (m_groupMgr != 0)
    delete m_groupMgr;

  if (m_spellShell != 0)
    delete m_spellShell;
  
  if (m_spells != 0)
    delete m_spells;

  if (m_mapMgr != 0)
    delete m_mapMgr;

  if (m_spawnShell != 0)
    delete m_spawnShell;

  if (m_categoryMgr != 0)
    delete m_categoryMgr;

  if (m_filterMgr != 0)
    delete m_filterMgr;

  if (m_dateTimeMgr != 0)
    delete m_dateTimeMgr;

  if (m_eqStrings != 0)
    delete m_eqStrings;

  if (m_player != 0)
    delete m_player;

  if (m_guildShell != 0)
    delete m_guildShell;

  if (m_guildmgr != 0)
    delete m_guildmgr;

  if (m_zoneMgr != 0)
    delete m_zoneMgr;
  
  if (m_packet != 0)
    delete m_packet;
}

void EQInterface::createFileMenu() {

   // File Menu
   QMenu* pFileMenu = new QMenu("&File");
   menuBar()->addMenu(pFileMenu);
   pFileMenu->addAction("&Save Preferences", this, SLOT(savePrefs()),
           Qt::CTRL|Qt::Key_S);
   pFileMenu->addAction("Open &Map", m_mapMgr, SLOT(loadMap()), Qt::Key_F1);
   pFileMenu->addAction("&Import &Map", m_mapMgr, SLOT(importMap()));
   pFileMenu->addAction("Sa&ve Map", m_mapMgr, SLOT(saveMap()), Qt::Key_F2);
   pFileMenu->addAction("Save SOE Map", m_mapMgr, SLOT(saveSOEMap()));
   pFileMenu->addAction("Reload Guilds File", m_guildmgr, SLOT(readGuildList()));
   pFileMenu->addAction("Save Guilds File", m_guildmgr, SLOT(writeGuildList()));
   pFileMenu->addAction("Add Spawn Category", this, SLOT(addCategory()),
           Qt::ALT|Qt::Key_C);
   pFileMenu->addAction("Rebuild SpawnList", this, SLOT(rebuildSpawnList()),
           Qt::ALT|Qt::Key_R);
   pFileMenu->addAction("Reload Categories", this, SLOT(reloadCategories()),
           Qt::CTRL|Qt::Key_R);
   pFileMenu->addAction("Select Next", this, SLOT(selectNext()),
           Qt::CTRL|Qt::Key_Right);
   pFileMenu->addAction("Select Prev", this, SLOT(selectPrev()),
           Qt::CTRL|Qt::Key_Left);
   pFileMenu->addAction("Save Selected Spawns Path", this,
           SLOT(saveSelectedSpawnPath(void)));
   pFileMenu->addAction("Save NPC Spawn Paths", this, SLOT(saveSpawnPaths(void)));
   if (m_packet->playbackPackets() != PLAYBACK_OFF)
   {
     pFileMenu->addAction("Inc Playback Speed", m_packet, SLOT(incPlayback()),
             Qt::CTRL|Qt::Key_X);
     pFileMenu->addAction("Dec Playback Speed", m_packet, SLOT(decPlayback()),
             Qt::CTRL|Qt::Key_Z);
   }
   pFileMenu->addAction("&Quit", qApp, SLOT(quit()));
}

void EQInterface::createViewMenu() {

   QString section = "Interface";

   // View menu
   QMenu* pViewMenu = new QMenu("&View");
   menuBar()->addMenu(pViewMenu);

   m_action_view_ExpWindow = pViewMenu->addAction("Experience Window",
           this, SLOT(toggle_view_ExpWindow()));
   m_action_view_ExpWindow->setCheckable(true);

   m_action_view_CombatWindow = pViewMenu->addAction("Combat Window",
           this, SLOT(toggle_view_CombatWindow()));
   m_action_view_CombatWindow->setCheckable(true);

   pViewMenu->addSeparator();

   m_action_view_SpellList = pViewMenu->addAction("Spell List", this,
           SLOT(toggle_view_SpellList()));
   m_action_view_SpellList->setCheckable(true);

   m_action_view_SpawnList = pViewMenu->addAction("Spawn List", this,
           SLOT(toggle_view_SpawnList()));
   m_action_view_SpawnList->setCheckable(true);

   m_action_view_SpawnList2 = pViewMenu->addAction("Spawn List 2", this,
           SLOT(toggle_view_SpawnList2()));
   m_action_view_SpawnList2->setCheckable(true);

   m_action_view_SpawnPointList = pViewMenu->addAction("Spawn Point List",
           this, SLOT(toggle_view_SpawnPointList()));
   m_action_view_SpawnPointList->setCheckable(true);

   m_action_view_PlayerStats = pViewMenu->addAction("Player Stats", this,
           SLOT(toggle_view_PlayerStats()));
   m_action_view_PlayerStats->setCheckable(true);
   m_action_view_PlayerStats->setChecked(m_statList != 0);

   m_action_view_PlayerSkills = pViewMenu->addAction("Player Skills", this,
           SLOT(toggle_view_PlayerSkills()));
   m_action_view_PlayerSkills->setCheckable(true);

   m_action_view_Compass = pViewMenu->addAction("Compass", this,
           SLOT(toggle_view_Compass()));
   m_action_view_Compass->setCheckable(true);

   QMenu* subMenu = new QMenu("Maps");
   for (int i = 0; i < maxNumMaps; i++)
   {
        QString mapName = "Map ";
        if (i > 0)
            mapName += QString::number(i + 1);
        m_action_view_Map[i] = subMenu->addAction(mapName);
        m_action_view_Map[i]->setCheckable(true);
        m_action_view_Map[i]->setData(i);
        m_action_view_Map[i]->setChecked(m_map[i] != 0);
   }
   pViewMenu->addMenu(subMenu);
   connect (subMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_view_Map(QAction*)));

   subMenu = new QMenu("ChannelMessages");
   QString messageWindowName;
   for (int i = 0; i < maxNumMessageWindows; i++)
   {
        messageWindowName = "Channel Messages ";
        if (i > 0)
            messageWindowName += QString::number(i + 1);
        m_action_view_MessageWindow[i] = subMenu->addAction(messageWindowName);
        m_action_view_MessageWindow[i]->setCheckable(true);
        m_action_view_MessageWindow[i]->setData(i);
        m_action_view_MessageWindow[i]->setChecked(m_messageWindow[i] != 0);
   }
   pViewMenu->addMenu(subMenu);
   connect (subMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_view_ChannelMsgs(QAction*)));

   m_action_view_NetDiag = pViewMenu->addAction("Network Diagnostics", this,
           SLOT(toggle_view_NetDiag()));
   m_action_view_NetDiag->setCheckable(true);

   m_action_view_GuildListWindow = pViewMenu->addAction("Guild Member List",
           this, SLOT(toggle_view_GuildList()));
   m_action_view_GuildListWindow->setCheckable(true);

   pViewMenu->addSeparator();

   // View -> PlayerStats
   m_statWinMenu = new QMenu("&Player Stats");
   m_action_view_PlayerStats_Options = pViewMenu->addMenu(m_statWinMenu);

   m_action_view_PlayerStats_Stats[LIST_HP] = m_statWinMenu->addAction("Hit Points");
   m_action_view_PlayerStats_Stats[LIST_HP]->setData(LIST_HP);
   m_action_view_PlayerStats_Stats[LIST_HP]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_MANA] = m_statWinMenu->addAction("Mana");
   m_action_view_PlayerStats_Stats[LIST_MANA]->setData(LIST_MANA);
   m_action_view_PlayerStats_Stats[LIST_MANA]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_STAM] = m_statWinMenu->addAction("Stamina");
   m_action_view_PlayerStats_Stats[LIST_STAM]->setData(LIST_STAM);
   m_action_view_PlayerStats_Stats[LIST_STAM]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_EXP] = m_statWinMenu->addAction("Experience");
   m_action_view_PlayerStats_Stats[LIST_EXP]->setData(LIST_EXP);
   m_action_view_PlayerStats_Stats[LIST_EXP]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_ALTEXP] = m_statWinMenu->addAction("Alt Experience");
   m_action_view_PlayerStats_Stats[LIST_ALTEXP]->setData(LIST_ALTEXP);
   m_action_view_PlayerStats_Stats[LIST_ALTEXP]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_FOOD] = m_statWinMenu->addAction("Food");
   m_action_view_PlayerStats_Stats[LIST_FOOD]->setData(LIST_FOOD);
   m_action_view_PlayerStats_Stats[LIST_FOOD]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_WATR] = m_statWinMenu->addAction("Water");
   m_action_view_PlayerStats_Stats[LIST_WATR]->setData(LIST_WATR);
   m_action_view_PlayerStats_Stats[LIST_WATR]->setCheckable(true);

   m_statWinMenu->addSeparator();

   m_action_view_PlayerStats_Stats[LIST_STR] = m_statWinMenu->addAction("Strength");
   m_action_view_PlayerStats_Stats[LIST_STR]->setData(LIST_STR);
   m_action_view_PlayerStats_Stats[LIST_STR]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_STA] = m_statWinMenu->addAction("Stamina");
   m_action_view_PlayerStats_Stats[LIST_STA]->setData(LIST_STA);
   m_action_view_PlayerStats_Stats[LIST_STA]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_CHA] = m_statWinMenu->addAction("Charisma");
   m_action_view_PlayerStats_Stats[LIST_CHA]->setData(LIST_CHA);
   m_action_view_PlayerStats_Stats[LIST_CHA]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_DEX] = m_statWinMenu->addAction("Dexterity");
   m_action_view_PlayerStats_Stats[LIST_DEX]->setData(LIST_DEX);
   m_action_view_PlayerStats_Stats[LIST_DEX]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_INT] = m_statWinMenu->addAction("Intelligence");
   m_action_view_PlayerStats_Stats[LIST_INT]->setData(LIST_INT);
   m_action_view_PlayerStats_Stats[LIST_INT]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_AGI] = m_statWinMenu->addAction("Agility");
   m_action_view_PlayerStats_Stats[LIST_AGI]->setData(LIST_AGI);
   m_action_view_PlayerStats_Stats[LIST_AGI]->setCheckable(true);

   m_action_view_PlayerStats_Stats[LIST_WIS] = m_statWinMenu->addAction("Wisdom");
   m_action_view_PlayerStats_Stats[LIST_WIS]->setData(LIST_WIS);
   m_action_view_PlayerStats_Stats[LIST_WIS]->setCheckable(true);

   m_statWinMenu->addSeparator();

   m_action_view_PlayerStats_Stats[LIST_MR] = m_statWinMenu->addAction("Magic Res");
   m_action_view_PlayerStats_Stats[LIST_MR]->setData(LIST_MR);
   m_action_view_PlayerStats_Stats[LIST_MR]->setCheckable(true);
   m_action_view_PlayerStats_Stats[LIST_MR]->setEnabled(false);

   m_action_view_PlayerStats_Stats[LIST_FR] = m_statWinMenu->addAction("Fire Res");
   m_action_view_PlayerStats_Stats[LIST_FR]->setData(LIST_FR);
   m_action_view_PlayerStats_Stats[LIST_FR]->setCheckable(true);
   m_action_view_PlayerStats_Stats[LIST_FR]->setEnabled(false);

   m_action_view_PlayerStats_Stats[LIST_CR] = m_statWinMenu->addAction("Cold Res");
   m_action_view_PlayerStats_Stats[LIST_CR]->setData(LIST_CR);
   m_action_view_PlayerStats_Stats[LIST_CR]->setCheckable(true);
   m_action_view_PlayerStats_Stats[LIST_CR]->setEnabled(false);

   m_action_view_PlayerStats_Stats[LIST_DR] = m_statWinMenu->addAction("Disease Res");
   m_action_view_PlayerStats_Stats[LIST_DR]->setData(LIST_DR);
   m_action_view_PlayerStats_Stats[LIST_DR]->setCheckable(true);
   m_action_view_PlayerStats_Stats[LIST_DR]->setEnabled(false);

   m_action_view_PlayerStats_Stats[LIST_PR] = m_statWinMenu->addAction("Poison Res");
   m_action_view_PlayerStats_Stats[LIST_PR]->setData(LIST_PR);
   m_action_view_PlayerStats_Stats[LIST_PR]->setCheckable(true);
   m_action_view_PlayerStats_Stats[LIST_PR]->setEnabled(false);

   m_statWinMenu->addSeparator();

   m_action_view_PlayerStats_Stats[LIST_AC] = m_statWinMenu->addAction("Armor Class");
   m_action_view_PlayerStats_Stats[LIST_AC]->setData(LIST_AC);
   m_action_view_PlayerStats_Stats[LIST_AC]->setCheckable(true);
   m_action_view_PlayerStats_Stats[LIST_AC]->setEnabled(false);

   connect (m_statWinMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_view_StatWin(QAction*)));

   // View -> PlayerSkills
   m_skillWinMenu = new QMenu("Player &Skills");
   m_action_view_PlayerSkills_Options = pViewMenu->addMenu(m_skillWinMenu);
   m_action_view_PlayerSkills_Options->setEnabled(m_skillList != 0);

   m_action_view_PlayerSkills_Languages = m_skillWinMenu->addAction("&Langauges");
   m_action_view_PlayerSkills_Languages->setCheckable(true);
   m_action_view_PlayerSkills_Languages->setData(0);

   connect (m_skillWinMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_view_SkillWin(QAction*)));

   // View -> SpawnList
   m_spawnListMenu = new QMenu("Spawn &List");
   m_action_view_SpawnList_Options = pViewMenu->addMenu(m_spawnListMenu);
   m_action_view_SpawnList_Options->setEnabled(m_spawnList != 0);

   m_action_view_SpawnList_Cols[tSpawnColName] = m_spawnListMenu->addAction("&Name");
   m_action_view_SpawnList_Cols[tSpawnColName]->setCheckable(true);
   m_action_view_SpawnList_Cols[tSpawnColName]->setData(tSpawnColName);

   m_action_view_SpawnList_Cols[tSpawnColLevel] = m_spawnListMenu->addAction("&Level");
   m_action_view_SpawnList_Cols[tSpawnColLevel]->setData(tSpawnColLevel);
   m_action_view_SpawnList_Cols[tSpawnColLevel]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColHP] = m_spawnListMenu->addAction("&HP");
   m_action_view_SpawnList_Cols[tSpawnColHP]->setData(tSpawnColHP);
   m_action_view_SpawnList_Cols[tSpawnColHP]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColGuildID] = m_spawnListMenu->addAction("Guild Tag");
   m_action_view_SpawnList_Cols[tSpawnColGuildID]->setData(tSpawnColGuildID);
   m_action_view_SpawnList_Cols[tSpawnColGuildID]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColMaxHP] = m_spawnListMenu->addAction("&Max HP");
   m_action_view_SpawnList_Cols[tSpawnColMaxHP]->setData(tSpawnColMaxHP);
   m_action_view_SpawnList_Cols[tSpawnColMaxHP]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColXPos] = m_spawnListMenu->addAction("Coord &1");
   m_action_view_SpawnList_Cols[tSpawnColXPos]->setData(tSpawnColXPos);
   m_action_view_SpawnList_Cols[tSpawnColXPos]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColYPos] = m_spawnListMenu->addAction("Coord &2");
   m_action_view_SpawnList_Cols[tSpawnColYPos]->setData(tSpawnColYPos);
   m_action_view_SpawnList_Cols[tSpawnColYPos]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColZPos] = m_spawnListMenu->addAction("Coord &3");
   m_action_view_SpawnList_Cols[tSpawnColZPos]->setData(tSpawnColZPos);
   m_action_view_SpawnList_Cols[tSpawnColZPos]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColID] = m_spawnListMenu->addAction("I&D");
   m_action_view_SpawnList_Cols[tSpawnColID]->setData(tSpawnColID);
   m_action_view_SpawnList_Cols[tSpawnColID]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColDist] = m_spawnListMenu->addAction("&Dist");
   m_action_view_SpawnList_Cols[tSpawnColDist]->setData(tSpawnColDist);
   m_action_view_SpawnList_Cols[tSpawnColDist]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColRace] = m_spawnListMenu->addAction("&Race");
   m_action_view_SpawnList_Cols[tSpawnColRace]->setData(tSpawnColRace);
   m_action_view_SpawnList_Cols[tSpawnColRace]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColClass] = m_spawnListMenu->addAction("&Class");
   m_action_view_SpawnList_Cols[tSpawnColClass]->setData(tSpawnColClass);
   m_action_view_SpawnList_Cols[tSpawnColClass]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColInfo] = m_spawnListMenu->addAction("&Info");
   m_action_view_SpawnList_Cols[tSpawnColInfo]->setData(tSpawnColInfo);
   m_action_view_SpawnList_Cols[tSpawnColInfo]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColSpawnTime] = m_spawnListMenu->addAction("Spawn &Time");
   m_action_view_SpawnList_Cols[tSpawnColSpawnTime]->setData(tSpawnColSpawnTime);
   m_action_view_SpawnList_Cols[tSpawnColSpawnTime]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColDeity] = m_spawnListMenu->addAction("&Deity");
   m_action_view_SpawnList_Cols[tSpawnColDeity]->setData(tSpawnColDeity);
   m_action_view_SpawnList_Cols[tSpawnColDeity]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColBodyType] = m_spawnListMenu->addAction("&Body Type");
   m_action_view_SpawnList_Cols[tSpawnColBodyType]->setData(tSpawnColBodyType);
   m_action_view_SpawnList_Cols[tSpawnColBodyType]->setCheckable(true);

   m_action_view_SpawnList_Cols[tSpawnColGuildID] = m_spawnListMenu->addAction("Guild Tag");
   m_action_view_SpawnList_Cols[tSpawnColGuildID]->setData(tSpawnColGuildID);
   m_action_view_SpawnList_Cols[tSpawnColGuildID]->setCheckable(true);

   connect (m_spawnListMenu, SIGNAL(triggered(QAction*)),
           this, SLOT(toggle_view_SpawnListCol(QAction*)));

   pViewMenu->addSeparator();

   QAction* tmpAction;
   // View -> DockedWin
   m_dockedWinMenu = new QMenu("&Docked");
   tmpAction = pViewMenu->addMenu(m_dockedWinMenu);

   tmpAction = m_dockedWinMenu->addAction("Spawn &List");
   tmpAction->setCheckable(true);
   tmpAction->setData(0);
   tmpAction->setChecked(m_isSpawnListDocked);

   tmpAction = m_dockedWinMenu->addAction("Spawn &List 2");
   tmpAction->setCheckable(true);
   tmpAction->setData(6);
   tmpAction->setChecked(m_isSpawnList2Docked);

   tmpAction = m_dockedWinMenu->addAction("Spawn P&oint List");
   tmpAction->setCheckable(true);
   tmpAction->setData(5);
   tmpAction->setChecked(m_isSpawnPointListDocked);

   tmpAction = m_dockedWinMenu->addAction("&Player Stats");
   tmpAction->setCheckable(true);
   tmpAction->setData(1);
   tmpAction->setChecked(m_isStatListDocked);

   tmpAction = m_dockedWinMenu->addAction("Player &Skills");
   tmpAction->setCheckable(true);
   tmpAction->setData(2);
   tmpAction->setChecked(m_isSkillListDocked);

   tmpAction = m_dockedWinMenu->addAction("Sp&ell List");
   tmpAction->setCheckable(true);
   tmpAction->setData(3);
   tmpAction->setChecked(m_isSpellListDocked);

   tmpAction = m_dockedWinMenu->addAction("&Compass");
   tmpAction->setCheckable(true);
   tmpAction->setData(4);
   tmpAction->setChecked(m_isCompassDocked);

   // insert Map docking options
   // NOTE: Always insert Map docking options at the end of the Docked menu
   for (int i = 0; i < maxNumMaps; i++)
   {
        QString mapName = "Map ";
        if (i > 0)
            mapName += QString::number(i + 1);
        tmpAction = m_dockedWinMenu->addAction(mapName);
        tmpAction->setCheckable(true);
        tmpAction->setData(i + mapDockBase);
        tmpAction->setChecked(m_isMapDocked[i]);
   }

   connect (m_dockedWinMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_view_DockedWin(QAction*)));

   // View -> DockableWin
   m_dockableWinMenu = new QMenu("&Dockable");
   pViewMenu->addMenu(m_dockableWinMenu);

   tmpAction = m_dockableWinMenu->addAction("Spawn &List");
   tmpAction->setCheckable(true);
   tmpAction->setData(0);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableSpawnList", section, true));

   tmpAction = m_dockableWinMenu->addAction("Spawn &List 2");
   tmpAction->setCheckable(true);
   tmpAction->setData(6);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableSpawnList2", section, true));

   tmpAction = m_dockableWinMenu->addAction("Spawn P&oint List");
   tmpAction->setCheckable(true);
   tmpAction->setData(5);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableSpawnPointList", section, true));

   tmpAction = m_dockableWinMenu->addAction("&Player Stats");
   tmpAction->setCheckable(true);
   tmpAction->setData(1);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockablePlayerStats", section, true));

   tmpAction = m_dockableWinMenu->addAction("Player &Skills");
   tmpAction->setCheckable(true);
   tmpAction->setData(2);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockablePlayerSkills", section, true));

   tmpAction = m_dockableWinMenu->addAction("Sp&ell List");
   tmpAction->setCheckable(true);
   tmpAction->setData(3);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableSpellList", section, true));

   tmpAction = m_dockableWinMenu->addAction("&Compass");
   tmpAction->setCheckable(true);
   tmpAction->setData(4);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableCompass", section, true));

   tmpAction = m_dockableWinMenu->addAction("E&xperience Window");
   tmpAction->setCheckable(true);
   tmpAction->setData(7);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableExperienceWindow", section, false));

   tmpAction = m_dockableWinMenu->addAction("Com&bat Window");
   tmpAction->setCheckable(true);
   tmpAction->setData(8);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableCombatWindow", section, false));

   tmpAction = m_dockableWinMenu->addAction("&Guild List");
   tmpAction->setCheckable(true);
   tmpAction->setData(9);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableGuildListWindow", section, true));

   tmpAction = m_dockableWinMenu->addAction("&Net Diag");
   tmpAction->setCheckable(true);
   tmpAction->setData(10);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("DockableNetDialog", section, true));

   // insert Map docking options
   subMenu = new QMenu("Maps");
   for (int i = 0; i < maxNumMaps; i++)
   {
        QString mapName = "Map ";
        QString mapPrefName = "Map";
        if (i > 0)
        {
            mapName += QString::number(i + 1);
            mapPrefName + QString::number(i + 1);
        }
        tmpAction = subMenu->addAction(mapName);
        tmpAction->setCheckable(true);
        tmpAction->setData(i + mapDockBase);
        tmpAction->setChecked(pSEQPrefs->getPrefBool(QString("Dockable") + mapName,
                    section, true));
   }
   connect (subMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_view_DockableWin(QAction*)));
   m_dockableWinMenu->addMenu(subMenu);

   // insert Message Window docking options
   subMenu = new QMenu("Channel Messages");
   QString messagePrefName = "DockableMessageWindow";
   for (int i = 0; i < maxNumMessageWindows; i++)
   {
       QString messageWindowName = "Channel Messages ";
       if (i > 0)
           messageWindowName += QString::number(i + 1);
       tmpAction = subMenu->addAction(messageWindowName);
       tmpAction->setCheckable(true);
       tmpAction->setData(i + messageWindowDockBase);
       tmpAction->setChecked(pSEQPrefs->getPrefBool(messagePrefName + QString::number(i),
                   section, false));
   }
   connect (subMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_view_DockableWin(QAction*)));
   m_dockableWinMenu->addMenu(subMenu);

   connect (m_dockableWinMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_view_DockableWin(QAction*)));

   // view menu checks are set by init_view_menu
   connect(pViewMenu, SIGNAL(aboutToShow()), this, SLOT(init_view_menu()));


}

void EQInterface::createOptionsMenu() {

   // Options Menu
   QMenu* pOptMenu = new QMenu("&Options");
   menuBar()->addMenu(pOptMenu);

   m_action_opt_Fast = pOptMenu->addAction("Fast Machine?", this,
       SLOT(toggle_opt_Fast()));
   m_action_opt_Fast->setCheckable(true);
   m_action_opt_Fast->setChecked(showeq_params->fast_machine);

   m_action_opt_ConSelect = pOptMenu->addAction("Select on Consider?", this,
       SLOT(toggle_opt_ConSelect()));
   m_action_opt_ConSelect->setCheckable(true);
   m_action_opt_ConSelect->setChecked(m_selectOnConsider);

   m_action_opt_TarSelect = pOptMenu->addAction("Select on Target?", this,
       SLOT(toggle_opt_TarSelect()));
   m_action_opt_TarSelect->setCheckable(true);
   m_action_opt_TarSelect->setChecked(m_selectOnTarget);

   m_action_opt_TarDeselect = pOptMenu->addAction("Deselect on Untarget?", this,
       SLOT(toggle_opt_TarDeselect()));
   m_action_opt_TarDeselect->setCheckable(true);
   m_action_opt_TarDeselect->setChecked(m_deselectOnUntarget);


   m_action_opt_KeepSelectedVisible = pOptMenu->addAction("Keep Selected Visible?",
       this, SLOT(toggle_opt_KeepSelectedVisible()));
   m_action_opt_KeepSelectedVisible->setCheckable(true);
   m_action_opt_KeepSelectedVisible->setChecked(showeq_params->keep_selected_visible);

   m_action_opt_LogSpawns = pOptMenu->addAction("Log Spawns", this,
       SLOT(toggle_opt_LogSpawns()));
   m_action_opt_LogSpawns->setCheckable(true);
   m_action_opt_LogSpawns->setChecked(m_spawnLogger != 0);

   m_action_opt_BazaarData = pOptMenu->addAction("Bazaar Searches",
       this, SLOT(toggle_opt_BazaarData()));
   m_action_opt_BazaarData->setCheckable(true);
   m_action_opt_BazaarData->setChecked(m_bazaarLog != 0);

   m_action_opt_ResetMana = pOptMenu->addAction("Reset Max Mana", this,
       SLOT(resetMaxMana()));

   m_action_opt_PvPTeams = pOptMenu->addAction("PvP Teams", this,
       SLOT(toggle_opt_PvPTeams()));
   m_action_opt_PvPTeams->setCheckable(true);
   m_action_opt_PvPTeams->setChecked(showeq_params->pvp);

   m_action_opt_PvPDeity = pOptMenu->addAction("PvP Deity", this,
       SLOT(toggle_opt_PvPDeity()));
   m_action_opt_PvPDeity->setCheckable(true);
   m_action_opt_PvPDeity->setChecked(showeq_params->deitypvp);

   QAction* tmpAction;
   tmpAction = pOptMenu->addAction("Create Unknown Spawns", this,
           SLOT(toggle_opt_CreateUnknownSpawns(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(showeq_params->createUnknownSpawns);

   tmpAction = pOptMenu->addAction("Use EQ Retarded Coordinates", this,
           SLOT(toggle_opt_RetardedCoords(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(showeq_params->retarded_coords);

   tmpAction = pOptMenu->addAction("Use Unix System Time for Spawn Time", this,
           SLOT(toggle_opt_SystimeSpawntime(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(showeq_params->systime_spawntime);

   tmpAction = pOptMenu->addAction("Record Spawn Walk Paths", this,
           SLOT(toggle_opt_WalkPathRecord(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(showeq_params->walkpathrecord);

   QMenu* subMenu = new QMenu("Walk Path Length");
   QSpinBox* walkPathLengthSpinBox = new QSpinBox(subMenu);
   walkPathLengthSpinBox->setMinimum(0);
   walkPathLengthSpinBox->setMaximum(8192);
   walkPathLengthSpinBox->setSingleStep(1);
   walkPathLengthSpinBox->setValue(showeq_params->walkpathlength);
   connect(walkPathLengthSpinBox, SIGNAL(valueChanged(int)), this,
       SLOT(set_opt_WalkPathLength(int)));
   QWidgetAction* walkPathLengthSpinBoxAction = new QWidgetAction(subMenu);
   walkPathLengthSpinBoxAction->setDefaultWidget(walkPathLengthSpinBox);
   subMenu->addAction(walkPathLengthSpinBoxAction);
   pOptMenu->addMenu(subMenu);

   // SaveState SubMenu
   QMenu* pSaveStateMenu = new QMenu("&Save State");
   pOptMenu->addMenu(pSaveStateMenu);

   tmpAction = pSaveStateMenu->addAction("&Player", this,
           SLOT(toggle_opt_save_PlayerState(bool)));
   tmpAction ->setCheckable(true);
   tmpAction ->setChecked(showeq_params->savePlayerState);

   tmpAction = pSaveStateMenu->addAction("&Zone", this,
       SLOT(toggle_opt_save_ZoneState(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(showeq_params->saveZoneState);

   tmpAction = pSaveStateMenu->addAction("&Spawns", this,
       SLOT(toggle_opt_save_Spawns(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(showeq_params->saveSpawns);

   pSaveStateMenu->addAction("Base &Filename...", this,
       SLOT(set_opt_save_BaseFilename(void)));

   pSaveStateMenu->addSeparator();

   subMenu = new QMenu("Spawn Save Frequency (s)");
   QSpinBox* saveFrequencySpinBox = new QSpinBox(subMenu);
   saveFrequencySpinBox->setMinimum(1);
   saveFrequencySpinBox->setMaximum(320);
   saveFrequencySpinBox->setSingleStep(1);
   saveFrequencySpinBox->setValue(showeq_params->saveSpawnsFrequency / 1000);
   connect(saveFrequencySpinBox, SIGNAL(valueChanged(int)), this,
       SLOT(set_opt_save_SpawnFrequency(int)));
   QWidgetAction* saveFrequencySpinBoxAction = new QWidgetAction(subMenu);
   saveFrequencySpinBoxAction->setDefaultWidget(saveFrequencySpinBox);
   subMenu->addAction(saveFrequencySpinBoxAction);
   pSaveStateMenu->addMenu(subMenu);

   pOptMenu->addAction("Clear Channel Messages",this,
       SLOT(opt_clearChannelMsgs()));

   // Con Color base menu
   QMenu* conColorBaseMenu = new QMenu("Con &Colors");
   tmpAction = conColorBaseMenu->addAction("Gray Spawn Base...");
   tmpAction->setData(tGraySpawn);

   tmpAction = conColorBaseMenu->addAction("Green Spawn Base...");
   tmpAction->setData(tGreenSpawn);

   tmpAction = conColorBaseMenu->addAction("Light Blue Spawn Base...");
   tmpAction->setData(tCyanSpawn);

   tmpAction = conColorBaseMenu->addAction("Blue Spawn Base...");
   tmpAction->setData(tBlueSpawn);

   tmpAction = conColorBaseMenu->addAction("Even Spawn...");
   tmpAction->setData(tEvenSpawn);

   tmpAction = conColorBaseMenu->addAction("Yellow Spawn Base...");
   tmpAction->setData(tYellowSpawn);

   tmpAction = conColorBaseMenu->addAction("Red Spawn Base...");
   tmpAction->setData(tRedSpawn);

   tmpAction = conColorBaseMenu->addAction("Unknown Spawn...");
   tmpAction->setData(tUnknownSpawn);

   connect(conColorBaseMenu, SIGNAL(triggered(QAction*)), this,
       SLOT(select_opt_conColorBase(QAction*)));

   pOptMenu->addMenu(conColorBaseMenu);

   pOptMenu->addAction("Map Colors...", this, SLOT(select_opt_mapColors()));

   m_action_opt_UseUpdateRadius = pOptMenu->addAction("Use EQ's Update Radius",
       this, SLOT(toggle_opt_UseUpdateRadius()));
   m_action_opt_UseUpdateRadius->setCheckable(true);
   m_action_opt_UseUpdateRadius->setChecked(showeq_params->useUpdateRadius);

}

void EQInterface::createNetworkMenu() {

   // Network Menu
   m_netMenu = new QMenu("&Network");
   menuBar()->addMenu(m_netMenu);

   m_netMenu->addAction("Monitor &Next EQ Client Seen", this,
           SLOT(set_net_monitor_next_client()));
   m_netMenu->addAction("Monitor EQ Client &IP Address...", this,
           SLOT(set_net_client_IP_address()));
   m_netMenu->addAction("Monitor EQ Client &MAC Address...", this,
           SLOT(set_net_client_MAC_address()));
   m_netMenu->addAction("Set &Device...", this, SLOT(set_net_device()));

   m_action_net_sessiontrack = m_netMenu->addAction("Session Tracking", this,
           SLOT(toggle_net_session_tracking(bool)));
   m_action_net_sessiontrack->setCheckable(true);
   m_action_net_sessiontrack->setChecked(m_packet->session_tracking());

   QAction* tmpAction;
   tmpAction = m_netMenu->addAction("&Real Time Thread", this,
           SLOT(toggle_net_real_time_thread(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_packet->realtime());

   QMenu* captureMenu = new QMenu("Packet Capture Configuration");
   m_netMenu->addMenu(captureMenu);


   QWidgetAction* captureSettingNotice = new QWidgetAction(captureMenu);
   QLabel* tmpLabel = new QLabel("NOTE: You must save preferences and restart ShowEQ for these changes to take effect (for now)");
   tmpLabel->setWordWrap(true);
   tmpLabel->setContentsMargins(30, 11, 11, 11);
   captureSettingNotice->setDefaultWidget(tmpLabel);
   captureMenu->addAction(captureSettingNotice);

   captureMenu->addSeparator();

   QMenu* tmpMenu = new QMenu("Snapshot Length (KB)");
   QSpinBox* snapLenSpinBox = new QSpinBox(tmpMenu);
   snapLenSpinBox->setMinimum(1);
   snapLenSpinBox->setMaximum(64);
   snapLenSpinBox->setValue(pSEQPrefs->getPrefInt("CaptureSnapLen", "Network", 1));
   connect(snapLenSpinBox, SIGNAL(valueChanged(int)), this, SLOT(set_net_capture_snap_len(int)));
   QWidgetAction* snapLenWidgetAction = new QWidgetAction(tmpMenu);
   snapLenWidgetAction->setDefaultWidget(snapLenSpinBox);
   tmpMenu->addAction(snapLenWidgetAction);
   captureMenu->addMenu(tmpMenu);

   tmpMenu = new QMenu("Capture Buffer Size (MB)");
   QSpinBox* captureBufferSizeSpinBox = new QSpinBox();
   captureBufferSizeSpinBox->setMinimum(2);
   captureBufferSizeSpinBox->setMaximum(128);
   captureBufferSizeSpinBox->setValue(pSEQPrefs->getPrefInt("CaptureBufferSize", "Network", 2));
   connect(captureBufferSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(set_net_capture_buffer_size(int)));
   QWidgetAction* captureBufferSizeWidgetAction = new QWidgetAction(tmpMenu);
   captureBufferSizeWidgetAction->setDefaultWidget(captureBufferSizeSpinBox);
   tmpMenu->addAction(captureBufferSizeWidgetAction);
   captureMenu->addMenu(tmpMenu);

   m_netMenu->addSeparator();

   // Log menu
   QMenu* pLogMenu = new QMenu("Lo&g");
   m_netMenu->addMenu(pLogMenu);

   m_action_log_AllPackets = pLogMenu->addAction("All Packets", this,
           SLOT(toggle_log_AllPackets()), Qt::Key_F5);
   m_action_log_AllPackets->setCheckable(true);
   m_action_log_AllPackets->setChecked(m_globalLog != 0);

   m_action_log_WorldData = pLogMenu->addAction("World Data", this,
           SLOT(toggle_log_WorldData()), Qt::Key_F6);
   m_action_log_WorldData->setCheckable(true);
   m_action_log_WorldData->setChecked(m_worldLog != 0);

   m_action_log_ZoneData = pLogMenu->addAction("Zone Data", this,
           SLOT(toggle_log_ZoneData()), Qt::Key_F7);
   m_action_log_ZoneData->setCheckable(true);
   m_action_log_ZoneData->setChecked(m_zoneLog != 0);

   m_action_log_UnknownData = pLogMenu->addAction("Unknown Data", this,
           SLOT(toggle_log_UnknownData()), Qt::Key_F8);
   m_action_log_UnknownData->setCheckable(true);
   m_action_log_UnknownData->setChecked(m_unknownZoneLog != 0);

   m_action_view_UnknownData = pLogMenu->addAction("View Unknown Data", this,
           SLOT(toggle_view_UnknownData()), Qt::Key_F9);
   m_action_view_UnknownData->setCheckable(true);
   m_action_view_UnknownData->setChecked(pSEQPrefs->getPrefBool("ViewUnknown",
               "PacketLogging", false));

   m_action_log_RawData = pLogMenu->addAction("Raw Data", this,
           SLOT(toggle_log_RawData()), Qt::Key_F10);
   m_action_log_RawData->setCheckable(true);
   m_action_log_RawData->setChecked(pSEQPrefs->getPrefBool("LogRawPackets",
               "PacketLogging", false));

   m_filterZoneDataMenu = new QMenu("Filter Zone Data");
   pLogMenu->addMenu(m_filterZoneDataMenu);

   m_action_log_Filter_ZoneData_Client = m_filterZoneDataMenu->addAction("Client",
           this, SLOT(toggle_log_Filter_ZoneData_Client()));
   m_action_log_Filter_ZoneData_Client->setCheckable(true);

   m_action_log_Filter_ZoneData_Server = m_filterZoneDataMenu->addAction("Server",
           this, SLOT(toggle_log_Filter_ZoneData_Server()));
   m_action_log_Filter_ZoneData_Server->setCheckable(true);


   // OpCode Monitor
   QMenu* pOpCodeMenu = new QMenu("OpCode Monitor");
   m_netMenu->addMenu(pOpCodeMenu);

   m_action_opcode_monitor = pOpCodeMenu->addAction("&OpCode Monitoring", this,
           SLOT(toggle_opcode_monitoring()), Qt::CTRL|Qt::ALT|Qt::Key_O);
   m_action_opcode_monitor->setCheckable(true);
   m_action_opcode_monitor->setChecked(m_opcodeMonitorLog != 0);

   pOpCodeMenu->addAction("&Reload Monitored OpCode List...", this,
           SLOT(set_opcode_monitored_list()), Qt::CTRL|Qt::ALT|Qt::Key_R);

   QString section = "OpCodeMonitoring";

   m_action_opcode_view = pOpCodeMenu->addAction("&View Monitored OpCode Matches",
           this, SLOT(toggle_opcode_view()));
   m_action_opcode_view->setCheckable(true);
   m_action_opcode_view->setChecked(pSEQPrefs->getPrefBool("View", section, false));

   m_action_opcode_log = pOpCodeMenu->addAction("&Log Monitored OpCode Matches",
           this, SLOT(toggle_opcode_log()));
   m_action_opcode_log->setCheckable(true);
   m_action_opcode_log->setChecked(pSEQPrefs->getPrefBool("Log", section, false));

   pOpCodeMenu->addAction("Log &Filename...", this, SLOT(select_opcode_file()));

   m_netMenu->addSeparator();

   section = "Interface";

   // Advanced menu
   QMenu* subMenu = new QMenu("Advanced");
   QMenu* subSubMenu = new QMenu("Arq Seq Give Up");
   QSpinBox* arqSeqGiveUpSpinBox = new QSpinBox(subSubMenu);
   arqSeqGiveUpSpinBox->setMinimum(32);
   arqSeqGiveUpSpinBox->setMaximum(1024);
   arqSeqGiveUpSpinBox->setSingleStep(8);
   arqSeqGiveUpSpinBox->setValue(m_packet->arqSeqGiveUp());
   connect(arqSeqGiveUpSpinBox, SIGNAL(valueChanged(int)),
	   this, SLOT(set_net_arq_giveup(int)));
   QWidgetAction* arqSeqGiveUpSpinBoxAction = new QWidgetAction(subSubMenu);
   arqSeqGiveUpSpinBoxAction->setDefaultWidget(arqSeqGiveUpSpinBox);
   subSubMenu->addAction(arqSeqGiveUpSpinBoxAction);
   subMenu->addMenu(subSubMenu);
   m_netMenu->addMenu(subMenu);

}

void EQInterface::createCharacterMenu() {

   // Character Menu
   m_charMenu = new QMenu("&Character");
   menuBar()->addMenu(m_charMenu);
   QAction* tmpAction;
   tmpAction = m_charMenu->addAction("Use Auto Detected Settings", this,
           SLOT(toggleAutoDetectPlayerSettings(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_player->useAutoDetectedSettings());

   // Character -> Level
   m_charLevelMenu = new QMenu("Choose &Level");
   m_charMenu->addMenu(m_charLevelMenu);
   m_levelSpinBox = new QSpinBox(this);
   m_levelSpinBox->setObjectName("m_levelSpinBox");
   m_levelSpinBox->setMinimum(1);
   m_levelSpinBox->setMaximum(125);
   m_levelSpinBox->setSingleStep(1);
   QWidgetAction* levelSpinBoxAction = new QWidgetAction(m_charLevelMenu);
   levelSpinBoxAction->setDefaultWidget(m_levelSpinBox);
   m_charLevelMenu->addAction( levelSpinBoxAction );
   m_levelSpinBox->setWrapping( true );
   m_levelSpinBox->setButtonSymbols(QSpinBox::PlusMinus);
   m_levelSpinBox->setPrefix("Level: ");
   connect(m_levelSpinBox, SIGNAL(valueChanged(int)), this,
           SLOT(SetDefaultCharacterLevel(int)));
   m_levelSpinBox->setValue(m_player->defaultLevel());

   // Character -> Class
   m_charClassMenu = new QMenu("Choose &Class");
   m_charMenu->addMenu(m_charClassMenu);
   for( int i = 0; i < PLAYER_CLASSES; i++)
   {
       m_action_character_Class[i] = m_charClassMenu->addAction(player_classes[i]);
       m_action_character_Class[i]->setCheckable(true);
       m_action_character_Class[i]->setData(i+1);
       if(i+1 == m_player->defaultClass())
          m_action_character_Class[i]->setChecked(true);
   }
   connect (m_charClassMenu, SIGNAL(triggered(QAction*)), this,
               SLOT(SetDefaultCharacterClass(QAction*)));

   // Character -> Race
   m_charRaceMenu = new QMenu("Choose &Race");
   m_charMenu->addMenu(m_charRaceMenu);
   for( int i = 0; i < PLAYER_RACES; i++)
   {
       m_action_character_Race[i] = m_charRaceMenu->addAction(player_races[i]);
       m_action_character_Race[i]->setCheckable(true);

       switch (i) {
           case 12:
               m_action_character_Race[i]->setData(128);
               break;
           case 13:
               m_action_character_Race[i]->setData(130);
               break;
           case 14:
               m_action_character_Race[i]->setData(330);
               break;
           default:
               m_action_character_Race[i]->setData(i+1);
               break;
       }

       if(m_action_character_Race[i]->data() == m_player->defaultRace())
           m_action_character_Race[i]->setChecked(true);
   }
   connect (m_charRaceMenu, SIGNAL(triggered(QAction*)), this,
               SLOT(SetDefaultCharacterRace(QAction*)));


}

void EQInterface::createFiltersMenu() {

   // Filters Menu
   QMenu* filterMenu = new QMenu("Fi&lters");
   menuBar()->addMenu(filterMenu);

   filterMenu->addAction("&Reload Filters", m_filterMgr, SLOT(loadFilters()),
           Qt::Key_F3);
   filterMenu->addAction("&Save Filters", m_filterMgr, SLOT(saveFilters()),
           Qt::Key_F4);
   filterMenu->addAction("&Edit Filters", this, SLOT(launch_filterlistwindow_filters()));
   filterMenu->addAction("&Edit Filters XML", this, SLOT(launch_editor_filters()));
   filterMenu->addAction("Select Fil&ter File", this, SLOT(select_filter_file()));

   filterMenu->addAction("Reload &Zone Filters", m_filterMgr,
           SLOT(loadZoneFilters()), Qt::SHIFT|Qt::Key_F3);
   filterMenu->addAction("S&ave Zone Filters", m_filterMgr,
           SLOT(saveZoneFilters()), Qt::SHIFT|Qt::Key_F4);
   filterMenu->addAction("Edit Zone Filters", this,
           SLOT(launch_filterlistwindow_zoneFilters()));
   filterMenu->addAction("Edit Zone Fi&lters XML", this,
           SLOT(launch_editor_zoneFilters()));

   filterMenu->addAction("Re&filter Spawns", m_spawnShell, SLOT(refilterSpawns()));

   QAction* tmpAction;
   tmpAction = filterMenu->addAction("&Is Case Sensitive", this,
           SLOT(toggle_filter_Case(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_filterMgr->caseSensitive());

   tmpAction = filterMenu->addAction("&Display Alert Info", this,
           SLOT(toggle_filter_AlertInfo(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("AlertInfo", "Filters"));

   tmpAction = filterMenu->addAction("&Use System Beep", this,
           SLOT(toggle_filter_UseSystemBeep(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_filterNotifications->useSystemBeep());

   tmpAction = filterMenu->addAction("Use &Commands", this,
           SLOT(toggle_filter_UseCommands(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_filterNotifications->useCommands());

   // Filter -> Log
   QMenu* filterLogMenu = new QMenu("&Log");
   filterMenu->addMenu(filterLogMenu);

   uint32_t filters = 0;
   if (m_filteredSpawnLog)
       filters = m_filteredSpawnLog->filters();

   tmpAction = filterLogMenu->addAction( "Alerts");
   tmpAction->setCheckable(true);
   tmpAction->setData(FILTER_FLAG_ALERT);
   tmpAction->setChecked(((filters & FILTER_FLAG_ALERT) != 0));

   tmpAction = filterLogMenu->addAction( "Locates");
   tmpAction->setCheckable(true);
   tmpAction->setData(FILTER_FLAG_LOCATE);
   tmpAction->setChecked(((filters & FILTER_FLAG_LOCATE) != 0));

   tmpAction = filterLogMenu->addAction( "Hunts");
   tmpAction->setCheckable(true);
   tmpAction->setData(FILTER_FLAG_HUNT);
   tmpAction->setChecked(((filters & FILTER_FLAG_HUNT) != 0));

   tmpAction = filterLogMenu->addAction( "Cautions");
   tmpAction->setCheckable(true);
   tmpAction->setData(FILTER_FLAG_CAUTION);
   tmpAction->setChecked(((filters & FILTER_FLAG_CAUTION) != 0));

   tmpAction = filterLogMenu->addAction( "Dangers");
   tmpAction->setCheckable(true);
   tmpAction->setData(FILTER_FLAG_DANGER);
   tmpAction->setChecked(((filters & FILTER_FLAG_DANGER) != 0));

   connect(filterLogMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_filter_Log(QAction*)));

   // Filter -> Commands
   QMenu* filterCmdMenu = new QMenu("&Audio Commands");
   filterMenu->addMenu(filterCmdMenu);

   tmpAction = filterCmdMenu->addAction( "Spawn...");
   tmpAction->setData(1);

   tmpAction = filterCmdMenu->addAction( "DeSpawn...");
   tmpAction->setData(2);

   tmpAction = filterCmdMenu->addAction( "Death...");
   tmpAction->setData(3);

   tmpAction = filterCmdMenu->addAction( "Locate...");
   tmpAction->setData(4);

   tmpAction = filterCmdMenu->addAction( "Caution...");
   tmpAction->setData(5);

   tmpAction = filterCmdMenu->addAction( "Hunt...");
   tmpAction->setData(6);

   tmpAction = filterCmdMenu->addAction( "Danger...");
   tmpAction->setData(7);

   connect(filterCmdMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(set_filter_AudioCommand(QAction*)));

}

void EQInterface::createInterfaceMenu() {

   // Interface Menu
   QMenu* pInterfaceMenu = new QMenu("&Interface");
   menuBar()->addMenu(pInterfaceMenu);

   pInterfaceMenu->addAction("Hide MenuBar", this, SLOT(toggle_view_menubar()));

   // Interface -> Style
   QMenu* pStyleMenu = new QMenu("&Style");
   pInterfaceMenu->addMenu(pStyleMenu);

   QStringList availableStyles = QStyleFactory::keys();

#if QT_VERSION >= 0x050000
   availableStyles.sort(Qt::CaseInsensitive);
#else
   availableStyles.sort();
#endif

   QString currentStyleName = qApp->style()->objectName();

   QStringList::Iterator styleItr = availableStyles.begin();

   QAction* tmpAction;
   while (styleItr != availableStyles.end()) {

     tmpAction = pStyleMenu->addAction(*styleItr);
     tmpAction->setCheckable(true);

     if (currentStyleName.toLower() == (*styleItr).toLower())
         tmpAction->setChecked(true);

     tmpAction->setData(*styleItr);
     ActionList_StyleMenu.append(tmpAction);

     ++styleItr;
   }
   
   connect (pStyleMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(selectTheme(QAction*)));

   QString section = "Interface";
   QString themeName = pSEQPrefs->getPrefString("ThemeName", section, "");

   //Use the text name if there is one.  If not, fall back to old numeric id.  If no
   //numeric id, then just use whatever Qt started with (no forced default)
   if (!themeName.isEmpty())
   {
     setTheme(themeName);
   } else {
     int themeId = pSEQPrefs->getPrefInt("Theme", section, -1);
     if (themeId >= 0)
       setTheme(themeId);
   }

   // Interface -> Status Bar
   QMenu* statusBarMenu = new QMenu("&Status Bar");
   pInterfaceMenu->addMenu(statusBarMenu);

   statusBarMenu->addAction("Show/Hide", this, SLOT(toggle_view_statusbar()));

   tmpAction = statusBarMenu->addAction( "Status");
   tmpAction->setCheckable(true);
   tmpAction->setData(1);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowStatus",
               "Interface_StatusBar", false));

   tmpAction = statusBarMenu->addAction( "Zone");
   tmpAction->setCheckable(true);
   tmpAction->setData(2);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowZone",
               "Interface_StatusBar", false));

   tmpAction = statusBarMenu->addAction( "Spawns");
   tmpAction->setCheckable(true);
   tmpAction->setData(3);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowSpawns",
               "Interface_StatusBar", false));

   tmpAction = statusBarMenu->addAction( "Experience");
   tmpAction->setCheckable(true);
   tmpAction->setData(4);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowExp",
               "Interface_StatusBar", false));

   tmpAction = statusBarMenu->addAction( "AA Experience");
   tmpAction->setCheckable(true);
   tmpAction->setData(5);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowExpAA",
               "Interface_StatusBar", false));

   tmpAction = statusBarMenu->addAction( "Packet Counter");
   tmpAction->setCheckable(true);
   tmpAction->setData(6);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowPacketCounter",
               "Interface_StatusBar", false));

   tmpAction = statusBarMenu->addAction( "EQ Time");
   tmpAction->setCheckable(true);
   tmpAction->setData(7);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowEQTime",
               "Interface_StatusBar", false));
   tmpAction = statusBarMenu->addAction( "Run Speed");
   tmpAction->setCheckable(true);
   tmpAction->setData(8);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowSpeed",
               "Interface_StatusBar", false));
   // ZEM code
   tmpAction = statusBarMenu->addAction( "ZEM");
   tmpAction->setCheckable(true);
   tmpAction->setData(9);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("ShowZEM",
               "Interface_StatusBar", false));

   connect (statusBarMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggle_main_statusbar_Window(QAction*)));

   m_terminalMenu = new QMenu("&Terminal");
   pInterfaceMenu->addMenu(m_terminalMenu);

   m_terminalTypeFilterMenu = new QMenu("MessageTypeFilter");
   m_terminalMenu->addMenu(m_terminalTypeFilterMenu);

   m_terminalTypeFilterMenu->addAction("&Enable All", this,
           SLOT(enableAllTypeFilters()));
   m_terminalTypeFilterMenu->addAction("&Disable All", this,
           SLOT(disableAllTypeFilters()));

   m_terminalTypeFilterMenu->addSeparator();

   QString typeName;
   uint64_t enabledTypes = m_terminal->enabledTypes();

   // iterate over the message types, filling in various menus and getting 
   // font color preferences
   for (int i = MT_Guild; i <= MT_Max; i++)
   {
     typeName = MessageEntry::messageTypeString((MessageType)i);
     if (!typeName.isEmpty())
     {
       m_action_term_MessageTypeFilters[i] = m_terminalTypeFilterMenu->addAction(typeName);
       m_action_term_MessageTypeFilters[i]->setCheckable(true);
       m_action_term_MessageTypeFilters[i]->setChecked(((uint64_t(1) << i) & enabledTypes) != 0);
       m_action_term_MessageTypeFilters[i]->setData(i);
     }
   }

   connect(m_terminalTypeFilterMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(toggleTypeFilter(QAction*)));

  m_terminalShowUserFilterMenu = new QMenu("User Message Filter - &Show");
  m_terminalMenu->addMenu(m_terminalShowUserFilterMenu);

  m_terminalShowUserFilterMenu->addAction("&Enable All", this,
          SLOT(enableAllShowUserFilters()));
  m_terminalShowUserFilterMenu->addAction("&Disable All", this,
          SLOT(disableAllShowUserFilters()));
  m_terminalShowUserFilterMenu->addSeparator();

  m_terminalHideUserFilterMenu = new QMenu("User Message Filter - &Hide");
  m_terminalMenu->addMenu(m_terminalHideUserFilterMenu);

  m_terminalHideUserFilterMenu->addAction("&Enable All", this,
          SLOT(enableAllHideUserFilters()));
  m_terminalHideUserFilterMenu->addAction("&Disable All", this,
          SLOT(disableAllHideUserFilters()));

  m_terminalHideUserFilterMenu->addSeparator();

  uint32_t enabledShowUserFilters = m_terminal->enabledShowUserFilters();
  uint32_t enabledHideUserFilters = m_terminal->enabledHideUserFilters();
  const MessageFilter* filter;
  for(int i = 0; i < maxMessageFilters; i++)
  {
    filter = m_messageFilters->filter(i);
    if (filter)
    {
      m_action_term_ShowUserFilters[i] =
          m_terminalShowUserFilterMenu->addAction(filter->name());
      m_action_term_ShowUserFilters[i]->setCheckable(true);
      m_action_term_ShowUserFilters[i]->setData(i);
      m_action_term_ShowUserFilters[i]->setChecked(
              (1 << i) & enabledShowUserFilters);

      m_action_term_HideUserFilters[i] =
          m_terminalHideUserFilterMenu->addAction(filter->name());
      m_action_term_HideUserFilters[i]->setCheckable(true);
      m_action_term_HideUserFilters[i]->setData(i);
      m_action_term_HideUserFilters[i]->setChecked(
              (1 << i) & enabledHideUserFilters);
    }
  }

  connect(m_terminalShowUserFilterMenu, SIGNAL(triggered(QAction*)), this,
          SLOT(toggleShowUserFilter(QAction*)));
  connect(m_terminalHideUserFilterMenu, SIGNAL(triggered(QAction*)), this,
          SLOT(toggleHideUserFilter(QAction*)));

  m_terminalMenu->addAction("Edit User &Message Filters...", this,
          SLOT(showMessageFilterDialog()));

  connect(m_messageFilters, SIGNAL(added(uint32_t, uint8_t, const MessageFilter&)),
          this, SLOT(addUserFilterMenuEntry(uint32_t, uint8_t, const MessageFilter&)));
  connect(m_messageFilters, SIGNAL(removed(uint32_t, uint8_t)), this,
          SLOT(removeUserFilterMenuEntry(uint32_t, uint8_t)));

   m_terminalMenu->addSeparator();

   tmpAction = m_terminalMenu->addAction("&Display Type", this,
           SLOT(toggleDisplayType(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_terminal->displayType());

   tmpAction = m_terminalMenu->addAction("Display T&ime/Date", this,
           SLOT(toggleDisplayTime(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_terminal->displayDateTime());

   tmpAction = m_terminalMenu->addAction("Display &EQ Date/Time", this,
           SLOT(toggleEQDisplayTime(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_terminal->displayEQDateTime());

   tmpAction = m_terminalMenu->addAction("&Use Color", this,
           SLOT(toggleUseColor(bool)));
   tmpAction->setCheckable(true);
   tmpAction->setChecked(m_terminal->useColor());


   pInterfaceMenu->addAction( "Formatted Messages File...", this,
           SLOT(select_main_FormatFile()));

   pInterfaceMenu->addAction( "Spells File...", this,
           SLOT(select_main_SpellsFile()));


}

void EQInterface::createWindowMenu() {

   // insert Window menu
   menuBar()->addMenu(m_windowMenu);

   // All of the Window menu items that don't automatically get inserted
   // have to be manually placed in the right positions.

   // Interface -> WindowCaption
   m_windowCaptionMenu = new QMenu("Window &Caption");
   m_windowMenu->insertMenu(m_windowMenu->actions()[0], m_windowCaptionMenu);

   QAction* tmpAction;
   tmpAction = m_windowCaptionMenu->addAction("&Main Window...");
   tmpAction->setData(5);

   tmpAction = m_windowCaptionMenu->addAction("Spawn &List...");
   tmpAction->setData(0);

   tmpAction = m_windowCaptionMenu->addAction("Spawn List &2...");
   tmpAction->setData(10);

   tmpAction = m_windowCaptionMenu->addAction("Spawn P&oint List...");
   tmpAction->setData(9);

   tmpAction = m_windowCaptionMenu->addAction("&Player Stats...");
   tmpAction->setData(1);

   tmpAction = m_windowCaptionMenu->addAction("Player &Skills...");
   tmpAction->setData(2);

   tmpAction = m_windowCaptionMenu->addAction("Spell L&ist...");
   tmpAction->setData(3);

   tmpAction = m_windowCaptionMenu->addAction("&Compass...");
   tmpAction->setData(4);

   tmpAction = m_windowCaptionMenu->addAction("&Experience Window...");
   tmpAction->setData(6);

   tmpAction = m_windowCaptionMenu->addAction("Comb&at Window...");
   tmpAction->setData(7);

   tmpAction = m_windowCaptionMenu->addAction("&Network Diagnostics...");
   tmpAction->setData(8);

   // insert Map docking options 
   // NOTE: Always insert Map docking options at the end of the Docked menu
   for (int i = 0; i < maxNumMaps; i++)
   {
        QString mapName = "Map";
        if (i > 0)
            mapName += QString::number(i + 1);
        tmpAction = m_windowCaptionMenu->addAction(mapName);
        tmpAction->setData(i + mapCaptionBase);
   }

   connect (m_windowCaptionMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(set_main_WindowCaption(QAction*)));

   // Interface -> Window Font
   QMenu* windowFontMenu = new QMenu("&Font");
   m_windowMenu->insertMenu(m_windowMenu->actions()[1], windowFontMenu);

   tmpAction = windowFontMenu->addAction("&Application Default...", this,
           SLOT(set_main_Font()));
   tmpAction->setData(-1);

   tmpAction = windowFontMenu->addAction("Main Window Status Font...", this,
           SLOT(set_main_statusbar_Font()));
   tmpAction->setData(-1);
   //   x = windowFontMenu->insertItem("&Main Window");
   //   windowFontMenu->setItemParameter(x, 5);

   tmpAction = windowFontMenu->addAction("Spawn &List...");
   tmpAction->setData(0);

   tmpAction = windowFontMenu->addAction("Spawn List &2...");
   tmpAction->setData(10);

   tmpAction = windowFontMenu->addAction("Spawn P&oint List...");
   tmpAction->setData(9);

   tmpAction = windowFontMenu->addAction("&Player Stats...");
   tmpAction->setData(1);

   tmpAction = windowFontMenu->addAction("Player &Skills...");
   tmpAction->setData(2);

   tmpAction = windowFontMenu->addAction("Spell L&ist...");
   tmpAction->setData(3);

   tmpAction = windowFontMenu->addAction("&Compass...");
   tmpAction->setData(4);

   tmpAction = windowFontMenu->addAction("&Experience Window...");
   tmpAction->setData(6);

   tmpAction = windowFontMenu->addAction("Comb&at Window...");
   tmpAction->setData(7);

   tmpAction = windowFontMenu->addAction("&Network Diagnostics...");
   tmpAction->setData(8);

   // insert Map docking options 
   // NOTE: Always insert Map docking options at the end of the Docked menu
   for (int i = 0; i < maxNumMaps; i++)
   {
        QString mapName = "Map";
        if (i > 0)
            mapName += QString::number(i + 1);
        tmpAction = windowFontMenu->addAction(mapName);
        tmpAction->setData(i + mapCaptionBase);
   }

   connect (windowFontMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(set_main_WindowFont(QAction*)));


   tmpAction = new QAction(QString("Save Window Sizes && Positions"), m_windowMenu);
   m_windowMenu->insertAction(m_windowMenu->actions()[2], tmpAction);
   tmpAction->setCheckable(true);
   tmpAction->setChecked(pSEQPrefs->getPrefBool("SavePosition", "Interface", true));
   connect(tmpAction, SIGNAL(triggered(bool)), this,
           SLOT(toggle_main_SavePosition(bool)));

   tmpAction = new QAction(QString("Restore Window Positions"), m_windowMenu);
   m_windowMenu->insertAction(m_windowMenu->actions()[3], tmpAction);
   tmpAction->setCheckable(true);
   tmpAction->setChecked (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true));
   connect(tmpAction, SIGNAL(triggered(bool)), this,
           SLOT(toggle_main_UseWindowPos(bool)));

   m_windowMenu->insertSeparator(m_windowMenu->actions()[4]);


}

void EQInterface::createDebugMenu() {

   // Debug menu
   QMenu* pDebugMenu = new QMenu("&Debug");
   menuBar()->addMenu(pDebugMenu);
   pDebugMenu->addAction("List I&nterface", this, SLOT(listInterfaceInfo()));
   pDebugMenu->addAction("List S&pawns", this, SLOT(listSpawns()), Qt::ALT|Qt::CTRL|Qt::Key_P);
   pDebugMenu->addAction("List &Drops", this, SLOT(listDrops()), Qt::ALT|Qt::CTRL|Qt::Key_D);
   pDebugMenu->addAction("List &Map Info", this, SLOT(listMapInfo()), Qt::ALT|Qt::CTRL|Qt::Key_M);
   pDebugMenu->addAction("List G&uild Info", m_guildmgr, SLOT(listGuildInfo()));
   pDebugMenu->addAction("List &Group", this, SLOT(listGroup()), Qt::ALT|Qt::CTRL|Qt::Key_G);
   pDebugMenu->addAction("List Guild M&embers", this, SLOT(listGuild()), Qt::ALT|Qt::CTRL|Qt::Key_E);
   pDebugMenu->addAction("Dump Spawns", this, SLOT(dumpSpawns()), Qt::ALT|Qt::SHIFT|Qt::CTRL|Qt::Key_P);
   pDebugMenu->addAction("Dump Drops", this, SLOT(dumpDrops()), Qt::ALT|Qt::SHIFT|Qt::CTRL|Qt::Key_D);
   pDebugMenu->addAction("Dump Map Info", this, SLOT(dumpMapInfo()), Qt::ALT|Qt::SHIFT|Qt::CTRL|Qt::Key_M);
   pDebugMenu->addAction("Dump Guild Info", this , SLOT(dumpGuildInfo()));
   pDebugMenu->addAction("Dump SpellBook Info", this , SLOT(dumpSpellBook()));
   pDebugMenu->addAction("Dump Group", this, SLOT(dumpGroup()), Qt::ALT|Qt::CTRL|Qt::SHIFT|Qt::Key_G);
   pDebugMenu->addAction("Dump Guild Members", this, SLOT(dumpGuild()), Qt::ALT|Qt::CTRL|Qt::SHIFT|Qt::Key_E);
   pDebugMenu->addAction("List &Filters", m_filterMgr, SLOT(listFilters()), Qt::ALT|Qt::CTRL|Qt::Key_F);
   pDebugMenu->addAction("List &Zone Filters", m_filterMgr, SLOT(listZoneFilters()));


}

void EQInterface::createStatusBar() {

   QString statusBarSection = "Interface_StatusBar";
   int sts_widget_count = 0; // total number of widgets visible on status bar

   //Status widget
     m_stsbarStatus = new QLabel(statusBar());
     m_stsbarStatus->setObjectName("Status");
     m_stsbarStatus->setMinimumWidth(80);
     m_stsbarStatus->setText(QString("ShowEQ %1").arg(VERSION));
     statusBar()->addWidget(m_stsbarStatus, 8);

   //Zone widget
     m_stsbarZone = new QLabel(statusBar());
     m_stsbarZone->setObjectName("Zone");
     m_stsbarZone->setText("Zone: [unknown]");
     statusBar()->addWidget(m_stsbarZone, 2);

   //Mobs widget
     m_stsbarSpawns = new QLabel(statusBar());
     m_stsbarSpawns->setObjectName("Mobs");
     m_stsbarSpawns->setText("Mobs:");
     statusBar()->addWidget(m_stsbarSpawns, 1);

   //Exp widget
     m_stsbarExp = new QLabel(statusBar());
     m_stsbarExp->setObjectName("Exp");
     m_stsbarExp->setText("Exp [unknown]");
     statusBar()->addWidget(m_stsbarExp, 2);

   //ExpAA widget
     m_stsbarExpAA = new QLabel(statusBar());
     m_stsbarExpAA->setObjectName("ExpAA");
     m_stsbarExpAA->setText("ExpAA [unknown]");
     statusBar()->addWidget(m_stsbarExpAA, 2);

   //Pkt widget
     m_stsbarPkt = new QLabel(statusBar());
     m_stsbarPkt->setObjectName("Pkt");
     m_stsbarPkt->setText("Pkt 0");
     statusBar()->addWidget(m_stsbarPkt, 1);

   //EQTime widget
     m_stsbarEQTime = new QLabel(statusBar());
     m_stsbarEQTime->setObjectName("EQTime");
     m_stsbarEQTime->setText("EQTime");
     statusBar()->addWidget(m_stsbarEQTime, 1);

   // Run Speed widget
     m_stsbarSpeed = new QLabel(statusBar());
     m_stsbarSpeed->setObjectName("Speed");
     m_stsbarSpeed->setText("Run Speed:");
     statusBar()->addWidget(m_stsbarSpeed, 1);

   // ZEM code
   // Zone Exp Mult widget
     m_stsbarZEM = new QLabel(statusBar());
     m_stsbarZEM->setObjectName("ZEM");
     m_stsbarZEM->setText("ZEM: [unknown]");
     statusBar()->addWidget(m_stsbarZEM, 1);

     // setup the status fonts correctly
     restoreStatusFont();

   if (!pSEQPrefs->getPrefBool("ShowStatus", statusBarSection, true))
     m_stsbarStatus->hide();
   else
     sts_widget_count++;
  
   if (!pSEQPrefs->getPrefBool("ShowZone", statusBarSection, true))
     m_stsbarZone->hide();
   else
     sts_widget_count++;

   if (!pSEQPrefs->getPrefBool("ShowSpawns", statusBarSection, false))
     m_stsbarSpawns->hide();
   else
     sts_widget_count++;

   if (!pSEQPrefs->getPrefBool("ShowExp", statusBarSection, false))
     m_stsbarExp->hide();
   else
     sts_widget_count++;

   if (!pSEQPrefs->getPrefBool("ShowExpAA", statusBarSection, false))
     m_stsbarExpAA->hide();
   else
     sts_widget_count++;

   if (!pSEQPrefs->getPrefBool("ShowPacketCounter", statusBarSection, false))
     m_stsbarPkt->hide();
   else
     sts_widget_count++;

   if (!pSEQPrefs->getPrefBool("ShowEQTime", statusBarSection, true))
     m_stsbarEQTime->hide();
   else
     sts_widget_count++;

   if (!pSEQPrefs->getPrefBool("ShowSpeed", statusBarSection, false))
     m_stsbarSpeed->hide();
   else
     sts_widget_count++;

   // ZEM code
   if (!pSEQPrefs->getPrefBool("ShowZEM", statusBarSection, false))
     m_stsbarZEM->hide();
   else
     sts_widget_count++;


   //hide the statusbar if no visible widgets
   if (!sts_widget_count || !pSEQPrefs->getPrefBool("StatusBarActive", statusBarSection, 1))
      statusBar()->hide();
}


void EQInterface::restoreStatusFont()
{
   QFont defFont;
   defFont.setPointSize(8);
   QFont statusFont = pSEQPrefs->getPrefFont("StatusFont", "Interface", 
					     defFont);
 
   int statusFixedHeight = statusFont.pointSize() + 6;

   // set the correct font information and sizes of the status bar widgets
   m_stsbarStatus->setFont(statusFont);
   m_stsbarStatus->setFixedHeight(statusFixedHeight);
   m_stsbarZone->setFont(statusFont);
   m_stsbarZone->setFixedHeight(statusFixedHeight);
   m_stsbarSpawns->setFont(statusFont);
   m_stsbarSpawns->setFixedHeight(statusFixedHeight);
   m_stsbarExp->setFont(statusFont);
   m_stsbarExp->setFixedHeight(statusFixedHeight);
   m_stsbarExpAA->setFont(statusFont);
   m_stsbarExpAA->setFixedHeight(statusFixedHeight);
   m_stsbarPkt->setFont(statusFont);
   m_stsbarPkt->setFixedHeight(statusFixedHeight);
   m_stsbarEQTime->setFont(statusFont);
   m_stsbarEQTime->setFixedHeight(statusFixedHeight);
   m_stsbarSpeed->setFont(statusFont);
   m_stsbarSpeed->setFixedHeight(statusFixedHeight);
   // ZEM code
   m_stsbarZEM->setFont(statusFont);
   m_stsbarZEM->setFixedHeight(statusFixedHeight);
}

void EQInterface::toggle_view_StatWin(QAction* stat)
{
   int statnum = stat->data().value<int>();

   if (stat->isChecked())
   {
       stat->setChecked(true);
       if (m_statList != 0)
           m_statList->statList()->enableStat(statnum, true);
   }
   else
   {
       stat->setChecked(false);
       if (m_statList != 0)
           m_statList->statList()->enableStat(statnum, false);
   }
}

void EQInterface::toggle_view_SkillWin(QAction* skill)
{
  if (skill->isChecked())
  {
      skill->setChecked(true);

    if ((skill == m_action_view_PlayerSkills_Languages) && (m_skillList != 0))
        m_skillList->skillList()->showLanguages(true);
  }
  else
  {
      skill->setChecked(false);

    if ((skill == m_action_view_PlayerSkills_Languages) && (m_skillList != 0))
        m_skillList->skillList()->showLanguages(false);
  }
}

void EQInterface::toggle_view_SpawnListCol( QAction* col )
{
  int colnum = col->data().value<int>();
  if (m_spawnList != 0)
    m_spawnList->spawnList()->setColumnVisible(colnum, col->isChecked());
}

void EQInterface::toggle_view_DockedWin(QAction* win )
{
  SEQWindow* widget = 0;
  int winnum;
  QString preference;

  // get the window number parameter
  winnum = win->data().value<int>();

  // get the new menu item state
  bool docked = win->isChecked();

  switch(winnum)
  {
  case 0: // Spawn List
    // note the new setting
    m_isSpawnListDocked = docked;

    // reparent the Spawn List
    widget = m_spawnList;

    // preference
    preference = "DockedSpawnList";
    break;
  case 1: // Player Stats
    // note the new setting
    m_isStatListDocked = docked;

    // reparent the Stat List
    widget = m_statList;

    // preference
    preference = "DockedPlayerStats";
    break;
  case 2: // Player Skills
    // note the new setting
    m_isSkillListDocked = docked;

    // reparent the Skill List
    widget = m_skillList;

    // preference
    preference = "DockedPlayerSkills";
    break;
  case 3: // Spell List
    // note the new setting
    m_isSpellListDocked = docked;

    // reparent the Skill List
    widget = m_spellList;

    // preference
    preference = "DockedSpellList";
    break;
  case 4: // Compass
    // note the new setting
    m_isCompassDocked = docked;

    // reparent the Skill List
    widget = m_compass;

    // preference
    preference = "DockedCompass";
    break;
  case 5: // Spawn Point List
    // note the new setting
    m_isSpawnPointListDocked = docked;

    // reparent the Spawn List
    widget = m_spawnPointList;

    // preference
    preference = "DockedSpawnPointList";
    break;
  case 6: // Spawn List 2
    // note the new setting
    m_isSpawnList2Docked = docked;

    // reparent the Spawn List
    widget = m_spawnList2;

    // preference
    preference = "DockedSpawnList2";
    break;
  default:
    // use default for maps since the number of them can be changed via a
    // constant (maxNumMaps)
    if ((winnum >= mapDockBase) && (winnum < (mapDockBase + maxNumMaps)))
    {
      int i = winnum - mapDockBase;

      // note the new setting
      m_isMapDocked[i] = docked;

      // reparent teh appropriate map
      widget = m_map[i];

      QString tmpPrefSuffix = "";
      if (i > 0)
          tmpPrefSuffix = QString::number(i + 1);

      // preference
      preference = "DockedMap" + tmpPrefSuffix;
    }

    break;
    };

  // save new setting
  pSEQPrefs->setPrefBool(preference, "Interface", docked);

  // attempt to undock the window
  if (widget)
  {
    widget->setFloating(!docked);
    if (!docked)
      widget->activateWindow();

    // make the widget update it's geometry
    widget->updateGeometry();
  }
}


void EQInterface::toggle_view_DockableWin(QAction* win)
{
  SEQWindow* widget = 0;
  int winnum;
  QString preference;

  // get the window number parameter
  winnum = win->data().value<int>();

  // get the new menu item state
  bool dockable = win->isChecked();

  switch(winnum)
  {
  case 0: // Spawn List
    widget = m_spawnList;

    // preference
    preference = "DockableSpawnList";
    break;
  case 1: // Player Stats
    widget = m_statList;

    // preference
    preference = "DockablePlayerStats";
    break;
  case 2: // Player Skills
    widget = m_skillList;

    // preference
    preference = "DockablePlayerSkills";
    break;
  case 3: // Spell List
    widget = m_spellList;

    // preference
    preference = "DockableSpellList";
    break;
  case 4: // Compass
    widget = m_compass;

    // preference
    preference = "DockableCompass";
    break;
  case 5: // Spawn Point List
    widget = m_spawnPointList;

    // preference
    preference = "DockableSpawnPointList";
    break;
  case 6: // Spawn List 2
    widget = m_spawnList2;

    // preference
    preference = "DockableSpawnList2";
    break;
  case 7: // Experience Window
    widget = m_expWindow;

    preference = "DockableExperienceWindow";
    break;
  case 8: // Combat Window
    widget = m_combatWindow;

    preference = "DockableCombatWindow";
    break;
  case 9: // Guild List Window
    widget = m_guildListWindow;

    preference = "DockableGuildListWindow";
    break;
  case 10: // NetDiag
    widget = m_netDiag;

    preference = "DockableNetDiag";
    break;
  default:
    // use default for maps since the number of them can be changed via a 
    // constant (maxNumMaps)
    if ((winnum >= mapDockBase) && (winnum < (mapDockBase + maxNumMaps)))
    {
      int i = winnum - mapDockBase;

      // reparent teh appropriate map
      widget = m_map[i];

      QString tmpPrefSuffix = "";
      if (i > 0)
	tmpPrefSuffix = QString::number(i + 1);

      // preference
      preference = "DockableMap" + tmpPrefSuffix;
    }
    else if ((winnum >= messageWindowDockBase) && 
	     (winnum < (messageWindowDockBase + maxNumMessageWindows)))
    {
      int i = winnum - messageWindowDockBase;

      // reparent teh appropriate map
      widget = m_messageWindow[i];

      QString tmpPrefSuffix = "";
      tmpPrefSuffix = QString::number(i);

      // preference
      preference = "DockableMessageWindow" + tmpPrefSuffix;
    }

    break;
    };

  // save new setting
  pSEQPrefs->setPrefBool(preference, "Interface", dockable);

  // attempt to undock the window
  if (widget)
    setDockEnabled(widget, dockable);
}

void EQInterface::set_main_WindowCaption(QAction* win)
{
  QWidget* widget = 0;
  int winnum;
  QString window;

  // get the window number parameter
  winnum = win->data().value<int>();

  switch(winnum)
  {
  case 0: // Spawn List
    widget = m_spawnList;

    window = "Spawn List";
    break;
  case 1: // Player Stats
    widget = m_statList;

    window = "Player Stats";
    break;
  case 2: // Player Skills
    widget = m_skillList;

    window = "Player Skills";
    break;
  case 3: // Spell List
    widget = m_spellList;

    window = "Spell List";
    break;
  case 4: // Compass
    widget = m_compass;

    window = "Compass";
    break;
  case 5: // Interface
    widget = this;

    window = "Main Window";
    break;
  case 6: // Experience Window
    widget = m_expWindow;

    window = "Experience Window";
    break;
  case 7: // Combat Window
    widget = m_combatWindow;

    window = "Combat Window";
    break;
  case 8: // Network Diagnostics
    widget = m_netDiag;

    window = "Network Diagnostics";
    break;
  case 9: // Spawn Point List
    widget = m_spawnPointList;

    window = "Spawn Point List";
    break;
  case 10: // Spawn List
    widget = m_spawnList2;

    window = "Spawn List 2";
    break;
  default:
    // use default for maps since the number of them can be changed via a 
    // constant (maxNumMaps)
    if ((winnum >= mapCaptionBase) && (winnum < (mapCaptionBase + maxNumMaps)))
    {
      int i = winnum - mapCaptionBase;

      widget = m_map[i];
    }

    break;
  };

  // attempt to undock the window
  if (widget != 0)
  {
    bool ok = false;
    QString caption =
      QInputDialog::getText(this, "ShowEQ " + window + "Caption",
              "Enter caption for the " + window + ":",
              QLineEdit::Normal, widget->windowTitle(),
              &ok);

    // if the user entered a caption and clicked ok, set the windows caption
    if (ok)
      widget->setWindowTitle(caption);
  }
}


void EQInterface::set_main_WindowFont(QAction* win)
{
  int winnum;

  // get the window number parameter
  winnum = win->data().value<int>();

  bool ok = false;
  QFont newFont;
  SEQWindow* window = 0;
  QString title;
  
  //
  // NOTE: Yeah, this sucks for now, but until the architecture gets cleaned
  // up it will have to do
  switch(winnum)
  {
  case -1:
      // since the entire submenu is bound to this function, this function
      // gets called for font changes that aren't handled here (default app
      // and status bar).  So we've set the ID to -1 in both case, and we
      // ignore it here.
      //
      // TODO this can be fixed by creating an action group for the menu,
      // adding the relevant entries to to, and then changing the connect
      // call to connect the action group instead of the entire menu.
      return;
  case 0: // Spawn List
    title = "Spawn List";
    
    window = m_spawnList;
    break;
  case 1: // Player Stats
    title = "Player Stats";
    
    window = m_statList;
    break;
  case 2: // Player Skills
    title = "Player Skills";

    window = m_skillList;
    break;
  case 3: // Spell List
    title = "Spell List";
    
    window = m_spellList;
    break;
  case 4: // Compass
    title = "Compass";
    
    window = m_compass;
    break;
  case 5: // Interface
    // window = "Main Window";
    break;
  case 6: // Experience Window
    title = "Experience Window";
    
    window = m_expWindow;
    break;
  case 7: // Combat Window
    title = "Combat Window";

    window = m_combatWindow;
    break;
  case 8: // Network Diagnostics
    title = "Network Diagnostics";

    window = m_netDiag;
    break;
  case 9: // Spawn Point List
    title = "Spawn Point List";

    window = m_spawnPointList;
    break;
  case 10: // Spawn List
    title = "Spawn List 2";
    
    window = m_spawnList2;
    break;
  default:
    // use default for maps since the number of them can be changed via a 
    // constant (maxNumMaps)
    if ((winnum >= mapCaptionBase) && (winnum < (mapCaptionBase + maxNumMaps)))
    {
      int i = winnum - mapCaptionBase;
      if (i)
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
	title = QString::asprintf("Map %d", i);
#else
	title.sprintf("Map %d", i);
#endif
      else
	title = "Map";

      window = m_map[i];
    }
    break;
  };

  if (window != 0)
  {
    // get a new font
    newFont = QFontDialog::getFont(&ok, window->font(), 
				   this, "ShowEQ " + title + " Font");
    
    
    // if the user entered a font and clicked ok, set the windows font
    if (ok)
      window->setWindowFont(newFont);
  }
}

void EQInterface::set_main_Font()
{
  QString name = "ShowEQ - Application Font";
  bool ok = false;

  // get a new application font
  QFont newFont;
  newFont = QFontDialog::getFont(&ok, QApplication::font(),
          this, name);

  // if the user clicked ok and selected a valid font, set it
  if (ok)
  {
    // set the new application font
    qApp->setFont( newFont );

    // set the preference for future sessions
    pSEQPrefs->setPrefFont("Font", "Interface", newFont);

    // make sure the windows that override the application font, do so
    emit restoreFonts();
  }
}


void EQInterface::select_main_FormatFile()
{
  QString formatFile = pSEQPrefs->getPrefString("FormatFile", "Interface", 
						"eqstr_us.txt");

  QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", formatFile);

  QString newFormatFile =
    QFileDialog::getOpenFileName(this, "Select Format File",
            fileInfo.absoluteFilePath(), "*.txt");

  // if the newFormatFile name is not empty, then the user selected a file
  if (!newFormatFile.isEmpty())
  {
    // set the new format file to use
    pSEQPrefs->setPrefString("FormatFile", "Interface", newFormatFile);

    // reload the format strings
    loadFormatStrings();
  }
}

void EQInterface::select_main_SpellsFile()
{
  QString spellsFile = pSEQPrefs->getPrefString("SpellsFile", "Interface", 
						"spells_us.txt");

  QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", spellsFile);

  QString newSpellsFile =
    QFileDialog::getOpenFileName(this, "Select Spells File",
            fileInfo.absoluteFilePath(), "*.txt");

  // if the newFormatFile name is not empty, then the user selected a file
  if (!newSpellsFile.isEmpty())
  {
    // set the new format file to use
    pSEQPrefs->setPrefString("SpellsFile", "Interface", newSpellsFile);

    // reload the spells
    m_spells->loadSpells(newSpellsFile);
  }
}

void EQInterface::toggle_main_statusbar_Window(QAction* field)
{
  QWidget* window = 0;
  QString preference;

  switch (field->data().value<int>())
  {
  case 1:
    window = m_stsbarStatus;

    preference = "ShowStatus";
    break;
  case 2:
    window = m_stsbarZone;

    preference = "ShowZone";
    break;
  case 3:
    window = m_stsbarSpawns;

    preference = "ShowSpawns";
    break;
  case 4:
    window = m_stsbarExp;

    preference = "ShowExp";
    break;
  case 5:
    window = m_stsbarExpAA;

    preference = "ShowExpAA";
    break;
  case 6:
    window = m_stsbarPkt;

    preference = "ShowPacketCounter";
    break;
  case 7:
    window = m_stsbarEQTime;

    preference = "ShowEQTime";
    break;
  case 8:
    window = m_stsbarSpeed;

    preference = "ShowSpeed";
    break;
  // ZEM code
  case 9:
    window = m_stsbarZEM;

    preference = "ShowZEM";
    break;

  default:
    return;
  }

  if (window == 0)
    return;

  // should the window be visible
  bool show = !window->isVisible();

  // show or hide the window as necessary
  if (show)
    window->show();
  else
    window->hide();

  // check/uncheck the menu item
  field->setChecked(show);

  // set the preference for future sessions
  pSEQPrefs->setPrefBool(preference, "Interface_StatusBar", show);
}

void EQInterface::set_main_statusbar_Font()
{
  QString name = "ShowEQ - Status Font";
  bool ok = false;

  // setup a default new status font
  QFont newFont = QApplication::font();
  newFont.setPointSize(8);

  // get new status font
  newFont = QFontDialog::getFont(&ok,
          pSEQPrefs->getPrefFont("StatusFont", "Interface", newFont),
          this, name);

  // if the user clicked ok and selected a valid font, set it
  if (ok)
  {
    // set the preference for future sessions
    pSEQPrefs->setPrefFont("StatusFont", "Interface", newFont);

    // make sure to reset the status font since the previous call may have 
    // changed it
    restoreStatusFont();
  }
}

void
EQInterface::toggle_main_SavePosition(bool enable)
{
    pSEQPrefs->setPrefBool("SavePosition", "Interface", enable);
}

void
EQInterface::toggle_main_UseWindowPos(bool enable)
{
    pSEQPrefs->setPrefBool("UseWindowPos", "Interface", enable);
}

//
// save prefs
//
void
EQInterface::savePrefs(void)
{
  seqDebug("==> EQInterface::savePrefs()");

  QString section = "Interface";
  bool savePos = pSEQPrefs->getPrefBool("SavePosition", section, true);

   if( isVisible() && savePos)
   {
     seqDebug("\tisVisible()");

     QString dockPrefsState = saveState().toBase64();
     pSEQPrefs->setPrefString("DockingInfoState", section, dockPrefsState);

     QString dockPrefsGeometry = saveGeometry().toBase64();
     pSEQPrefs->setPrefString("DockingInfoGeometry", section, dockPrefsGeometry);

     // send savePrefs signal out
     emit saveAllPrefs();

     // save prefs to file
     pSEQPrefs->save();
   }
} // end savePrefs


void EQInterface::setCaption(const QString& text)
{
  QMainWindow::setWindowTitle(text);

  pSEQPrefs->setPrefString("Caption", "Interface", windowTitle());
}


void EQInterface::loadFormatStrings()
{
  // get the name of the format file
  QString formatFileName = pSEQPrefs->getPrefString("FormatFile", "Interface", 
						    "eqstr_us.txt");

  QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", 
							   formatFileName);

  // load the strings
  m_eqStrings->load(fileInfo.absoluteFilePath());
}

void
EQInterface::select_filter_file(void)
{
  QString filterFile = QFileDialog::getOpenFileName(this, "Select Filter Config",
          m_filterMgr->filterFile(), QString("ShowEQ Filter Files (*.xml)"));

  if (!filterFile.isEmpty())
    m_filterMgr->loadFilters(filterFile);
}

void EQInterface::toggle_filter_Case(bool cs)
{

  m_filterMgr->setCaseSensitive(cs);
  pSEQPrefs->setPrefBool("IsCaseSensitive", "Filters", 
			 m_filterMgr->caseSensitive());
}

void EQInterface::toggle_filter_AlertInfo(bool enable)
{
  pSEQPrefs->setPrefBool("AlertInfo", "Filters", enable);
}

void EQInterface::toggle_filter_UseSystemBeep(bool enable)
{
  m_filterNotifications->setUseSystemBeep(enable);
}

void EQInterface::toggle_filter_UseCommands(bool enable)
{
  m_filterNotifications->setUseCommands(enable);
}

void EQInterface::toggle_filter_Log(QAction* flag)
{
  if (!m_filteredSpawnLog)
    createFilteredSpawnLog();

  uint32_t filters = m_filteredSpawnLog->filters();
  uint32_t filter = flag->data().value<uint32_t>();

  if (filters & filter)
    filters &= ~filter;
  else
    filters |= filter;

  m_filteredSpawnLog->setFilters(filters);

  flag->setChecked((filters & filter) != 0);
  pSEQPrefs->setPrefBool("Log", "Filters", filters);
}

void EQInterface::set_filter_AudioCommand(QAction* type)
{
  QString property;
  QString prettyName;
  switch(type->data().value<int>())
  {
  case 1:
    property = "SpawnAudioCommand";
    prettyName = "Spawn";
    break;
  case 2:
    property = "DeSpawnAudioCommand";
    prettyName = "DeSpawn";
    break;
  case 3:
    property = "DeathAudioCommand";
    prettyName = "Death";
    break;
  case 4:
    property = "LocateSpawnAudioCommand";
    prettyName = "Locate Spawn";
    break;
  case 5:
    property = "CautionSpawnAudioCommand";
    prettyName = "Caution Spawn";
    break;
  case 6:
    property = "HuntSpawnAudioCommand";
    prettyName = "Hunt Spawn";
    break;
  case 7:
    property = "DangerSpawnAudioCommand";
    prettyName = "Danger Spawn";
    break;
  default: 
    return;
  }

  QString value = pSEQPrefs->getPrefString(property, "Filters",
					   "/usr/bin/esdplay " PKGDATADIR "/spawn.wav &");

  bool ok = false;
  QString command =
    QInputDialog::getText(this, "ShowEQ " + prettyName + "Command",
            "Enter command line to use for " + prettyName + "'s:",
            QLineEdit::Normal, value,
            &ok);

  if (ok)
    pSEQPrefs->setPrefString(property, "Filters", command);
}

void EQInterface::listSpawns (void)
{
#ifdef DEBUG
  qDebug ("listSpawns()");
#endif /* DEBUG */

  QString outText;

  // open the output data stream
  QTextStream out(&outText, QIODevice::WriteOnly);
  
   // dump the spawns 
  m_spawnShell->dumpSpawns(tSpawn, out);

  seqInfo(outText.toLatin1().data());
}

void EQInterface::listDrops (void)
{
#ifdef DEBUG
  qDebug ("listDrops()");
#endif /* DEBUG */
  QString outText;

  // open the output data stream
  QTextStream out(&outText, QIODevice::WriteOnly);

  // dump the drops
  m_spawnShell->dumpSpawns(tDrop, out);

  seqInfo(outText.toLatin1().data());
}

void EQInterface::listMapInfo(void)
{
#ifdef DEBUG
  qDebug ("listMapInfo()");
#endif /* DEBUG */
  QString outText;

  // open the output data stream
  QTextStream out(&outText, QIODevice::WriteOnly);

  // dump map managers info
  m_mapMgr->dumpInfo(out);

  // iterate over all the maps
  for (int i = 0; i < maxNumMaps; i++)
  {
    // if this map has been instantiated, dump it's info
    if (m_map[i] != 0)
      m_map[i]->dumpInfo(out);
  }

  seqInfo(outText.toLatin1().data());
}

void EQInterface::listInterfaceInfo(void)
{
#ifdef DEBUG
  qDebug ("listInterfaceInfo()");
#endif /* DEBUG */

  //FIXME - writing directly from the main interface no longer works with QT4.
  //it may be possible to extract the info from saveState and saveGeometry, or
  //it may be necessary to enumerate each window and get the layout
  //info individually

  QString outText;

  // open the output data stream
  QTextStream out(&outText, QIODevice::WriteOnly);

  out << "Map window layout info:" << ENDL;
  out << "-----------------------" << ENDL;
  //FIXME out << *this;
  out << "FIXME" << ENDL;
  out << "-----------------------" << ENDL;

  seqInfo(outText.toLatin1().data());
}

void EQInterface::listGroup(void)
{
#ifdef DEBUG
  qDebug ("listGroup()");
#endif /* DEBUG */
  QString outText;

  // open the output data stream
  QTextStream out(&outText, QIODevice::WriteOnly);

  // dump the drops
  m_groupMgr->dumpInfo(out);

  seqInfo(outText.toLatin1().data());
}


void EQInterface::listGuild(void)
{
#ifdef DEBUG
  qDebug ("listGuild()");
#endif /* DEBUG */
  QString outText;

  // open the output data stream
  QTextStream out(&outText, QIODevice::WriteOnly);

  // dump the drops
  m_guildShell->dumpMembers(out);

  seqInfo(outText.toLatin1().data());
}

void EQInterface::dumpSpawns (void)
{
#ifdef DEBUG
  qDebug ("dumpSpawns()");
#endif /* DEBUG */
  
  QString logFile = pSEQPrefs->getPrefString("DumpSpawnsFilename", "Interface",
					     "dumpspawns.txt");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

  // open the output data stream
  QFile file(logFileInfo.absoluteFilePath());
  file.open(QIODevice::WriteOnly);
  QTextStream out(&file);
  
  // dump the spawns 
  m_spawnShell->dumpSpawns(tSpawn, out);
}

void EQInterface::dumpDrops (void)
{
#ifdef DEBUG
  qDebug ("dumpDrops()");
#endif /* DEBUG */

  QString logFile = pSEQPrefs->getPrefString("DumpDropsFilename", "Interface",
					     "dumpdrops.txt");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

  // open the output data stream
  QFile file(logFileInfo.absoluteFilePath());
  file.open(QIODevice::WriteOnly);
  QTextStream out(&file);

  // dump the drops
  m_spawnShell->dumpSpawns(tDrop, out);
}

void EQInterface::dumpMapInfo(void)
{
#ifdef DEBUG
  qDebug ("dumpMapInfo()");
#endif /* DEBUG */

  QString logFile = pSEQPrefs->getPrefString("DumpMapInfoFilename", 
					     "Interface",
					     "mapinfo.txt");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

  // open the output data stream
  QFile file(logFileInfo.absoluteFilePath());
  file.open(QIODevice::WriteOnly);
  QTextStream out(&file);

  // dump map managers info
  m_mapMgr->dumpInfo(out);

  // iterate over all the maps
  for (int i = 0; i < maxNumMaps; i++)
  {
    // if this map has been instantiated, dump it's info
    if (m_map[i] != 0)
      m_map[i]->dumpInfo(out);
  }
}

void EQInterface::dumpGuildInfo(void)
{
  QString logFile = pSEQPrefs->getPrefString("GuildsDumpFile", 
					     "Interface", 
					     "guilds.txt");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

  emit guildList2text(logFileInfo.absoluteFilePath());
}

void EQInterface::dumpSpellBook(void)
{
#ifdef DEBUG
  qDebug ("dumpSpellBook");
#endif /* DEBUG */

  QString logFile = pSEQPrefs->getPrefString("DumpSpellBookFilename", 
					     "Interface", 
					     "spellbook.txt");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

  // open the output data stream
  QFile file(logFileInfo.absoluteFilePath());
  file.open(QIODevice::WriteOnly);
  QTextStream out(&file);
  QString txt;

  seqInfo("Dumping Spell Book to '%s'\n", 
          file.fileName().toUtf8().data());
  out << "Spellbook of " << m_player->name() << " a level " 
      << m_player->level() << " " << m_player->raceString() 
      << " " << m_player->classString()
      << ENDL;

  uint8_t playerClass = m_player->classVal();

  uint32_t spellid;
  for (uint32_t i = 0; i < MAX_SPELLBOOK_SLOTS; i++)
  {
    spellid = m_player->getSpellBookSlot(i);
    if (spellid == 0xffffffff)
      continue;

    const Spell* spell = m_spells->spell(spellid);

    QString spellName;

    if (spell)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
        txt = QString::asprintf("%.3d %.2d %.2d %#4.04x %02d\t%s",
                i, ((i / 8) + 1), ((i % 8) + 1),
                spellid, spell->level(playerClass),
                spell->name().toLatin1().data());
#else
        txt.sprintf("%.3d %.2d %.2d %#4.04x %02d\t%s",
                i, ((i / 8) + 1), ((i % 8) + 1),
                spellid, spell->level(playerClass),
                spell->name().toLatin1().data());
#endif
    }
    else
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
        txt = QString::asprintf("%.3d %.2d %.2d %#4.04x   \t%s",
                i, ((i / 8) + 1), ((i % 8) + 1),
                spellid,
                spell_name(spellid).toLatin1().data());
#else
        txt.sprintf("%.3d %.2d %.2d %#4.04x   \t%s",
                i, ((i / 8) + 1), ((i % 8) + 1),
                spellid,
                spell_name(spellid).toLatin1().data());
#endif
    }

    out << txt << ENDL;
  }
}

void EQInterface::dumpGroup(void)
{
#ifdef DEBUG
  qDebug ("dumpGroup()");
#endif /* DEBUG */

  QString logFile = pSEQPrefs->getPrefString("DumpGroupFilename", "Interface",
					     "dumpgroup.txt");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

  // open the output data stream
  QFile file(logFileInfo.absoluteFilePath());
  file.open(QIODevice::WriteOnly);
  QTextStream out(&file);

  // dump the drops
  m_groupMgr->dumpInfo(out);
}

void EQInterface::dumpGuild(void)
{
#ifdef DEBUG
  qDebug ("dumpGuild()");
#endif /* DEBUG */

  QString logFile = pSEQPrefs->getPrefString("DumpGuildFilename", "Interface",
					     "dumpguild.txt");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

  // open the output data stream
  QFile file(logFileInfo.absoluteFilePath());
  file.open(QIODevice::WriteOnly);
  QTextStream out(&file);

  // dump the drops
  m_guildShell->dumpMembers(out);
}

void
EQInterface::launch_editor_filters(void)
{
  EditorWindow * ew = new EditorWindow(m_filterMgr->filterFile().toLatin1().data());
  ew->setWindowTitle(m_filterMgr->filterFile());
  ew->show();
}

void
EQInterface::launch_filterlistwindow_filters(void)
{
  FilterListWindow * ew = new FilterListWindow(m_filterMgr->filterFile(), this);
}

void
EQInterface::launch_editor_zoneFilters(void)
{
  EditorWindow * ew = new EditorWindow(m_filterMgr->zoneFilterFile().toLatin1().data());
  ew->setWindowTitle(m_filterMgr->zoneFilterFile());
  ew->show();
}

void
EQInterface::launch_filterlistwindow_zoneFilters(void)
{
  FilterListWindow * ew = new FilterListWindow(m_filterMgr->zoneFilterFile(), this);
}

void
EQInterface::toggle_opt_ConSelect (void)
{
  m_selectOnConsider = !(m_selectOnConsider);
  m_action_opt_ConSelect->setChecked(m_selectOnConsider);
  pSEQPrefs->setPrefBool("SelectOnCon", "Interface", m_selectOnConsider);
}

void
EQInterface::toggle_opt_TarSelect (void)
{
  m_selectOnTarget = !(m_selectOnTarget);
  m_action_opt_TarSelect->setChecked(m_selectOnTarget);
  pSEQPrefs->setPrefBool("SelectOnTarget", "Interface", m_selectOnTarget);
}

void
EQInterface::toggle_opt_TarDeselect (void)
{
  m_deselectOnUntarget = !(m_deselectOnUntarget);
  m_action_opt_TarDeselect->setChecked(m_deselectOnUntarget);
  pSEQPrefs->setPrefBool("DeselectOnUntarget", "Interface", m_deselectOnUntarget);
}

void
EQInterface::toggle_opt_Fast (void)
{
  showeq_params->fast_machine = !(showeq_params->fast_machine);
  m_action_opt_Fast->setChecked(showeq_params->fast_machine);
  pSEQPrefs->setPrefBool("FastMachine", "Misc", showeq_params->fast_machine);
}

void
EQInterface::toggle_opt_KeepSelectedVisible (void)
{
  showeq_params->keep_selected_visible = !(showeq_params->keep_selected_visible);
  m_action_opt_KeepSelectedVisible->setChecked(showeq_params->keep_selected_visible);
  pSEQPrefs->setPrefBool("KeepSelected", "Interface",
          showeq_params->keep_selected_visible);
}

void
EQInterface::toggle_opt_UseUpdateRadius (void)
{
  showeq_params->useUpdateRadius = !(showeq_params->useUpdateRadius);
  m_action_opt_UseUpdateRadius->setChecked(showeq_params->useUpdateRadius);
  pSEQPrefs->setPrefBool("UseUpdateRadius", "Interface",
          showeq_params->useUpdateRadius);
}

/* Check and uncheck Log menu options & set EQPacket logging flags */
void EQInterface::toggle_log_AllPackets (void)
{
  if (m_globalLog)
  {
    delete m_globalLog;
    m_globalLog = 0;
  }
  else
    createGlobalLog();

  bool state = (m_globalLog != 0);
  m_action_log_AllPackets->setChecked(state);
  pSEQPrefs->setPrefBool("LogAllPackets", "PacketLogging", state);
}

void EQInterface::toggle_log_WorldData (void)
{
  if (m_worldLog)
  {
    delete m_worldLog;
    m_worldLog = 0;
  }
  else
    createWorldLog();

  bool state = (m_worldLog != 0);
  m_action_log_WorldData->setChecked(state);
  pSEQPrefs->setPrefBool("LogWorldPackets", "PacketLogging", state);
}

void EQInterface::toggle_log_ZoneData (void)
{
  if (m_zoneLog)
  {
    delete m_zoneLog;
    m_zoneLog = 0;
  }
  else
    createZoneLog();

  bool state = (m_zoneLog != 0);
  m_action_log_ZoneData->setChecked(state);
  pSEQPrefs->setPrefBool("LogZonePackets", "PacketLogging", state);
}

void EQInterface::toggle_log_Filter_ZoneData_Client (void)
{
   bool state = true;
   if(showeq_params->filterZoneDataLog == DIR_Client)
   {
      showeq_params->filterZoneDataLog = 0;
      state = false;
   }
   else
   {
      showeq_params->filterZoneDataLog = DIR_Client;
   }
   m_action_log_Filter_ZoneData_Client->setChecked(state);
   m_action_log_Filter_ZoneData_Server->setChecked(false);
}

void EQInterface::toggle_log_Filter_ZoneData_Server (void)
{
   bool state = true;
   if(showeq_params->filterZoneDataLog == DIR_Server)
   {
      showeq_params->filterZoneDataLog = 0;
      state = false;
   }
   else
   {
      showeq_params->filterZoneDataLog = DIR_Server;
   }
   m_action_log_Filter_ZoneData_Server->setChecked(state);
   m_action_log_Filter_ZoneData_Client->setChecked(false);
}

void EQInterface::toggle_opt_BazaarData (void)
{
  if (m_bazaarLog)
  {
    disconnect(m_bazaarLog,0,0,0);
    delete m_bazaarLog;
    m_bazaarLog = 0;
  }
  else
    createBazaarLog();

  bool state = (m_bazaarLog != 0);
  m_action_opt_BazaarData->setChecked(state);
  pSEQPrefs->setPrefBool("LogBazaarPackets", "PacketLogging", state);
}

void EQInterface::toggle_log_UnknownData (void)
{
  if (m_unknownZoneLog)
  {
    delete m_unknownZoneLog;
    m_unknownZoneLog = 0;
  }
  else
    createUnknownZoneLog();

  bool state = (m_unknownZoneLog != 0);
  m_action_log_UnknownData->setChecked(state);
  pSEQPrefs->setPrefBool("LogUnknownZonePackets", "PacketLogging", state);
}

void EQInterface::toggle_log_RawData (void)
{
  bool state = !pSEQPrefs->getPrefBool("LogRawPackets", "PacketLogging",
				       false);

  if (m_worldLog)
    m_worldLog->setRaw(state);

  if (m_zoneLog)
    m_zoneLog->setRaw(state);

  m_action_log_RawData->setChecked(state);
  pSEQPrefs->setPrefBool("LogRawPackets", "PacketLogging", state);
}

/* Check and uncheck View menu options */
void
EQInterface::toggle_view_ChannelMsgs(QAction* msgwin)
{
  int winNum = msgwin->data().value<int>();

  bool wasVisible = ((m_messageWindow[winNum] != 0) && 
		     (m_messageWindow[winNum]->isVisible()));

  if (!wasVisible)
    showMessageWindow(winNum);
  else
  {
    // save any preference changes
    m_messageWindow[winNum]->savePrefs();

    // hide it 
    m_messageWindow[winNum]->hide();

    // remove its window menu
    removeWindowMenu(m_messageWindow[winNum]);

    // then delete it
    delete m_messageWindow[winNum];

    // make sure to clear it's variable
    m_messageWindow[winNum] = 0;
  }

  QString tmpPrefSuffix = "";
  if (winNum > 0)
    tmpPrefSuffix = QString::number(winNum + 1);
  
  QString tmpPrefName = QString("ShowMessageWindow") + tmpPrefSuffix;

  pSEQPrefs->setPrefBool(tmpPrefName, "Interface", !wasVisible); 
}

void
EQInterface::toggle_view_UnknownData (void)
{
  bool state = !pSEQPrefs->getPrefBool("ViewUnknown", "PacketLogging", 
				       false);

  if (m_unknownZoneLog)
    m_unknownZoneLog->setView(state);

  m_action_view_UnknownData->setChecked(state);
  pSEQPrefs->setPrefBool("ViewUnknown", "PacketLogging", state);
}

void EQInterface::toggle_view_ExpWindow ()
{
    if (!m_expWindow->isVisible())
    {
       m_expWindow->show();
       insertWindowMenu(m_expWindow);
    }
    else
    {
       m_expWindow->hide();
       removeWindowMenu(m_expWindow);
    }

    pSEQPrefs->setPrefBool("ShowExpWindow", "Interface",
			   m_expWindow->isVisible());
}

void EQInterface::toggle_view_CombatWindow (void)
{
  if (!m_combatWindow->isVisible())
  {
    m_combatWindow->show();
    insertWindowMenu(m_combatWindow);
  }
  else
  {
    m_combatWindow->hide();
    removeWindowMenu(m_combatWindow);
  }

  pSEQPrefs->setPrefBool("ShowCombatWindow", "Interface", 
			 m_combatWindow->isVisible());
}

void
EQInterface::toggle_view_SpawnList(void)
{
  bool wasVisible = ((m_spawnList != 0) && m_spawnList->isVisible());

  if (!wasVisible)
  {
    showSpawnList();

    // enable it's options sub-menu
    m_action_view_SpawnList_Options->setEnabled(true);
  }
  else
  {
    // save it's preferences
    m_spawnList->savePrefs();

    // hide it
    m_spawnList->hide();

    // disable it's options sub-menu
    m_action_view_SpawnList_Options->setEnabled(false);

    // remove its window menu
    removeWindowMenu(m_spawnList);

    // delete the window
    delete m_spawnList;

    // make sure to clear it's variable
    m_spawnList = 0;
  }

  pSEQPrefs->setPrefBool("ShowSpawnList", "Interface", !wasVisible);
}

void
EQInterface::toggle_view_SpawnList2(void)
{
  bool wasVisible = ((m_spawnList2 != 0) && m_spawnList2->isVisible());

  if (!wasVisible)
    showSpawnList2();
  else 
  {
    // save it's preferences
    m_spawnList2->savePrefs();

    // hide it
    m_spawnList2->hide();

    // remove its window menu
    removeWindowMenu(m_spawnList2);

    // delete the window
    delete m_spawnList2;

    // make sure to clear it's variable
    m_spawnList2 = 0;
  }

  pSEQPrefs->setPrefBool("ShowSpawnList2", "Interface", !wasVisible);
}

void
EQInterface::toggle_view_SpawnPointList(void)
{
  bool wasVisible = ((m_spawnPointList != 0) && 
		     m_spawnPointList->isVisible());

  if (!wasVisible)
    showSpawnPointList();
  else 
  {
    // save it's preferences
    m_spawnPointList->savePrefs();

    // hide it
    m_spawnPointList->hide();

    // remove its window menu
    removeWindowMenu(m_spawnPointList);

    // delete the window
    delete m_spawnPointList;

    // make sure to clear it's variable
    m_spawnPointList = 0;
  }

  pSEQPrefs->setPrefBool("ShowSpawnPointList", "Interface", !wasVisible);
}

void EQInterface::toggle_view_SpellList(void)
{
  bool wasVisible = ((m_spellList != 0) && (m_spellList->isVisible()));

  if (!wasVisible)
    showSpellList();
  else
  {
    // save it's preferences
    m_spellList->savePrefs();
    
    // hide it
    m_spellList->hide();

    // remove its window menu
    removeWindowMenu(m_spellList);

    // delete it
    delete m_spellList;
    
    // make sure to clear it's variable
    m_spellList = 0;
  }

  pSEQPrefs->setPrefBool("ShowSpellList", "Interface", !wasVisible); 
}

void EQInterface::toggle_view_PlayerStats(void)
{
  bool wasVisible = ((m_statList != 0) && m_statList->isVisible());

  if (!wasVisible)
  {
    showStatList();

    // enable it's options sub-menu
    m_action_view_PlayerStats_Options->setEnabled(true);
  }
  else 
  {
    // save it's preferences
    m_statList->savePrefs();

    // hide it
    m_statList->hide();

    // disable it's options sub-menu
    m_action_view_PlayerStats_Options->setEnabled(false);

    // remove its window menu
    removeWindowMenu(m_statList);

    // then delete it
    delete m_statList;

    // make sure to clear it's variable
    m_statList = 0;
  }
  
  pSEQPrefs->setPrefBool("ShowPlayerStats", "Interface", !wasVisible);
}

void EQInterface::toggle_view_PlayerSkills(void)
{
  bool wasVisible = ((m_skillList != 0) && m_skillList->isVisible());

  if (!wasVisible)
  {
    showSkillList();

    m_action_view_PlayerSkills_Options->setEnabled(true);
  }
  else
  {
    // save any preference changes
    m_skillList->savePrefs();

    // if it's not visible, hide it
    m_skillList->hide();

    // disable it's options sub-menu
    m_action_view_PlayerSkills_Options->setEnabled(false);

    // remove its window menu
    removeWindowMenu(m_skillList);

    // then delete it
    delete m_skillList;

    // make sure to clear it's variable
    m_skillList = 0;
  }

  pSEQPrefs->setPrefBool("ShowPlayerSkills", "Interface", !wasVisible);
}

void
EQInterface::toggle_view_Compass(void)
{
  bool wasVisible = ((m_compass != 0) && (m_compass->isVisible()));

  if (!wasVisible)
    showCompass();
  else
  {
    // if it's not visible, hide it
    m_compass->hide();

    // remove its window menu
    removeWindowMenu(m_compass);

    // then delete it
    delete m_compass;

    // make sure to clear it's variable
    m_compass = 0;
  }

  pSEQPrefs->setPrefBool("ShowCompass", "Interface", !wasVisible);
}

void EQInterface::toggle_view_Map(QAction* map)
{
  int mapNum = map->data().value<int>();

  bool wasVisible = ((m_map[mapNum] != 0) && 
		     (m_map[mapNum]->isVisible()));

  if (!wasVisible)
    showMap(mapNum);
  else
  {
    // save any preference changes
    m_map[mapNum]->savePrefs();

    // hide it 
    m_map[mapNum]->hide();

    // remove its window menu
    removeWindowMenu(m_map[mapNum]);

    // then delete it
    delete m_map[mapNum];

    // make sure to clear it's variable
    m_map[mapNum] = 0;
  }

  QString tmpPrefSuffix = "";
  if (mapNum > 0)
    tmpPrefSuffix = QString::number(mapNum + 1);
  
  QString tmpPrefName = QString("ShowMap") + tmpPrefSuffix;

  pSEQPrefs->setPrefBool(tmpPrefName, "Interface", !wasVisible); 
}

void
EQInterface::toggle_view_NetDiag(void)
{
  bool wasVisible = ((m_netDiag != 0) && (m_netDiag->isVisible()));

  if (!wasVisible)
    showNetDiag();
  else
  {
    // if it's not visible, hide it
    m_netDiag->hide();

    // remove its window menu
    removeWindowMenu(m_netDiag);

    // then delete it
    delete m_netDiag;

    // make sure to clear it's variable
    m_netDiag = 0;
  }

  pSEQPrefs->setPrefBool("ShowNetStats", "Interface", !wasVisible);
}

void
EQInterface::toggle_view_GuildList(void)
{
  bool wasVisible = ((m_guildListWindow != 0) && 
		     (m_guildListWindow->isVisible()));

  if (!wasVisible)
    showGuildList();
  else
  {
    // if it's not visible, hide it
    m_guildListWindow->hide();

    // remove its window menu
    removeWindowMenu(m_guildListWindow);

    // then delete it
    delete m_guildListWindow;

    // make sure to clear it's variable
    m_guildListWindow = 0;
  }

  pSEQPrefs->setPrefBool("ShowGuildList", "Interface", !wasVisible);
}

bool 
EQInterface::getMonitorOpCodeList(const QString& title, 
				  QString& opCodeList)
{
  bool ok = false;
  QString newMonitorOpCode_List =
      QInputDialog::getText(this, title,
              "A list of OpCodes seperated by commas...\n"
              "\n"
              "Each Opcode has 4 arguments, only one of which is actually necessary...\n"
              "They are:\n"
              "OpCode:    16-bit HEX value of the OpCode\n"
              "            (REQUIRED - No Default)\n"
              "\n"
              "Alias:     Name used when displaying the Opcode\n"
              "            (DEFAULT: Monitored OpCode)\n"
              "\n"
              "Direction: 1 = Client ---> Server\n"
              "           2 = Client <--- Server\n"
              "           3 = Client <--> Server (BOTH)\n"
              "            (DEFAULT: 3)\n"
              "\n"
              "Show known 1 = Show if OpCode is marked as known.\n"
              "           0 = Ignore if OpCode is known.\n"
              "            (DEFAULT: 0)\n"
              "\n"
              "The way which you include the arguments in the list of OpCodes is:\n"
              "adding a ':' inbetween arguments and a ',' after the last OpCode\n"
              "argument.\n"
              "\n"
              "(i.e. 7F21:Mana Changed:3:1, 7E21:Unknown Spell Event(OUT):1,\n"
              "      7E21:Unknown Spell Event(IN):2 )\n",
      QLineEdit::Normal,
      opCodeList,
      &ok);

  if (ok)
    opCodeList = newMonitorOpCode_List;

  return ok;
}

void
EQInterface::toggle_opcode_monitoring()
{
  if(m_opcodeMonitorLog == 0)
  {
    QString section = "OpCodeMonitoring";
    QString opCodeList = pSEQPrefs->getPrefString("OpCodeList", section, "");
    bool ok = getMonitorOpCodeList("ShowEQ - Enable OpCode Monitor",
				   opCodeList);

    if (ok && !opCodeList.isEmpty())
    {
      createOPCodeMonitorLog(opCodeList);

      // set the list of monitored opcodes
      pSEQPrefs->setPrefString("OpCodeList", section, opCodeList);


      seqInfo("OpCode monitoring is now ENABLED...\nUsing list:\t%s", opCodeList.toLatin1().data());
    }
  }
  else
  {
    delete m_opcodeMonitorLog;
    m_opcodeMonitorLog = 0;

    seqInfo("OpCode monitoring has been DISABLED...");
  }

  bool state = (m_opcodeMonitorLog != 0);
  m_action_opcode_monitor->setChecked(state);
  pSEQPrefs->setPrefBool("Enable", "OpCodeMonitoring", state);
}

void EQInterface::set_opcode_monitored_list(void)
{
  QString section = "OpCodeMonitoring";
  QString opCodeList = pSEQPrefs->getPrefString("OpCodeList", section, "");
  bool ok = getMonitorOpCodeList("ShowEQ - Reload OpCode Monitor", opCodeList);

  if (ok && m_opcodeMonitorLog)
  {
    m_opcodeMonitorLog->init(opCodeList);

    seqInfo("The monitored OpCode list has been reloaded...\nUsing list:\t%s",
            opCodeList.toLatin1().data());

    // set the list of monitored opcodes
    pSEQPrefs->setPrefString("OpCodeList", section, opCodeList);
  }
}


void EQInterface::toggle_opcode_log()
{
  QString section = "OpCodeMonitoring";
  bool state = !pSEQPrefs->getPrefBool("Log", section, false);

  if (m_opcodeMonitorLog)
  {
    m_opcodeMonitorLog->setLog(state);

    state = m_opcodeMonitorLog->log();
  }

  m_action_opcode_log->setChecked(state);
  pSEQPrefs->setPrefBool("Log", section, state);
}

void EQInterface::toggle_opcode_view()
{
  QString section = "OpCodeMonitoring";
  bool state = !pSEQPrefs->getPrefBool("View", section, false);

  if (m_opcodeMonitorLog)
    m_opcodeMonitorLog->setView(state);

  m_action_opcode_view->setChecked(state);
  pSEQPrefs->setPrefBool("View", section, state);
}


void
EQInterface::select_opcode_file(void)
{
  QString section = "OpCodeMonitoring";

  QString logFile = pSEQPrefs->getPrefString("LogFilename",
					     section,
					     "opcodemonitor.log");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);

  logFile =
    QFileDialog::getSaveFileName(this, "ShowEQ - OpCode Log File",
            logFileInfo.absoluteFilePath(), "*.log");

  // set log filename
  if (!logFile.isEmpty())
    pSEQPrefs->setPrefString("LogFilename", section, logFile);
}

void EQInterface::resetMaxMana(void)
{
  if (m_statList != 0)
    m_statList->statList()->resetMaxMana();
}

void
EQInterface::toggle_opt_LogSpawns (void)
{
  bool state = (m_spawnLogger == 0);

    if (state)
      createSpawnLog();
    else
    {
      // delete the spawn logger
      delete m_spawnLogger;

      // make sure to clear it's varialbe
      m_spawnLogger = 0;
    }

    m_action_opt_LogSpawns->setChecked(state);
    pSEQPrefs->setPrefBool("LogSpawns", "Misc", state);
}

void
EQInterface::toggle_opt_PvPTeams (void)
{
    showeq_params->pvp = !(showeq_params->pvp);
    m_action_opt_PvPTeams->setChecked(showeq_params->pvp);
    pSEQPrefs->setPrefBool("PvPTeamColoring", "Interface", showeq_params->pvp);
}

void
EQInterface::toggle_opt_PvPDeity (void)
{
    showeq_params->deitypvp = !(showeq_params->deitypvp);
    m_action_opt_PvPDeity->setChecked(showeq_params->deitypvp);
    pSEQPrefs->setPrefBool("DeityPvPTeamColoring", "Interface", showeq_params->deitypvp);
}

void
EQInterface::toggle_opt_CreateUnknownSpawns (bool enable)
{
    showeq_params->createUnknownSpawns = enable;
    pSEQPrefs->setPrefBool("CreateUnknownSpawns", "Misc", showeq_params->createUnknownSpawns);
}

void
EQInterface::toggle_opt_WalkPathRecord (bool enable)
{
    showeq_params->walkpathrecord = enable;
    pSEQPrefs->setPrefBool("WalkPathRecording", "Misc", showeq_params->walkpathrecord);
}

void
EQInterface::set_opt_WalkPathLength(int len)
{
  if ((len > 0) && (len <= 8192))
    showeq_params->walkpathlength = len;

  pSEQPrefs->setPrefInt("WalkPathLength", "Misc", showeq_params->walkpathlength);
}

void
EQInterface::toggle_opt_RetardedCoords (bool enable)
{
    showeq_params->retarded_coords = enable;
    pSEQPrefs->setPrefBool("RetardedCoords", "Interface", showeq_params->retarded_coords);
}

void
EQInterface::toggle_opt_SystimeSpawntime (bool enable)
{
    showeq_params->systime_spawntime = enable;
    pSEQPrefs->setPrefBool("SystimeSpawntime", "Interface", showeq_params->systime_spawntime);
}

void 
EQInterface::select_opt_conColorBase(QAction* con)
{
  ColorLevel level = (ColorLevel) con->data().value<int>();

  // get the current color
  QColor color = m_player->conColorBase(level);

  // get the new color
  QColor newColor = QColorDialog::getColor(color, this, QString("ShowEQ - Con Color"));

  // only set if the user selected a valid color and clicked ok
  if (newColor.isValid())
  {
    // set the new con color
    m_player->setConColorBase(level, newColor);
    
    // force the spawn lists to get rebuilt with the new colors
    rebuildSpawnList();
  }
}

void EQInterface::select_opt_mapColors()
{
    m_mapColorDialog->show();
}

void EQInterface::setExp(uint32_t totalExp, uint32_t totalTick,
			  uint32_t minExpLevel, uint32_t maxExpLevel, 
			  uint32_t tickExpLevel)
{
  if (m_stsbarExp)
  {
    char expperc[32];
    sprintf(expperc, "%.2f", totalTick*100.0/330.0);

    m_stsbarExp->setText(QString("Exp: %1 (%2/330, %3%)")
      .arg(Commanate(totalExp)).arg(totalTick).arg(expperc));
  }
}

void EQInterface::newExp(uint32_t newExp, uint32_t totalExp, 
			 uint32_t totalTick,
			 uint32_t minExpLevel, uint32_t maxExpLevel, 
			 uint32_t tickExpLevel)
{
  uint32_t leftExp = maxExpLevel - totalExp;

  if (newExp)
  {
    uint32_t needKills = leftExp / newExp;
    // format a string for the status bar
    if (m_stsbarStatus)
      m_stsbarStatus->setText(QString("Exp: %1; %2 (%3/330); %4 left [~ %5 kills]")
			      .arg(Commanate(newExp))
			      .arg(Commanate(totalExp - minExpLevel))
			      .arg(totalTick)
			      .arg(Commanate(leftExp))
			      .arg(needKills));

    if (m_stsbarExp)
    {
      char expperc[32];
      sprintf(expperc, "%.2f", totalTick*100.0/330.0);

      m_stsbarExp->setText(QString("Exp: %1 (%2/330, %3%)")
        .arg(Commanate(totalExp)).arg(totalTick).arg(expperc));
    }
  }
  else
  {
    if (m_stsbarStatus)
      m_stsbarStatus->setText(QString("Exp: <%1; %2 (%3/330); %4 left")
			      .arg(Commanate(tickExpLevel))
			      .arg(Commanate(totalExp - minExpLevel))
			      .arg(totalTick).arg(Commanate(leftExp)));

    if (m_stsbarExp)
    {
      char expperc[32];
      sprintf(expperc, "%.2f", totalTick*100.0/330.0);

      m_stsbarExp->setText(QString("Exp: %1 (%2/330, %3%)")
        .arg(Commanate(totalExp)).arg(totalTick).arg(expperc));
    }
  }
}

void EQInterface::setAltExp(uint32_t totalExp,
			    uint32_t maxExp, uint32_t tickExp, 
			    uint32_t aapoints)
{
  if (m_stsbarExpAA)
    m_stsbarExpAA->setText(QString("ExpAA: %1").arg(totalExp));
}

void EQInterface::newAltExp(uint32_t newExp, uint32_t totalExp, 
			    uint32_t totalTick, 
			    uint32_t maxExp, uint32_t tickExp, 
			    uint32_t aapoints)
{
  if (m_stsbarExpAA)
  {
    char aaperc[32];
    sprintf(aaperc, "%.2f", totalTick*100.0/330.0);

    m_stsbarExpAA->setText(QString("ExpAA: %1 (%2/330, %3%)")
        .arg(Commanate(totalExp)).arg(totalTick).arg(aaperc));
  }
}

void EQInterface::levelChanged(uint8_t level)
{
  QString tempStr;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  tempStr = QString::asprintf("New Level: %u", level);
#else
  tempStr.sprintf("New Level: %u", level);
#endif
  if (m_stsbarStatus)
    m_stsbarStatus->setText(tempStr);
}

//
// TODO:  clear after timeout miliseconds
//
void
EQInterface::stsMessage(const QString &string, int timeout)
{
  if (m_stsbarStatus)
    m_stsbarStatus->setText(string);
}

void
EQInterface::numSpawns(int num)
{
  // only update once per sec
  static int lastupdate = 0;
  if ( (mTime() - lastupdate) < 1000)
    return;
  lastupdate = mTime();

   QString tempStr;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
   tempStr = QString::asprintf("Mobs: %d", num);
#else
   tempStr.sprintf("Mobs: %d", num);
#endif
   m_stsbarSpawns->setText(tempStr);
}

void
EQInterface::newSpeed(double speed)
{
  // update twice per sec
  static int lastupdate = 0;
  if ( (mTime() - lastupdate) < 500)
    return;
  lastupdate = mTime();

   QString tempStr;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
   tempStr = QString::asprintf("Run Speed: %3.6f", speed);
#else
   tempStr.sprintf("Run Speed: %3.6f", speed);
#endif
   m_stsbarSpeed->setText(tempStr);
}

void 
EQInterface::resetPacket(int num, int stream)
{
  if(stream != (int)zone2client);
  // if passed 0 reset the average
  m_packetStartTime = mTime();
  m_initialcount = num;
}

void
EQInterface::numPacket(int num, int stream)
{

  if(stream != (int)zone2client)
    return;
  // start the timer of not started
  if (!m_packetStartTime)
    m_packetStartTime = mTime();

  // only update once per sec
  static int lastupdate = 0;
  if ( (mTime() - lastupdate) < 1000)
    return;
  lastupdate = mTime();
  

   QString tempStr;
   int delta = mTime() - m_packetStartTime;
   num -= m_initialcount;
   if (num && delta)
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     tempStr = QString::asprintf("Pkt: %d (%2.1f)", num, (float) (num<<10) / (float) delta);
#else
     tempStr.sprintf("Pkt: %d (%2.1f)", num, (float) (num<<10) / (float) delta);
#endif
   else   
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     tempStr = QString::asprintf("Pkt: %d", num);
#else
     tempStr.sprintf("Pkt: %d", num);
#endif

   m_stsbarPkt->setText(tempStr);
}

void EQInterface::attack2Hand1(const uint8_t* data)
{
  // const attack2Struct * atk2 = (const attack2Struct*)data;
}

void EQInterface::action2Message(const uint8_t* data)
{
  action2Struct *action2 = (action2Struct*)data;
  const Item* target = m_spawnShell->findID(tSpawn, action2->target);
  const Item* source = m_spawnShell->findID(tSpawn, action2->source);
  emit combatSignal(action2->target, action2->source, action2->type, action2->spell, action2->damage, 
		    (target != 0) ? target->name() : QString("Unknown"), (source != 0) ? source->name() : QString("Unknown"));
}

// belith - combatKillSpawn, fix for the combat window
//          this displays a killing shot on a mob in combat records
void EQInterface::combatKillSpawn(const uint8_t* data)
{
  const newCorpseStruct *deadspawn = (const newCorpseStruct *)data;
  // only show my kills
  if (deadspawn && deadspawn->killerId == m_player->id())
  {
    const Item* target = m_spawnShell->findID(tSpawn, deadspawn->spawnId);
    const Item* source = m_spawnShell->findID(tSpawn, deadspawn->killerId);
    emit combatSignal(deadspawn->spawnId, deadspawn->killerId,
		      (deadspawn->type == -25) ? 231 : deadspawn->type,
		      deadspawn->spellId, deadspawn->damage,
		      (target != 0) ? target->name() : QString("Unknown"),
		      (source != 0) ? source->name() : QString("Unknown"));
  }
}

void EQInterface::updatedDateTime(const QDateTime& dt)
{
  
  m_stsbarEQTime->setText(dt.toString(pSEQPrefs->getPrefString("DateTimeFormat", "Interface", "ddd MMM dd,yyyy - hh:mm ap")));
}

void EQInterface::syncDateTime(const QDateTime& dt)
{
  QString dateString = dt.toString(pSEQPrefs->getPrefString("DateTimeFormat", "Interface", "ddd MMM dd,yyyy - hh:mm ap"));

  m_stsbarEQTime->setText(dateString);
}

void EQInterface::zoneBegin(const QString& shortZoneName)
{
  emit newZoneName(shortZoneName);
  float percentZEM = ((float)(m_zoneMgr->zoneExpMultiplier()-0.75)/0.75)*100;
  QString tempStr;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  tempStr = QString::asprintf("ZEM: %3.2f%%", percentZEM);
#else
  tempStr.sprintf("ZEM: %3.2f%%", percentZEM);
#endif
  if (m_stsbarZEM)
    m_stsbarZEM->setText(tempStr);
}

void EQInterface::zoneEnd(const QString& shortZoneName, 
			  const QString& longZoneName)
{
  emit newZoneName(longZoneName);
  stsMessage("");
  float percentZEM = ((float)(m_zoneMgr->zoneExpMultiplier()-0.75)/0.75)*100;
  QString tempStr;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  tempStr = QString::asprintf("ZEM: %3.2f%%", percentZEM);
#else
  tempStr.sprintf("ZEM: %3.2f%%", percentZEM);
#endif
  if (m_stsbarZEM)
    m_stsbarZEM->setText(tempStr);
}

void EQInterface::zoneChanged(const QString& shortZoneName)
{
  QString tempStr;
  stsMessage("- Busy Zoning -");
  emit newZoneName(shortZoneName);
  float percentZEM = ((float)(m_zoneMgr->zoneExpMultiplier()-0.75)/0.75)*100;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  tempStr = QString::asprintf("ZEM: %3.2f%%", percentZEM);
#else
  tempStr.sprintf("ZEM: %3.2f%%", percentZEM);
#endif
  if (m_stsbarZEM)
    m_stsbarZEM->setText(tempStr);
}

void EQInterface::clientTarget(const uint8_t* data)
{

  const clientTargetStruct* cts = (const clientTargetStruct*)data;

  if (cts->newTarget == 0 && m_deselectOnUntarget)
  {
      m_selectedSpawn = 0;
      emit selectSpawn(m_selectedSpawn);
      updateSelectedSpawnStatus(m_selectedSpawn);
      return;
  }

  if (!m_selectOnTarget)
    return;

  // try to find the targeted spawn in the spawn shell
  const Item* item = m_spawnShell->findID(tSpawn, cts->newTarget);

  // if found, make it the currently selected target
  if (item)
  {
    // note the new selection
    m_selectedSpawn = item;
    
    // notify others of the new selected spawn
    emit selectSpawn(m_selectedSpawn);

    // update the spawn status
    updateSelectedSpawnStatus(m_selectedSpawn);
  }
}

void EQInterface::spawnSelected(const Item* item)
{
  // note the new selection
  m_selectedSpawn = item;
  
  // notify others of the new selected spawn
  emit selectSpawn(m_selectedSpawn);

  // update the spawn status
  updateSelectedSpawnStatus(m_selectedSpawn);
}

void EQInterface::spawnConsidered(const Item* item)
{
  if (item == 0)
    return;

  if (!m_selectOnConsider)
    return;

  // note the new selection
  m_selectedSpawn = item;
  
  // notify others of the new selected spawn
  emit selectSpawn(m_selectedSpawn);
  
  // update the spawn status
  updateSelectedSpawnStatus(m_selectedSpawn);
}

void EQInterface::addItem(const Item* item)
{
  uint32_t filterFlags = item->filterFlags();

  if (filterFlags & FILTER_FLAG_LOCATE)
  {
    // note the new selection
    m_selectedSpawn = item;
    
    // notify others of the new selected spawn
    emit selectSpawn(m_selectedSpawn);
    
    // update the spawn status
    updateSelectedSpawnStatus(m_selectedSpawn);
  } // End LOCATE Filter alerting
}


void EQInterface::delItem(const Item* item)
{
  // if this is the selected spawn, then there isn't a selected spawn anymore
  if (m_selectedSpawn == item)
  {
    m_selectedSpawn = 0;
    stsMessage("");

    // notify others of the new selected spawn
    emit selectSpawn(m_selectedSpawn);
  }
}

void EQInterface::killSpawn(const Item* item)
{
  if (m_selectedSpawn != item)
    return;

  // update status message, notifying that selected spawn has died
  QString string = m_selectedSpawn->name() + " died";

  stsMessage(string);
}

void EQInterface::changeItem(const Item* item)
{
  // if this isn't the selected spawn, nothing more to do
  if (item != m_selectedSpawn)
    return;

  updateSelectedSpawnStatus(item);
}

void EQInterface::updateSelectedSpawnStatus(const Item* item)
{
  if (item == 0)
  {
    stsMessage("");
    return;
  }

  const Spawn* spawn = 0;

  if ((item->type() == tSpawn) || (item->type() == tPlayer))
    spawn = (const Spawn*)item;

  // construct a message for the status message display
  QString string("");
  if (spawn != 0)
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    string = QString::asprintf("%d: %s:%d (%d/%d) Pos:", // "%d/%d/%d (%d) %s %s Item:%s",
            item->id(),
            item->name().toUtf8().data(),
            spawn->level(), spawn->HP(),
            spawn->maxHP());
#else
    string.sprintf("%d: %s:%d (%d/%d) Pos:", // "%d/%d/%d (%d) %s %s Item:%s",
            item->id(),
            item->name().toUtf8().data(),
            spawn->level(), spawn->HP(),
            spawn->maxHP());
#endif
  else
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    string = QString::asprintf("%d: %s: Pos:", // "%d/%d/%d (%d) %s %s Item:%s",
            item->id(),
            item->name().toUtf8().data());
#else
    string.sprintf("%d: %s: Pos:", // "%d/%d/%d (%d) %s %s Item:%s",
            item->id(),
            item->name().toUtf8().data());
#endif

  if (showeq_params->retarded_coords)
    string += QString::number(item->y()) + "/" 
      + QString::number(item->x()) + "/" 
      + QString::number(item->z());
  else
    string += QString::number(item->x()) + "/" 
      + QString::number(item->y()) + "/" 
      + QString::number(item->z());

  string += QString(" (") 
    + QString::number(item->calcDist(m_player->x(),
				     m_player->y(),
				     m_player->z()))
    + ") " + item->raceString() + " " + item->classString();

  // just call the status message method
  stsMessage(string);
}

void EQInterface::addCategory(void)
{
  if (m_categoryMgr)
    m_categoryMgr->addCategory();
}

void EQInterface::reloadCategories(void)
{
  if (m_categoryMgr)
    m_categoryMgr->reloadCategories();
}

void EQInterface::rebuildSpawnList()
{
  if (m_spawnList)
    m_spawnList->spawnList()->rebuildSpawnList();

  if (m_spawnList2)
    m_spawnList2->rebuildSpawnList();
}

void EQInterface::selectNext(void)
{
  if (m_spawnList)
    m_spawnList->spawnList()->selectNext();
}

void EQInterface::selectPrev(void)
{
  if (m_spawnList)
    m_spawnList->spawnList()->selectPrev();
}

void EQInterface::saveSelectedSpawnPath(void)
{
  QString fileName;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  fileName = QString::asprintf("%s_mobpath.map", m_zoneMgr->shortZoneName().toLatin1().data());
#else
  fileName.sprintf("%s_mobpath.map", m_zoneMgr->shortZoneName().toLatin1().data());
#endif

  QFileInfo fileInfo = m_dataLocationMgr->findWriteFile("maps", fileName, false);

  QFile mobPathFile(fileInfo.absoluteFilePath());
  if (mobPathFile.open(QIODevice::Append | QIODevice::WriteOnly))
  {
    QTextStream out(&mobPathFile);
    // append the selected spawns path to the end
    saveSpawnPath(out, m_selectedSpawn);

    seqInfo("Finished appending '%s'!\n", fileName.toLatin1().data());
  }
}

void EQInterface::saveSpawnPaths(void)
{
  QString fileName;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  fileName = QString::asprintf("%s_mobpath.map", m_zoneMgr->shortZoneName().toLatin1().data());
#else
  fileName.sprintf("%s_mobpath.map", m_zoneMgr->shortZoneName().toLatin1().data());
#endif

  QFileInfo fileInfo = m_dataLocationMgr->findWriteFile("maps", fileName, false);

  QFile mobPathFile(fileInfo.absoluteFilePath());
  if (mobPathFile.open(QIODevice::Truncate | QIODevice::WriteOnly))
  {
    QTextStream out(&mobPathFile);
    // map header line
    out << m_zoneMgr->longZoneName() << ","
	<< m_zoneMgr->shortZoneName() << ",0,0" << ENDL;

    // iterate over the spawns adding their paths to the file
    ItemConstIterator it(m_spawnShell->getConstMap(tSpawn));
    const Item* item;
    while (it.hasNext())
    {
      it.next();
      item = it.value();
      if (!item)
          break;

      if ((item->NPC() == SPAWN_NPC) || 
	  (item->NPC() == SPAWN_NPC_CORPSE) ||
	  (item->NPC() == SPAWN_NPC_UNKNOWN))
	saveSpawnPath(out, it.value());
    }

    seqInfo("Finished writing '%s'!\n", fileName.toLatin1().data());
  }
}

void EQInterface::saveSpawnPath(QTextStream& out, const Item* item)
{
  if (item == 0)
    return;

  const Spawn* spawn = spawnType(item);

  if (spawn == 0)
    return;

   const SpawnTrackList& trackList = spawn->trackList();
   SpawnTrackListIterator trackIt(spawn->trackList());
   int cnt = trackList.count();

   // only make a line if there is more then one point
   if (cnt < 2)
     return;

   const SpawnTrackPoint* trackPoint;
   
   out << "M," << spawn->realName() << ",blue," << trackList.count();
   //iterate over the track, writing out the points
   while (trackIt.hasNext())
   {
     trackPoint = trackIt.next();
     if (!trackPoint)
         break;

     out << "," << trackPoint->x() 
         << "," <<  trackPoint->y()
         << "," << trackPoint->z();
   }
   out << ENDL;
}

void EQInterface::toggle_net_real_time_thread(bool realtime)
{
   m_packet->setRealtime(realtime);
   pSEQPrefs->setPrefBool("RealTimeThread", "Network", realtime);
}

void EQInterface::set_net_monitor_next_client()
{
  // start monitoring the next client seen
  m_packet->monitorNextClient();

  // set it as the address to monitor next session
  pSEQPrefs->setPrefString("IP", "Network", m_packet->ip());
}

void EQInterface::set_net_client_IP_address()
{
  QStringList iplst;
  for( int l = 0; l < 5; l++)
  iplst += ipstr[l];
  bool ok = false;
  QString address =
     QInputDialog::getItem(this, "ShowEQ - EQ Client IP Address",
             "Enter IP address of EQ client",
             iplst, 0, true, &ok);
  if (ok)
  {
	for (int i = 4; i > 0; i--)
	ipstr[i] = ipstr[ i - 1 ];
	ipstr[0] = address;
    // start monitoring the new address
    m_packet->monitorIPClient(address);

    // set it as the address to monitor next session
    pSEQPrefs->setPrefString("IP", "Network", m_packet->ip());
  }
}

void EQInterface::set_net_client_MAC_address()
{
  QStringList maclst;
  for( int l = 0; l < 5; l++)
  maclst += macstr[l];
  bool ok = false;
  QString address =
     QInputDialog::getItem(this, "ShowEQ - EQ Client MAC Address",
             "Enter MAC address of EQ client",
             maclst, 0, true, &ok);
  if (ok)
  {
    if (address.length() != 17 && !address.isEmpty())
    {
      seqWarn("Invalid MAC Address (%s)! Ignoring!", address.toLatin1().data());
      return;
    }
 	for (int i = 4; i > 0; i--)
	macstr[i] = macstr[ i - 1 ];
	macstr[0] = address;
    // start monitoring the new address
    m_packet->monitorMACClient(address);

    // set it as the address to monitor next session
    pSEQPrefs->setPrefString("MAC", "Network", m_packet->mac());
  }
}


QString EQInterface::promptForNetDevice()
{
    m_deviceList = enumerateNetworkDevices();

    int current = 0;
    if (m_packet)
        current = m_deviceList.indexOf(m_packet->device());

    bool ok = false;
    QString selected = QInputDialog::getItem(this, "ShowEQ - Device",
            "Enter the device to sniff for EQ Packets:",
            m_deviceList, current, true, &ok);

    if (ok)
        return selected;
    else
        return QString();
}


void EQInterface::set_net_device()
{

    QString selected = promptForNetDevice();

    if (!selected.isEmpty())
    {
        if (m_packet)
        {
            // start monitoring the device
            m_packet->monitorDevice(selected);
        }

        // set it as the device to monitor next session
        pSEQPrefs->setPrefString("Device", "Network", selected);
    }
}

void EQInterface::set_net_capture_snap_len(int len)
{
  m_packet->setSnapLen(len);
  pSEQPrefs->setPrefInt("CaptureSnapLen", "Network", len);
}

void EQInterface::set_net_capture_buffer_size(int size)
{
  m_packet->setBufferSize(size);
  pSEQPrefs->setPrefInt("CaptureBufferSize", "Network", size);
}

void EQInterface::set_net_arq_giveup(int giveup)
{
  // set the Arq Seq Give Up length
  m_packet->setArqSeqGiveUp(uint16_t(giveup));

  // set it as the value to use next session
  pSEQPrefs->setPrefInt("ArqSeqGiveUp", "Network", m_packet->arqSeqGiveUp());
}

void EQInterface::toggle_net_session_tracking(bool enable)
{
  m_packet->session_tracking(enable);
  m_action_net_sessiontrack->setChecked(enable);
  pSEQPrefs->setPrefBool("SessionTracking", "Network", enable);
}

void EQInterface::toggleAutoDetectPlayerSettings (bool enable)
{
  m_player->setUseAutoDetectedSettings(enable);
}

/* Choose the character's level */
void EQInterface::SetDefaultCharacterLevel(int level)
{
  m_player->setDefaultLevel(level);
}

/* Choose the character's class */
void EQInterface::SetDefaultCharacterClass(QAction* class_)
{
   for (int i = 0; i < PLAYER_CLASSES; i++)
   {
     if (m_action_character_Class[i] == class_)
     {
       m_action_character_Class[i]->setChecked(true);
       m_player->setDefaultClass(m_action_character_Class[i]->data().value<int>());
     }
     else
     {
       m_action_character_Class[i]->setChecked(false);
     }
   }
}

/* Choose the character's race */
void EQInterface::SetDefaultCharacterRace(QAction* race)
{
   for (int i = 0; i < PLAYER_RACES; i++)
   {
     if (m_action_character_Race[i] == race)
     {
       m_action_character_Race[i]->setChecked(true);
       m_player->setDefaultRace(m_action_character_Race[i]->data().value<int>());
     }
     else
     {
       m_action_character_Race[i]->setChecked(false);
     }
   }
}

void EQInterface::toggle_view_menubar()
{
   if (menuBar()->isVisible())
       menuBar()->hide();
   else
       menuBar()->show();
}

void EQInterface::toggle_view_statusbar()
{
   if (statusBar()->isVisible())
       statusBar()->hide();
   else
       statusBar()->show();
   pSEQPrefs->setPrefBool("StatusBarActive", "Interface_StatusBar", statusBar()->isVisible());
}

void EQInterface::init_view_menu()
{
  // need to check for 0 before checking if is visible for dynamicly
  // created windows
  m_action_view_PlayerSkills->setChecked((m_skillList != 0) &&
                m_skillList->isVisible());
  m_action_view_PlayerSkills_Options->setEnabled((m_skillList !=0) &&
                m_skillList->isVisible());

  m_action_view_PlayerStats->setChecked((m_statList != 0) &&
                m_statList->isVisible());
  m_action_view_PlayerStats_Options->setEnabled((m_statList != 0) &&
                m_statList->isVisible());

  m_action_view_SpawnList->setChecked((m_spawnList != 0) &&
                m_spawnList->isVisible());
  m_action_view_SpawnList_Options->setEnabled((m_spawnList !=0) &&
                m_spawnList->isVisible());

  m_action_view_SpawnList2->setChecked((m_spawnList2 != 0) &&
                m_spawnList2->isVisible());

  m_action_view_SpawnPointList->setChecked((m_spawnPointList != 0) &&
                m_spawnPointList->isVisible());

  m_action_view_Compass->setChecked((m_compass != 0) && m_compass->isVisible());

  m_action_view_NetDiag->setChecked((m_netDiag != 0) && m_netDiag->isVisible());

  m_action_view_GuildListWindow->setChecked((m_guildListWindow != 0) &&
                m_guildListWindow->isVisible());

  m_action_view_SpellList->setChecked((m_spellList != 0) &&
                m_spellList->isVisible());

  // loop over the maps
  for (int i = 0; i < maxNumMaps; i++)
      m_action_view_Map[i]->setChecked((m_map[i] != 0) && m_map[i]->isVisible());

  // loop over the message windows
  for (int i = 0; i < maxNumMessageWindows; i++)
      m_action_view_MessageWindow[i]->setChecked((m_messageWindow[i] != 0) &&
              m_messageWindow[i]->isVisible());

  // set the checkmarks for windows that are always created, but not always
  // visible
  m_action_view_ExpWindow->setChecked((m_expWindow != 0) &&
          m_expWindow->isVisible());
  m_action_view_CombatWindow->setChecked((m_combatWindow != 0) &&
          m_combatWindow->isVisible());

   // set initial view options
  if (m_spawnList != 0)
  {
    SEQListView* spawnList = m_spawnList->spawnList();

    // make sure the menu bar settings are correct
    for (int i = 0; i < tSpawnColMaxCols; i++)
    {
      m_action_view_SpawnList_Cols[i]->setChecked(spawnList->columnVisible(i));
    }
  }

  if (m_statList != 0)
  {
    StatList* statList = m_statList->statList();
    // make sure the menu items are checked
    for (int i = 0; i < LIST_MAXLIST; i++)
      m_action_view_PlayerStats_Stats[i]->setChecked(statList->statShown(i));
  }

  if (m_skillList != 0)
  {
    // make sure the proper menu items are checked
    m_action_view_PlayerSkills_Languages->setChecked(
            m_skillList->skillList()->showLanguages());
  }
}


void EQInterface::toggle_opt_save_PlayerState(bool enable)
{
  showeq_params->savePlayerState = enable;
  pSEQPrefs->setPrefBool("PlayerState", "SaveState",
          showeq_params->savePlayerState);
}

void EQInterface::toggle_opt_save_ZoneState(bool enable)
{
  showeq_params->saveZoneState = enable;
  pSEQPrefs->setPrefBool("ZoneState", "SaveState",
          showeq_params->saveZoneState);
}

void EQInterface::toggle_opt_save_Spawns(bool enable)
{
  showeq_params->saveSpawns = enable;
  pSEQPrefs->setPrefBool("Spawns", "SaveState", showeq_params->saveSpawns);

  if (showeq_params->saveSpawns)
    m_spawnShell->saveSpawns();
}

void EQInterface::set_opt_save_SpawnFrequency(int frequency)
{
  showeq_params->saveSpawnsFrequency = frequency * 1000;
  pSEQPrefs->setPrefInt("SpawnsFrequency", "SaveState", 
			showeq_params->saveSpawnsFrequency);
}

void EQInterface::set_opt_save_BaseFilename()
{
  QString fileName =
    QFileDialog::getSaveFileName(this, "Save State Base Filename",
            showeq_params->saveRestoreBaseFilename);

  if (!fileName.isEmpty())
  {
    // set it to be the new base filename
    showeq_params->saveRestoreBaseFilename = fileName;
    
    // set preference to use for next session
    pSEQPrefs->setPrefString("BaseFilename", "SaveState", 
			     showeq_params->saveRestoreBaseFilename);
  }
}

void EQInterface::opt_clearChannelMsgs()
{
  // clear the messages
  m_messages->clear();
}


void EQInterface::showMessageFilterDialog(void)
{
  // create the filter dialog, if necessary
  if (!m_messageFilterDialog)
    m_messageFilterDialog = new MessageFilterDialog(m_messageFilters, 
						    "ShowEQ Message Filters",
						    this, "messagefilterdialog");

  // show the message filter dialog
  m_messageFilterDialog->show();
}

void EQInterface::toggleTypeFilter(QAction* type)
{

  if (type->text() == "&Enable All" || type->text() == "&Disable All")
      return;

  uint64_t enabledTypes = m_terminal->enabledTypes();

  int id = type->data().value<int>();

  if (((uint64_t(1) << id) & enabledTypes) != 0)
    enabledTypes &= ~(uint64_t(1) << id);
  else
    enabledTypes |= (uint64_t(1) << id);

  m_terminal->setEnabledTypes(enabledTypes);

  // (un)check the appropriate menu item
  type->setChecked(((enabledTypes & (uint64_t(1) << id))));
}

void EQInterface::disableAllTypeFilters()
{
  m_terminal->setEnabledTypes(0);

  // uncheck all the menu items
  QString typeName;
  for (int i = MT_Guild; i <= MT_Max; i++)
  {
    typeName = MessageEntry::messageTypeString((MessageType)i);
    if (!typeName.isEmpty() && m_action_term_MessageTypeFilters[i])
      m_action_term_MessageTypeFilters[i]->setChecked(false);
  }
}

void EQInterface::enableAllTypeFilters()
{
  m_terminal->setEnabledTypes(0xFFFFFFFFFFFFFFFFULL);

  // check all the menu items
  QString typeName;
  for (int i = MT_Guild; i <= MT_Max; i++)
  {
    typeName = MessageEntry::messageTypeString((MessageType)i);
    if (!typeName.isEmpty())
      m_action_term_MessageTypeFilters[i]->setChecked(true);
  }
}

void EQInterface::toggleShowUserFilter(QAction* filter)
{

  if (filter->text() == "&Enable All" || filter->text() == "&Disable All")
      return;

  uint32_t enabledShowUserFilters = m_terminal->enabledShowUserFilters();

  int id = filter->data().value<int>();

  // toggle whether the filter is enabled/disabled
  if (((1 << id) & enabledShowUserFilters) != 0)
    enabledShowUserFilters &= ~(1 << id);
  else
    enabledShowUserFilters |= (1 << id);

  m_terminal->setEnabledShowUserFilters(enabledShowUserFilters);

  // (un)check the appropriate menu item
  filter->setChecked(((enabledShowUserFilters & (1 << id)) != 0));
}

void EQInterface::disableAllShowUserFilters()
{
  // set and save all filters disabled setting
  m_terminal->setEnabledShowUserFilters(0);

  // uncheck all the menu items
  QString typeName;
  for (int i = 0; i < maxMessageFilters; i++)
  {
    if (m_messageFilters->filter(i))
      m_action_term_ShowUserFilters[i]->setChecked(false);
  }
}

void EQInterface::enableAllShowUserFilters()
{
  // set and save all filters enabled flag
  m_terminal->setEnabledShowUserFilters(0xFFFFFFFF);

  // check all the menu items
  QString typeName;
  for (int i = 0; i < maxMessageFilters; i++)
  {
    if (m_messageFilters->filter(i))
      m_action_term_ShowUserFilters[i]->setChecked(true);
  }
}

void EQInterface::toggleHideUserFilter(QAction* filter)
{
  if (filter->text() == "&Enable All" || filter->text() == "&Disable All")
      return;

  uint32_t enabledHideUserFilters = m_terminal->enabledHideUserFilters();

  int id = filter->data().value<int>();

  // toggle whether the filter is enabled/disabled
  if (((1 << id) & enabledHideUserFilters) != 0)
    enabledHideUserFilters &= ~(1 << id);
  else
    enabledHideUserFilters |= (1 << id);

  m_terminal->setEnabledHideUserFilters(enabledHideUserFilters);

  // (un)check the appropriate menu item
  filter->setChecked(((enabledHideUserFilters & (1 << id)) != 0));
}

void EQInterface::disableAllHideUserFilters()
{
  // set and save all filters disabled setting
  m_terminal->setEnabledHideUserFilters(0);

  // uncheck all the menu items
  QString typeName;
  for (int i = 0; i < maxMessageFilters; i++)
  {
    if (m_messageFilters->filter(i))
      m_action_term_HideUserFilters[i]->setChecked(false);
  }
}

void EQInterface::enableAllHideUserFilters()
{
  // set and save all filters enabled flag
  m_terminal->setEnabledHideUserFilters(0xFFFFFFFF);

  // check all the menu items
  QString typeName;
  for (int i = 0; i < maxMessageFilters; i++)
  {
    if (m_messageFilters->filter(i))
      m_action_term_HideUserFilters[i]->setChecked(true);
  }
}

void EQInterface::addUserFilterMenuEntry(uint32_t mask, uint8_t filterid,
        const MessageFilter& filter)
{
  //We shouldn't inherit the state of the filter that occupied this number,
  //so we'll start with it disabled/off.
  m_action_term_ShowUserFilters[filterid] =
      m_terminalShowUserFilterMenu->addAction(filter.name());
  m_action_term_ShowUserFilters[filterid]->setCheckable(true);
  m_action_term_ShowUserFilters[filterid]->setData(filterid);
  m_action_term_ShowUserFilters[filterid]->setChecked(false);

  uint32_t enabledShowUserFilters = m_terminal->enabledShowUserFilters();
  m_terminal->setEnabledShowUserFilters(enabledShowUserFilters & ~mask);

  m_action_term_HideUserFilters[filterid] =
      m_terminalHideUserFilterMenu->addAction(filter.name());
  m_action_term_HideUserFilters[filterid]->setCheckable(true);
  m_action_term_HideUserFilters[filterid]->setData(filterid);
  m_action_term_HideUserFilters[filterid]->setChecked(false);

  uint32_t enabledHideUserFilters = m_terminal->enabledHideUserFilters();
  m_terminal->setEnabledHideUserFilters(enabledHideUserFilters & ~mask);
}

void EQInterface::removeUserFilterMenuEntry(uint32_t mask, uint8_t filterid)
{
  QAction* tmpAction = m_action_term_ShowUserFilters[filterid];
  m_terminalShowUserFilterMenu->removeAction(tmpAction);
  m_action_term_ShowUserFilters[filterid] = 0;
  delete tmpAction;

  uint32_t enabledShowUserFilters = m_terminal->enabledShowUserFilters();
  m_terminal->setEnabledHideUserFilters(enabledShowUserFilters & ~mask);

  tmpAction = m_action_term_HideUserFilters[filterid];
  m_terminalHideUserFilterMenu->removeAction(tmpAction);
  m_action_term_HideUserFilters[filterid] = 0;
  delete tmpAction;

  uint32_t enabledHideUserFilters = m_terminal->enabledHideUserFilters();
  m_terminal->setEnabledHideUserFilters(enabledHideUserFilters & ~mask);
}
void EQInterface::toggleDisplayType(bool enable)
{
  // toggle the display of message types
  m_terminal->setDisplayType(enable);
}

void EQInterface::toggleDisplayTime(bool enable)
{
  // toggle the display of message time
  m_terminal->setDisplayDateTime(enable);
}

void EQInterface::toggleEQDisplayTime(bool enable)
{
  m_terminal->setDisplayEQDateTime(enable);
}

void EQInterface::toggleUseColor(bool enable)
{
  m_terminal->setUseColor(enable);
}

QString EQInterface::setTheme(QString name)
{
    static QFont OrigFont = qApp->font();
    static QPalette OrigPalette = qApp->palette();;

    QString currentStyleName = qApp->style()->objectName();

    QStringList availableStyles = QStyleFactory::keys();

    if (!availableStyles.contains(name, Qt::CaseInsensitive))
        return currentStyleName;

    qApp->setPalette(OrigPalette);
    QStyle* newStyle = QStyleFactory::create(name);
    qApp->setStyle(newStyle);


    MenuActionList::Iterator iter;

    for ( iter = ActionList_StyleMenu.begin(); iter != ActionList_StyleMenu.end(); ++iter)
    {
      if ((*iter)->data().value<QString>().toLower() == name.toLower())
      {
        (*iter)->setChecked(true);
        currentStyleName = (*iter)->data().value<QString>();
      }
      else
      {
        (*iter)->setChecked(false);
      }
    }

    emit styleChanged();

    return currentStyleName;
}

int EQInterface::setTheme(int id)
{

    int current_theme = 2;

    MenuActionList::Iterator iter;

    for ( iter = ActionList_StyleMenu.begin(); iter != ActionList_StyleMenu.end(); ++iter)
    {
      if ((*iter)->isChecked())
        current_theme = (*iter)->data().value<int>();
    }

    QString new_theme;

    switch ( id )
    {
      case 1: // plastique
        new_theme = "plastique";
        break;
      case 2: // windows
        new_theme = "windows";
        break;
      case 3: // cde
      case 4: // cde polished
        new_theme = "cde";
        break;
      case 5: // motif
        new_theme = "motif";
        break;
      case 6: // cleanlooks
        new_theme = "cleanlooks";
        break;
      default: // system default
        return current_theme;
        break;
    }

    QString set_theme = setTheme(new_theme);

    emit restoreFonts();

    if (new_theme.toLower() == set_theme.toLower())
        return id;
    else
        return current_theme;
}

void EQInterface::selectTheme(QAction* selection)
{
  QString theme = setTheme(selection->data().value<QString>());
  pSEQPrefs->setPrefString("ThemeName", "Interface", theme);
}

void EQInterface::showMap(int i)
{
  if ((i > maxNumMaps) || (i < 0))
    return;

  // if it doesn't exist, create it
  if (m_map[i] == 0)
  {
    int mapNum = i + 1;
    QString mapPrefName = "Map";
    QString mapName = QString("map") + QString::number(mapNum);
    QString mapCaption = "Map ";

    if (i != 0)
    {
      mapPrefName += QString::number(mapNum);
      mapCaption += QString::number(mapNum);
    }

    m_map[i] = new MapFrame(m_filterMgr,
            m_mapMgr,
            m_player,
            m_spawnShell,
            m_zoneMgr,
            m_spawnMonitor,
            mapPrefName,
            mapCaption,
            mapName.toLatin1().data(),
            0);


   setDockEnabled(m_map[i],
           pSEQPrefs->getPrefBool(QString("Dockable") + mapPrefName,
               "Interface", true));

    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock",
            m_map[i]->preferenceName(), Qt::RightDockWidgetArea);
    addDockWidget(edge, m_map[i]);
    if (!m_isMapDocked[i])
        m_map[i]->setFloating(!m_isMapDocked[i]);

    connect(this, SIGNAL(saveAllPrefs(void)), m_map[i], SLOT(savePrefs()));
    connect(this, SIGNAL(restoreFonts(void)), m_map[i], SLOT(restoreFont(void)));

    // Get the map...
    Map* map = m_map[i]->map();

    // supply the Map slots with signals from EQInterface
    connect (this, SIGNAL(selectSpawn(const Item*)),
            map, SLOT(selectSpawn(const Item*)));

    // supply EQInterface slots with signals from Map
    connect (map, SIGNAL(spawnSelected(const Item*)),
            this, SLOT(spawnSelected(const Item*)));

    // insert its menu into the window menu
    insertWindowMenu(m_map[i]);
  }

  // make sure it's visible
  m_map[i]->show();
  m_map[i]->activateWindow();
}

void EQInterface::showMessageWindow(int i)
{
  if ((i > maxNumMessageWindows) || (i < 0))
    return;

  // if it doesn't exist, create it
  if (m_messageWindow[i] == 0)
  {
    int winNum = i + 1;
    QString prefName = "MessageWindow" + QString::number(winNum);
    QString name = QString("messageWindow") + QString::number(winNum);
    QString caption = "Channel Messages ";

    if (i != 0)
      caption += QString::number(winNum);

    m_messageWindow[i] = new MessageWindow(m_messages, m_messageFilters,
            prefName, caption,
            0, name.toLatin1().data());

   setDockEnabled(m_messageWindow[i], 
		  pSEQPrefs->getPrefBool(QString("Dockable") + prefName,
					 "Interface", false));
    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_messageWindow[i]->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_messageWindow[i]);
    m_messageWindow[i]->setFloating(!m_isMessageWindowDocked[i]);

    connect(this, SIGNAL(saveAllPrefs(void)),
	    m_messageWindow[i], SLOT(savePrefs(void)));
    connect(this, SIGNAL(restoreFonts(void)),
	    m_messageWindow[i], SLOT(restoreFont(void)));

    // insert its menu into the window menu
    insertWindowMenu(m_messageWindow[i]);
  }

  // make sure it's visible
  m_messageWindow[i]->show();
  m_messageWindow[i]->activateWindow();

}

void EQInterface::showSpawnList(void)
{
  // if it doesn't exist, create it.
  if (m_spawnList == 0)
  {
    m_spawnList = new SpawnListWindow (m_player, m_spawnShell, m_categoryMgr,
				       0, "spawnlist");
    setDockEnabled(m_spawnList, 
		   pSEQPrefs->getPrefBool("DockableSpawnList",
					  "Interface", true));

    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_spawnList->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_spawnList);
    m_spawnList->setFloating(!m_isSpawnListDocked);

     // connections from spawn list to interface
     connect (m_spawnList->spawnList(), SIGNAL(spawnSelected(const Item*)),
	      this, SLOT(spawnSelected(const Item*)));

     // connections from interface to spawn list
     connect (this, SIGNAL(selectSpawn(const Item*)),
	      m_spawnList->spawnList(), SLOT(selectSpawn(const Item*)));
     connect(this, SIGNAL(saveAllPrefs(void)),
	     m_spawnList, SLOT(savePrefs(void)));
     connect(this, SIGNAL(restoreFonts(void)),
	     m_spawnList, SLOT(restoreFont(void)));

     connect(this, SIGNAL(styleChanged()), m_spawnList, SLOT(styleChanged()));

    // insert its menu into the window menu
    insertWindowMenu(m_spawnList);
  }

  // make sure it's visible
  m_spawnList->show();
  m_spawnList->activateWindow();
}

void EQInterface::showSpawnList2(void)
{
  // if it doesn't exist, create it.
  if (m_spawnList2 == 0)
  {
    m_spawnList2 = new SpawnListWindow2(m_player, m_spawnShell, 
					m_categoryMgr,
					0, "spawnlist2");
   setDockEnabled(m_spawnList2, 
		  pSEQPrefs->getPrefBool("DockableSpawnList2",
					 "Interface", true));
    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_spawnList2->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_spawnList2);
    m_spawnList2->setFloating(!m_isSpawnList2Docked);

     // connections from spawn list to interface
     connect (m_spawnList2, SIGNAL(spawnSelected(const Item*)),
	      this, SLOT(spawnSelected(const Item*)));

     // connections from interface to spawn list
     connect (this, SIGNAL(selectSpawn(const Item*)),
	      m_spawnList2, SLOT(selectSpawn(const Item*)));
     connect(this, SIGNAL(saveAllPrefs(void)),
	     m_spawnList2, SLOT(savePrefs(void)));
     connect(this, SIGNAL(restoreFonts(void)),
	     m_spawnList2, SLOT(restoreFont(void)));

     connect(this, SIGNAL(styleChanged()), m_spawnList2, SLOT(styleChanged()));

    // insert its menu into the window menu
    insertWindowMenu(m_spawnList2);
  }

  // make sure it's visible
  m_spawnList2->show();
  m_spawnList2->activateWindow();
}

void EQInterface::showSpawnPointList(void)
{
  // if it doesn't exist, create it.
  if (m_spawnPointList == 0)
  {
      m_spawnPointList = new SpawnPointWindow(m_spawnMonitor, 0, "spawnlist");
      setDockEnabled(m_spawnPointList,
              pSEQPrefs->getPrefBool("DockableSpawnPointList", "Interface", true));
      Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock",
              m_spawnPointList->preferenceName(), Qt::LeftDockWidgetArea);
      addDockWidget(edge, m_spawnPointList);
      m_spawnPointList->setFloating(!m_isSpawnPointListDocked);

      // connections from interface to spawn list
      connect(this, SIGNAL(saveAllPrefs(void)),
              m_spawnPointList, SLOT(savePrefs(void)));
      connect(this, SIGNAL(restoreFonts(void)),
              m_spawnPointList, SLOT(restoreFont(void)));

      // insert its menu into the window menu
      insertWindowMenu(m_spawnPointList);
  }

  // make sure it's visible
  m_spawnPointList->show();
  m_spawnPointList->activateWindow();
}

void EQInterface::showStatList(void)
{
  // if it doesn't exist, create it
  if (m_statList == 0)
  {
    m_statList = new StatListWindow(m_player, 0, "stats");
    setDockEnabled(m_statList, pSEQPrefs->getPrefBool("DockablePlayerStats",
                "Interface", true));
    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_statList->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_statList);
    m_statList->setFloating(!m_isStatListDocked);

    // connect stat list slots to interface signals
    connect(this, SIGNAL(saveAllPrefs(void)), m_statList, SLOT(savePrefs(void)));
    connect(this, SIGNAL(restoreFonts(void)), m_statList, SLOT(restoreFont(void)));

    // insert its menu into the window menu
    insertWindowMenu(m_statList);
  }

  // make sure it's visible
  m_statList->show();
  m_statList->activateWindow();
}

void EQInterface::showSkillList(void)
{
  // if it doesn't exist, create it
  if (m_skillList == 0)
  {
    m_skillList = new SkillListWindow(m_player, 0, "skills");
    setDockEnabled(m_skillList,
            pSEQPrefs->getPrefBool("DockablePlayerSkills", "Interface", true));
    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_skillList->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_skillList);
    m_skillList->setFloating(!m_isSkillListDocked);

    // connect skill list slots to interfaces signals
    connect(this, SIGNAL(saveAllPrefs(void)), m_skillList, SLOT(savePrefs(void)));
    connect(this, SIGNAL(restoreFonts(void)), m_skillList, SLOT(restoreFont(void)));

    // insert its menu into the window menu
    insertWindowMenu(m_skillList);
  }

  // make sure it's visible
  m_skillList->show();
  m_skillList->activateWindow();
}

void EQInterface::showSpellList(void)
{
  // if it doesn't exist, create it
  if (m_spellList == 0)
  {
    m_spellList = new SpellListWindow(m_spellShell, this, "spelllist");
    setDockEnabled(m_spellList,
            pSEQPrefs->getPrefBool("DockableSpellList", "Interface", true));
    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_spellList->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_spellList);
    m_spellList->setFloating(!m_isSpellListDocked);

    SpellList* spellList = m_spellList->spellList();

    // connect SpellShell to SpellList
    connect(m_spellShell, SIGNAL(addSpell(const SpellItem *)),
	    spellList, SLOT(addSpell(const SpellItem *)));
    connect(m_spellShell, SIGNAL(delSpell(const SpellItem *)),
	    spellList, SLOT(delSpell(const SpellItem *)));
    connect(m_spellShell, SIGNAL(changeSpell(const SpellItem *)),
	    spellList, SLOT(changeSpell(const SpellItem *)));
    connect(m_spellShell, SIGNAL(clearSpells()),
	    spellList, SLOT(clear()));
    connect(this, SIGNAL(saveAllPrefs(void)),
	    m_spellList, SLOT(savePrefs(void)));
    connect(this, SIGNAL(restoreFonts(void)),
	    m_spellList, SLOT(restoreFont(void)));

    // insert its menu into the window menu
    insertWindowMenu(m_spellList);
  }

  // make sure it's visible
  m_spellList->show();
  m_spellList->activateWindow();
}

void EQInterface::showCompass(void)
{
  // if it doesn't exist, create it.
  if (m_compass == 0)
  {
    m_compass = new CompassFrame(m_player, 0, "compass");
    setDockEnabled(m_compass,
            pSEQPrefs->getPrefBool("DockableCompass", "Interface", true));
    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_compass->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_compass);
    m_compass->setFloating(!m_isCompassDocked);

    // supply the compass slots with EQInterface signals
    connect (this, SIGNAL(selectSpawn(const Item*)),
	     m_compass, SLOT(selectSpawn(const Item*)));
    connect(this, SIGNAL(restoreFonts(void)),
	    m_compass, SLOT(restoreFont(void)));
    connect(this, SIGNAL(saveAllPrefs(void)),
	    m_compass, SLOT(savePrefs(void)));

    // insert its menu into the window menu
    insertWindowMenu(m_compass);
 }

  // make sure it's visible
  m_compass->show();
  m_compass->activateWindow();
}

void EQInterface::showNetDiag()
{
  if (m_netDiag == 0)
  {
    m_netDiag = new NetDiag(m_packet, 0, "NetDiag");
    setDockEnabled(m_netDiag,
            pSEQPrefs->getPrefBool("DockableNetDiag", "Interface", true));
    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_netDiag->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_netDiag);
    m_netDiag->setFloating(true);

    connect(this, SIGNAL(restoreFonts(void)), m_netDiag, SLOT(restoreFont(void)));
    connect(this, SIGNAL(saveAllPrefs(void)), m_netDiag, SLOT(savePrefs(void)));

    // insert its menu into the window menu
    insertWindowMenu(m_netDiag);
  }

  // make sure it's visible
  m_netDiag->show();
  m_netDiag->activateWindow();
}

void EQInterface::showGuildList(void)
{
  if (!m_guildListWindow)
  {
    m_guildListWindow = new GuildListWindow(m_player, m_guildShell, 0, "GuildList");
    setDockEnabled(m_guildListWindow,
            pSEQPrefs->getPrefBool("DockableGuildListWindow", "Interface", true));
    Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", 
            m_guildListWindow->preferenceName(), Qt::LeftDockWidgetArea);
    addDockWidget(edge, m_guildListWindow);
    m_guildListWindow->setFloating(true);

    connect(this, SIGNAL(restoreFonts(void)), m_guildListWindow, SLOT(restoreFont(void)));
    connect(this, SIGNAL(saveAllPrefs(void)), m_guildListWindow, SLOT(savePrefs(void)));

    // insert its menu into the window menu
    insertWindowMenu(m_guildListWindow);
  }

  // make sure it's visible
  m_guildListWindow->show();
  m_guildListWindow->activateWindow();
}

void EQInterface::createFilteredSpawnLog(void)
{
  if (m_filteredSpawnLog)
    return;

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs",
							   "filtered_spawns.log");
  
  m_filteredSpawnLog = new FilteredSpawnLog(m_dateTimeMgr, m_filterMgr,
					    logFileInfo.absoluteFilePath());

  connect(m_spawnShell, SIGNAL(addItem(const Item*)),
	  m_filteredSpawnLog, SLOT(addItem(const Item*)));
  connect(m_spawnShell, SIGNAL(delItem(const Item*)),
	  m_filteredSpawnLog, SLOT(delItem(const Item*)));
  connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
	  m_filteredSpawnLog, SLOT(killSpawn(const Item*)));
}

void EQInterface::createSpawnLog(void)
{
  // if the spawnLogger already exists, then nothing to do... 
  if (m_spawnLogger)
    return;

  QString logFile = pSEQPrefs->getPrefString("SpawnLogFilename",
					     "Misc", 
					     "spawnlog.txt");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
  
  logFile = logFileInfo.absoluteFilePath();

   // create the spawn logger
   m_spawnLogger = new SpawnLog(m_dateTimeMgr, logFile);

   // initialize it with the current state
   QString shortZoneName = m_zoneMgr->shortZoneName();
   if (!shortZoneName.isEmpty())
     m_spawnLogger->logNewZone(shortZoneName);

   // Connect SpawnLog slots to ZoneMgr signals
   connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)),
	   m_spawnLogger, SLOT(logNewZone(const QString&)));

   // Connect SpawnLog slots to EQPacket signals
#if 0 // No longer used as of 5-22-2008
   m_packet->connect2("OP_ZoneSpawns", SP_Zone, DIR_Server,
		      "spawnStruct", SZC_Modulus,
		      m_spawnLogger, SLOT(logZoneSpawns(const uint8_t*, size_t)));
#endif
// OP_NewSpawn is deprecated in the client
//    m_packet->connect2("OP_NewSpawn", SP_Zone, DIR_Server,
// 		      "spawnStruct", SZC_Match,
// 		      m_spawnLogger, SLOT(logNewSpawn(const uint8_t*)));
   
   // Connect SpawnLog slots to SpawnShell signals
   connect(m_spawnShell, SIGNAL(addItem(const Item*)),
           m_spawnLogger, SLOT(logNewSpawn(const Item *)));
   connect(m_spawnShell, SIGNAL(delItem(const Item*)),
	   m_spawnLogger, SLOT(logDeleteSpawn(const Item *)));
   connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
	   m_spawnLogger, SLOT(logKilledSpawn(const Item *, const Item*, uint16_t)));
}

void EQInterface::createGlobalLog(void)
{
  if (m_globalLog)
    return;

  QString logFile = pSEQPrefs->getPrefString("GlobalLogFilename",
					     "PacketLogging",
					     "global.log");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
  
  m_globalLog = new PacketLog(*m_packet, 
			      logFileInfo.absoluteFilePath(),
			      this, "GlobalLog");

  connect(m_packet, SIGNAL(newPacket(const EQUDPIPPacketFormat&)),
	  m_globalLog, SLOT(logData(const EQUDPIPPacketFormat&)));
}

void EQInterface::createWorldLog(void)
{
  if (m_worldLog)
    return;

  QString logFile = pSEQPrefs->getPrefString("WorldLogFilename",
					     "PacketLogging",
					     "world.log");
  
  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
  
  m_worldLog = new PacketStreamLog(*m_packet, 
				   logFileInfo.absoluteFilePath(),
				   this, "WorldLog");

  m_worldLog->setRaw(pSEQPrefs->getPrefBool("LogRawPackets", "PacketLogging",
					   false));

  connect(m_packet, SIGNAL(rawWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t)),
	  m_worldLog, SLOT(rawStreamPacket(const uint8_t*, size_t, uint8_t, uint16_t)));
  connect(m_packet, SIGNAL(decodedWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)),
	  m_worldLog, SLOT(decodedStreamPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)));
}

void EQInterface::createZoneLog(void)
{
  if (m_zoneLog)
    return;
  
  QString logFile = pSEQPrefs->getPrefString("ZoneLogFilename",
					     "PacketLogging",
					     "zone.log");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
  
  m_zoneLog = new PacketStreamLog(*m_packet, 
				  logFileInfo.absoluteFilePath(),
				  this, "ZoneLog");

  m_zoneLog->setRaw(pSEQPrefs->getPrefBool("LogRawPackets", "PacketLogging",
					   false));
  
  m_zoneLog->setDir(0);

  connect(m_packet, SIGNAL(rawZonePacket(const uint8_t*, size_t, uint8_t, uint16_t)),
	  m_zoneLog, SLOT(rawStreamPacket(const uint8_t*, size_t, uint8_t, uint16_t)));
  connect(m_packet, SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)),
	  m_zoneLog, SLOT(decodedStreamPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)));
}

void EQInterface::createBazaarLog(void)
{
  if (m_bazaarLog)
    return;
  
  QString logFile = pSEQPrefs->getPrefString("BazaarLogFilename",
					     "PacketLogging",
					     "bazaar.log");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
  
  m_bazaarLog = new BazaarLog(*m_packet,
			      logFileInfo.absoluteFilePath(),
			      this,
			      *m_spawnShell,
			      "BazaarLog");
  m_packet->connect2("OP_BazaarSearch", SP_Zone, DIR_Server,
		     "bazaarSearchResponseStruct", SZC_Modulus,
		     m_bazaarLog, SLOT(bazaarSearch(const uint8_t*, size_t, uint8_t)));
}

void EQInterface::createUnknownZoneLog(void)
{
  if (m_unknownZoneLog)
    return;

  QString section = "PacketLogging";

  QString logFile = pSEQPrefs->getPrefString("UnknownZoneLogFilename",
					     section,
					     "unknownzone.log");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
  
  logFile = logFileInfo.absoluteFilePath();

  m_unknownZoneLog = new UnknownPacketLog(*m_packet, 
					  logFile,
					  this, "UnknownLog");

  m_unknownZoneLog->setView(pSEQPrefs->getPrefBool("ViewUnknown", section, 
						   false));

  connect(m_packet, SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
	  m_unknownZoneLog, SLOT(packet(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
  connect(m_packet, SIGNAL(decodedWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
	  m_unknownZoneLog, SLOT(packet(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
}

void EQInterface::createOPCodeMonitorLog(const QString& opCodeList)
{
  if (m_opcodeMonitorLog)
    return;
  
  QString section = "OpCodeMonitoring";

  QString logFile = pSEQPrefs->getPrefString("LogFilename", 
					     section, 
					     "opcodemonitor.log");

  QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
  
  logFile = logFileInfo.absoluteFilePath();

  m_opcodeMonitorLog = new OPCodeMonitorPacketLog(*m_packet, 
						  logFile,
						  this, "OpCodeMonitorLog");
  
  m_opcodeMonitorLog->init(opCodeList);
  m_opcodeMonitorLog->setLog(pSEQPrefs->getPrefBool("Log", section, false));
  m_opcodeMonitorLog->setView(pSEQPrefs->getPrefBool("View", section, false));
  
  connect(m_packet, SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
	  m_opcodeMonitorLog, SLOT(packet(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
}


void EQInterface::insertWindowMenu(SEQWindow* window)
{
  QMenu* menu = window->menu();
  if (menu)
  {
    menu->setTitle(window->windowTitle());
    // insert the windows menu into the window menu
    QAction* menuAction = m_windowMenu->addMenu(menu);

    // insert it into the window to window menu id dictionary
    m_windowsMenus.insert((void*)window, menuAction);
  }
}

void EQInterface::removeWindowMenu(SEQWindow* window)
{
  // find the windows menu id
  QAction* menuAction = m_windowsMenus.value((void*)window, nullptr);

  // if the window had a menu, then remove it
  if (menuAction)
  {
    m_windowMenu->removeAction(menuAction);

    // remove the item from the list
    delete m_windowsMenus.take(window);
  }
}

void EQInterface::setDockEnabled(QDockWidget* dw, bool enable)
{
  if (enable)
      dw->setAllowedAreas(Qt::AllDockWidgetAreas);
  else
      dw->setAllowedAreas(Qt::NoDockWidgetArea);
}

void EQInterface::contextMenuEvent(QContextMenuEvent* event)
{
    event->ignore();
}

#ifndef QMAKEBUILD
#include "interface.moc"
#endif

