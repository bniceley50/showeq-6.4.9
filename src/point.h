/*
 *  point.h
 *  Copyright 2001-2005, 2019 by the respective ShowEQ Developers
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

// Original Author: Zaphod (dohpaz@users.sourceforge.net)
//   interfaces modeled after QPoint and QPointArray interface
//   but for 3D data of arbitraty type T.

//
// NOTE: Trying to keep this file ShowEQ/Everquest independent to allow it
// to be reused for other Show{} style projects. 
//

#ifndef __POINT_H_
#define __POINT_H_

#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif
#include <cmath>
#include <cstdarg>

#include <QPoint>

template <class _T>
class Point3D 
{
 public:
  typedef _T dimension_type;

  // constructors
  Point3D();
  Point3D(_T x, _T y, _T z);
  Point3D(const Point3D& point);
  Point3D(const QPoint& point);

  // virtual destructor
  virtual ~Point3D();

  // operators
  Point3D& operator=(const Point3D& point);
  Point3D& operator+=(const Point3D& point);
  Point3D& operator-=(const Point3D& point);
  Point3D& operator*=(int coef);
  Point3D& operator*=(double coef);
  Point3D& operator/=(int coef);
  Point3D& operator/=(double coef);
  bool operator==(const Point3D& point) const;
  bool isEqual(_T x, _T y, _T z) const;

  // get methods
  _T x() const { return m_x; }
  _T y() const { return m_y; }
  _T z() const { return m_z; }

  // test method
  bool isNull() const;

  // set methods
  void setPoint(_T x, _T y, _T z)
    { m_x = x; m_y = y; m_z = z; }
  void setPoint(const Point3D& point)
    { m_x = point.m_x; m_y = point.m_y; m_z = point.m_z; }
  void setXPos(_T x) { m_x = x; }
  void setYPos(_T y) { m_y = y; }
  void setZPos(_T z) { m_z = z; }

  // utility methods
  
  // add values to point
  void addPoint(_T x, _T y, _T z);

  // retrieve a QPoint of this point
  QPoint qpoint() const { return QPoint(x(), y()); }
  
  // retrieve offset point
  QPoint offsetPoint(const QPoint& centerPoint, double ratio);
  QPoint inverseOffsetPoint(const QPoint& centerPoint, double ratio);

  // Calculate distance in 2 space ignoring Z dimension
  uint32_t calcDist2DInt(_T x, _T y) const;
  uint32_t calcDist2DInt(const Point3D& point) const
    { return calcDist2DInt(point.x(), point.y()); }
  uint32_t calcDist2DInt(const QPoint& point) const
    { return calcDist2DInt(point.x(), point.y()); }
  double calcDist2D(_T x, _T y) const;
  double calcDist2D(const Point3D& point) const
    { return calcDist2D(point.x(), point.y()); }
  double calcDist2D(const QPoint& point) const
    { return calcDist2D(point.x(), point.y()); }

  // Calculate distance in 3 space
  uint32_t calcDistInt(_T x, _T y, _T z = 0) const;
  uint32_t calcDistInt(const Point3D& point) const
    { return calcDistInt(point.x(), point.y(), point.z()); }
  double calcDist(_T x, _T y, _T z = 0) const;
  double calcDist(const Point3D& point) const
    { return calcDist(point.x(), point.y(), point.z()); }

 protected:
  // position information
  _T m_x;
  _T m_y;
  _T m_z;
};

// default constructor
template <class _T> inline
Point3D<_T>::Point3D()
{
  setPoint(0, 0, 0);
}

// copy constructor
template <class _T> inline
Point3D<_T>::Point3D(const Point3D<_T>& point)
{
  setPoint(point.x(), point.y(), point.z());
}

// copy constructor
template <class _T> inline
Point3D<_T>::Point3D(const QPoint& point)
{
  setPoint(point.x(), point.y(), 0);
}

// convenience constructor
template <class _T> inline
Point3D<_T>::Point3D(_T x, _T y, _T z)
{
  setPoint(x, y, z);
}

// obligatory virtual destructor
template <class _T> inline
Point3D<_T>::~Point3D<_T>()
{
}

// assignment operator
template <class _T> inline
Point3D<_T>& Point3D<_T>::operator=(const Point3D<_T>& point)
{
  setPoint(point.x(), point.y(), point.z());
  return *this;
}

// math operators
template <class _T> inline
Point3D<_T>& Point3D<_T>::operator+=(const Point3D<_T>& point)
{
  m_x += point.x();
  m_y += point.y();
  m_z += point.z();
  return *this;
}

template <class _T> inline
Point3D<_T>& Point3D<_T>::operator-=(const Point3D<_T>& point)
{
  m_x -= point.x();
  m_y -= point.y();
  m_z -= point.z();
  return *this;
}

// scaling operators
template <class _T> inline
Point3D<_T>& Point3D<_T>::operator*=(int coef)
{
  m_x *= coef;
  m_y *= coef;
  m_z *= coef;
  return *this;
}

template <class _T> inline
Point3D<_T>& Point3D<_T>::operator*=(double coef)
{
  m_x *= coef;
  m_y *= coef;
  m_z *= coef;
  return *this;
}

template <class _T> inline
Point3D<_T>& Point3D<_T>::operator/=(int coef)
{
  m_x /= coef;
  m_y /= coef;
  m_z /= coef;
  return *this;
}

template <class _T> inline
Point3D<_T>& Point3D<_T>::operator/=(double coef)
{
  m_x /= coef;
  m_y /= coef;
  m_z /= coef;
  return *this;
}

// equivalency operator
template <class _T> inline
bool Point3D<_T>::operator==(const Point3D<_T>& point) const
{
  return ((x() == point.x()) &&
	  (y() == point.y()) &&
	  (z() == point.z()));
}

template <class _T> inline
bool Point3D<_T>::isEqual(_T x, _T y, _T z) const
{
  return ((m_x == x) &&
	  (m_y == y) &&
	  (m_z == z));
}

// returns true if all 3 dimensions are 0
template <class _T> inline
bool Point3D<_T>::isNull() const
{ 
  return ((x() == 0) && (y() == 0) && (z() == 0));
}

// utility methods

template <class _T> inline
void Point3D<_T>::addPoint(_T x, _T y, _T z)
{
  m_x += x;
  m_y += y;
  m_z += z;
}

template <class _T> inline
QPoint Point3D<_T>::offsetPoint(const QPoint& centerPoint, double ratio)
{ 
  return QPoint((centerPoint.x() - (int)(x() / ratio)),
		(centerPoint.y() - (int)(y() / ratio)));
}

template <class _T> inline
QPoint Point3D<_T>::inverseOffsetPoint(const QPoint& centerPoint, double ratio)
{
  return QPoint(int(rint((centerPoint.x() - x()) * ratio)),
		int(rint((centerPoint.y() - y()) * ratio)));
}

// Calculate distance using/returning int in 2 space ignoring any Z dimension
template <class _T> inline
uint32_t Point3D<_T>::calcDist2DInt(_T x_, _T y_) const
{
  int32_t xDiff = x() - x_;
  int32_t yDiff = y() - y_;
  
  return uint32_t(sqrt(double((yDiff * yDiff) + (xDiff * xDiff))));
}

// Calculate distance using/returning double in 2 space ignoring any Z dimension
template <class _T> inline
double Point3D<_T>::calcDist2D(_T x_, _T y_) const
{
  double xDiff = double(x()) - double(x_);
  double yDiff = double(y()) - double(y_);
  return sqrt((xDiff * xDiff) + (yDiff * yDiff));
}

// Calculate distance using/returning int
template <class _T> inline
uint32_t Point3D<_T>::calcDistInt(_T x_, _T y_, _T z_) const
{
  int32_t xDiff = x() - x_;
  int32_t yDiff = y() - y_;
  int32_t zDiff = z() - z_;

  return uint32_t(sqrt(double((yDiff * yDiff) + 
			      (xDiff * xDiff) + 
			      (zDiff * zDiff))));
}

// Calculate distance using/returning double
template <class _T> inline
double Point3D<_T>::calcDist(_T x_, _T y_, _T z_) const
{
  double xDiff = double(x()) - double(x_);
  double yDiff = double(y()) - double(y_);
  double zDiff = double(z()) - double(z_);

  return sqrt((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff));
}

#endif // __POINT_H_
