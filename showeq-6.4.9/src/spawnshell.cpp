/*
 *  spawnshell.cpp
 *  Copyright 2001 Crazy Joe Divola (cjd1@users.sourceforge.net)
 *  Portions Copyright 2001-2003,2007 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2001-2019 by the respective ShowEQ Developers
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

/*
 * Adapted from spawnlist.cpp - Crazy Joe Divola (cjd1@users.sourceforge.net)
 * Date   - 7/31/2001
 */

#include "spawnshell.h"
#include "filtermgr.h"
#include "zonemgr.h"
#include "player.h"
#include "util.h"
#include "guild.h"
#include "packetcommon.h"
#include "diagnosticmessages.h"
#include "netstream.h"

#include <QFile>
#include <QDataStream>
#include <QTextStream>

#ifdef __FreeBSD__
#include <sys/types.h>
#endif
#include <climits>
#include <cmath>


#pragma message("Once our minimum supported Qt version is greater than 5.14, this check can be removed and ENDL replaced with Qt::endl")
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
#define ENDL Qt::endl
#else
#define ENDL endl
#endif

//----------------------------------------------------------------------
// useful macro definitions

// define this to have the spawnshell print diagnostics
//#define SPAWNSHELL_DIAG

// define this to diagnose structures passed in to SpawnShell
//#define SPAWNSHELL_DIAG_STRUCTS

// define this to have the spawnshell validate names to help spot errors
//#define SPAWNSHELL_NAME_VALIDATE

//----------------------------------------------------------------------
// constants
static const char magicStr[5] = "spn4"; // magic is the size of uint32_t + a null
static const uint32_t* magic = (uint32_t*)magicStr;
static const char * Spawn_Corpse_Designator = "'s corpse";

//----------------------------------------------------------------------
// Handy utility function
#ifdef SPAWNSHELL_NAME_VALIDATE
static bool isValidName(const char* name, size_t len)
{
  int i = 0;

  // loop over the string until the maximum length is reached
  while (i < len)
  {
    // if the terminating NULL has been found, we're done
    if ( name[i] == 0 )
      break;

    // if the current character is outside the normal range, fail the name
    if ( (name[i] < ' ') || (name[i] > '~') ) 
      return false;

    // keep going until done
    i++;
  }

  // if we finished with i being the buffer length, fail the name
  // because it's not NULL terminated
  if (i == len)
    return false;

  // it's a real name, return success
  return true;
}
#endif

//----------------------------------------------------------------------
// SpawnShell
SpawnShell::SpawnShell(FilterMgr& filterMgr, 
		       ZoneMgr* zoneMgr, 
		       Player* player,
                       GuildMgr* guildMgr)
  : QObject(NULL),
    m_zoneMgr(zoneMgr),
    m_player(player),
    m_filterMgr(filterMgr),
    m_guildMgr(guildMgr),
    m_spawns(),
    m_drops(),
    m_doors(),
    m_players()
{
   setObjectName("spawnshell");
   m_cntDeadSpawnIDs = 0;
   m_posDeadSpawnIDs = 0;
   for (int i = 0; i < MAX_DEAD_SPAWNIDS; i++)
     m_deadSpawnID[i] = 0;

   // bogus list
   m_players.insert(0, m_player);

   // connect the FilterMgr's signals to SpawnShells slots
   connect(&m_filterMgr, SIGNAL(filtersChanged()),
	   this, SLOT(refilterSpawns()));
   connect(&m_filterMgr, SIGNAL(runtimeFiltersChanged(uint8_t)),
	   this, SLOT(refilterSpawnsRuntime()));

   // connect SpawnShell slots to ZoneMgr signals
   connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)),
	   this, SLOT(clear(void)));
   connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)),
	   this, SLOT(clear(void)));

   // connect Player signals to SpawnShell signals
   connect(m_player, SIGNAL(changeItem(const Item*, uint32_t)),
	   this, SIGNAL(changeItem(const Item*, uint32_t)));
   connect(m_player, SIGNAL(playerUpdate(const uint8_t*, size_t, uint8_t)),
           this, SLOT(playerUpdate2(const uint8_t*, size_t, uint8_t)));

   // connect Player signals to SpawnShell slots
   connect(m_player, SIGNAL(changedID(uint16_t, uint16_t)),
	   this, SLOT(playerChangedID(uint16_t, uint16_t)));

   // connect to guildmgr to receive notifications of guild tag updates
   connect(m_guildMgr, SIGNAL(guildTagUpdated(uint32_t)),
           this, SLOT(updateGuildTag(uint32_t)));

   // restore the spawn list if necessary
   if (showeq_params->restoreSpawns)
     restoreSpawns();

   // create the timer
   m_timer = new QTimer(this);

   // connect the timer
   connect(m_timer, SIGNAL(timeout()),
	   this, SLOT(saveSpawns(void)));

   // start the timer (changed to oneshot to help prevent a backlog on slower
   // machines)
   if (showeq_params->saveSpawns)
   {
     m_timer->setSingleShot(true);
     m_timer->start(showeq_params->saveSpawnsFrequency);
   }
}

SpawnShell::~SpawnShell()
{
    clear();
}

void SpawnShell::clear(void)
{
#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::clear()");
#endif

   emit clearItems();

   qDeleteAll(m_spawns);
   m_spawns.clear();

   qDeleteAll(m_doors);
   m_doors.clear();

   qDeleteAll(m_drops);
   m_drops.clear();

   // clear the players list, reinsert the player
   m_players.clear();
   m_players.insert(0, m_player);

   // emit an changeItem for the player
   emit changeItem(m_player, tSpawnChangedALL);

   m_cntDeadSpawnIDs = 0;
   m_posDeadSpawnIDs = 0;
   for (int i = 0; i < MAX_DEAD_SPAWNIDS; i++)
     m_deadSpawnID[i] = 0;
} // end clear

const Item* SpawnShell::findID(spawnItemType type, int id)
{
  const Item* item = NULL;
  
  if ((type == tSpawn) && (id == m_player->id()))
    return (const Item*)m_player;

  if (type != tPlayer)
    item = getMap(type).value(id, nullptr);

  return item;
}

const Item* SpawnShell::findClosestItem(spawnItemType type, 
					int16_t x, int16_t y,
					double& minDistance)
{
   ItemMap& theMap = getMap(type);
   ItemIterator it(theMap);
   double distance;
   Item* item;
   Item* closest = NULL;

   // find closest spawn

   // iterate over all the items in the map
   while (it.hasNext())
   {
     it.next();

     // get the item
     item = it.value();
     if (!item)
         break;

     // calculate the distance from the specified point
     distance = item->calcDist(x, y);

     // is this distance closer?
     if (distance < minDistance)
     {
       // yes, note it
       minDistance = distance;
       closest = item;
     }
   }

   // return the closest item.
   return closest;
}

void SpawnShell::updateGuildTag(uint32_t guildId)
{
    ItemIterator it(m_spawns);
    Spawn* spawn;

    while (it.hasNext())
    {
        it.next();

        spawn = (Spawn*)it.value();
        if (!spawn)
            break;

        if (guildId == spawn->guildID())
        {
            spawn->setGuildTag(m_guildMgr->guildIdToName(spawn->guildID(), spawn->guildServerID()));
            spawn->updateLastChanged();
            emit changeItem(spawn, tSpawnChangedALL);
        }
    }

    ItemIterator pl(m_players);

    while (pl.hasNext())
    {
        pl.next();

        spawn = (Spawn*)pl.value();
        if (!spawn)
            break;

        if (guildId == spawn->guildID())
        {
            spawn->setGuildTag(m_guildMgr->guildIdToName(spawn->guildID(), spawn->guildServerID()));
            spawn->updateLastChanged();
            emit changeItem(spawn, tSpawnChangedALL);
        }
    }
}


Spawn* SpawnShell::findSpawnByName(const QString& name)
{
  ItemIterator it(m_spawns);
  Spawn* spawn;

  while (it.hasNext())
  {
    it.next();

    // the item and coerce it to the Spawn type
    spawn = (Spawn*)it.value();
    if (!spawn)
        break;

    if (name == spawn->name())
      return spawn;
  }

  if (name == m_player->name())
    return m_player;

  return NULL;
}

void SpawnShell::deleteItem(spawnItemType type, int id)
{
#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::deleteItem()");
#endif
   ItemMap& theMap = getMap(type);

   Item* item = theMap.value(id, nullptr);

   if (item != NULL)
   {
     emit delItem(item);
     theMap.remove(id);

     // send notifcation of new spawn count
     emit numSpawns(m_spawns.count());
   }
}

bool SpawnShell::updateFilterFlags(Item* item)
{
  uint8_t level = 0;

  if (item->type() == tSpawn)
    level = ((Spawn*)item)->level();

  // get the filter flags
  uint32_t flags = m_filterMgr.filterMask(item->filterString(), level);

  // see if the new filter flags are different from the old ones
  if (flags != item->filterFlags())
  {
    // yes, set the new filter flags
    item->setFilterFlags(flags);

    // return true to indicate that the flags have changed
    return true;
  }

  // flags haven't changed
  return false;
}

bool SpawnShell::updateRuntimeFilterFlags(Item* item)
{
  uint8_t level = 0;

  if (item->type() == tSpawn)
    level = ((Spawn*)item)->level();

  // get the filter flags
  uint32_t flags = m_filterMgr.runtimeFilterMask(item->filterString(), level);

  // see if the new filter flags are different from the old ones
  if (flags != item->runtimeFilterFlags())
  {
    // yes, set the new filter flags
    item->setRuntimeFilterFlags(flags);

    // return true to indicate that the flags have changed
    return true;
  }

  // flags haven't changed
  return false;
}

void SpawnShell::dumpSpawns(spawnItemType type, QTextStream& out)
{
   ItemIterator it(getMap(type));

   while (it.hasNext())
   {
       it.next();
       if (!it.value())
           break;

       out << it.value()->dumpString() << ENDL;
   }
}

// same-name slots, connecting to Packet signals
// this packet is variable in length.  everything is dwords except the "idFile" field
// which can be variable
void SpawnShell::newGroundItem(const uint8_t* data, size_t len, uint8_t dir)
{
   if (m_zoneMgr->isZoning())
      return;

   if (dir != DIR_Server)
      return;

   if (!data)
      return;

   NetStream netStream(data, len);
   makeDropStruct ds;
   QString name;
   union { uint32_t n; float f; } x;
   memset(&ds, 0, sizeof(makeDropStruct));

   // read drop id
   ds.dropId = netStream.readUInt32NC();

   // read name
   name = netStream.readText();
   if(name.length())
   {
      strcpy(ds.idFile, name.toLatin1().data());
      name.resize(0);
   }

   // read past zone id
   netStream.readUInt32NC();

   // read past zone instance
   netStream.readUInt32NC();

   // read past unknown dword field
   netStream.readUInt32NC();

   // read heading
   x.n = netStream.readUInt32NC();
   ds.heading = x.f;

   // read past unknown dword field
   netStream.readUInt32NC();

   // read past unknown dword field
   netStream.readUInt32NC();

   // read past unknown dword field
   netStream.readUInt32NC();

   // read y pos
   x.n = netStream.readUInt32NC();
   ds.y = x.f;

   // read x pos
   x.n = netStream.readUInt32NC();
   ds.x = x.f;

   // read z pos
   x.n = netStream.readUInt32NC();
   ds.z = x.f;

#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::newGroundItem(makeDropStruct *)");
#endif

  Drop* item = (Drop*)m_drops.value(ds.dropId, nullptr);
  if (item != NULL)
  {
    item->update(&ds, name);
    if (!showeq_params->fast_machine)
       item->setDistanceToPlayer(m_player->calcDist2DInt(*item));
    else
       item->setDistanceToPlayer(m_player->calcDist(*item));
    updateFilterFlags(item);
    item->updateLastChanged();
    emit changeItem(item, tSpawnChangedALL);
  }
  else
  {
    item = new Drop(&ds, name);
    if (!showeq_params->fast_machine)
       item->setDistanceToPlayer(m_player->calcDist2DInt(*item));
     else
       item->setDistanceToPlayer(m_player->calcDist(*item));
    updateFilterFlags(item);
    m_drops.insert(ds.dropId, item);
    emit addItem(item);
  }
}

void SpawnShell::removeGroundItem(const uint8_t* data, size_t, uint8_t dir)
{
#ifdef SPAWNSHELL_DIAG
  seqDebug("SpawnShell::removeGroundItem(remDropStruct *)");
#endif

  // if zoning, then don't do anything
  if (m_zoneMgr->isZoning())
    return;

  if (dir != DIR_Server)
    return;

  const remDropStruct *d = (const remDropStruct *)data;

  if (d)
    deleteItem(tDrop, d->dropId);
}

void SpawnShell::newDoorSpawns(const uint8_t* data, size_t len, uint8_t dir)
{
  int nDoors = len / sizeof(doorStruct);
  const doorStruct* doors = (const doorStruct*)data;
  for (int i = 0; i < nDoors; i++)
    newDoorSpawn(doors[i], sizeof(doorStruct), dir);
}

void SpawnShell::newDoorSpawn(const doorStruct& d, size_t len, uint8_t dir)
{
#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::newDoorSpawn(doorStruct*)");
#endif
   Item* item = m_doors.value(d.doorId, nullptr);
   if (item != NULL)
   {
     Door* door = (Door*)item;
     door->update(&d);
     if (!showeq_params->fast_machine)
        item->setDistanceToPlayer(m_player->calcDist2DInt(*item));
     else
        item->setDistanceToPlayer(m_player->calcDist(*item));
     updateFilterFlags(door);
     item->updateLastChanged();
     emit changeItem(door, tSpawnChangedALL);
   }
   else
   {
     item = (Item*)new Door(&d);
     if (!showeq_params->fast_machine)
        item->setDistanceToPlayer(m_player->calcDist2DInt(*item));
     else
        item->setDistanceToPlayer(m_player->calcDist(*item));
     updateFilterFlags(item);
     m_doors.insert(d.doorId, item);
     emit addItem(item);
   }
}

void SpawnShell::zoneSpawns(const uint8_t* data, size_t len)
{
  int spawndatasize = len / sizeof(spawnStruct);

  const spawnStruct* zspawns = (const spawnStruct*)data;

  for (int i = 0; i < spawndatasize; i++)
  {
#if 0
  // Dump position updates for debugging spawn struct position changes
  for (int j=54; j<70; i++)
  {
      printf("%.2x", zspawns[i][j]);

      if ((j+1) % 8 == 0)
      {
          printf("    ");
      }
      else
      {
          printf(" ");
      }
  }
  printf("\n");
#endif

#if 0
    // Debug positioning without having to recompile everything...
#pragma pack(1)
struct pos
{
/*0004*/ unsigned pitch:12;
	 signed   animation:10;                    // velocity 
	 signed   deltaHeading:10;                 // change in heading 
/*0008*/ signed   z:19;                            // z coord (3rd loc value)
	 signed   deltaZ:13;                       // change in z
/*0012*/ signed   deltaY:13;                       // change in y
	 unsigned heading:12;                      // heading 
         unsigned padding01:7;
/*0016*/ signed   y:19;                            // y coord (2nd loc value)
	 signed   deltaX:13;                       // change in x
/*0020*/ signed   x:19;                            // x coord (1st loc value)
	 unsigned padding02:13;
/*0024*/ signed   unknown0001;                     // ***Placeholder
/*0028*/	         
};
#endif

#if 0
#pragma pack(0)
    struct pos *p = (struct pos *)(data + i*sizeof(spawnStruct) + 151);
    printf("[%.2x](%f, %f, %f), dx %f dy %f dz %f head %f dhead %f anim %d (%x, %x, %x, %x)\n",
            zspawns[i].spawnId, 
            float(p->x)/8.0, float(p->y/8.0), float(p->z)/8.0, 
            float(p->deltaX)/4.0, float(p->deltaY)/4.0, 
            float(p->deltaZ)/4.0, 
            float(p->heading), float(p->deltaHeading),
            p->animation, p->padding0000, 
            p->padding0005, p->padding0006, p->padding0014);
#endif
    newSpawn(zspawns[i]);
  }
}

int32_t SpawnShell::fillSpawnStruct(spawnStruct *spawn, const uint8_t *data, size_t len, bool checkLen)
{
   /*
   This reads data from the variable-length spawn struct
   */

	// uncomment for debug info
//#define FILLSPAWNSTRUCT_DIAG

   NetStream netStream(data, len);
   int32_t retVal;
   uint32_t race, nTmp;
   uint8_t i;

   QString name = netStream.readText();

   if(name.length())
      strcpy(spawn->name, name.toLatin1().data());

#ifdef FILLSPAWNSTRUCT_DIAG
   seqDebug("SpawnShell::fillSpawnStruct ---- %s", name.latin1());
#endif

   spawn->spawnId = netStream.readUInt32NC();

   spawn->level = netStream.readUInt8();
   // skip the next 12 bytes
   netStream.skipBytes(12);

   spawn->NPC = netStream.readUInt8();
   spawn->miscData = netStream.readUInt32NC();

   spawn->otherData = netStream.readUInt8();

   // skip unknown3, unknown4
   netStream.skipBytes(8);

   /* & 1 is no longer chest/untargetable.  It is now bazaar /buyer flag as of 01/16/13.
    * Not sure where chest/untargetable flag went, maybe combined with aura now.  Disabled to prevent crashes zoning into bazaar and seems
    * to have no negative impact so far.
   if(spawn->otherData & 1)
   {
      // it's a chest or untargetable

      do
         i = netStream.readUInt8();
      while(i);

      do
         i = netStream.readUInt8();
      while(i);

      do
         i = netStream.readUInt8();
      while(i);

      // skip next 3 longs
      netStream.skipBytes(12);

      // next it loops through 9 longs, but we can just skip them
      netStream.skipBytes(36);

      // skip 1 byte
      netStream.skipBytes(1);

      // skip the last long
      netStream.skipBytes(4);
   }
   */

   if(spawn->aura)	    // aura stuff
   {
       netStream.readText();	// skip 3 variable len strings
       netStream.readText();
       netStream.readText();
       netStream.skipBytes(50);	// and 50 static bytes
   }

   spawn->charProperties = netStream.readUInt8();
#ifdef FILLSPAWNSTRUCT_DIAG
		   seqDebug("charProperties = %X", spawn->charProperties);
#endif

   i = spawn->charProperties;
   //Handle body type of 0.  Started seeing this in Field of Scale after 01/16/13 patch.
   if(i == 0)
   {
      spawn->bodytype = 0;
   }
   else
   {
      do
      {
           nTmp =  netStream.readUInt32NC();

           if(i == spawn->charProperties)
           {
                   spawn->bodytype = nTmp;
#ifdef FILLSPAWNSTRUCT_DIAG
                   seqDebug("bodytype = %d", spawn->bodytype);
#endif
           }
      }
      while(--i);
   }

   spawn->curHp = netStream.readUInt8();
#ifdef FILLSPAWNSTRUCT_DIAG
   seqDebug("curHP=%d", spawn->curHp);
#endif

   // skip facestyle, walk/run speeds, unknown5
   netStream.skipBytes(35);

   spawn->race = netStream.readUInt32NC();
   spawn->holding = netStream.readUInt8();
   spawn->deity = netStream.readUInt32NC();
   spawn->guildID = netStream.readUInt32NC();
   spawn->guildServerID = netStream.readUInt32NC();
   /* spawn->guildstatus = netStream.readUInt32NC();	disappeared 11/14/2018 */
   spawn->guildstatus = 0;
   spawn->class_ = netStream.readUInt32NC();

#ifdef FILLSPAWNSTRUCT_DIAG
   seqDebug("race=%08X holding=%02X deity=%08X guildID=%08X guildstatus=%08X class_=%02X ",
		   spawn->race, spawn->holding, spawn->deity, spawn->guildID, spawn->guildstatus, spawn->class_);
#endif

   netStream.skipBytes(1);

   spawn->state = netStream.readUInt8();
   spawn->light = netStream.readUInt8();

   netStream.skipBytes(1);

   name = netStream.readText();

   if(name.length() > 0 && name.length() < sizeof(spawn->lastName))
   {
      strcpy(spawn->lastName, name.toLatin1().data());
   }

   netStream.skipBytes(2);

   spawn->petOwnerId = netStream.readUInt32NC();

   // 12 bytes added to NPC only in 06/19/2013.
   if (spawn->NPC == 1)
   {
       netStream.skipBytes(49);
   }
   else
   {
       netStream.skipBytes(37);
   }
   race = spawn->race;

   // this is how the client checks if equipment should be read.
   if(spawn->NPC == 0 || race <= 12 || race == 128 || race == 130 || race == 330 || race == 522)
   {
      // skip color
      netStream.skipBytes(36);
      for(i = 0; i < 9; i++)
      {
         spawn->equipment[i].itemId = netStream.readUInt32NC();
         spawn->equipment[i].equip3 = netStream.readUInt32NC();
         spawn->equipment[i].equip2 = netStream.readUInt32NC();
         spawn->equipment[i].equip1 = netStream.readUInt32NC();
         spawn->equipment[i].equip0 = netStream.readUInt32NC();
      }
   } else {
      netStream.skipBytes(20);
      spawn->equipment[7].itemId = netStream.readUInt32NC();
      spawn->equipment[7].equip3 = netStream.readUInt32NC();
      spawn->equipment[7].equip2 = netStream.readUInt32NC();
      spawn->equipment[7].equip1 = netStream.readUInt32NC();
      spawn->equipment[7].equip0 = netStream.readUInt32NC();
      // secondary
      spawn->equipment[8].itemId = netStream.readUInt32NC();
      spawn->equipment[8].equip3 = netStream.readUInt32NC();
      spawn->equipment[8].equip2 = netStream.readUInt32NC();
      spawn->equipment[8].equip1 = netStream.readUInt32NC();
      spawn->equipment[8].equip0 = netStream.readUInt32NC();
   }

   for (int i = 0; i < (sizeof(spawn->posData)/sizeof(spawn->posData[0])); ++i) {
       spawn->posData[i] = netStream.readUInt32NC();
   }

   if(spawn->hasTitle)
   {
      name = netStream.readText();
      strcpy(spawn->title, name.toLatin1().data());
   }

   if(spawn->hasSuffix)
   {
      name = netStream.readText();
      strcpy(spawn->suffix, name.toLatin1().data());
   }

   // unknowns
   netStream.skipBytes(8);

   spawn->isMercenary = netStream.readUInt8();

   // unknowns
   netStream.skipBytes(66);

   // now we're at the end

   retVal = netStream.pos() - netStream.data();

   if(checkLen && (int32_t)len != retVal)
   {
      seqDebug("SpawnShell::fillSpawnStruct - expected length: %d, read: %d for spawn '%s'", len, retVal, spawn->name);
   }

   return retVal;
}

void SpawnShell::zoneEntry(const uint8_t* data, size_t len)
{
  // Zone Entry. Sent when players are added to the zone.

  spawnStruct *spawn = new spawnStruct;

  memset(spawn,0,sizeof(spawnStruct));

  fillSpawnStruct(spawn,data,len,true);

 #ifdef SPAWNSHELL_DIAG
  seqDebug("SpawnShell::zoneEntry(spawnStruct *(name='%s'))", spawn->name);
 #endif

  Item *item;

  if(!strcmp(spawn->name,m_player->realName().toLatin1().data()))
  {
    // Multiple zoneEntry packets are received for your spawn after you zone
    m_player->setPlayerID(spawn->spawnId);
    m_player->update(spawn);
    emit changeItem(m_player, tSpawnChangedALL);
  }
  else
  {
    if((item=m_spawns.value(spawn->spawnId, nullptr)))
    {
        // Update existing spawn
      Spawn *s=(Spawn*)item;
      s->update(spawn);
    }
    else
    {
        // Create a new spawn
      newSpawn(*spawn);
    }
  }
}

void SpawnShell::newSpawn(const uint8_t* data)
{
  // if zoning, then don't do anything
  if (m_zoneMgr->isZoning())
    return;

  const spawnStruct* spawn = (const spawnStruct*)data;

  newSpawn(*spawn);
}

void SpawnShell::newSpawn(const spawnStruct& s)
{
#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::newSpawn(spawnStruct *(name='%s'))", s.name);
#endif
   // if this is the SPAWN_SELF it's the player
   if (s.NPC == SPAWN_SELF)
     return;

   // not the player, so check if it's a recently deleted spawn
   for (int i =0; i < m_cntDeadSpawnIDs; i++)
   {
     if ((m_deadSpawnID[i] != 0) && (m_deadSpawnID[i] == s.spawnId))
     {
       // found a match, remove it from the deleted spawn list
       m_deadSpawnID[i] = 0;

       /* Commented this out because it wasn't adding shrouded spawns.
          Shrouded spawns get deleted from the zone first then added
          as a new spawn.  leaving this here in case another work-around
          needs to be found. (ieatacid - 6-8-2008)

       // let the user know what's going on
       seqInfo("%s(%d) has already been removed from the zone before we processed it.", 
	      s.name, s.spawnId);

       // and stop the attempt to add the spawn.
       return;
       */
     }
   }

   Item* item = m_spawns.value(s.spawnId, nullptr);
   if (item != NULL)
   {
     Spawn* spawn = (Spawn*)item;
     spawn->update(&s);
     updateFilterFlags(spawn);
     updateRuntimeFilterFlags(spawn);
     item->updateLastChanged();

     spawn->setGuildTag(m_guildMgr->guildIdToName(spawn->guildID(), spawn->guildServerID()));

     if (!showeq_params->fast_machine)
        item->setDistanceToPlayer(m_player->calcDist2DInt(*item));
     else
        item->setDistanceToPlayer(m_player->calcDist(*item));

     emit changeItem(item, tSpawnChangedALL);
   }
   else
   {
     item = new Spawn(&s);
     Spawn* spawn = (Spawn*)item;
     updateFilterFlags(spawn);
     updateRuntimeFilterFlags(spawn);
     m_spawns.insert(s.spawnId, item);

     spawn->setGuildTag(m_guildMgr->guildIdToName(spawn->guildID(), spawn->guildServerID()));

     if (!showeq_params->fast_machine)
        item->setDistanceToPlayer(m_player->calcDist2DInt(*item));
     else
        item->setDistanceToPlayer(m_player->calcDist(*item));

     emit addItem(item);

     // send notification of new spawn count
     emit numSpawns(m_spawns.count());
   }
}

void SpawnShell::playerUpdate2(const uint8_t* data, size_t len, uint8_t dir)
{
  if (m_zoneMgr->isZoning())
    return;

  //This payload is normally handled by Player::playerUpdateSelf,
  //but sometimes it contains an update for a different spawn, such as
  //an Eye of Zomm cast by the player, a rowboat being controlled by the
  //player, etc.  So in that case, we'll handle it.
  const playerSelfPosStruct *pupdate = (const playerSelfPosStruct *)data;

  int16_t py = int16_t(pupdate->y);
  int16_t px = int16_t(pupdate->x);
  int16_t pz = int16_t(pupdate->z);
  int16_t pdeltaX = int16_t(pupdate->deltaX);
  int16_t pdeltaY = int16_t(pupdate->deltaY);
  int16_t pdeltaZ = int16_t(pupdate->deltaZ);


  updateSpawn(pupdate->spawnId, px, py, pz, pdeltaX, pdeltaY, pdeltaZ,
          pupdate->heading, pupdate->deltaHeading, pupdate->animation);
}

void SpawnShell::playerUpdate(const uint8_t* data, size_t len, uint8_t dir)
{
  // if zoning, then don't do anything
  if (m_zoneMgr->isZoning())
    return;

#if 0
  // Dump position updates for debugging client update changes
  for (int i=0; i<len; i++)
  {
      printf("%.2x", data[i]);

      if ((i+1) % 8 == 0)
      {
          printf("    ");
      }
      else
      {
          printf(" ");
      }
  }
  printf("\n");
#endif

  const playerSpawnPosStruct *pupdate = (const playerSpawnPosStruct *)data;

  if (dir != DIR_Client)
  {
    int16_t y = pupdate->y >> 3;
    int16_t x = pupdate->x >> 3;
    int16_t z = pupdate->z >> 3;
    
    int16_t dy = pupdate->deltaY >> 2;
    int16_t dx = pupdate->deltaX >> 2;
    int16_t dz = pupdate->deltaZ >> 2;
    
#if 0
    // Debug positioning without having to recompile everything...
#pragma pack(1)
    struct pos
{
	/*0000*/ uint16_t spawnId;
	/*0002*/ uint16_t spawnId2;
	/*0004*/ unsigned pitch:12;
                 signed   y:19;                            // y coord (2nd loc value)		 
	         unsigned padding00:1;
	/*0008*/ signed   x:19;                            // x coord (1st loc value)	 
                 signed   deltaX:13;                       // change in x
	/*0012*/ signed   deltaHeading:10;                 // change in heading 
	         signed   z:19;                            // z coord (3rd loc value)
	         unsigned padding01:3;		 
	/*0016*/ unsigned heading:12;                      // heading 
		 signed   animation:10;                    // velocity
	         unsigned padding02:10;		 
	/*0020*/ signed   deltaY:13;                       // change in y
                 signed   deltaZ:13;                       // change in z 
	         unsigned padding03:6;		 
	/*0024*/ 
};
#endif

#if 0
#pragma pack(0)
    struct pos *p = (struct pos *)data;
    if (p->spawnId == 0x49fd)
        printf("[%.2x](%f, %f, %f), dx %f dy %f dz %f\n  head %d dhead %d anim %d pitch %d (%x, %x, %x, %x, %x, %x)\n",
                p->spawnId, float(p->x)/8.0, float(p->y/8.0), float(p->z)/8.0,
                float(p->deltaX)/4.0, float(p->deltaY)/4.0,
                float(p->deltaZ)/4.0,
                p->heading, p->deltaHeading,
                p->animation, p->pitch,
                p->padding00, p->padding01, p->padding02, p->padding03 );
#endif

    updateSpawn(pupdate->spawnId, x, y, z, dx, dy, dz,
		pupdate->heading, pupdate->deltaHeading,pupdate->animation);
  }
}

void SpawnShell::npcMoveUpdate(const uint8_t* data, size_t len, uint8_t dir)
{
/*
 * Wire format:
 * 2 bytes - spawnId
 * 6 bit - fieldSpecifier bitmask
 * 19 bit - y
 * 19 bit - x
 * 19 bit - z
 * 12 bit - heading
 * [Variable fields]
 *
 * Depending on bits set in fields:
 * 1 = 12 bit pitch
 * 2 = 10 bit delta heading  
 * 4 = 10 bit velocity
 * 8 = 13 bit delta y
 * 16= 13 bit delta x
 * 32 = 13 bit delta z
 *
 * Fields are in that order. For example, if the fieldSpecifier is
 * 1, then there is just 12 bits of pitch. If the fieldSpecifier is
 * 7, then there will be 10 bits of delta heading, 10 bits of animation,
 * and 13 bits of delta y. Other non-specified values are 0.
 *
 * Oh and the byte order needs to be converted too. How nice.
 */
#define MASK_PITCH 0x01
#define MASK_DELTA_HEADING 0x02
#define MASK_ANIMATION 0x04
#define MASK_DELTA_Y 0x08
#define MASK_DELTA_X 0x10
#define MASK_DELTA_Z 0x20

    // Variable length movement packet. Sanity check.
	if ((len < 13) || (len > 24)) 
    {
        // Ignore it.
		seqWarn("Ignoring invalid length %d for movement packet", len);
		return;
	}

    // if zoning, then don't do anything
    if (m_zoneMgr->isZoning())
    {
        return;
    }

    // Pull data from the header.
    BitStream stream(data, len);
    
    // spawnId.
    uint16_t spawnId = stream.readUInt(16);

    // BSH 13 Apr 2011 -- garbage added in packet
    stream.readUInt(16);


    // 6 bit field specifier.
    uint8_t fieldSpecifier = stream.readUInt(6);

    // 19 bit coords. 12 bit heading. All signed.
    int16_t y = stream.readInt(19) >> 3;
    int16_t x = stream.readInt(19) >> 3;
    int16_t z = stream.readInt(19) >> 3;
    int16_t heading = stream.readInt(12);

    // Variable fields are 0 unless specified.
    int16_t deltaX = 0;
    int16_t deltaY = 0;
    int16_t deltaZ = 0;
    int8_t deltaHeading = 0;
    int16_t velocity = 0;
    int16_t pitch = 0;
    
    if (fieldSpecifier & MASK_PITCH)
    {
        // Pull off pitch. Seq doesn't pay attention to this.
        pitch = stream.readInt(12);
    }
    if (fieldSpecifier & MASK_DELTA_HEADING)
    {
        // Pull off deltaHeading. It is 10 bits in length. Signed.
        deltaHeading = stream.readInt(10) >> 2;
    }
    if (fieldSpecifier & MASK_ANIMATION)
    {
        // Pull off velocity. It is 10 bits in length.
        velocity = stream.readInt(10) >> 2;
    }
    if (fieldSpecifier & MASK_DELTA_Y)
    {
        // Pull off deltaY. It is 13 bits in length. Signed.
        deltaY = stream.readInt(13) >> 2;
    }
    if (fieldSpecifier & MASK_DELTA_X)
    {
        // Pull off deltaX. It is 13 bits in length. Signed,
        deltaX = stream.readInt(13) >> 2;
    }
    if (fieldSpecifier & MASK_DELTA_Z)
    {
        // Pull off deltaZ. It is 13 bits in length. Signed.
        deltaZ = stream.readInt(13) >> 2;
    }

    // And send the update.
	updateSpawn(spawnId, x, y, z, 
        deltaX, deltaY, deltaZ, heading, deltaHeading, velocity);
}

void SpawnShell::updateSpawn(uint16_t id, 
			     int16_t x, int16_t y, int16_t z,
			     int16_t xVel, int16_t yVel, int16_t zVel,
			     int8_t heading, int8_t deltaHeading,
			     uint8_t animation)
{
#ifdef SPAWNSHELL_DIAG
    seqDebug("SpawnShell::updateSpawn(id=%d, x=%d, y=%d, z=%d, xVel=%d, yVel=%d, zVel=%d)", 
        id, x, y, z, xVel, yVel, zVel);
#endif

    Item* item;
   
    if (id == m_player->id())
    {
        item = m_player;
    }
    else
    {
        item = m_spawns.value(id, nullptr);
    }

    if (item != NULL)
    {
        Spawn* spawn = (Spawn*)item;

        spawn->setPos(x, y, z,
		    showeq_params->walkpathrecord,
		    showeq_params->walkpathlength);
        spawn->setAnimation(animation);

        spawn->setDeltas(xVel, yVel, zVel);
        spawn->setHeading(heading, deltaHeading);

        // Distance
        if (!showeq_params->fast_machine)
            item->setDistanceToPlayer(m_player->calcDist2DInt(*item));
        else
            item->setDistanceToPlayer(m_player->calcDist(*item));
        
        spawn->updateLast();
        item->updateLastChanged();
        emit changeItem(item, tSpawnChangedPosition);
    }
    else if (showeq_params->createUnknownSpawns)
    {
        // not the player, so check if it's a recently deleted spawn
        for (int i =0; i < m_cntDeadSpawnIDs; i++)
        {
            // check dead spawn list for spawnID, if it was deleted, shouldn't
            // see new position updates, so therefore this is probably 
            // for a new spawn (spawn ID being reused)
            if ((m_deadSpawnID[i] != 0) && (m_deadSpawnID[i] == id))
            {
                // found a match, ignore it
                m_deadSpawnID[i] = 0;

                seqInfo("(%d) had been removed from the zone, but saw a position update on it, so assuming bogus update.", 
                    id);

                return;
            }
        }

        item = new Spawn(id, x, y, z, xVel, yVel, zVel, 
            heading, deltaHeading, animation);
        updateFilterFlags(item);
        updateRuntimeFilterFlags(item);
        m_spawns.insert(id, item);
        emit addItem(item);

#ifdef SPAWNSHELL_DIAG
        seqDebug("SpawnShell::updateSpawn created unknown spawn (id=%u)", id);
#endif

        // send notification of new spawn count
        emit numSpawns(m_spawns.count());
    }
}

void SpawnShell::updateSpawns(const uint8_t* data)
{
  // if zoning, then don't do anything
  if (m_zoneMgr->isZoning())
    return;

  const spawnPositionUpdate* updates = (const spawnPositionUpdate*)data;
  updateSpawn(updates->spawnId, 
	      updates->x >> 3, updates->y >> 3, updates->z >> 3,
	      0,0,0,updates->heading,0,0);
}

void SpawnShell::updateSpawnInfo(const uint8_t* data)
{
   const SpawnUpdateStruct* su = (const SpawnUpdateStruct*)data;
#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::updateSpawnInfo(id=%d, sub=%d, hp=%d, maxHp=%d)", 
	  su->spawnId, su->subcommand, su->arg1, su->arg2);
#endif

   Item* item = m_spawns.value(su->spawnId, nullptr);
   if (item != NULL)
   {
     Spawn* spawn = (Spawn*)item;
     switch(su->subcommand) {
     case 17: // current hp update
       spawn->setHP(su->arg1);
       item->updateLastChanged();
       emit changeItem(item, tSpawnChangedHP);
       break;
     }
   }
}

void SpawnShell::renameSpawn(const uint8_t* data)
{
    const spawnRenameStruct* rename = (const spawnRenameStruct*)data;
#ifdef SPAWNSHELL_DIAG
    seqDebug("SpawnShell::renameSpawn(oldname=%s, newname=%s)",
             rename->old_name, rename->new_name);
#endif
    
    Spawn* renameMe = findSpawnByName(rename->old_name);

    if (renameMe != NULL)
    {
        renameMe->setName(rename->new_name);

        uint32_t changeType = tSpawnChangedName;

        if (updateFilterFlags(renameMe))
          changeType |= tSpawnChangedFilter;
        if (updateRuntimeFilterFlags(renameMe))
          changeType |= tSpawnChangedRuntimeFilter;

        renameMe->updateLastChanged();
        emit changeItem(renameMe, tSpawnChangedName);
    }
    else
    {
        seqWarn("SpawnShell: tried to rename %s to %s, but the original mob didn't exist in the spawn list", rename->old_name, rename->new_name);
    }
}

void SpawnShell::illusionSpawn(const uint8_t* data)
{
    const spawnIllusionStruct* illusion = (const spawnIllusionStruct*)data;
#ifdef SPAWNSHELL_DIAG
    seqDebug("SpawnShell::illusionSpawn(id=%d, name=%s, new race=%d)",
             illusion->spawnId, illusion->name, illusion->race);
#endif

    Item* item = m_spawns.value(illusion->spawnId, nullptr);

    if (item != NULL)
    {
        Spawn* spawn = (Spawn*) item;

        // Update what we can
        spawn->setGender(illusion->gender);
        spawn->setRace(illusion->race);

        spawn->updateLastChanged();
        emit changeItem(spawn, tSpawnChangedALL);
#ifdef SPAWNSHELL_DIAG
        seqDebug("SpawnShell: Illusioned %s (id=%d) into race %d",
                 illusion->name, illusion->spawnId, illusion->race);
#endif
    }
    else
    {
        // Someone with an illusion up zoning in will generate an
        // OP_Illusion BEFORE the OP_NewSpawn, so they won't be
        // in the spawn list. Their spawnStruct will have their
        // illusioned race anyways.
    }
}

void SpawnShell::shroudSpawn(const uint8_t* data, size_t len, uint8_t dir)
{
    // Self or other person shrouding. newSpawn handled updates too.

    NetStream netStream(data,len);

    uint32_t spawnID=netStream.readUInt32NC();
    uint16_t spawnStructSize=netStream.readUInt16NC();
    spawnStructSize-=6;

    if(spawnID!=m_player->id())
    {
        // Shrouding other player
        spawnShroudOther *shroud = new spawnShroudOther;
        fillSpawnStruct(&shroud->spawn,netStream.pos(),spawnStructSize,true);
        seqInfo("Shrouding %s (id=%d)", shroud->spawn.name, shroud->spawn.spawnId);
        newSpawn(shroud->spawn);
    }
    else
    {
        // Shrouding yourself.
        spawnShroudSelf *shroud = new spawnShroudSelf;

        fillSpawnStruct(&shroud->spawn,netStream.pos(),spawnStructSize,true);
        netStream.skipBytes(spawnStructSize);
        memcpy(&shroud->profile,netStream.pos(),sizeof(playerProfileStruct));
        seqInfo("Shrouding %s (id=%d)", shroud->spawn.name, shroud->spawn.spawnId);

        m_player->loadProfile(shroud->profile);

        // We just updated a lot of stuff.
        updateFilterFlags(m_player);
        updateRuntimeFilterFlags(m_player);
        m_player->updateLastChanged();
        emit changeItem(m_player, tSpawnChangedALL);
    }
}

void SpawnShell::updateSpawnAppearance(const uint8_t* data)
{
    const spawnAppearanceStruct* app = (const spawnAppearanceStruct*)data;
#ifdef SPAWNSHELL_DIAG
    seqDebug("SpawnShell::updateSpawnAppearance(id=%d, sub=%d, parm=%08x)",
             app->spawnId, app->type, app->parameter);
#endif

   Item* item = m_spawns.value(app->spawnId, nullptr);

   if (item != NULL)
   {
       Spawn* spawn = (Spawn*)item;

       switch(app->type) 
       {
           case 1: // level update
               spawn->setLevel(app->parameter);
               spawn->updateLastChanged();
               emit changeItem(spawn, tSpawnChangedLevel);
               break;
       }

      /* Other types for OP_SpawnAppearance (from eqemu guys)
       0  - this causes the client to keel over and zone to bind point
       1  - level, parm = spawn level
       3  - 0 = visible, 1 = invisible
       4  - 0 = blue, 1 = pvp (red)
       5  - light type emitted by player (lightstone, shiny shield)
       14 - anim, 100=standing, 110=sitting, 111=ducking, 115=feigned, 105=looting
       15 - sneak, 0 = normal, 1 = sneaking
       16 - server to client, sets player spawn id
       17 - Client->Server, my HP has changed (like regen tic)
       18 - linkdead, 0 = normal, 1 = linkdead
       19 - lev, 0=off, 1=flymode, 2=levitate
       20 - GM, 0 = normal, 1 = GM - all odd numbers seem to make it GM
       21 - anon, 0 = normal, 1 = anon, 2 = roleplay
       22 - guild id
       23 - guild rank, 0=member, 1=officer, 2=leader
       24 - afk, 0 = normal, 1 = afk
       28 - autosplit, 0 = normal, 1 = autosplit on
       29 - spawn's size
       31 -change PC's name's color to NPC color 0 = normal, 1 = npc name
       */
   }
}

void SpawnShell::updateNpcHP(const uint8_t* data)
{
  const hpNpcUpdateStruct* hpupdate = (const hpNpcUpdateStruct*)data;
#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::updateNpcHP(id=%d, maxhp=%d hp=%d)", 
	  hpupdate->spawnId, hpupdate->maxHP, hpupdate->curHP);
#endif
   Item* item = m_spawns.value(hpupdate->spawnId, nullptr);
   if (item != NULL)
   {
     Spawn* spawn = (Spawn*)item;
     spawn->setHP(hpupdate->curHP);
     spawn->setMaxHP(hpupdate->maxHP);
     item->updateLastChanged();
     emit changeItem(item, tSpawnChangedHP);
   }
}

void SpawnShell::spawnWearingUpdate(const uint8_t* data)
{
  const wearChangeStruct *wearing = (const wearChangeStruct *)data;
  Item* item = m_spawns.value(wearing->spawnId, nullptr);
  if (item != NULL)
  {
    // ZBTEMP: Find newItemID
    //Spawn* spawn = (Spawn*)item;
    //    spawn->setEquipment(wearing->wearSlotId, wearing->newItemId);
    uint32_t changeType = tSpawnChangedWearing;
    if (updateFilterFlags(item))
      changeType |= tSpawnChangedFilter;
    if (updateRuntimeFilterFlags(item))
      changeType |= tSpawnChangedRuntimeFilter;
    item->updateLastChanged();
    emit changeItem(item, changeType);
  }
}

void SpawnShell::consMessage(const uint8_t* data, size_t, uint8_t dir) 
{
  const considerStruct * con = (const considerStruct*)data;
  Item* item;
  Spawn* spawn;

  if (dir == DIR_Client)
  {
    if (con->playerid != con->targetid) 
    {
      item = m_spawns.value(con->targetid, nullptr);
      if (item != NULL)
      {
	spawn = (Spawn*)item;

	// note that this spawn has been considered
	spawn->setConsidered(true);
	
	emit spawnConsidered(item);
      }
    }
    return;
  }

  // is it you that you've conned?
  if (con->playerid != con->targetid) 
  {
    // find the spawn if it exists
    item = m_spawns.value(con->targetid, nullptr);

    // has the spawn been seen before?
    if (item != NULL)
    {
      // yes
      Spawn* spawn = (Spawn*)item;

      // note that this spawn has been considered
      spawn->setConsidered(true);

      emit spawnConsidered(item);
    } // end if spawn found
  } // else not yourself
} // end consMessage()

void SpawnShell::removeSpawn(const uint8_t* data, size_t len, uint8_t dir)
{
  if(dir==DIR_Client)
    return;
  const removeSpawnStruct* rmSpawn = (const removeSpawnStruct*)data;
#ifdef SPAWNSHELL_DIAG
  seqDebug("SpawnShell::removeSpawn(id=%d)", rmSpawn->spawnId);
#endif

  Item *item;

  if(len==sizeof(removeSpawnStruct))
  {
// BSH
	deleteItem(tSpawn, rmSpawn->spawnId);
// BSH

    if(!rmSpawn->removeSpawn)
    {
      // Remove a spawn from outside the update radius
      if(showeq_params->useUpdateRadius)
      {
        // Remove it
        deleteItem(tSpawn, rmSpawn->spawnId);
      }
      else
      {
        // Set flag to change its icon
        if((item=m_spawns.value(rmSpawn->spawnId, nullptr)))
        {
          Spawn *s=(Spawn*)item;
          s->setNotUpdated(true);
        }
      }
    }
  }
  else if((len+1)!=sizeof(removeSpawnStruct))
  {
    seqWarn("OP_RemoveSpawn (dataLen: %d) doesn't match: sizeof(removeSpawnStruct): %d",
            len,sizeof(removeSpawnStruct));
  }
}

void SpawnShell::deleteSpawn(const uint8_t* data)
{
  const deleteSpawnStruct* delspawn = (const deleteSpawnStruct*)data;
#ifdef SPAWNSHELL_DIAG
  seqDebug("SpawnShell::deleteSpawn(id=%d)", delspawn->spawnId);
#endif
  if (m_posDeadSpawnIDs < (MAX_DEAD_SPAWNIDS - 1))
    m_posDeadSpawnIDs++;
  else
    m_posDeadSpawnIDs = 0;

  if (m_cntDeadSpawnIDs < MAX_DEAD_SPAWNIDS)
    m_cntDeadSpawnIDs++;

  m_deadSpawnID[m_posDeadSpawnIDs] = delspawn->spawnId;

  deleteItem(tSpawn, delspawn->spawnId);
}

void SpawnShell::killSpawn(const uint8_t* data)
{
  const newCorpseStruct* deadspawn = (const newCorpseStruct*)data;
#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::killSpawn(id=%d, kid=%d)", 
	  deadspawn->spawnId, deadspawn->killerId);
#endif
   Item* item;

   if (deadspawn->spawnId != m_player->id())
   {
       item = m_spawns.value(deadspawn->spawnId, nullptr);
   }
   else
   {
       item = m_player;
   }

   if (item != NULL)
   {
     Spawn* spawn = (Spawn*)item;
     
     // ZBTEMP: This is temporary until we can find a better way
     // set the last kill info on the player (do this before changing name)
     
     // only call setLastKill if *you* killed the spawn
     if(deadspawn->killerId == m_player->id())
     {
         m_player->setLastKill(spawn->name(), spawn->level());
     }

     spawn->killSpawn();
     updateFilterFlags(item);
     updateRuntimeFilterFlags(item);

     spawn->setName(spawn->realName() + Spawn_Corpse_Designator);

     Item* killer;
     killer = m_spawns.value(deadspawn->killerId, nullptr);
     emit killSpawn(item, killer, deadspawn->killerId);
   }
}

void SpawnShell::respawnFromHover(const uint8_t* data, size_t len, uint8_t dir)
{
   if(dir != DIR_Client)
      return;
#ifdef SPAWNSHELL_DIAG
   seqDebug("SpawnShell::respawnFromHover()");
#endif

    // Our old player is a corpse, but we're rising from the dead. So
    // we need to pop a corpse to represent our deadselves, invalidate
    // the player, and then let the OP_ZoneEntry that is coming for the repop
    // fix the player.
    uint16_t corpseId = m_player->id();

    // invalidate the player by severing it from its Id.
    m_player->setID(0);

    // Pop a corpse
    Spawn* corpse = new Spawn((Spawn*) m_player, corpseId);

    updateFilterFlags(corpse);
    updateRuntimeFilterFlags(corpse);
    m_spawns.insert(corpse->id(), corpse);

    corpse->setGuildTag(m_guildMgr->guildIdToName(corpse->guildID(), corpse->guildServerID()));

    emit addItem(corpse);

    // send notification of new spawn count
    emit numSpawns(m_spawns.count());
}

void SpawnShell::corpseLoc(const uint8_t* data)
{
  const corpseLocStruct* corpseLoc = (const corpseLocStruct*)data;
  Item* item = m_spawns.value(corpseLoc->spawnId, nullptr);
  if (item != NULL)
  {
    Spawn* spawn = (Spawn*)item;

    // set the corpses location, and make sure it's not moving... 
    if ((spawn->NPC() == SPAWN_PLAYER) || (spawn->NPC() == SPAWN_PC_CORPSE))
    {
        spawn->setPos(int16_t(corpseLoc->y), int16_t(corpseLoc->x), 
                      int16_t(corpseLoc->z),
                      showeq_params->walkpathrecord,
                      showeq_params->walkpathlength);
    }
    else 
    {
        spawn->setPos(int16_t(corpseLoc->x), int16_t(corpseLoc->y), 
                      int16_t(corpseLoc->z),
                      showeq_params->walkpathrecord,
                      showeq_params->walkpathlength);
    }
    spawn->killSpawn();
    spawn->updateLast();
    spawn->updateLastChanged();
    
    // signal that the spawn has changed
    emit killSpawn(item, NULL, 0);
  }
}

void SpawnShell::playerChangedID(uint16_t oldPlayerID, uint16_t newPlayerID)
{
  // remove the player from the list (if it had a 0 id)
  deleteItem(tPlayer, 0);

  if (oldPlayerID == newPlayerID)
      return;

  //remove the old player 
  deleteItem(tPlayer, oldPlayerID);


  //if the new ID already exists an unknown spawn
  deleteItem(tSpawn, newPlayerID);

  // re-insert the player into the list
  m_players.insert(newPlayerID, m_player);

  emit changeItem(m_player, tSpawnChangedALL);
}

void SpawnShell::refilterSpawns()
{
  refilterSpawns(tSpawn);
  refilterSpawns(tDrop);
  refilterSpawns(tDoors);
}

void SpawnShell::refilterSpawns(spawnItemType type)
{
   ItemMap& theMap = getMap(type);
   ItemIterator it(theMap);

   if (type == tSpawn)
   {
     Spawn* spawn;
     // iterate over all the items in the map
     while (it.hasNext())
     {
       it.next();

       // get the item
       spawn = (Spawn*)it.value();
       if (!spawn)
           break;

       // update the flags, if they changed, send a notification
       if (updateFilterFlags(spawn))
       {
    	 spawn->updateLastChanged();
    	 emit changeItem(spawn, tSpawnChangedFilter);
       }
     }
   }
   else
   {
     Item* item;
     // iterate over all the items in the map
     while (it.hasNext())
     {
       it.next();

       // get the item
       item = it.value();
       if (!item)
           break;

       // update the flags, if they changed, send a notification
       if (updateFilterFlags(item))
       {
		 item->updateLastChanged();
		 emit changeItem(item, tSpawnChangedFilter);
       }
     }
   }
}

void SpawnShell::refilterSpawnsRuntime()
{
  refilterSpawnsRuntime(tSpawn);
  refilterSpawnsRuntime(tDrop);
  refilterSpawnsRuntime(tDoors);
}

void SpawnShell::refilterSpawnsRuntime(spawnItemType type)
{
   ItemIterator it(getMap(type));

   if (type == tSpawn)
   {
     Spawn* spawn;
     // iterate over all the items in the map
     while (it.hasNext())
     {
       it.next();

       // get the item
       spawn = (Spawn*)it.value();
       if (!spawn)
           break;

       // update the flags, if they changed, send a notification
       if (updateRuntimeFilterFlags(spawn))
       {
		 spawn->updateLastChanged();
		 emit changeItem(spawn, tSpawnChangedRuntimeFilter);
       }
     }
   }
   else
   {
     Item* item;
     // iterate over all the items in the map
     while (it.hasNext())
     {
       it.next();

       // get the item
       item = it.value();
       if (!item)
           break;

       // update the flags, if they changed, send a notification
       if (updateRuntimeFilterFlags(item))
       {
		 item->updateLastChanged();
		 emit changeItem(item, tSpawnChangedRuntimeFilter);
       }
     }
   }
}

void SpawnShell::saveSpawns(void)
{
  QFile keyFile(showeq_params->saveRestoreBaseFilename + "Spawns.dat");
  if (keyFile.open(QIODevice::WriteOnly))
  {
    QDataStream d(&keyFile);

    // write the magic string
    d << *magic;

    // write a test value at the top of the file for a validity check
    uint32_t testVal = sizeof(spawnStruct);
    d << testVal;

    // save the name of the current zone
    d << m_zoneMgr->shortZoneName().toLower();

    // save the spawns
    ItemMap& theMap = getMap(tSpawn);

    // save the number of spawns
    testVal = theMap.count();
    d << testVal;

    ItemIterator it(theMap);
    Spawn* spawn;

    // iterate over all the items in the map
    while (it.hasNext())
    {
      it.next();

      // get the spawn
      spawn = (Spawn*)it.value();
      if (!spawn)
          break;

      // save the spawn id
      d << spawn->id();

      // save the spawn
      spawn->saveSpawn(d);
    }
  }

   // re-start the timer
   if (showeq_params->saveSpawns)
   {
     m_timer->setSingleShot(true);
     m_timer->start(showeq_params->saveSpawnsFrequency);
   }
}

void SpawnShell::restoreSpawns(void)
{
  QString fileName = showeq_params->saveRestoreBaseFilename + "Spawns.dat";
  QFile keyFile(fileName);
  if (keyFile.open(QIODevice::ReadOnly))
  {
    size_t i;
    uint32_t testVal;
    uint16_t id;
    Spawn* item;

    QDataStream d(&keyFile);

    // check the magic string
    uint32_t magicTest;
    d >> magicTest;

    if (magicTest != *magic)
    {
      seqWarn("Failure loading %s: Bad magic string!",
              fileName.toLatin1().data());
      return;
    }

    // check the test value at the top of the file
    d >> testVal;
    if (testVal != sizeof(spawnStruct))
    {
      seqWarn("Failure loading %s: Bad spawnStruct size!",
              fileName.toLatin1().data());
      return;
    }

    // attempt to validate that the info is from the current zone
    QString zoneShortName;
    d >> zoneShortName;
    if (zoneShortName != m_zoneMgr->shortZoneName().toLower())
    {
      seqWarn("\aWARNING: Restoring spawns for potentially incorrect zone (%s != %s)!",
              zoneShortName.toLatin1().data(),
              m_zoneMgr->shortZoneName().toLower().toLatin1().data());
    }

    // read the expected number of elements
    d >> testVal;

    // read in the spawns
    for (i = 0; i < testVal; i++)
    {
      // get the spawn id
      d >> id;

      // re-create the spawn
      item = new Spawn(d, id);

      // filter and add it to the list
      updateFilterFlags(item);
      updateRuntimeFilterFlags(item);
      m_spawns.insert(id, item);
      emit addItem(item);
    }

    emit numSpawns(m_spawns.count());

    seqInfo("Restored SPAWNS: count=%d!",
            m_spawns.count());
  }
  else
  {
    seqWarn("Failure loading %s: Unable to open!",
            fileName.toLatin1().data());
  }
}

#ifndef QMAKEBUILD
#include "spawnshell.moc"
#endif
