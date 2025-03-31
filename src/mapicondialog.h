/*
 *  mapicondialog.h
 *  Copyright 2001-2003 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2005, 2019-2020 by the respective ShowEQ Developers
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

#ifndef _MAPICONDIALOG_H_
#define _MAPICONDIALOG_H_

#include "ui_mapicondialog.h"

class MapIconDialog : public QDialog, protected Ui::MapIconDialog
{
    Q_OBJECT

    public:
        MapIconDialog(QWidget* parent = 0);
        ~MapIconDialog();

    public slots:
        virtual void apply();
        virtual void revert();
        virtual void init();
        virtual void destroy();
        virtual void setMapIcons( MapIcons * mapIcons );
        virtual void mapIconCombo_activated( int id );
        virtual void imagePenColor_clicked();
        virtual void imageBrushColor_clicked();
        virtual void highlightPenColor_clicked();
        virtual void highlightBrushColor_clicked();
        virtual void line0PenColor_clicked();
        virtual void line1PenColor_clicked();
        virtual void line2PenColor_clicked();
        virtual void walkPathPenColor_clicked();
        virtual void setupMapIconDisplay();

    protected:
        MapIcons* m_mapIcons;
        MapIcon m_currentMapIcon;
        MapIcon m_currentMapIconBackup;
        MapIconType m_currentMapIconType;

};

#endif // end _MAPICONDIALOG_H_
