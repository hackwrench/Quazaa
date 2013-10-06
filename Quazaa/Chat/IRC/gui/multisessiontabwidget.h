/*
* Copyright (C) 2008-2013 J-P Nurmi <jpnurmi@gmail.com>
* Copyright (C) 2010-2013 SmokeX <smokexjc@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef MULTISESSIONTABWIDGET_H
#define MULTISESSIONTABWIDGET_H

#include "tabwidget.h"

class Session;
class IrcMessage;
class CWidgetIrcMessageView;
class SessionTabWidget;

class MultiSessionTabWidget : public CTabWidget
{
	Q_OBJECT

public:
	MultiSessionTabWidget(QWidget* parent = 0);

	QList<Session*> sessions() const;

	SessionTabWidget* currentWidget() const;
	SessionTabWidget* widgetAt(int index) const;
	SessionTabWidget* sessionWidget(Session* session) const;

	QByteArray saveSplitter() const;

public slots:
	void addSession(Session* session);
	void removeSession(Session* session);

	void restoreSplitter(const QByteArray& state);
	void applySettings();

signals:
	void splitterChanged(const QByteArray& state);

private slots:
	void updateTab(int index = -1);
	void tabActivated(int index);
	void onTabMenuRequested(int index, const QPoint& pos);

private:
	struct MainTabWidgetData {
	} d;
};

#endif // MULTISESSIONTABWIDGET_H
