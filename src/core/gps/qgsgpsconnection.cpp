/***************************************************************************
                          qgsgpsconnection.cpp  -  description
                          --------------------
    begin                : November 30th, 2009
    copyright            : (C) 2009 by Marco Hugentobler
    email                : marco at hugis dot net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgpsconnection.h"

#include <QCoreApplication>
#include <QTime>
#include <QIODevice>
#include <QStringList>
#include <QFileInfo>

#include "qgsnmeaconnection.h"
#include "qgslogger.h"
#include "info.h"


bool QgsGpsInformation::isValid() const
{
  bool valid = false;
  if ( status == 'V' || fixType == NMEA_FIX_BAD || quality == 0 ) // some sources say that 'V' indicates position fix, but is below acceptable quality
  {
    valid = false;
  }
  else if ( fixType == NMEA_FIX_2D )
  {
    valid = true;
  }
  else if ( status == 'A' || fixType == NMEA_FIX_3D || quality > 0 ) // good
  {
    valid = true;
  }

  return valid;
}

QgsGpsInformation::FixStatus QgsGpsInformation::fixStatus() const
{
  FixStatus fixStatus = NoData;

  // no fix if any of the three report bad; default values are invalid values and won't be changed if the corresponding NMEA msg is not received
  if ( status == 'V' || fixType == NMEA_FIX_BAD || quality == 0 ) // some sources say that 'V' indicates position fix, but is below acceptable quality
  {
    fixStatus = NoFix;
  }
  else if ( fixType == NMEA_FIX_2D ) // 2D indication (from GGA)
  {
    fixStatus = Fix2D;
  }
  else if ( status == 'A' || fixType == NMEA_FIX_3D || quality > 0 ) // good
  {
    fixStatus = Fix3D;
  }
  return fixStatus;
}

QString QgsGpsInformation::qualityDescription() const
{
  switch ( quality )
  {
    case 8:
      return QCoreApplication::translate( "QgsGpsInformation", "Simulation mode" );

    case 7:
      return QCoreApplication::translate( "QgsGpsInformation", "Manual input mode" );

    case 6:
      return QCoreApplication::translate( "QgsGpsInformation", "Estimated" );

    case 5:
      return QCoreApplication::translate( "QgsGpsInformation", "Float RTK" );

    case 4:
      return QCoreApplication::translate( "QgsGpsInformation", "Fixed RTK" );

    case 3:
      return QCoreApplication::translate( "QgsGpsInformation", "PPS" );

    case 2:
      return QCoreApplication::translate( "QgsGpsInformation", "DGPS" );

    case 1:
      return QCoreApplication::translate( "QgsGpsInformation", "Autonomous" );

    case 0:
      return QCoreApplication::translate( "QgsGpsInformation", "Invalid" );

    default:
      return QCoreApplication::translate( "QgsGpsInformation", "Unknown (%1)" ).arg( QString::number( quality ) );
  }
}

QgsGpsConnection::QgsGpsConnection( QIODevice *dev )
  : QObject( nullptr )
  , mSource( dev )
{
  clearLastGPSInformation();
  QObject::connect( dev, &QIODevice::readyRead, this, &QgsGpsConnection::parseData );
}

QgsGpsConnection::~QgsGpsConnection()
{
  cleanupSource();
}

bool QgsGpsConnection::connect()
{
  if ( !mSource )
  {
    return false;
  }

  const bool connected = mSource->open( QIODevice::ReadWrite | QIODevice::Unbuffered );
  if ( connected )
  {
    mStatus = Connected;
  }
  return connected;
}

bool QgsGpsConnection::close()
{
  if ( !mSource )
  {
    return false;
  }

  mSource->close();
  return true;
}

void QgsGpsConnection::cleanupSource()
{
  if ( mSource )
  {
    mSource->close();
  }
  mSource.reset();
}

void QgsGpsConnection::setSource( QIODevice *source )
{
  cleanupSource();
  mSource.reset( source );
  clearLastGPSInformation();
}

void QgsGpsConnection::clearLastGPSInformation()
{
  mLastGPSInformation = QgsGpsInformation();
}
