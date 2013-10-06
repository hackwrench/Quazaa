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

#include "multisessiontabwidget.h"
#include "sessiontabwidget.h"
#include "tabwidget_p.h"
#include "quazaasettings.h"
#include "session.h"
#include <QShortcut>
#include <QTabBar>

MultiSessionTabWidget::MultiSessionTabWidget(QWidget* parent) : CTabWidget(parent)
{
	setTabPosition(QTabWidget::West);
	setStyleSheet(".MainTabWidget::pane { border: 0px; }");

	connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabActivated(int)));
	connect(this, SIGNAL(tabMenuRequested(int, QPoint)), this, SLOT(onTabMenuRequested(int, QPoint)));

	QShortcut* shortcut = new QShortcut(QKeySequence::New, this);
	connect(shortcut, SIGNAL(activated()), this, SIGNAL(newTabRequested()));

	applySettings();
}

QList<Session*> MultiSessionTabWidget::sessions() const
{
	QList<Session*> list;
	for (int i = 0; i < count(); ++i) {
		SessionTabWidget* tabWidget = qobject_cast<SessionTabWidget*>(widget(i));
		if (tabWidget)
			list += tabWidget->session();
	}
	return list;
}

SessionTabWidget *MultiSessionTabWidget::currentWidget() const
{
	return qobject_cast<SessionTabWidget*>(QTabWidget::currentWidget());
}

SessionTabWidget *MultiSessionTabWidget::widgetAt(int index) const
{
	return qobject_cast<SessionTabWidget*>(QTabWidget::widget(index));
}

void MultiSessionTabWidget::addSession(Session* session)
{
	SessionTabWidget* tab = new SessionTabWidget(session, this);
	connect(tab, SIGNAL(sessionClosed(Session*)), this, SLOT(removeSession(Session*)));
	connect(tab, SIGNAL(splitterChanged(QByteArray)), this, SLOT(restoreSplitter(QByteArray)));
	tab->applySettings();

	int index = addTab(tab, session->name().isEmpty() ? session->host() : session->name());
	setCurrentIndex(index);
	updateTab(index);

	connect(tab, SIGNAL(highlightStatusChanged(bool)), this, SLOT(updateTab()));
	connect(tab, SIGNAL(inactiveStatusChanged(bool)), this, SLOT(updateTab()));
	connect(tab, SIGNAL(alertStatusChanged(bool)), this, SLOT(updateTab()));
	connect(session, SIGNAL(nameChanged(QString)), this, SLOT(updateTab()));
	connect(session, SIGNAL(networkChanged(QString)), this, SLOT(updateTab()));
}

void MultiSessionTabWidget::removeSession(Session* session)
{
	SessionTabWidget* tabWidget = sessionWidget(session);
	if (tabWidget) {
		removeTab(indexOf(tabWidget));
		tabWidget->deleteLater();
	}
}

SessionTabWidget* MultiSessionTabWidget::sessionWidget(Session* session) const
{
	for (int i = 0; i < count(); ++i) {
		SessionTabWidget* tabWidget = widgetAt(i);
		if (tabWidget && tabWidget->session() == session)
			return tabWidget;
	}
	return 0;
}

void MultiSessionTabWidget::tabActivated(int index)
{
	if (index < count() - 1) {
		SessionTabWidget* tab = qobject_cast<SessionTabWidget*>(widget(index));
		if (tab) {
			setWindowFilePath(tab->tabText(tab->currentIndex()));
			QMetaObject::invokeMethod(tab, "delayedTabReset");
		}
	}
}

void MultiSessionTabWidget::applySettings()
{
	CTabBar* tb = static_cast<CTabBar*>(tabBar());
	tb->setNavigationShortcut(CTabBar::Next, QKeySequence(quazaaSettings.Chat.Shortcuts.value(IrcShortcutType::NavigateDown)));
	tb->setNavigationShortcut(CTabBar::Previous, QKeySequence(quazaaSettings.Chat.Shortcuts.value(IrcShortcutType::NavigateUp)));
	tb->setNavigationShortcut(CTabBar::NextUnread, QKeySequence(quazaaSettings.Chat.Shortcuts.value(IrcShortcutType::NextUnreadDown)));
	tb->setNavigationShortcut(CTabBar::PreviousUnread, QKeySequence(quazaaSettings.Chat.Shortcuts.value(IrcShortcutType::NextUnreadUp)));
	tb->setVisible(quazaaSettings.Chat.Layout == "tabs");

	QColor color(quazaaSettings.Chat.Colors.value(IrcColorType::Highlight));
	setTabTextColor(Alert, color);
	setTabTextColor(Highlight, color);

	for (int i = 0; i < count(); ++i) {
		SessionTabWidget* tabWidget = qobject_cast<SessionTabWidget*>(widget(i));
		if (tabWidget)
			tabWidget->applySettings();
	}
}

QByteArray MultiSessionTabWidget::saveSplitter() const
{
	QByteArray state;
	for (int i = count() - 1; state.isNull() && i >= 0; --i) {
		SessionTabWidget* tabWidget = qobject_cast<SessionTabWidget*>(widget(i));
		if (tabWidget)
			state = tabWidget->saveSplitter();
	}
	return state;
}

void MultiSessionTabWidget::restoreSplitter(const QByteArray& state)
{
	for (int i = 0; i < count(); ++i) {
		SessionTabWidget* tabWidget = widgetAt(i);
		if (tabWidget) {
			tabWidget->blockSignals(true);
			tabWidget->restoreSplitter(state);
			tabWidget->blockSignals(false);
		}
	}
	emit splitterChanged(state);
}

void MultiSessionTabWidget::updateTab(int index)
{
	SessionTabWidget* tab = 0;
	if (index != -1)
		tab = qobject_cast<SessionTabWidget*>(widget(index));
	else if (Session* session = qobject_cast<Session*>(sender()))
		tab = sessionWidget(session);
	else
		tab = qobject_cast<SessionTabWidget*>(sender());
	index = indexOf(tab);

	if (tab && index != -1 && tab->session()) {
		setTabAlert(index, tab->hasTabAlert());
		setTabInactive(index, tab->isTabInactive());
		setTabHighlight(index, tab->hasTabHighlight());
		setTabText(index, tab->session()->name().isEmpty() ?
				   tab->session()->host() : tab->session()->name());
	}
}

void MultiSessionTabWidget::onTabMenuRequested(int index, const QPoint& pos)
{
	if (index < count() - 1) {
		SessionTabWidget* tabWidget = qobject_cast<SessionTabWidget*>(widget(index));
		if (tabWidget)
			QMetaObject::invokeMethod(tabWidget, "onTabMenuRequested", Q_ARG(int, 0), Q_ARG(QPoint, pos));
	}
}
