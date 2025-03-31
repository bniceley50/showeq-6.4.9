/*
 *  editor.h - text file editor
 *  Copyright 2001, 2019 by the respective ShowEQ Developers
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

#ifndef EDITOR_H
#define EDITOR_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QMenu>

class QTextEdit;
class QToolBar;
class QMenu;

class EditorWindow: public QMainWindow
{
    Q_OBJECT
public:
    EditorWindow();
    EditorWindow( const char *fileName );
    ~EditorWindow();

protected:
    void closeEvent( QCloseEvent* );

private slots:
    void load();
    void load( const char *fileName );
    void save();
    void saveAs();

private:
    QTextEdit *e;
    QToolBar *fileTools;
    QString filename;
};


#endif

