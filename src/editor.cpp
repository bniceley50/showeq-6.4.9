/*
 *  editor.cpp - text file editor
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

/* Implementation of text editor class */
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <QToolBar>
#include <QStatusBar>
#include <QMenu>
#include <QMenuBar>
#include <QMainWindow>
#include <QFileDialog>
#include <QToolButton>
#include <QTextStream>
#include <QPaintDevice>
#include <QObject>
#include <QTextEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QCloseEvent>

#include "util.h"
#include "toolbaricons.h"

#include "editor.h"

EditorWindow::EditorWindow(const char *fileName)
     : QMainWindow( 0 )
 {
     setObjectName("ShowEQ - Editor");
     setAttribute(Qt::WA_DeleteOnClose);
     QPixmap openIcon, saveIcon;
     openIcon = ToolbarIcons::FileOpen();
     saveIcon = ToolbarIcons::FileSave();

     fileTools = new QToolBar(this);
     fileTools->setWindowTitle( tr( "File Operations" ) );
     fileTools->addAction(openIcon, "Open File", this, SLOT(load()));
     fileTools->addAction(saveIcon, "Save File", this, SLOT(save()));
     addToolBar(fileTools);

     QMenu * file = new QMenu("&File", this);
     menuBar()->addMenu(file);

     file->addAction( openIcon, "&Open", this, SLOT(load()), Qt::CTRL+Qt::Key_O );

     file->addAction( saveIcon, "&Save", this, SLOT(save()), Qt::CTRL+Qt::Key_S );
     file->addSeparator();
     file->addAction( "&Close Editor", this, SLOT(close()), Qt::CTRL+Qt::Key_W );

     e = new QTextEdit(this);
     e->setObjectName("editor");
     e->setFocus();
     setCentralWidget( e );

     statusBar()->showMessage( "Ready", 2000 );

     resize( 600, 450 );
     filename = (QString)fileName;
     load(fileName);
 }

 EditorWindow::~EditorWindow()
 {
 }

 void EditorWindow::load()
 {
     QString fn = QFileDialog::getOpenFileName(this);
     if ( !fn.isEmpty() )
         load(fn.toLatin1().data());
     else
         statusBar()->showMessage( "File Open Cancelled", 2000 );
 }

 void EditorWindow::load( const char *fileName )
 {
     QFile f( fileName );
     if ( !f.open( QIODevice::ReadOnly ) )
         return;

     e->clear();

     QTextStream t(&f);
     while ( !t.atEnd() ) {
         QString s = t.readLine();
         e->append( s );
     }
     f.close();

     e->repaint();
     e->document()->setModified( false );
     setWindowTitle( fileName );
     QString s;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     s = QString::asprintf( "Opened %s", fileName );
#else
     s.sprintf( "Opened %s", fileName );
#endif
     statusBar()->showMessage( s, 2000 );
 }

 void EditorWindow::save()
 {
     if ( filename.isEmpty() ) {
         saveAs();
         return;
     }

     QString text = e->toPlainText();
     QFile f( filename );
     if ( !f.open( QIODevice::WriteOnly ) ) {
         statusBar()->showMessage( QString("Could not write to %1").arg(filename),
                               2000 );
         return;
     }

     QTextStream t( &f );
     t << text;
     f.close();

     e->document()->setModified( false );

     setWindowTitle( filename );

     statusBar()->showMessage( QString( "Saved %1" ).arg( filename ), 2000 );
 }

 void EditorWindow::saveAs()
 {
     QString fn = QFileDialog::getSaveFileName(this);
     if ( !fn.isEmpty() ) {
         filename = fn;
         save();
     } else {
         statusBar()->showMessage( "Saving cancelled", 2000 );
     }
 }

 void EditorWindow::closeEvent( QCloseEvent* ce )
 {
     if ( !e->document()->isModified() ) {
         ce->accept();
         return;
     }

     switch( QMessageBox::information( this, "ShowEQ Editor",
                                       "The document has been changed since "
                                       "the last save.",
                                       "Save Now", "Cancel", "Lose Changes",
                                       0, 1 ) ) {
     case 0:
         save();
         ce->accept();
         break;
     case 1:
     default: // just for sanity
         ce->ignore();
         break;
     case 2:
         ce->accept();
         break;
     }
 }

#ifndef QMAKEBUILD
#include "editor.moc"
#endif 
