/*
 *  spawn.h
 *  Copyright 2001 Crazy Joe Divola (cjd1@users.sourceforge.net)
 *  Copyright 2001-2010, 2013, 2019 by the respective ShowEQ Developers
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
 * Adapted from spawnlist.h - Crazy Joe Divola (cjd1@users.sourceforge.net)
 * Date   - 7/31/2001
 */

// Major rework of SpawnShell{} classes - Zaphod (dohpaz@users.sourceforge.net)
//   Based on stuff from CJD's SpawnShell{} and stuff from SpawnList and
//   from Map.  Some optimization ideas adapted from SINS.

#ifndef _SPAWN_H_
#define _SPAWN_H_

#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif
#include <cmath>
#include <ctime>

#include <QList>
#include <QDateTime>
#include <QDataStream>

#include "everquest.h"
#include "point.h"

//----------------------------------------------------------------------
// forward declarations
class SpawnShell;

struct spawnStruct;
struct makeDropStruct;
struct doorStruct;

//----------------------------------------------------------------------
// enumerated types
// type of item
enum spawnItemType 
{ 
  tUnknown, 
  tDoors, 
  tDrop,
  tSpawn,
  tPlayer
};

// slots that an item can be warn in
enum itemWearSlot
{
  tHead          = 0,
  tChest         = 1,
  tArms          = 2,
  tWaist         = 3,
  tGloves        = 4,
  tLegs          = 5,
  tFeet          = 6,
  tPrimary       = 7,
  tSecondary     = 8,
  tUnknown1      = 9,

  // these are for bookeeping
  tNumWearSlots  = 10,
  tFirstMaterial = tHead,
  tLastMaterial  = tFeet,
  tFirstWeapon   = tPrimary,
  tLastWeapon    = tSecondary,
  tLastCoreWearSlot = tSecondary,
};

// type of change done to item
enum changeType
{
  tSpawnChangedNone = 0,
  tSpawnChangedPosition = 1,
  tSpawnChangedHP = 2,
  tSpawnChangedWearing = 4,
  tSpawnChangedFlags = 8,
  tSpawnChangedLevel = 16,
  tSpawnChangedNPC = 32,
  tSpawnChangedFilter = 64,
  tSpawnChangedRuntimeFilter = 128,
  tSpawnChangedConsidered = 256,
  tSpawnChangedName = 512,
  tSpawnChangedALL = 1023, // sum of all previous change types 
};


//----------------------------------------------------------------------
// type definitions
typedef Point3D<int16_t> EQPoint;
typedef EQPoint SpawnTrackPoint;
typedef QList<SpawnTrackPoint*> SpawnTrackList;
typedef QListIterator<SpawnTrackPoint*> SpawnTrackListIterator;

//----------------------------------------------------------------------
// constants

// default minimum distance for finding the closest item
const double DEFAULT_MIN_DISTANCE = HUGE_VAL;

//----------------------------------------------------------------------
// Item
class Item : public EQPoint
{
 public:
  // constructor/destructor
  Item(spawnItemType t, uint16_t id);
  virtual ~Item();

  // get methods
  spawnItemType type() const { return m_type; }
  uint16_t id() const { return m_ID; }
  QString realName() const { return m_name; }
  time_t lastChanged() const { return m_lastChanged; }
  int8_t heading() const { return m_heading; }
  int16_t headingAngle() const { return 360 - (m_heading * 360) / 256; }
  const QTime& lastUpdate() const { return m_lastUpdate; }
  QString lastUpdateStr() const 
    { return m_lastUpdate.toString(); }
  const QTime& spawnTime() const { return m_spawnTime; }
  QString spawnTimeStr() const
    { return m_spawnTime.toString(); }
  uint32_t filterFlags() const { return m_filterFlags; }
  uint32_t runtimeFilterFlags() const { return m_runtimeFilterFlags; }
  uint8_t NPC() const { return m_NPC; }
  double getFDistanceToPlayer() const { return m_fdist; }
  uint32_t getIDistanceToPlayer() const { return m_idist; }

  // virtual methods that provide reasonable default values/behaviour
  virtual QString name() const;
  virtual QString transformedName() const;
  virtual uint16_t race() const;
  virtual QString raceString() const;
  virtual uint8_t classVal() const;
  virtual QString classString() const;
  virtual QString info() const;
  virtual QString filterString() const;
  virtual QString dumpString() const;

  // set methods
  void setDistanceToPlayer(double);
  void setDistanceToPlayer(uint32_t);
  void setPos(int16_t x, int16_t y, int16_t z);
  void setHeading(int8_t heading) { m_heading = heading; }

  void setName(const char *name)
    { m_name = QString::fromUtf8(name); }

  void setName(const QString& name)
    { m_name = name; }

  void updateLast()
  {
    m_lastUpdate.restart();
  }
  void updateLastChanged()
  {
    m_lastChanged = time(NULL);
  }

  void setFilterFlags(uint32_t filterFlags) { m_filterFlags = filterFlags; }
  void setRuntimeFilterFlags(uint32_t filterFlags) 
    { m_runtimeFilterFlags = filterFlags; }

 protected:
  void setNPC(uint8_t NPC) { m_NPC = NPC; }

  // common item data
  QString m_name;
  uint32_t m_filterFlags;
  uint32_t m_runtimeFilterFlags;

  // persisted info below
  QTime m_lastUpdate; 
  QTime m_spawnTime; 
  time_t m_lastChanged;
  uint16_t m_ID;
  uint8_t m_NPC;
  int8_t m_heading;
  spawnItemType m_type;
  double m_fdist;
  uint32_t m_idist;
};

//----------------------------------------------------------------------
// Spawn
class Spawn : public Item
{
 public:
  // constructors/destructor
  Spawn();
  Spawn(const spawnStruct* s);

  // create an unknown spawn using the data available.
  Spawn(uint16_t id, 
	int16_t x, int16_t y, int16_t z,
	int16_t deltaX, int16_t deltaY, int16_t deltaZ,
	int8_t heading, int8_t deltaHeading,
	uint8_t animation);

  // restore spawn from QDataStream
  Spawn(QDataStream&, uint16_t id);
  Spawn(Spawn*, uint16_t id);
  virtual ~Spawn();

  // save spawn to QDataStream
  void saveSpawn(QDataStream& d);
  
  // spawn specific get methods
  int16_t deltaX() const { return m_deltaX; }
  int16_t deltaY() const { return m_deltaY; }
  int16_t deltaZ() const { return m_deltaZ; }
  int8_t deltaHeading() const { return m_deltaHeading; }
  uint8_t animation() const { return m_animation; }
  int32_t HP() const { return m_curHP; }
  int32_t maxHP() const { return m_maxHP; }
  uint16_t guildID() const { return m_guildID; }
  uint16_t guildServerID() const { return m_guildServerID; }
  QString guildTag() const { return m_guildTag; }
  uint16_t petOwnerID() const { return m_petOwnerID; }
  uint8_t light() const { return m_light; }
  QString lightName() const;
  uint8_t gender() const { return m_gender; }
  QString genderName() const;
  QString deityName() const;
  int16_t deityTeam() const { return m_deityTeam; }
  int16_t raceTeam() const { return m_raceTeam; }
  bool considered() const { return m_considered; }
  EquipStruct equipment(uint8_t wearingSlot) const 
    { return m_equipment[wearingSlot]; }
  QString equipmentStr(uint8_t wearingSlot) const;
  uint8_t typeflag() const { return m_typeflag; }
  uint8_t gm() const {return m_gm; }
  QString typeString() const;
  const SpawnTrackList& trackList() const { return m_spawnTrackList; }
  SpawnTrackList& trackList() { return m_spawnTrackList; }
  QString cleanedName() const;
  bool approximatePosition(bool animating, 
			   const QTime& curTime,
			   EQPoint& newPos) const;
  bool isNotUpdated() const { return m_notUpdated; }

  // spawn related methods that sub-classes may override
  virtual QString lastName() const;
  virtual int level() const;
  virtual uint16_t deity() const;

  // virtual get method overloads
  virtual QString transformedName() const;
  virtual uint16_t race() const;
  virtual QString raceString() const;
  virtual uint8_t classVal() const;
  virtual QString classString() const;
  virtual QString info() const;
  virtual QString filterString() const;
  virtual QString dumpString() const;

  // convenience test methods
  bool isSelf() const { return (m_NPC == SPAWN_SELF); }
  bool isCorpse() const 
  {
    return ((m_NPC == SPAWN_PC_CORPSE) ||
	    (m_NPC == SPAWN_NPC_CORPSE));
  }
  bool isPlayer() const
  {
    return ((m_NPC == SPAWN_PLAYER) ||
	    (m_NPC == SPAWN_SELF));
  }
  bool isOtherPlayer() const
    { return (m_NPC == SPAWN_PLAYER); }
  bool isNPC() const 
    { return (m_NPC == SPAWN_NPC); }
  bool isUnknown() const
    { return (m_NPC == SPAWN_NPC_UNKNOWN); }
  bool isSameRaceTeam(const Spawn* spawn) const
    { return (raceTeam() == spawn->raceTeam()); }
  bool isSameDeityTeam(const Spawn* spawn) const
    { return (deityTeam() == spawn->deityTeam()); }
  bool isMount() const
    { return m_isMount; }
  bool isAura() const
    { return m_isAura; }
  bool isMercenary() const
    { return m_isMercenary; }

  // virtual set method overload
  void setPos(int16_t x, int16_t Pos, int16_t z,
	      bool walkpathrecord = false, size_t walkpathlength = 0);

  // update methods 

  // updates all of the, irregardless of existing values
  void update(const spawnStruct* s);

  // updates the data, careful not to overwrite existing correct data
  void backfill(const spawnStruct* s);

  // change spawn state
  virtual void killSpawn();

  // spawn specific set methods
  void setDeltas(int16_t deltaX, int16_t deltaY, int16_t deltaZ);
  void setHeading(int8_t heading, int8_t deltaHeading)
    { m_heading = heading; m_deltaHeading = deltaHeading; }
  void setDeltaHeading(int8_t deltaHeading) { m_deltaHeading = deltaHeading; }
  void setAnimation(uint8_t animation) { m_animation = animation; }
  void setPetOwnerID(uint16_t petOwnerID) { m_petOwnerID = petOwnerID; }
  void setLight(uint8_t light) { m_light = light; }
  void setGender(uint8_t gender) { m_gender = gender; }
  void setDeity(uint16_t deity) { m_deity = deity; calcDeityTeam(); }
  void setConsidered(bool considered) { m_considered = considered; }
  void setRace(uint16_t race) { m_race = race; calcRaceTeam(); }
  void setClassVal(uint8_t classVal) { m_class = classVal; }
  void setHP(int32_t HP) { m_curHP = HP; }
  void setMaxHP(int32_t maxHP) { m_maxHP = maxHP; }
  void setGuildID(uint16_t GuildID) { m_guildID = GuildID; }
  void setGuildServerID(uint16_t GuildServerID) { m_guildServerID = GuildServerID; }
  void setGuildTag(QString GuildTag) { m_guildTag = GuildTag; }
  void setLevel(int level) { m_level = level; }
  void setEquipment(uint8_t wearSlot, EquipStruct item)
    { if (wearSlot < tNumWearSlots) { m_equipment[wearSlot] = item; } }
  void setNPC(uint8_t NPC) { m_NPC = NPC; }
  void setTypeflag(uint8_t typeflag) { m_typeflag = typeflag; }
  void setGM(uint8_t gm) { m_gm = gm; }
  void setIsMount(bool isMount) { m_isMount = isMount; }
  void setIsMercenary(uint8_t isMercenary) {m_isMercenary = (isMercenary != 0); }
  void setIsAura(unsigned aura) {m_isAura = (aura != 0); }
  void setID(uint16_t id) { m_ID = id; }
  void setLastName(const char * lastName)
    { m_lastName = QString::fromUtf8(lastName); }
  void setLastName(const QString& lastName)
    { m_lastName = lastName; }
  void setNotUpdated(bool notUpdated) { m_notUpdated = notUpdated; }


 protected:
  void calcRaceTeam();
  void calcDeityTeam();
  bool calcIsMount(uint32_t, uint8_t);

  // spawn specific data
  QString m_lastName;
  QString m_guildTag;
  SpawnTrackList m_spawnTrackList;
  int m_cookedDeltaXFixPt;
  int m_cookedDeltaYFixPt;
  int m_cookedDeltaZFixPt;
  int16_t m_deltaX;
  int16_t m_deltaY;
  int16_t m_deltaZ;
  int8_t m_deltaHeading;

  // persisted info below
  uint16_t m_petOwnerID;
  int32_t m_curHP;
  int32_t m_maxHP;
  uint16_t m_guildID;
  uint16_t m_guildServerID;
  uint16_t m_deity;
  int16_t m_deityTeam;
  EquipStruct m_equipment[tNumWearSlots];
  uint16_t m_race;
  int16_t m_raceTeam;
  int m_level;
  uint8_t m_gender;
  uint8_t m_class;
  uint8_t m_light;
  uint8_t m_typeflag;
  uint8_t m_animation;
  uint8_t m_gm;
  bool m_isMount;
  bool m_isMercenary;
  bool m_isAura;
  bool m_considered;
  bool m_notUpdated;
};


//----------------------------------------------------------------------
// Door
class Door : public Item
{
 public:
  Door(const doorStruct* d);
  virtual ~Door();

  // virtual get method overloads
  virtual QString raceString() const;
  virtual QString classString() const;

  uint32_t zonePoint() const { return m_zonePoint; }
  
  // update methods
  void update(const doorStruct* d);

  void setZonePoint(uint32_t zonePoint) { m_zonePoint = zonePoint; }
    
 protected:
  uint32_t m_zonePoint;
};

//----------------------------------------------------------------------
// Drop
class Drop : public Item
{
 public:
  // constructor/destructor
  Drop(const makeDropStruct* d, const QString& name);
  virtual ~Drop();

  // drop specific get methods
  uint32_t itemNr() const { return m_itemNr; }
  QString idFile() const { return m_idFile; }

  // virtual get method overloads
  virtual QString raceString() const;
  virtual QString classString() const;
  
  // update methods
  void update(const makeDropStruct* d, const QString& name);

  // drop specific set methods
  void setItemNr(uint32_t itemNr) 
    {  m_itemNr = itemNr; }
  void setIdFile(const QString& idFile)
    { m_idFile = idFile; }

 protected:
  // drop specific data
  uint32_t m_itemNr;
  QString m_idFile;
};

//----------------------------------------------------------------------
// Item safe casts inlines
inline const Spawn* spawnType(const Item* item)
{
  // if this is an item of spawn type, return the pointer to Spawn, 
  // return otherwise NULL
  if (item && ((item->type() == tSpawn) || (item->type() == tPlayer)))
    return (const Spawn*)item;
  else
    return NULL; // otherwise NULL
}

inline Spawn* spawnType(Item* item)
{
  // if this is an item of spawn type, return the pointer to Spawn
  if (item && ((item->type() == tSpawn) || (item->type() == tPlayer)))
    return (Spawn*)item;
  else
    return NULL; // otherwise NULL
}


inline const Door* doorType(const Item* item)
{
  // if this is an item of door type, return the pointer to Door
  if (item && (item->type() == tDoors))
    return (const Door*)item;
  else
    return NULL; // otherwise NULL
}

inline Door* doorType(Item* item)
{
  // if this is an item of door type, return the pointer to Door
  if (item && (item->type() == tDoors))
    return (Door*)item;
  else
    return NULL; // otherwise NULL
}


inline const Drop* dropType(const Item* item)
{
  // if this is an item of drop type, return the pointer to Drop
  if (item && (item->type() == tDrop))
    return (const Drop*)item;
  else
    return NULL; // otherwise NULL
}

inline Drop* dropType(Item* item)
{
  // if this is an item of drop type, return the pointer to Drop
  if (item && (item->type() == tDrop))
    return (Drop*)item;
  else
    return NULL; // otherwise NULL
}

#endif // _SPAWN_H_

