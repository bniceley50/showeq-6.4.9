/*
 *  cgiconv.h
 *  Copyright 2001 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *  Copyright 2019 by the respective ShowEQ Developers
 *
 *  Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net)
 *  for use under the terms of the GNU General Public License,
 *  incorporated herein by reference.
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

#ifndef CGICONV_H
#define CGICONV_H

#include <QString>
#include <QList>

// class for handling POST action CGI forms
class CGI
{
 public:
  CGI();
  
  // Process the CGI data
  void processCGIData();

  // Log CGI Data
  void logCGIData(const QString& filename);

  // retrieve the total number of parameters
  int getParamCount() { return cgiParams.count(); }

  // retrieve the name of the specified parameter
  QString getParamName(int paramNum);

  // retrieve the number of instances of the specified parameter
  int getParamNameCount(const QString& paramName);

  // retrieve the value of the specified parameter
  QString getParamValue(const QString& paramName, int instance = 0);

  // retrieve the query string
  QString getQueryString() { return query_string; }

  // retrieve various CGI environment information
  QString getAuthType();

  QString getGatewayInterface();

  QString getHTTPAccept();
  QString getHTTPAcceptEncoding();
  QString getHTTPAcceptLanguage();
  QString getHTTPConnection();
  QString getHTTPReferer();
  QString getHTTPUserAgent();

  QString getRemoteHost();
  QString getRemoteAddress();
  QString getRemotePort();

  QString getRequestURI();
  QString getScriptName();

  QString getServerAdmin();
  QString getServerName();
  QString getServerPort();
  QString getServerProtocol();
  QString getServerSoftware();
  
 protected:
  // protected class for storing parsed CGI parameters
  class CGIParam
  {
  public:
    // Constructor (of course)
    CGIParam(const QString& paramName, const QString& paramValue)
    {
      name = paramName;
      value = paramValue;
    }
    
    // methods to retrieve the name and value of the parameter
    QString getName() { return name; }
    QString getValue() { return value; }
  private:
    // private storage of the parameter name and value (not to be modified)
    QString name;
    QString value;
  };

  typedef QList<CGIParam*> CGIParamList;

  // unescape the URL (decodes x-www-form-urlencoded)
  QString unescapeURL(QString url);

 private:
  // the query string
  QString query_string;

  // the list of cgi parameters
  CGIParamList cgiParams;
};

#endif // CGICONV_H
