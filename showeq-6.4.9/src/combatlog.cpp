/*
 *  combatlog.cpp
 *  Copyright 2002-2007, 2019 by the respective ShowEQ Developers
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

#include "combatlog.h"
#include "player.h"
#include "util.h"
#include "diagnosticmessages.h"

#include <QTimer>
#include <QGroupBox>
#include <QMessageBox>
#include <QLayout>
#include <QLabel>
#include <QGridLayout>
#include <QMenu>
#include <QVBoxLayout>
#include <cstdio>
#include <ctime>

#define DEBUGCOMBAT

#undef DEBUGCOMBAT


////////////////////////////////////////////
//  CombatOffenseRecord implementation
////////////////////////////////////////////
CombatOffenseRecord::CombatOffenseRecord( int iType, Player* p, int iSpell) :
	m_iType(iType),
	m_iSpell(iSpell),
	m_player(p),
	m_iHits(0),
	m_iMisses(0),
	m_iMinDamage(65536),
	m_iMaxDamage(0),
	m_iTotalDamage(0)
{

}


void CombatOffenseRecord::addHit(int iDamage)
{
	if(iDamage <= 0)
		return;

	m_iHits++;
	m_iTotalDamage += iDamage;

	if(iDamage > 0 && iDamage < m_iMinDamage)
		m_iMinDamage = iDamage;

	if(iDamage > m_iMaxDamage)
		m_iMaxDamage = iDamage;

}


////////////////////////////////////////////
//  CombatDefenseRecord implementation
////////////////////////////////////////////
CombatDefenseRecord::CombatDefenseRecord(Player* p) :
	m_player(p)
{
  clear();
}

void CombatDefenseRecord::clear(void)
{
  m_iHits = 0;
  m_iMisses = 0;
  m_iBlocks = 0;
  m_iParries = 0;
  m_iRipostes = 0;
  m_iDodges = 0;
  m_iMinDamage = 65536;
  m_iMaxDamage = 0;
  m_iTotalDamage = 0;
  m_iTotalAttacks = 0;
}

void CombatDefenseRecord::addHit(int iDamage)
{
	if(iDamage <= 0)
		return;

	m_iTotalAttacks++;
	m_iHits++;
	m_iTotalDamage += iDamage;

	if(iDamage > 0 && iDamage < m_iMinDamage)
		m_iMinDamage = iDamage;

	if(iDamage > m_iMaxDamage)
		m_iMaxDamage = iDamage;

}

void CombatDefenseRecord::addMiss(int iMissReason)
{
	m_iTotalAttacks++;

	switch(iMissReason)
	{
		case COMBAT_MISS:
		{
			m_iMisses++;
			break;
		}
		case COMBAT_BLOCK:
		{
			m_iBlocks++;
			break;
		}
		case COMBAT_PARRY:
		{
			m_iParries++;
			break;
		}
		case COMBAT_RIPOSTE:
		{
			m_iRipostes++;
			break;
		}
		case COMBAT_DODGE:
		{
			m_iDodges++;
			break;
		}
		default:
		{
#ifdef DEBUGCOMBAT
		  seqDebug("CombatDefenseRecord::addMiss:WARNING: invalid miss reason");
#endif
			break;
		}
	}

}


////////////////////////////////////////////
//	CombatMobRecord implementation
////////////////////////////////////////////
CombatMobRecord::CombatMobRecord(int iID, int iStartTime, Player* p) :
m_iID(iID),
m_player(p),
m_iStartTime(iStartTime),
m_iLastTime(iStartTime),
m_iDamageGiven(0),
m_dDPS(0.0),
m_iDamageTaken(0),
m_dMobDPS(0.0)
{

}

double CombatMobRecord::getDPS()
{
	int iTimeElapsed = (m_iLastTime - m_iStartTime) / 1000;

	if(iTimeElapsed > 0)
	{
		m_dDPS = (double)m_iDamageGiven / (double)iTimeElapsed;
	}

	return m_dDPS;
}

double CombatMobRecord::getMobDPS()
{
	int iTimeElapsed = (m_iLastTime - m_iStartTime) / 1000;

	if(iTimeElapsed > 0)
	{
		m_dMobDPS = (double)m_iDamageTaken / (double)iTimeElapsed;
	}

	return m_dMobDPS;
}

void CombatMobRecord::addHit(int iTarget, int iSource, int iDamage)
{

	int iPlayerID = m_player->id();

	if(iSource == iPlayerID && iTarget == m_iID)
	{
		//	update m_iLastTime
		m_iLastTime = mTime();

		if(iDamage > 0)
		{
			m_iDamageGiven += iDamage;
		}
	}
	else if(iSource == m_iID && iTarget == iPlayerID)
	{
		//	update m_iLastTime
		m_iLastTime = mTime();

		if(iDamage > 0)
		{
			m_iDamageTaken += iDamage;
		}
	}

}

////////////////////////////////////////////
//	CombatWindow implementation
////////////////////////////////////////////

CombatWindow::~CombatWindow()
{
	if(m_combat_defense_record != 0)
	{
		delete m_combat_defense_record;
		m_combat_defense_record = 0;
	}

    qDeleteAll(m_combat_offense_list);
    m_combat_offense_list.clear();

    qDeleteAll(m_combat_mob_list);
    m_combat_mob_list.clear();
}

CombatWindow::CombatWindow(Player* player,
			   QWidget* parent, const char* name)
  : SEQWindow("Combat", "ShowEQ - Combat", parent, name),
    m_player(player),
    m_iCurrentDPSTotal(0),
    m_iDPSStartTime(0),
    m_iDPSTimeLast(0),
    m_dDPS(0.0),
    m_dDPSLast(0.0)
{

    if (!name)
        setObjectName("Combat");

  /* Hopefully this is only called once to set up the window,
     so this is a good place to initialize some things which
     otherwise won't be. */

	m_combat_defense_record = new CombatDefenseRecord(player);

	initUI();
}

void CombatWindow::initUI()
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::initUI: starting...");
#endif

    QWidget* mainWidget = new QWidget();
    setWidget(mainWidget);

    QVBoxLayout* layout = new QVBoxLayout(mainWidget);
    layout->setContentsMargins(0, 0, 0, 0);

	m_menu_bar = new QMenuBar(this);
	layout->addWidget(m_menu_bar);

	m_tab = new QTabWidget(this);
	layout->addWidget(m_tab);

	m_widget_offense = initOffenseWidget();
	m_tab->addTab(m_widget_offense, "&Offense");

	m_widget_defense = initDefenseWidget();
	m_tab->addTab(m_widget_defense, "&Defense");

	m_widget_mob = initMobWidget();
	m_tab->addTab(m_widget_mob, "&Mobs");

	m_clear_menu = new QMenu("&Clear");
	m_clear_menu->addAction("Clear Offense Stats", this, SLOT(clearOffense()));
	m_clear_menu->addAction("Clear Mob Stats", this, SLOT(clearMob()));

	m_menu_bar->addMenu(m_clear_menu);

	updateOffense();
	updateDefense();
	updateMob();

#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::initUI: finished...");
#endif
}

QWidget* CombatWindow::initOffenseWidget()
{
    QWidget *pWidget = new QWidget(m_tab);

    m_layout_offense = new QVBoxLayout(pWidget);

    QGroupBox *listGBox = new QGroupBox(pWidget);
    m_layout_offense->addWidget(listGBox);

    m_listview_offense = new SEQListView(preferenceName(), listGBox);
    m_listview_offense->addColumn("Type");
    m_listview_offense->addColumn("Hit");
    m_listview_offense->addColumn("Miss");
    m_listview_offense->addColumn("Ratio");
    m_listview_offense->addColumn("Avg");
    m_listview_offense->addColumn("Min");
    m_listview_offense->addColumn("Max");
    m_listview_offense->addColumn("Total");

    m_listview_offense->restoreColumns();

    m_listview_offense->setMinimumSize(m_listview_offense->sizeHint().width(), 200);

    QHBoxLayout * listGBoxLayout = new QHBoxLayout(listGBox);
    listGBoxLayout->addWidget(m_listview_offense);

    QGroupBox *summaryGBox = new QGroupBox("Summary", pWidget);
    QHBoxLayout *summaryGBoxLayout = new QHBoxLayout(summaryGBox);

	m_layout_offense->addWidget(summaryGBox);


    QGridLayout *summaryGridLayout = new QGridLayout();

    summaryGBoxLayout->addLayout(summaryGridLayout);

    summaryGridLayout->addWidget(new QLabel("Total Damage:", summaryGBox), 0, 0);
    m_label_offense_totaldamage = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_offense_totaldamage, 0, 1);

    summaryGridLayout->addWidget(new QLabel("Avg Melee:", summaryGBox), 0, 2);
    m_label_offense_avgmelee = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_offense_avgmelee, 0, 3);

    summaryGridLayout->addWidget(new QLabel("% from Special:", summaryGBox), 1, 0);
    m_label_offense_percentspecial = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_offense_percentspecial, 1, 1);

    summaryGridLayout->addWidget(new QLabel("Avg Special:", summaryGBox), 1, 2);
    m_label_offense_avgspecial = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_offense_avgspecial, 1, 3);

    summaryGridLayout->addWidget(new QLabel("% from NonMelee:", summaryGBox), 2, 0);
    m_label_offense_percentnonmelee = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_offense_percentnonmelee, 2, 1);

    summaryGridLayout->addWidget(new QLabel("Avg NonMelee:", summaryGBox), 2, 2);
    m_label_offense_avgnonmelee = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_offense_avgnonmelee, 2, 3);

    summaryGridLayout->setColumnStretch(1, 1);
    summaryGridLayout->setColumnStretch(3, 1);
    summaryGridLayout->setSpacing(5);

	return pWidget;
}

QWidget* CombatWindow::initDefenseWidget()
{
	QWidget *pWidget = new QWidget(m_tab);
	m_layout_defense = new QVBoxLayout(pWidget);

    QGroupBox *avoidanceGBox = new QGroupBox("Avoidance", pWidget);
    QHBoxLayout *avoidanceGBoxLayout = new QHBoxLayout(avoidanceGBox);

    m_layout_defense->addWidget(avoidanceGBox);

    QGridLayout * avoidanceGridLayout = new QGridLayout();

    avoidanceGBoxLayout->addLayout(avoidanceGridLayout);

    avoidanceGridLayout->addWidget(new QLabel("Misses:", avoidanceGBox), 0, 0);
    m_label_defense_avoid_misses = new QLabel(avoidanceGBox);
    avoidanceGridLayout->addWidget(m_label_defense_avoid_misses, 0, 1);

    avoidanceGridLayout->addWidget(new QLabel("Blocks:", avoidanceGBox), 0, 2);
    m_label_defense_avoid_block = new QLabel(avoidanceGBox);
    avoidanceGridLayout->addWidget(m_label_defense_avoid_block, 0, 3);

    avoidanceGridLayout->addWidget(new QLabel("Parries:", avoidanceGBox), 0, 4);
    m_label_defense_avoid_parry = new QLabel(avoidanceGBox);
    avoidanceGridLayout->addWidget(m_label_defense_avoid_parry, 0, 5);

    avoidanceGridLayout->addWidget(new QLabel("Ripostes:", avoidanceGBox), 1, 0);
    m_label_defense_avoid_riposte = new QLabel(avoidanceGBox);
    avoidanceGridLayout->addWidget(m_label_defense_avoid_riposte, 1, 1);

    avoidanceGridLayout->addWidget(new QLabel("Dodges", avoidanceGBox), 1, 2);
    m_label_defense_avoid_dodge = new QLabel(avoidanceGBox);
    avoidanceGridLayout->addWidget(m_label_defense_avoid_dodge, 1, 3);

    avoidanceGridLayout->addWidget(new QLabel("Total:", avoidanceGBox), 1, 4);
    m_label_defense_avoid_total = new QLabel(avoidanceGBox);
    avoidanceGridLayout->addWidget(m_label_defense_avoid_total, 1, 5);

    avoidanceGridLayout->addItem(new QSpacerItem(1,1), 2, 0, 1, 6);

    avoidanceGridLayout->setColumnStretch(1, 1);
    avoidanceGridLayout->setColumnStretch(3, 1);
    avoidanceGridLayout->setColumnStretch(5, 1);
    avoidanceGridLayout->setRowStretch(2, 1);
    avoidanceGridLayout->setSpacing(5);


    QGroupBox *mitigationGBox = new QGroupBox("Mitigation", pWidget);
    QHBoxLayout *mitigationGBoxLayout = new QHBoxLayout(mitigationGBox);

    m_layout_defense->addWidget(mitigationGBox);

    QGridLayout *mitigationGridLayout = new QGridLayout();

    mitigationGBoxLayout->addLayout(mitigationGridLayout);

    mitigationGridLayout->addWidget(new QLabel("Avg. Hit:", mitigationGBox), 0, 0);
    m_label_defense_mitigate_avghit = new QLabel(mitigationGBox);
    mitigationGridLayout->addWidget(m_label_defense_mitigate_avghit, 0, 1);

    mitigationGridLayout->addWidget(new QLabel("Min:", mitigationGBox), 0, 2);
    m_label_defense_mitigate_minhit = new QLabel(mitigationGBox);
    mitigationGridLayout->addWidget(m_label_defense_mitigate_minhit, 0, 3);

    mitigationGridLayout->addWidget(new QLabel("Max:", mitigationGBox), 0, 4);
    m_label_defense_mitigate_maxhit = new QLabel(mitigationGBox);
    mitigationGridLayout->addWidget(m_label_defense_mitigate_maxhit, 0, 5);

    mitigationGridLayout->addItem(new QSpacerItem(1,1), 1, 0, 1, 6);

    mitigationGridLayout->setColumnStretch(1, 1);
    mitigationGridLayout->setColumnStretch(3, 1);
    mitigationGridLayout->setColumnStretch(5, 1);
    mitigationGridLayout->setRowStretch(1, 1);
    mitigationGridLayout->setSpacing(5);



    QGroupBox *summaryGBox = new QGroupBox("Summary", pWidget);
    QHBoxLayout *summaryGBoxLayout = new QHBoxLayout(summaryGBox);

    m_layout_defense->addWidget(summaryGBox);

    QGridLayout *summaryGridLayout = new QGridLayout();

    summaryGBoxLayout->addLayout(summaryGridLayout);

    summaryGridLayout->addWidget(new QLabel("Mob Attacks:", summaryGBox), 0, 0);
    m_label_defense_summary_mobattacks = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_defense_summary_mobattacks, 0, 1);

    summaryGridLayout->addWidget(new QLabel("% Avoided:", summaryGBox), 0, 2);
    m_label_defense_summary_percentavoided = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_defense_summary_percentavoided, 0, 3);

    summaryGridLayout->addWidget(new QLabel("Total Damage:", summaryGBox), 0, 4);
    m_label_defense_summary_totaldamage = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_defense_summary_totaldamage, 0, 5);

    summaryGridLayout->addItem(new QSpacerItem(1,1), 2, 0, 1, 6);

    summaryGridLayout->setColumnStretch(1, 1);
    summaryGridLayout->setColumnStretch(3, 1);
    summaryGridLayout->setColumnStretch(5, 1);
    summaryGridLayout->setRowStretch(1, 1);
    summaryGridLayout->setSpacing(5);

	return pWidget;
}

QWidget* CombatWindow::initMobWidget()
{
    QWidget *pWidget = new QWidget(m_tab);

    m_layout_mob = new QVBoxLayout(pWidget);

    QGroupBox *listGBox = new QGroupBox(pWidget);
    m_layout_mob->addWidget(listGBox);

    m_listview_mob = new SEQListView(preferenceName(), listGBox);
    m_listview_mob->addColumn("Time");
    m_listview_mob->addColumn("Name");
    m_listview_mob->addColumn("ID");
    m_listview_mob->addColumn("Duration");
    m_listview_mob->addColumn("Damage Given");
    m_listview_mob->addColumn("DPS");
    m_listview_mob->addColumn("Damage Taken");
    m_listview_mob->addColumn("MOB DPS");

    m_listview_mob->restoreColumns();

    m_listview_mob->setMinimumSize(m_listview_mob->sizeHint().width(), 200);

    QHBoxLayout *listGBoxLayout = new QHBoxLayout(listGBox);
    listGBoxLayout->addWidget(m_listview_mob);

    QGroupBox *summaryGBox = new QGroupBox("Summary", pWidget);
    QHBoxLayout *summaryGBoxLayout = new QHBoxLayout(summaryGBox);

    m_layout_mob->addWidget(summaryGBox);

    QGridLayout *summaryGridLayout = new QGridLayout();

    summaryGBoxLayout->addLayout(summaryGridLayout);

    summaryGridLayout->addWidget(new QLabel("Total Mobs", summaryGBox), 0, 0);
    m_label_mob_totalmobs = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_mob_totalmobs, 0, 1);

    summaryGridLayout->addWidget(new QLabel("Avg DPS:", summaryGBox), 0, 2);
    m_label_mob_avgdps = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_mob_avgdps, 0, 3);

    summaryGridLayout->addWidget(new QLabel("Current DPS:", summaryGBox), 1, 0);
    m_label_mob_currentdps = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_mob_currentdps, 1, 1);

    summaryGridLayout->addWidget(new QLabel("Last DPS:", summaryGBox), 1, 2);
    m_label_mob_lastdps = new QLabel(summaryGBox);
    summaryGridLayout->addWidget(m_label_mob_lastdps, 1, 3);

    summaryGridLayout->setColumnStretch(1, 1);
    summaryGridLayout->setColumnStretch(3, 1);
    summaryGridLayout->setSpacing(5);


	return pWidget;
}

void CombatWindow::savePrefs()
{
  // save the SEQWindow's prefs
  SEQWindow::savePrefs();

  // save the SEQListViews' prefs
  m_listview_mob->savePrefs();
  m_listview_offense->savePrefs();
}

void CombatWindow::updateOffense()
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::updateOffense starting...");
#endif


	QString s_totaldamage;
	QString s_percentspecial;
	QString s_percentnonmelee;
	QString s_avgmelee;
	QString s_avgspecial;
	QString s_avgnonmelee;

	int iTotalDamage = 0;
	int iTotalHits = 0;
	double dPercentSpecial = 0.0;
	double dPercentNonmelee = 0.0;
	double dAvgMelee = 0.0;
	double dAvgSpecial = 0.0;
	double dAvgNonmelee = 0.0;

	int iMeleeDamage = 0;
	int iMeleeHits = 0;
	int iSpecialDamage = 0;
	int iSpecialHits = 0;
	int iNonmeleeDamage = 0;
	int iNonmeleeHits = 0;


	//	empty the list so we can repopulate
	m_listview_offense->clear();

	CombatOffenseRecord *pRecord;

    QList<CombatOffenseRecord*>::iterator it;
    for(it = m_combat_offense_list.begin(); it != m_combat_offense_list.end(); ++it)
    {
        pRecord = *it;
        if (!pRecord)
            break;

        int iType = pRecord->getType();
		int iSpell = pRecord->getSpell();
		int iHits = pRecord->getHits();
		int iMisses = pRecord->getMisses();
		int iMinDamage = pRecord->getMinDamage();
		int iMaxDamage = pRecord->getMaxDamage();
		int iDamage = pRecord->getTotalDamage();

		double dAvgDamage = (double)iDamage / (double)iHits;
		double dRatio = (double)iHits / (double)iMisses;

		QString s_type;
		// Belith -- Damage shields are strange!
		switch(iType)
		{
			case 0:		// 1H Blunt
			case 1:		// 1H Slashing
			case 2:		// 2H Blunt
			case 3:		// 2H Slashing
			case 28:	// Hand To Hand
			case 36:	// Piercing
			case 7:		// Archery
			case 8:		// Backstab
			case 10:	// Bash
			case 21:	// Dragon Punch
			case 23:	// Eagle Strike
			case 26:	// Flying Kick
			case 30:	// Kick
			case 38:	// Round Kick
			case 51:	// Throwing
			case 52:	// Tiger Claw
			{
				// this is a normal skill
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
				s_type = QString::asprintf("%s(%d)", skill_name(iType).toLatin1().data(), iType);
#else
				s_type.sprintf("%s(%d)", skill_name(iType).toLatin1().data(), iType);
#endif
				break;
			}
			case 231:       // Non Melee Damage
			{
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
				s_type = QString::asprintf("Spell: %s(%d)", spell_name(iSpell).toLatin1().data(), iSpell);
#else
				s_type.sprintf("Spell: %s(%d)", spell_name(iSpell).toLatin1().data(), iSpell);
#endif
				break;
			}
			default:        // Damage Shield?
			{
				// 245 Mark of Retribution
				// 248 Flameshield of Ro? (45pt) (mage)
				// -11 Killing Blow with MoR
				// -8  Killing Blow with Ro? (45pt) (mage)
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
				s_type = QString::asprintf("Damage Shield: (%d)", iType);
#else
				s_type.sprintf("Damage Shield: (%d)", iType);
#endif
				break;
			}
		}
		QString s_hits;
		s_hits.setNum(iHits);
		QString s_misses;
		s_misses.setNum(iMisses);
		QString s_ratio;
		s_ratio = QString("%1 to 1").arg(dRatio);
		QString s_avgdamage;
		s_avgdamage.setNum(dAvgDamage);
		QString s_mindamage;
		s_mindamage.setNum(iMinDamage);
		QString s_maxdamage;
		s_maxdamage.setNum(iMaxDamage);
		QString s_damage;
		s_damage.setNum(iDamage);

        QStringList item_values;
        item_values << s_type << s_hits << s_misses << s_ratio
            << s_avgdamage << s_mindamage << s_maxdamage << s_damage;

        SEQListViewItem *pItem = new SEQListViewItem(m_listview_offense, item_values);

        pItem->setTextAlignment(0, Qt::AlignRight);
        pItem->setTextAlignment(1, Qt::AlignRight);
        pItem->setTextAlignment(2, Qt::AlignRight);
        pItem->setTextAlignment(3, Qt::AlignRight);
        pItem->setTextAlignment(4, Qt::AlignRight);
        pItem->setTextAlignment(5, Qt::AlignRight);
        pItem->setTextAlignment(6, Qt::AlignRight);
        pItem->setTextAlignment(7, Qt::AlignRight);


		switch(iType)
		{
			case 0:		// 1H Blunt
			case 1:		// 1H Slashing
			case 2:		// 2H Blunt
			case 3:		// 2H Slashing
			case 28:	// Hand To Hand
			case 36:	// Piercing
			{
				iMeleeDamage += iDamage;
				iMeleeHits += iHits;
				break;
			}
			case 7:		// Archery
			case 8:		// Backstab
			case 10:	// Bash
			case 21:	// Dragon Punch
			case 23:	// Eagle Strike
			case 26:	// Flying Kick
			case 30:	// Kick
			case 38:	// Round Kick
			case 51:	// Throwing
			case 52:	// Tiger Claw
			{
				iSpecialDamage += iDamage;
				iSpecialHits += iHits;
				break;
			}
			default:
			{
				iNonmeleeDamage += iDamage;
				iNonmeleeHits += iHits;
				break;
			}
		}
	}

	iTotalDamage = iMeleeDamage + iSpecialDamage + iNonmeleeDamage;
	iTotalHits = iMeleeHits + iSpecialHits + iNonmeleeHits;

	dPercentSpecial = ((double)iSpecialDamage / (double)iTotalDamage) * 100.0;
	dPercentNonmelee = ((double)iNonmeleeDamage / (double)iTotalDamage) * 100.0;

	dAvgMelee = (double)iMeleeDamage / (double)iMeleeHits;
	dAvgSpecial = (double)iSpecialDamage / (double)iSpecialHits;
	dAvgNonmelee = (double)iNonmeleeDamage / (double)iNonmeleeHits;

	s_totaldamage.setNum(iTotalDamage);
	s_percentspecial.setNum(dPercentSpecial);
	s_percentnonmelee.setNum(dPercentNonmelee);
	s_avgmelee.setNum(dAvgMelee);
	s_avgspecial.setNum(dAvgSpecial);
	s_avgnonmelee.setNum(dAvgNonmelee);

	m_label_offense_totaldamage->setText(s_totaldamage);
	m_label_offense_percentspecial->setText(s_percentspecial);
	m_label_offense_percentnonmelee->setText(s_percentnonmelee);
	m_label_offense_avgmelee->setText(s_avgmelee);
	m_label_offense_avgspecial->setText(s_avgspecial);
	m_label_offense_avgnonmelee->setText(s_avgnonmelee);


#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::updateOffense finished...");
#endif

}

void CombatWindow::updateDefense()
{
	int iMisses = m_combat_defense_record->getMisses();
	int iBlocks = m_combat_defense_record->getBlocks();
	int iParries = m_combat_defense_record->getParries();
	int iRipostes = m_combat_defense_record->getRipostes();
	int iDodges = m_combat_defense_record->getDodges();
	int iTotalAvoid = iMisses+iBlocks+iParries+iRipostes+iDodges;

	double dAvgHit = (double)m_combat_defense_record->getTotalDamage() / (double)m_combat_defense_record->getHits();
	int iMinHit = m_combat_defense_record->getMinDamage();
	int iMaxHit = m_combat_defense_record->getMaxDamage();

	int iMobAttacks = m_combat_defense_record->getTotalAttacks();
	double dAvoided = ((double)iTotalAvoid / (double)iMobAttacks) * 100.0;
	int iTotalDamage = m_combat_defense_record->getTotalDamage();


	m_label_defense_avoid_misses->setText(QString::number(iMisses));
	m_label_defense_avoid_block->setText(QString::number(iBlocks));
	m_label_defense_avoid_parry->setText(QString::number(iParries));
	m_label_defense_avoid_riposte->setText(QString::number(iRipostes));
	m_label_defense_avoid_dodge->setText(QString::number(iDodges));
	m_label_defense_avoid_total->setText(QString::number(iTotalAvoid));
	m_label_defense_mitigate_avghit->setText(QString::number(dAvgHit));
	m_label_defense_mitigate_minhit->setText(QString::number(iMinHit));
	m_label_defense_mitigate_maxhit->setText(QString::number(iMaxHit));
	m_label_defense_summary_mobattacks->setText(QString::number(iMobAttacks));
	m_label_defense_summary_percentavoided->setText(QString::number(dAvoided));
	m_label_defense_summary_totaldamage->setText(QString::number(iTotalDamage));

}

void CombatWindow::updateMob()
{

	int iTotalMobs = 0;
	double dAvgDPS = 0.0;
	double dDPSSum = 0.0;

	//	empty the list so we can repopulate
	m_listview_mob->clear();

	CombatMobRecord *pRecord;

    QList<CombatMobRecord*>::iterator it;
    for(it = m_combat_mob_list.begin(); it != m_combat_mob_list.end(); ++it)
    {
        pRecord = *it;
        if (!pRecord)
            break;
        int iID = pRecord->getID();
		int iDuration = pRecord->getDuration() / 1000;
		int iDamageGiven = pRecord->getDamageGiven();
		double dDPS = pRecord->getDPS();
		int iDamageTaken = pRecord->getDamageTaken();
		double dMobDPS = pRecord->getMobDPS();

		char s_time[64];
		time_t timev = pRecord->getTime();
		strftime(s_time, 64, "%m/%d %H:%M:%S", localtime(&timev));
		QString s_name = pRecord->getName();
		QString s_id = QString::number(iID);
		QString s_duration = QString::number(iDuration);
		QString s_damagegiven = QString::number(iDamageGiven);
		QString s_dps = QString::number(dDPS);
		QString s_iDamageTaken = QString::number(iDamageTaken);
		QString s_mobdps = QString::number(dMobDPS);

        QStringList item_values;
        item_values << s_time << s_name << s_id << s_duration << s_damagegiven
            << s_dps << s_iDamageTaken << s_mobdps;

        SEQListViewItem* pItem = new SEQListViewItem(m_listview_mob, item_values);

        pItem->setTextAlignment(0, Qt::AlignRight);
        pItem->setTextAlignment(1, Qt::AlignRight);
        pItem->setTextAlignment(2, Qt::AlignRight);
        pItem->setTextAlignment(3, Qt::AlignRight);
        pItem->setTextAlignment(4, Qt::AlignRight);
        pItem->setTextAlignment(5, Qt::AlignRight);
        pItem->setTextAlignment(6, Qt::AlignRight);
        pItem->setTextAlignment(7, Qt::AlignRight);


		iTotalMobs++;
		dDPSSum += dDPS;
	}

	if (iTotalMobs)
	  dAvgDPS = dDPSSum / (double)iTotalMobs;
	else
	  dAvgDPS = 0;

	m_label_mob_totalmobs->setText(QString::number(iTotalMobs));
	m_label_mob_avgdps->setText(QString::number(dAvgDPS));
	m_label_mob_currentdps->setText(QString::number(m_dDPS));
	m_label_mob_lastdps->setText(QString::number(m_dDPSLast));

}

void CombatWindow::addCombatRecord(int iTargetID, int iSourceID, int iType, int iSpell, int iDamage, QString tName, QString sName)
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addCombatRecord starting...");
	seqDebug("target=%d, source=%d, type=%d, spell=%d, damage=%d",
			iTargetID, iSourceID, iType, iSpell, iDamage);
#endif

	int iPlayerID = m_player->id();

	//	The one case we won't handle (for now) is where the Target
	//	and Source are the same.

	if(iTargetID == iPlayerID && iSourceID != iPlayerID)
	{
		addDefenseRecord(iDamage);
		updateDefense();
		addMobRecord(iTargetID, iSourceID, iDamage, tName, sName);
		updateMob();
	}
	else if(iSourceID == iPlayerID && iTargetID != iPlayerID)
	{
		addOffenseRecord(iType, iDamage, iSpell);
		updateOffense();
		// Belith -- Lets not add buffs, etc
		if ((iType == 231 && iDamage > 0) || iType != 231) {
			addMobRecord(iTargetID, iSourceID, iDamage, tName, sName);
			updateMob();
		}

		if(iDamage > 0)
			updateDPS(iDamage);
	}

#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addCombatRecord finished...");
#endif
}

void CombatWindow::addOffenseRecord(int iType, int iDamage, int iSpell)
{

#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addOffenseRecord starting...");
#endif

	bool bFoundRecord = false;

	CombatOffenseRecord *pRecord;

    QList<CombatOffenseRecord*>::iterator it;

    for(it = m_combat_offense_list.begin(); it != m_combat_offense_list.end(); ++it)
    {
        pRecord = *it;
        if (!pRecord)
            break;
        // Belith -- Lets match spells up as well
        if(pRecord->getType() == iType && pRecord->getType() != 231)
		{
			bFoundRecord = true;
			break;
		}
		if(pRecord->getType() == iType && pRecord->getType() == 231
			&& pRecord->getSpell() == iSpell)
		{
			bFoundRecord = true;
			break;
		}
	}

	if(!bFoundRecord)
	{
		// Belith -- Again lets skip buffs, etc
		if ((iDamage > 0 && iType == 231) || iType != 231) {
			pRecord = new CombatOffenseRecord(iType, m_player, iSpell);
			m_combat_offense_list.append(pRecord);
		}
	}

	if(iDamage > 0)
	{
		pRecord->addHit(iDamage);
	}
	else if (iType != 231)
	{
		pRecord->addMiss(iDamage);
	}

#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addOffenseRecord finished...");
#endif
}


void CombatWindow::addDefenseRecord(int iDamage)
{
	if(iDamage > 0)
		m_combat_defense_record->addHit(iDamage);
	else
		m_combat_defense_record->addMiss(iDamage);

}

void CombatWindow::addMobRecord(int iTargetID, int iSourceID, int iDamage, QString tName, QString sName)
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addMobRecord starting...");
#endif

	int iTimeNow = mTime();
	int iPlayerID = m_player->id();
	int iMobID;
	QString mobName;

	if(iPlayerID == iTargetID)
	{
		iMobID = iSourceID;
		mobName = sName;
	}
	else if(iPlayerID == iSourceID)
	{
		iMobID = iTargetID;
		mobName = tName;
	}
	else
	{
		//invalid record
		return;
	}


	bool bFoundRecord = false;

	CombatMobRecord *pRecord;

    QList<CombatMobRecord*>::iterator it;
    for(it = m_combat_mob_list.begin(); it != m_combat_mob_list.end(); ++it)
    {
        pRecord = *it;
        if (!pRecord)
            break;

        if(pRecord->getID() == iMobID)
		{
			bFoundRecord = true;
			break;
		}
	}

	if(!bFoundRecord)
	{
		pRecord = new CombatMobRecord(iMobID, iTimeNow, m_player);
		pRecord->setName(mobName);
		m_combat_mob_list.append(pRecord);
	}
	pRecord->setTime(time(0));
	pRecord->addHit(iTargetID, iSourceID, iDamage);


#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addMobRecord finished...");
#endif
}


void CombatWindow::updateDPS(int iDamage)
{

	int iTimeNow = mTime();

	//	reset if it's been 10 seconds without an update
	if(iTimeNow > (m_iDPSTimeLast + 10000))
	{
		//	reset DPS
		m_dDPSLast = m_dDPS;
		m_dDPS = 0;
		m_iDPSStartTime = iTimeNow;
		m_iCurrentDPSTotal = 0;
	}

	m_iDPSTimeLast = mTime();
	m_iCurrentDPSTotal += iDamage;

	int iTimeElapsed = (iTimeNow - m_iDPSStartTime) / 1000;

	if(iTimeElapsed > 0)
	{
		m_dDPS = (double)m_iCurrentDPSTotal / (double)iTimeElapsed;
	}

	m_label_mob_currentdps->setText(QString::number(m_dDPS));
	m_label_mob_lastdps->setText(QString::number(m_dDPSLast));


}


void CombatWindow::resetDPS()
{
	//	we'll let updateDPS do all the work
	//	by simply setting m_iDPSTimeLast to 0

	m_iDPSTimeLast = 0;

	updateDPS(0);
}

void CombatWindow::clearMob()
{
	switch( QMessageBox::information( this, "ShowEQ",
		"This function will clear all data listed on the mob "
		"tab.  Do you want to continue?",
		"&OK", "&Cancel", "", 1, 1 ) )
	{
		case 0:
            qDeleteAll(m_combat_mob_list);
			m_combat_mob_list.clear();
			updateMob();
			break;
		default:
			break;
	}
}

void CombatWindow::clearOffense()
{
	switch( QMessageBox::information( this, "ShowEQ",
		"This function will clear all data listed on the offense "
		"tab.  Do you want to continue?",
		"&OK", "&Cancel", "", 1, 1 ) )
	{
		case 0:
            qDeleteAll(m_combat_offense_list);
			m_combat_offense_list.clear();
			updateOffense();
			break;
		default:
			break;
	}
}

void CombatWindow::clear(void)
{
  m_combat_mob_list.clear();
  updateMob();
  m_combat_offense_list.clear();
  updateOffense();
  m_combat_defense_record->clear();
  updateDefense();
  resetDPS();
}

#ifndef QMAKEBUILD
#include "combatlog.moc"
#endif

