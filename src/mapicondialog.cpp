/*
 *  mapicondialog.cpp
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

#include "mapicondialog.h"
#include <QPixmap>
#include <QPainter>
#include <QColorDialog>

MapIconDialog::MapIconDialog(QWidget* parent):
    QDialog(parent)
{
    setupUi(this);
    init();
}

MapIconDialog::~MapIconDialog() {
    destroy();
}


void MapIconDialog::apply()
{
    QPen pen;
    QBrush brush;

    // get image settings
    m_currentMapIcon.setImage(m_useImage->isChecked());
    m_currentMapIcon.setImageStyle(MapIconStyle(m_imageImage->currentIndex()));
    m_currentMapIcon.setImageSize(MapIconSize(m_imageSize->currentIndex()));
    m_currentMapIcon.setImageFlash(m_imageFlash->isChecked());
    m_currentMapIcon.setImageUseSpawnColorPen(m_imageUseSpawnColorPen->isChecked());
    pen = m_currentMapIcon.imagePen();
    pen.setStyle(Qt::PenStyle(m_imagePenStyle->currentIndex()));
    pen.setWidth(m_imagePenWidth->value());
    m_currentMapIcon.setImagePen(pen);
    m_currentMapIcon.setImageUseSpawnColorBrush(m_imageUseSpawnColorBrush->isChecked());
    brush = m_currentMapIcon.imageBrush();
    brush.setStyle(Qt::BrushStyle(m_imageBrushStyle->currentIndex()));
    m_currentMapIcon.setImageBrush(brush);

    // get highlight settings
    m_currentMapIcon.setHighlight(m_useHighlight->isChecked());
    m_currentMapIcon.setHighlightStyle(MapIconStyle(m_highlightImage->currentIndex()));
    m_currentMapIcon.setHighlightSize(MapIconSize(m_highlightSize->currentIndex()));
    m_currentMapIcon.setHighlightFlash(m_highlightFlash->isChecked());
    m_currentMapIcon.setHighlightUseSpawnColorPen(m_highlightUseSpawnColorPen->isChecked());
    pen = m_currentMapIcon.highlightPen();
    pen.setStyle(Qt::PenStyle(m_highlightPenStyle->currentIndex()));
    pen.setWidth(m_highlightPenWidth->value());
    m_currentMapIcon.setHighlightPen(pen);
    m_currentMapIcon.setHighlightUseSpawnColorBrush(m_highlightUseSpawnColorBrush->isChecked());
    brush = m_currentMapIcon.highlightBrush();
    brush.setStyle(Qt::BrushStyle(m_highlightBrushStyle->currentIndex()));
    m_currentMapIcon.setHighlightBrush(brush);

    // get line settings
    m_currentMapIcon.setShowLine0(m_showLine0->isChecked());
    pen = m_currentMapIcon.line0Pen();
    pen.setStyle(Qt::PenStyle(m_line0PenStyle->currentIndex()));
    pen.setWidth(m_line0PenWidth->value());
    m_currentMapIcon.setLine0Pen(pen);
    m_currentMapIcon.setLine1Distance(m_line1Distance->value());
    pen = m_currentMapIcon.line1Pen();
    pen.setStyle(Qt::PenStyle(m_line1PenStyle->currentIndex()));
    pen.setWidth(m_line1PenWidth->value());
    m_currentMapIcon.setLine1Pen(pen);
    m_currentMapIcon.setLine2Distance(m_line2Distance->value());
    pen = m_currentMapIcon.line2Pen();
    pen.setStyle(Qt::PenStyle(m_line2PenStyle->currentIndex()));
    pen.setWidth(m_line2PenWidth->value());
    m_currentMapIcon.setLine2Pen(pen);

    // get general settings
    m_currentMapIcon.setShowName(m_showName->isChecked());
    m_currentMapIcon.setShowWalkPath(m_showWalkPath->isChecked());
    m_currentMapIcon.setUseWalkPathPen(m_useWalkPathPen->isChecked());
    pen = m_currentMapIcon.walkPathPen();
    pen.setStyle(Qt::PenStyle(m_walkPathPenStyle->currentIndex()));
    pen.setWidth(m_walkPathPenWidth->value());
    m_currentMapIcon.setWalkPathPen(pen);

    // make sure the map icons object has been set
    if (m_mapIcons)
    {
        // set the current map icon types settings
        m_mapIcons->setIcon(m_currentMapIconType, m_currentMapIcon);

        // retrieve the current map icon types settings (just to be sure)
        m_currentMapIcon = m_mapIcons->icon(m_currentMapIconType);
    }

    // re-setup the display
    setupMapIconDisplay();
}

void MapIconDialog::revert()
{
    // revert the map icon data to the last backup
    m_currentMapIcon = m_currentMapIconBackup;

    // re-setup the display
    setupMapIconDisplay();
}


void MapIconDialog::init()
{

    QString temp;
    // setup the map icons combo box
    for (int i = tIconTypeDrop; i <= tIconTypeMax; i++)
      m_mapIconCombo->insertItem(i-1, MapIcons::iconTypeName((MapIconType)i));

    int sizeWH = m_mapIconCombo->height() - 8;
    int size = sizeWH >> 1;
    QPoint point(size, size);
    // setup the image styles
    QPixmap pix(QSize(sizeWH+1, sizeWH+1));
    QPen pen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
    for (int i = tIconStyleNone; i <= tIconStyleMax; i++)
    {
        pix.fill(Qt::white);
        QPainter p(&pix);
        p.setPen(pen);
        p.setBrush(QBrush(Qt::gray));
        MapIcon::paintIconImage(MapIconStyle(i), p, point, size, sizeWH);
        p.end();

        temp = MapIcon::iconStyleName((MapIconStyle)i);
        m_imageImage->insertItem(i, pix, temp);
        m_highlightImage->insertItem(i, pix, temp);
    }

    // setup the image sizes
    for (int i = tIconSizeNone; i <= tIconSizeMax; i++)
    {
        temp = MapIcon::iconSizeName((MapIconSize)i);
        m_imageSize->insertItem(i, temp);
        m_highlightSize->insertItem(i, temp);
    }

    const QString penStyleNames[] =
    {
        "None",
        "Solid",
        "Dash",
        "Dot",
        "Dash Dot",
        "Dash Dot Dot"
    };

    // setup pen style names
    pen = QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
    for (int i = Qt::NoPen; i <= Qt::DashDotDotLine; i++)
    {
        pix.fill(Qt::white);
        QPainter p(&pix);
        pen.setStyle(Qt::PenStyle(i));
        p.setPen(pen);
        p.setBrush(QBrush(Qt::gray));
        p.drawLine(point.x() - size, point.y() - size,
                point.x() + size, point.y() + size);
        p.end();

        m_imagePenStyle->insertItem(i, pix, penStyleNames[i]);
        m_highlightPenStyle->insertItem(i, pix, penStyleNames[i]);
        m_line0PenStyle->insertItem(i, pix, penStyleNames[i]);
        m_line1PenStyle->insertItem(i, pix, penStyleNames[i]);
        m_line2PenStyle->insertItem(i, pix, penStyleNames[i]);
        m_walkPathPenStyle->insertItem(i, pix, penStyleNames[i]);
    }

    const QString brushStyleNames[] =
    {
        "None",
        "Solid",
        "94% Fill",
        "88% Fill",
        "63% Fill",
        "50% Fill",
        "37% Fill",
        "12% Fill",
        "6 % Fill",
        "Horizontal Lines",
        "Vertical Lines",
        "Crossing Lines",
        "Diagonal Lines /",
        "Diagonal Lines \\",
        "Diagonal Cross Lines",
    };

    // setup brush style names
    pen = QPen(Qt::black, 0, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
    pen.setWidth(0);
    for (int i = Qt::NoBrush; i <= Qt::DiagCrossPattern; i++)
    {
        pix.fill(Qt::white);
        QPainter p(&pix);
        p.setPen(pen);
        p.setBrush(QBrush(Qt::BrushStyle(i)));
        p.drawRect(point.x() - size, point.y() - size, sizeWH, sizeWH);
        p.end();

        m_imageBrushStyle->insertItem(i, pix, brushStyleNames[i]);
        m_highlightBrushStyle->insertItem(i, pix, brushStyleNames[i]);
    }

    // setup the display
    setupMapIconDisplay();

}

void MapIconDialog::destroy()
{

}

void MapIconDialog::setMapIcons(MapIcons* mapIcons)
{
    // set the map icons member
    m_mapIcons = mapIcons;

    // set the current map icon type
    m_currentMapIconType = MapIconType(m_mapIconCombo->currentIndex() + 1);
    m_currentMapIcon = m_mapIcons->icon(m_currentMapIconType);
    m_currentMapIconBackup = m_currentMapIcon;

    // setup the map icon display
    setupMapIconDisplay();
}

void MapIconDialog::mapIconCombo_activated( int id)
{
    // validate state and input
    if (!m_mapIcons || (id >= tIconTypeMax))
        return;

    // set the current map icon
    m_currentMapIconType = MapIconType(id + 1);
    m_currentMapIcon = m_mapIcons->icon(m_currentMapIconType);
    m_currentMapIconBackup = m_currentMapIcon;

    // setup the display
    setupMapIconDisplay();
}

void MapIconDialog::imagePenColor_clicked()
{
    QColor color = QColorDialog::getColor(m_currentMapIcon.imagePen().color(),
            this, windowTitle() + " Image Outline");

    if (color.isValid())
    {
        QPen pen = m_currentMapIcon.imagePen();
        pen.setColor(color);
        m_currentMapIcon.setImagePen(pen);
        QPalette p = m_imagePenColorSample->palette();
        p.setColor(m_imagePenColorSample->backgroundRole(), color);
        m_imagePenColorSample->setPalette(p);
    }
}

void MapIconDialog::imageBrushColor_clicked()
{
    QColor color = QColorDialog::getColor(m_currentMapIcon.imageBrush().color(),
            this, windowTitle() + " Image Fill");

    if (color.isValid())
    {
        QBrush brush = m_currentMapIcon.imageBrush();
        brush.setColor(color);
        m_currentMapIcon.setImageBrush(brush );
        QPalette p = m_imageBrushColorSample->palette();
        p.setColor(m_imageBrushColorSample->backgroundRole(), color);
        m_imageBrushColorSample->setPalette(p);
    }
}

void MapIconDialog::highlightPenColor_clicked()
{
    QColor color = QColorDialog::getColor(m_currentMapIcon.highlightPen().color(),
            this, windowTitle() + " Highlight Outline");

    if (color.isValid())
    {
        QPen pen = m_currentMapIcon.highlightPen();
        pen.setColor(color);
        m_currentMapIcon.setHighlightPen(pen);
        QPalette p = m_highlightPenColorSample->palette();
        p.setColor(m_highlightPenColorSample->backgroundRole(), color);
        m_highlightPenColorSample->setPalette(p);
    }
}

void MapIconDialog::highlightBrushColor_clicked()
{
    QColor color = QColorDialog::getColor(m_currentMapIcon.highlightBrush().color(),
            this, windowTitle() + " Highlight Fill");

    if (color.isValid())
    {
        QBrush brush = m_currentMapIcon.highlightBrush();
        brush.setColor(color);
        m_currentMapIcon.setHighlightBrush(brush );
        QPalette p = m_highlightBrushColorSample->palette();
        p.setColor(m_highlightBrushColorSample->backgroundRole(), color);
        m_highlightBrushColorSample->setPalette(p);
    }
}

void MapIconDialog::line0PenColor_clicked()
{
    QColor color = QColorDialog::getColor(m_currentMapIcon.line0Pen().color(),
            this, windowTitle() + " Line 0");

    if (color.isValid())
    {
        QPen pen = m_currentMapIcon.line0Pen();
        pen.setColor(color);
        m_currentMapIcon.setLine0Pen(pen);
        QPalette p = m_line0PenColorSample->palette();
        p.setColor(m_line0PenColorSample->backgroundRole(), color);
        m_line0PenColorSample->setPalette(p);
    }
}

void MapIconDialog::line1PenColor_clicked()
{
    QColor color = QColorDialog::getColor(m_currentMapIcon.line1Pen().color(),
            this, windowTitle() + " Line 1");

    if (color.isValid())
    {
        QPen pen = m_currentMapIcon.line1Pen();
        pen.setColor(color);
        m_currentMapIcon.setLine1Pen(pen);
        QPalette p = m_line1PenColorSample->palette();
        p.setColor(m_line1PenColorSample->backgroundRole(), color);
        m_line1PenColorSample->setPalette(p);
    }
}

void MapIconDialog::line2PenColor_clicked()
{
    QColor color = QColorDialog::getColor(m_currentMapIcon.line2Pen().color(),
            this, windowTitle() + " Line 2");

    if (color.isValid())
    {
        QPen pen = m_currentMapIcon.line2Pen();
        pen.setColor(color);
        m_currentMapIcon.setLine2Pen(pen);
        QPalette p = m_line2PenColorSample->palette();
        p.setColor(m_line2PenColorSample->backgroundRole(), color);
        m_line2PenColorSample->setPalette(p);
    }
}

void MapIconDialog::walkPathPenColor_clicked()
{
    QColor color = QColorDialog::getColor(m_currentMapIcon.walkPathPen().color(),
            this, windowTitle() + " Walk Path Line");

    if (color.isValid())
    {
        QPen pen = m_currentMapIcon.walkPathPen();
        pen.setColor(color);
        m_currentMapIcon.setWalkPathPen(pen);
        QPalette p = m_walkPathPenColorSample->palette();
        p.setColor(m_walkPathPenColorSample->backgroundRole(), color);
        m_walkPathPenColorSample->setPalette(p);
    }
}

void MapIconDialog::setupMapIconDisplay()
{
    // setup image fields
    m_useImage->setChecked(m_currentMapIcon.image());
    m_imageImage->setCurrentIndex(m_currentMapIcon.imageStyle());
    m_imageSize->setCurrentIndex(m_currentMapIcon.imageSize());
    m_imageFlash->setChecked(m_currentMapIcon.imageFlash());
    m_imageUseSpawnColorPen->setChecked(m_currentMapIcon.imageUseSpawnColorPen());
    QPalette p_ip = m_imagePenColorSample->palette();
    p_ip.setColor(m_imagePenColorSample->backgroundRole(), m_currentMapIcon.imagePen().color());
    m_imagePenColorSample->setPalette(p_ip);
    m_imagePenStyle->setCurrentIndex(m_currentMapIcon.imagePen().style());
    m_imagePenWidth->setValue(m_currentMapIcon.imagePen().width());
    m_imageUseSpawnColorBrush->setChecked(m_currentMapIcon.imageUseSpawnColorBrush());
    QPalette p_ib = m_imageBrushColorSample->palette();
    p_ib.setColor(m_imageBrushColorSample->backgroundRole(), m_currentMapIcon.imageBrush().color());
    m_imageBrushColorSample->setPalette(p_ib);

    m_imageBrushStyle->setCurrentIndex(m_currentMapIcon.imageBrush().style());

    // setup highlight fields
    m_useHighlight->setChecked(m_currentMapIcon.highlight());
    m_highlightImage->setCurrentIndex(m_currentMapIcon.highlightStyle());
    m_highlightSize->setCurrentIndex(m_currentMapIcon.highlightSize());
    m_highlightFlash->setChecked(m_currentMapIcon.highlightFlash());
    m_highlightUseSpawnColorPen->setChecked(m_currentMapIcon.highlightUseSpawnColorPen());
    QPalette p_hp = m_highlightPenColorSample->palette();
    p_hp.setColor(m_highlightPenColorSample->backgroundRole(), m_currentMapIcon.highlightPen().color());
    m_highlightPenColorSample->setPalette(p_hp);
    m_highlightPenStyle->setCurrentIndex(m_currentMapIcon.highlightPen().style());
    m_highlightPenWidth->setValue(m_currentMapIcon.highlightPen().width());
    m_highlightUseSpawnColorBrush->setChecked(m_currentMapIcon.highlightUseSpawnColorBrush());
    QPalette p_hb = m_highlightBrushColorSample->palette();
    p_hb.setColor(m_highlightBrushColorSample->backgroundRole(), m_currentMapIcon.highlightBrush().color());
    m_highlightBrushColorSample->setPalette(p_hb);
    m_highlightBrushStyle->setCurrentIndex(m_currentMapIcon.highlightBrush().style());

    // setup lines
    m_showLine0->setChecked(m_currentMapIcon.showLine0());
    QPalette p_l0 = m_line0PenColorSample->palette();
    p_l0.setColor(m_line0PenColorSample->backgroundRole(), m_currentMapIcon.line0Pen().color());
    m_line0PenColorSample->setPalette(p_l0);
    m_line0PenStyle->setCurrentIndex(m_currentMapIcon.line0Pen().style());
    m_line0PenWidth->setValue(m_currentMapIcon.line0Pen().width());
    m_line1Distance->setValue(m_currentMapIcon.line1Distance());
    QPalette p_l1 = m_line1PenColorSample->palette();
    p_l1.setColor(m_line1PenColorSample->backgroundRole(), m_currentMapIcon.line1Pen().color());
    m_line1PenColorSample->setPalette(p_l1);
    m_line1PenStyle->setCurrentIndex(m_currentMapIcon.line1Pen().style());
    m_line1PenWidth->setValue(m_currentMapIcon.line1Pen().width());
    m_line2Distance->setValue(m_currentMapIcon.line2Distance());
    QPalette p_l2 = m_line2PenColorSample->palette();
    p_l2.setColor(m_line2PenColorSample->backgroundRole(), m_currentMapIcon.line2Pen().color());
    m_line2PenColorSample->setPalette(p_l2);
    m_line2PenStyle->setCurrentIndex(m_currentMapIcon.line2Pen().style());
    m_line2PenWidth->setValue(m_currentMapIcon.line2Pen().width());

    // setup other
    m_showName->setChecked(m_currentMapIcon.showName());
    m_showWalkPath->setChecked(m_currentMapIcon.showWalkPath());
    m_useWalkPathPen->setChecked(m_currentMapIcon.useWalkPathPen());
    QPalette p_wp = m_walkPathPenColorSample->palette();
    p_wp.setColor(m_walkPathPenColorSample->backgroundRole(), m_currentMapIcon.walkPathPen().color());
    m_walkPathPenColorSample->setPalette(p_wp);
    m_walkPathPenStyle->setCurrentIndex(m_currentMapIcon.walkPathPen().style());
    m_walkPathPenWidth->setValue(m_currentMapIcon.walkPathPen().width());
}


#ifndef QMAKEBUILD
#include "mapicondialog.moc"
#endif
