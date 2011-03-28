//
// systemlog.cpp
//
// Copyright © Quazaa Development Team, 2009-2010.
// This file is part of QUAZAA (quazaa.sourceforge.net)
//
// Quazaa is free software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3 of
// the License, or (at your option) any later version.
//
// Quazaa is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Quazaa; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "systemlog.h"
#include <QMetaType>
#include <QtCore>

SystemLog systemLog;

SystemLog::SystemLog()
{
	qRegisterMetaType<LogSeverity::Severity>("LogSeverity::Severity");
}

void SystemLog::postLog(QString message, LogSeverity::Severity severity)
{
	switch(severity)
	{
	case LogSeverity::Debug:
		qDebug() << message;
		break;
	case LogSeverity::Warning:
		qWarning() << message;
		break;
	case LogSeverity::Critical:
		qCritical() << message;
		break;
	default:
		break;
	}
	emit logPosted(message, severity);
}
void SystemLog::postLog(LogSeverity::Severity severity, const char* format, ...)
{
	va_list argList;
	va_start(argList, format);
	QString message = QString().vsprintf(format, argList);
	postLog(message, severity);
	va_end(argList);
}
