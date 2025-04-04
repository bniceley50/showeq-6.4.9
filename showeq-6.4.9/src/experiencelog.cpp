/*
 *  experiencelog.cpp
 *  Copyright 2001-2007, 2019 by the respective ShowEQ Developers
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


#include "experiencelog.h"
#include "map.h"
#include "util.h"
#include "group.h"
#include "player.h"
#include "datalocationmgr.h"
#include "diagnosticmessages.h"
#include "zonemgr.h"

#include <cstdio>
#include <ctime>

#include <QTimer>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileInfo>
#include <QResizeEvent>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>

#define DEBUGEXP

#undef DEBUGEXP

ExperienceRecord::ExperienceRecord( const QString &mob_name, 
				    int mob_level,
				    long xp_gained, 
				    time_t time, 
				    const QString &zone_name, 
				    uint8_t classVal, uint8_t level, float zem,
				    float totalLevels, 
				    float groupBonus) 
  : m_class(classVal),
    m_level(level),
    m_zem(zem),
    m_totalLevels(totalLevels),
    m_groupBonus(groupBonus),
    m_zone_name(zone_name), 
    m_mob_name(mob_name),
    m_mob_level(mob_level),
    m_xp_gained(xp_gained),
    m_time(time)
{

}

const QString &ExperienceRecord::getMobName() const 
{
   return m_mob_name;

}

int ExperienceRecord::getMobLevel() const 
{
   return m_mob_level;

}

long ExperienceRecord::getExpGained() const 
{
   return m_xp_gained;

}

long ExperienceRecord::getExpValue() const 
{
   // TODO: This isn't right for all mob levels!
   //     But casey's working on it!
   return m_mob_level*m_mob_level*75;
}

long ExperienceRecord::getExpValueZEM() const 
{
  int ZEM = int(m_zem * 100);
  return m_mob_level*m_mob_level*ZEM;
}
 
long ExperienceRecord::getExpValuep() const 
{
   int p_penalty; 
   // WAR and ROG are at 10 since thier EXP is not scaled to compensate
   // for thier bonus
   switch (m_class)
   {
      case 1 : p_penalty = 10; break; // WAR
      case 2 : p_penalty = 10; break; // CLR
      case 3 : p_penalty = 14; break; // PAL
      case 4 : p_penalty = 14; break; // RNG
      case 5 : p_penalty = 14; break; // SHD
      case 6 : p_penalty = 10; break; // DRU
      case 7 : p_penalty = 12; break; // MNK
      case 8 : p_penalty = 14; break; // BRD
      case 9 : p_penalty = 10; break; // ROG
      case 10: p_penalty = 10; break; // SHM
      case 11: p_penalty = 11; break; // NEC
      case 12: p_penalty = 11; break; // WIZ
      case 13: p_penalty = 11; break; // MAG
      case 14: p_penalty = 11; break; // ENC
      default: /* why are we here? */
         p_penalty = 10; break; 
   }
   int baseExp = getExpValueZEM();
   return (int)((float)baseExp*((float)p_penalty/(float)10));
}

long ExperienceRecord::getExpValueg() const 
{
   int pExp = getExpValuep();
   int myLevel = m_level;

   return int(((float)pExp)*m_groupBonus*((float)myLevel/m_totalLevels));
}

time_t ExperienceRecord::getTime() const 
{
   return m_time;

}

const QString &ExperienceRecord::getZoneName() const 
{
   return m_zone_name;

}

ExperienceWindow::~ExperienceWindow() 
{
  if (m_log)
    ::fclose(m_log);

   qDeleteAll(m_exp_list);
   m_exp_list.clear();

   if (m_ZEM_menu)
       delete m_ZEM_menu;
   if (m_view_menu)
       delete m_view_menu;
   if (m_exp_rate_menu)
       delete m_exp_rate_menu;

}

ExperienceWindow::ExperienceWindow(const DataLocationMgr* dataLocMgr, 
				   Player* player, GroupMgr* groupMgr,
				   ZoneMgr* zoneMgr,
				   QWidget* parent, const char* name) 
  : SEQWindow("Experience", "ShowEQ - Experience", parent, name),
    m_dataLocMgr(dataLocMgr),
    m_player(player),
    m_group(groupMgr),
    m_zoneMgr(zoneMgr)
{
    if (!name)
        setObjectName("Experience");

  /* Hopefully this is only called once to set up the window,
     so this is a good place to initialize some things which
     otherwise won't be. */

   m_ratio = 1;
   m_timeframe = 0;
   m_calcZEM=0;
   m_ZEMviewtype = 0;

   QAction* tmpAction;

   m_view_menu = new QMenu("&View");
   QActionGroup* view_time_action_group = new QActionGroup(m_view_menu);

   tmpAction = m_view_menu->addAction( "&All Mobs", this, SLOT(viewAll()) );
   tmpAction->setCheckable(true);
   tmpAction->setChecked(true);
   view_time_action_group->addAction(tmpAction);
   tmpAction = m_view_menu->addAction( "Previous &15 Minutes", this, SLOT(view15Minutes()) );
   tmpAction->setCheckable(true);
   view_time_action_group->addAction(tmpAction);
   tmpAction = m_view_menu->addAction( "Previous &30 Minutes", this, SLOT(view30Minutes()) );
   tmpAction->setCheckable(true);
   view_time_action_group->addAction(tmpAction);
   tmpAction = m_view_menu->addAction( "Previous &60 Minutes", this, SLOT(view60Minutes()) );
   tmpAction->setCheckable(true);
   view_time_action_group->addAction(tmpAction);
   view_time_action_group->setExclusive(true);

   m_view_menu->addSeparator();
   QActionGroup* exp_rate_action_group = new QActionGroup(m_view_menu);
   m_exp_rate_menu = new QMenu("Experience &Rate");
   tmpAction = m_exp_rate_menu->addAction( "per &minute", this, SLOT(viewRatePerMinute()) );
   tmpAction->setCheckable(true);
   tmpAction->setChecked(true);
   exp_rate_action_group->addAction(tmpAction);
   tmpAction = m_exp_rate_menu->addAction( "per &hour", this, SLOT(viewRatePerHour()) );
   exp_rate_action_group->addAction(tmpAction);
   tmpAction->setCheckable(true);
   exp_rate_action_group->setExclusive(true);

   m_view_menu->addMenu(m_exp_rate_menu);

   m_view_menu->addSeparator();
   m_view_menu->addAction( "Clear Kills", this, SLOT(viewClear()) );

   m_view_menu->addSeparator();
   m_ZEM_menu = new QMenu("ZEM View Options");
   QActionGroup* zem_options_action_group = new QActionGroup(m_view_menu);
   tmpAction = m_ZEM_menu->addAction( "Calculated Value", this, SLOT(viewZEMcalculated()) );
   tmpAction->setCheckable(true);
   tmpAction->setChecked(true);
   zem_options_action_group->addAction(tmpAction);
   tmpAction = m_ZEM_menu->addAction( "Raw Value", this, SLOT(viewZEMraw()) );
   tmpAction->setCheckable(true);
   zem_options_action_group->addAction(tmpAction);
   tmpAction = m_ZEM_menu->addAction( "Percent Bonus", this, SLOT(viewZEMpercent()) );
   tmpAction->setCheckable(true);
   zem_options_action_group->addAction(tmpAction);
   zem_options_action_group->setExclusive(true);

   m_view_menu->addMenu(m_ZEM_menu);

   m_action_calc_zem = m_view_menu->addAction( "Calculate ZEM on next kill",
           this, SLOT(calcZEMNextKill()) );
   m_action_calc_zem->setCheckable(true);

   QWidget* mainWidget = new QWidget();
   setWidget(mainWidget);

   m_layout = new QVBoxLayout(mainWidget);
   m_layout->setContentsMargins(0, 0, 0, 0);

   m_menu_bar = new QMenuBar( this );
   m_menu_bar->addMenu(m_view_menu );
   //m_layout->addSpacing( m_menu_bar->height() + 5 );
   m_layout->addWidget(m_menu_bar);

   QGroupBox *listGBox = new QGroupBox( "Experience Log", this );
   m_layout->addWidget( listGBox );

   m_exp_listview = new SEQListView(preferenceName(), listGBox);
   m_exp_listview->addColumn("Time");
   m_exp_listview->addColumn("Mob");
   m_exp_listview->addColumn("Level");
   m_exp_listview->addColumn("Base Exp");
   m_exp_listview->addColumn("ZEM total");
   m_exp_listview->addColumn("Class total");
   m_exp_listview->addColumn("Group total");
   m_exp_listview->addColumn("Experience Gained");

   m_exp_listview->restoreColumns();

   m_exp_listview->setMinimumSize( m_exp_listview->sizeHint().width(),
      200 );

   QHBoxLayout * listGBoxLayout = new QHBoxLayout(listGBox);
   listGBoxLayout->addWidget(m_exp_listview);

   QGroupBox *statsGBox = new QGroupBox( "Statistics", this );
   QHBoxLayout *statsGBoxLayout = new QHBoxLayout(statsGBox);

   m_layout->addWidget( statsGBox );

   QGridLayout *statsGridLayout = new QGridLayout();

   statsGBoxLayout->addLayout(statsGridLayout);

   statsGridLayout->addWidget(new QLabel("Total Experience Received:", statsGBox), 0, 0);
   m_total_received = new QLabel(statsGBox);
   statsGridLayout->addWidget(m_total_received, 0, 1);

   statsGridLayout->addWidget(new QLabel("Play Time:", statsGBox), 0, 2);
   m_play_time = new QLabel(statsGBox);
   statsGridLayout->addWidget(m_play_time, 0, 3);

   statsGridLayout->addWidget(new QLabel("Total Mobs Killed:", statsGBox), 1, 0);
   m_mob_count = new QLabel(statsGBox);
   statsGridLayout->addWidget(m_mob_count, 1, 1);

   m_experience_rate_label = new QLabel( "Experience Rate (per minute):", statsGBox );
   statsGridLayout->addWidget(m_experience_rate_label, 1, 2);
   m_experience_rate = new QLabel(statsGBox);
   statsGridLayout->addWidget(m_experience_rate, 1, 3);

   statsGridLayout->addWidget(new QLabel("Average Experience per Mob:", statsGBox), 2, 0);
   m_average_per_mob = new QLabel(statsGBox);
   statsGridLayout->addWidget(m_average_per_mob, 2, 1);

   statsGridLayout->addWidget(new QLabel( "Estimated Kills To Level:", statsGBox), 2, 2);
   m_kills_to_level = new QLabel(statsGBox);
   statsGridLayout->addWidget(m_kills_to_level, 2, 3);

   statsGridLayout->addWidget(new QLabel( "Experience Remaining:", statsGBox), 3, 0);
   m_experience_remaining = new QLabel(statsGBox);
   statsGridLayout->addWidget(m_experience_remaining, 3, 1);

   statsGridLayout->addWidget(new QLabel( "Estimated Time To Level:", statsGBox ), 3, 2);
   m_time_to_level = new QLabel(statsGBox);
   statsGridLayout->addWidget(m_time_to_level, 3, 3);

   statsGridLayout->setColumnStretch( 1, 1 );
   statsGridLayout->setColumnStretch( 3, 1 );
   statsGridLayout->setSpacing( 5 );

   updateAverage( );

   // timer to update the average xp
   QTimer *timer = new QTimer( this );
   connect( timer, SIGNAL(timeout()), SLOT(updateAverage()));
   timer->start(15*1000); // calculate every 15 seconds

   QFileInfo fileInfo = m_dataLocMgr->findWriteFile("logs", "exp.log");

   m_log = fopen(fileInfo.absoluteFilePath().toLatin1().data(), "a");
   if (m_log == 0)
   {
      m_log_exp = 0;
      seqWarn("Error opening exp.log, no exp will be logged this session");
   }
   else
   {
     m_log_exp = 1;
   }

   fileInfo = m_dataLocMgr->findWriteFile("logs", "newexp.log");

   m_newExpLogFile = fileInfo.absoluteFilePath();
}

void ExperienceWindow::savePrefs()
{
  // save the SEQWindow's prefs
  SEQWindow::savePrefs();

  // save the listview's prefs
  m_exp_listview->savePrefs();
}

void ExperienceWindow::addExpRecord(const QString &mob_name,
   int mob_level, long xp_gained, QString zone_name ) 
{

   ExperienceRecord *xp = 
     new ExperienceRecord(mob_name, mob_level, xp_gained, time(0), zone_name, 
			  m_player->classVal(), m_player->level(), 
			  m_zoneMgr->zoneExpMultiplier(), 
			  m_group->totalLevels(),
			  m_group->groupBonus());

#ifdef DEBUGEXP
   resize( sizeHint() );
  qDebug("ExperienceWindow::addExpRecord()  '%s', lvl %d, exp %d",
      mob_name.toLatin1().data(), mob_level, xp_gained);
#endif

   if (m_log_exp)
      logexp(xp_gained, mob_level);

   m_exp_list.append( xp );

   // convert everything to string representations for the list view
   QString s_mob_name = mob_name;
   QString s_mob_level;
   s_mob_level.setNum(mob_level);
   QString s_xp_gained;
   s_xp_gained.setNum(xp_gained);
   QString s_xp_value;

   if (m_calcZEM)
   {
      calculateZEM(xp_gained, mob_level);
      m_calcZEM = 0;
      m_action_calc_zem->setChecked(false);
   }
   s_xp_value.setNum(xp->getExpValue());
   QString s_xp_valueZEM;
   switch (m_ZEMviewtype) {
      case 1 : s_xp_valueZEM.setNum(m_zoneMgr->zoneExpMultiplier()); break;
      case 2 : s_xp_valueZEM.setNum(((float)(m_zoneMgr->zoneExpMultiplier()-0.75)/0.75)*100);
         break;
      default: s_xp_valueZEM.setNum(xp->getExpValueZEM()); break;
   }
   QString s_xp_valuep;
   s_xp_valuep.setNum(xp->getExpValuep());
   QString s_xp_valueg;
   s_xp_valueg.setNum(xp->getExpValueg());

   char s_time[64];
   time_t timev = xp->getTime();
   strftime(s_time, 64, "%m/%d %H:%M:%S", localtime( &timev ));

   /* Update suggested by Shag */
   QStringList values;
   values << s_time << s_mob_name << s_mob_level << s_xp_value
       << s_xp_valueZEM << s_xp_valuep << s_xp_valueg << s_xp_gained;

   SEQListViewItem* new_exp_entry =
       new SEQListViewItem(m_exp_listview, values);

   m_exp_listview->setCurrentItem(new_exp_entry);

   m_exp_listview->scrollToItem(new_exp_entry);

   // Initial work on new logging mechanism with more data
   FILE* newlogfp = NULL;

   // open the file for append
   newlogfp = fopen(m_newExpLogFile.toLatin1().data(), "a");

   if (newlogfp != NULL)
   {
      // append a new record entry

      fprintf(newlogfp,
              "0\t%s\t%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%d\t%lu",
              s_time, s_mob_name.toLatin1().data(), mob_level,
              s_xp_value.toLatin1().data(), s_xp_valueZEM.toLatin1().data(),
              s_xp_valuep.toLatin1().data(), s_xp_valueg.toLatin1().data(),
              s_xp_gained.toLatin1().data(), m_player->name().toLatin1().data(),
              m_player->lastName().toLatin1().data(), m_player->level(),
              m_player->classVal(), m_group->groupSize());

      const Spawn* spawn;
      // continue with info for group members
      for (int i=0; i < MAX_GROUP_MEMBERS; i++)
      {
	spawn = m_group->memberBySlot(i);
	if (spawn)
	  fprintf(newlogfp, "\t%d", spawn->level());
      }

      // finish the record with a line
      fprintf(newlogfp, "\n"); 

      // close the file (so multiple ShowEQ instances can be up at once)
      fclose(newlogfp);
   }

   // and update the average display
   updateAverage();

}

void ExperienceWindow::updateAverage( ) 
{
   static bool force = true;

   // don't worry about updating the window if it's not even visible,
   // unless 
   if ( !force && !isVisible() )
      return;

   force = false;

   // calculate the earliest time to accept
   time_t time_cutoff = 0;
   double total_exp = 0;

   if (m_timeframe)
      time_cutoff = time(0) - (m_timeframe*60);

   // start at the end, add up the xp & mob count until we hit the
   // beginning of list
   // or the time cutoff
   QListIterator<ExperienceRecord*> it(m_exp_list);

   int mob_count = 0;
   time_t first_kill_time = 0;

   ExperienceRecord* rec;

   it.toBack();
   while (it.hasPrevious())
   {
       rec = it.previous();
       if (rec->getTime() < time_cutoff)
           break;

      total_exp+=rec->getExpGained();
      mob_count++;

      if ( rec->getTime() < first_kill_time || !first_kill_time )
         first_kill_time = rec->getTime();
   }

   // calculate the number of minutes that have passed
   double minutes = ( time(0) - first_kill_time ) / 60;
      
   if (!first_kill_time || minutes < 1)
      minutes = 0;

   // calculate and display the averages
   QString s_avg_exp;
   QString s_play_time;
   double avg_exp;

   if (!m_timeframe) {
      avg_exp = ( minutes ? total_exp/minutes : 0 );
      s_avg_exp=Commanate( (uint32_t)(avg_exp * m_ratio) );
      s_play_time.setNum( minutes );
      s_play_time+=" min";
   } 
   else 
   {
      avg_exp = total_exp/m_timeframe;
      s_avg_exp=Commanate( (uint32_t)(avg_exp * m_ratio) );
      s_play_time.setNum( m_timeframe );
      s_play_time+=" min";
   }
      
   if (m_ratio == 1)
      m_experience_rate_label->setText( "Experience Rate (per minute):" );
   else if (m_ratio == 60)
      m_experience_rate_label->setText( "Experience Rate (per hour):" );

   QString s_total_exp;
   s_total_exp=Commanate( (uint32_t)total_exp );

   QString s_mob_count;
   s_mob_count.setNum( (uint32_t)mob_count );
   
   QString s_mob_avg_exp;
   if (mob_count)
      s_mob_avg_exp=Commanate( (uint32_t)(total_exp/mob_count) );
   else
      s_mob_avg_exp="0";

   int exp_remaining;
   
   if ( (m_player->getMaxExp() > m_player->getCurrentExp()) &&
	(m_player->getCurrentExp() > 0))
   {
     /* since currentExp is calculated before maxExp when the decoder
	is broken, sometimes maxExp ends up set to zero or undefined.
	This can result in strange exp_remaining values, so some sanity
	checks have been added.  cpphack */
     exp_remaining = m_player->getMaxExp() - m_player->getCurrentExp();
   } 
   else
     exp_remaining = 0;

   QString s_exp_remaining;

   /* This now checks if the exp value is sensible, and if not
      it is displayed as "unknown". cpphack */
   if (exp_remaining > 0)
     s_exp_remaining=Commanate( exp_remaining );
   else
     s_exp_remaining="unknown";

   QString s_kills_to_level;
   if (mob_count)
      s_kills_to_level.setNum( exp_remaining / (total_exp/mob_count) );
   else
      s_kills_to_level="unknown";

   int time_to_level;
   (!avg_exp) ? time_to_level = 0 : time_to_level = (int)(exp_remaining / avg_exp);
   QString s_time_to_level;

   if ( avg_exp ) {
      if (time_to_level > 120)
         s_time_to_level = QString("%1 hours, %2 minutes")
            .arg( (int)(time_to_level/60)).arg( time_to_level % 60 );
      else if (time_to_level > 60)
         s_time_to_level = QString("1 hour, %2 minutes")
            .arg( time_to_level % 60 );
      else
         s_time_to_level = QString("%1 minutes").arg( (int)time_to_level );
   } else
      s_time_to_level="unknown";

   m_total_received->setText( s_total_exp );
   m_mob_count->setText( s_mob_count );
   m_average_per_mob->setText( s_mob_avg_exp );
   m_experience_remaining->setText( s_exp_remaining );
   
   m_play_time->setText( s_play_time );
   m_experience_rate->setText( s_avg_exp );
   m_kills_to_level->setText( s_kills_to_level );
   m_time_to_level->setText( s_time_to_level );

}

void ExperienceWindow::resizeEvent ( QResizeEvent *e ) 
{
  //   int mh = m_menu_bar->height() + 4;

   //m_layout->setGeometry( 0, mh, e->size().width(),  e->size().height()-mh );

}

void ExperienceWindow::viewAll()
{
   m_timeframe = 0;
   updateAverage();
}

void ExperienceWindow::view15Minutes()
{
   m_timeframe = 15;
   updateAverage();
}

void ExperienceWindow::view30Minutes()
{
   m_timeframe = 30;
   updateAverage();
}

void ExperienceWindow::view60Minutes()
{
   m_timeframe = 60;
   updateAverage();
}

void ExperienceWindow::viewRatePerHour()
{
   m_ratio = 60;
   updateAverage();
}

void ExperienceWindow::viewRatePerMinute()
{
   m_ratio = 1;
   updateAverage();
}

void ExperienceWindow::calcZEMNextKill()
{
   m_calcZEM = 1;
}

void ExperienceWindow::viewZEMcalculated()
{
   m_ZEMviewtype = 0;
}

void ExperienceWindow::viewZEMraw()
{
   m_ZEMviewtype = 1;
}

void ExperienceWindow::viewZEMpercent()
{
   m_ZEMviewtype = 2;
}

void ExperienceWindow::viewClear() 
{
   if (QMessageBox::information( this, "ShowEQ",
      "This function will clear all data listed in the experience "
      "log.  Do you want to continue?",
      "&OK", "&Cancel", "", 1, 1) == 0) 
   {
     clear();
   }
}

void ExperienceWindow::clear(void)
{
  m_exp_list.clear();
  m_exp_listview->clear();
}

void ExperienceWindow::logexp(long xp_gained, int mob_level) 
{
   if (!m_log_exp || (!m_log)) /* sanity */
      return;

   fprintf(m_log, "1 %d %d %ld %d %d %lu", 
	   (int)time(NULL), mob_level, 
      xp_gained, m_player->level(), 
      m_player->classVal(), m_group->groupSize());

   const Spawn* spawn;
   for (int i=0; i < MAX_GROUP_MEMBERS; i++)
   {
     spawn = m_group->memberBySlot(i);
     if (spawn)
       fprintf(m_log, " %d", spawn->level());
   }

   fprintf(m_log, "\n"); 
   fflush(m_log); /* some people like to tail -f and see live data :) */
}

void ExperienceWindow::calculateZEM(long xp_gained, int mob_level) 
{
   float gbonus=1.00;
   int penalty; 
   int myLevel = m_player->level();
   int group_ag;
   gbonus = m_group->groupBonus();
   group_ag = m_group->totalLevels();
   if (m_group->groupSize())
   {
     seqInfo("MY Level: %d GroupTot: %d BONUS   :%d", 
	     myLevel, group_ag, gbonus * 100);
   }
   // WAR and ROG are at 10 since thier EXP is not scaled to compensate
   // for thier bonus
   switch (m_player->classVal())
   {
      case 1 : penalty = 10; break; // WAR
      case 2 : penalty = 10; break; // CLR
      case 3 : penalty = 14; break; // PAL
      case 4 : penalty = 14; break; // RNG
      case 5 : penalty = 14; break; // SHD
      case 6 : penalty = 10; break; // DRU
      case 7 : penalty = 12; break; // MNK
      case 8 : penalty = 14; break; // BRD
      case 9 : penalty = 10; break; // ROG
      case 10: penalty = 10; break; // SHM
      case 11: penalty = 11; break; // NEC
      case 12: penalty = 11; break; // MAG
      case 13: penalty = 11; break; // ENC
      default: /* why are we here? */
         penalty = 10; break; 
   }
   unsigned char ZEM = (unsigned char) ((float)xp_gained*((float)((float)group_ag/(float)myLevel)*(float)((float)1.0/(float)gbonus))*((float)1/(float)(mob_level*mob_level))*((float)10/(float)penalty));
   seqInfo("xpgained: %ld group_ag: %d myLevel: %d gbonus: %d mob_level: %d penalty: %d ", xp_gained, group_ag, myLevel, gbonus, mob_level, penalty);
   seqInfo("ZEM - ZEM - ZEM ===== %d ", ZEM);
}

#ifndef QMAKEBUILD
#include "experiencelog.moc"
#endif

