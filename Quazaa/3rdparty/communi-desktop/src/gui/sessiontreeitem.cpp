/*
* Copyright (C) 2008-2013 The Communi Project
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

#include "sessiontreeitem.h"
#include "sessiontreewidget.h"
#include "itemdelegate.h"
#include "messageview.h"
#include "quazaasettings.h"
#include <IrcBuffer>

SessionTreeItem::SessionTreeItem(MessageView* view, QTreeWidget* parent) : QTreeWidgetItem(parent)
{
	d.view = view;
	d.highlighted = false;
	d.sortOrder = Manual;
	setText(0, view->receiver());
	setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
}

SessionTreeItem::SessionTreeItem(MessageView* view, QTreeWidgetItem* parent) : QTreeWidgetItem(parent)
{
	d.view = view;
	d.highlighted = false;
	d.sortOrder = Manual;
	setText(0, view->receiver());
	setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
}

SessionTreeItem::~SessionTreeItem()
{
}

IrcConnection* SessionTreeItem::connection() const
{
	return d.view->connection();
}

MessageView* SessionTreeItem::view() const
{
	return d.view;
}

SessionTreeItem* SessionTreeItem::findChild(const QString& name) const
{
	for (int i = 0; i < childCount(); ++i)
		if (child(i)->text(0).compare(name, Qt::CaseInsensitive) == 0)
			return static_cast<SessionTreeItem*>(child(i));
	return 0;
}

QVariant SessionTreeItem::data(int column, int role) const
{
	if (role == Qt::ForegroundRole) {
		SessionTreeWidget* tw = static_cast<SessionTreeWidget*>(treeWidget());
		if (!d.view->isActive())
			return tw->statusColor(SessionTreeWidget::Inactive);
		if (d.highlighted || (!isExpanded() && !d.highlightedChildren.isEmpty()))
			return tw->currentHighlightColor();
		return tw->statusColor(SessionTreeWidget::Active);
	} else if (role == ItemDelegate::BadgeColorRole) {
		SessionTreeWidget* tw = static_cast<SessionTreeWidget*>(treeWidget());
		if (!isSelected() && d.view->isActive() && d.highlighted)
			return tw->currentBadgeColor();
		return tw->statusColor(SessionTreeWidget::Badge);
	} else if (role == ItemDelegate::HighlightRole) {
		return d.highlighted;
	} else if (role == Qt::DecorationRole) {
		switch (column)
			case 0:
			{
				switch (d.view->viewType()) {
					case ViewInfo::Server:
						if(quazaaSettings.Chat.DarkTheme)
							return QIcon(":/Resource/Chat/NetworkDark.png");
						else
							return QIcon(":/Resource/Chat/Network.png");
					case ViewInfo::Channel:
						if(quazaaSettings.Chat.DarkTheme)
							return QIcon(":/Resource/Chat/ChannelDark.png");
						else
							return QIcon(":/Resource/Chat/Channel.png");
					case ViewInfo::Query:
						if(quazaaSettings.Chat.DarkTheme)
							return QIcon(":/Resource/Chat/NormalDark.png");
						else
							return QIcon(":/Resource/Chat/Normal.png");
			}
		}
	}
	return QTreeWidgetItem::data(column, role);
}

int SessionTreeItem::badge() const
{
	return data(1, ItemDelegate::BadgeRole).toInt();
}

void SessionTreeItem::setBadge(int badge)
{
	setData(1, ItemDelegate::BadgeRole, badge);
}

bool SessionTreeItem::isHighlighted() const
{
	return d.highlighted;
}

void SessionTreeItem::setHighlighted(bool highlighted)
{
	if (d.highlighted != highlighted) {
		d.highlighted = highlighted;
		if (SessionTreeItem* p = static_cast<SessionTreeItem*>(parent())) {
			if (highlighted)
				p->d.highlightedChildren.insert(this);
			else
				p->d.highlightedChildren.remove(this);
			if (!p->isExpanded())
				p->emitDataChanged();
		}
		emitDataChanged();
	}
}

void SessionTreeItem::sort(SortOrder order)
{
	if (d.sortOrder != order) {
		if (d.sortOrder == Manual)
			resetManualSortOrder();
		d.sortOrder = order;
		sortChildren(0, Qt::AscendingOrder);
	}
}

SessionTreeItem::SortOrder SessionTreeItem::currentSortOrder() const
{
	if (const SessionTreeItem* p = static_cast<const SessionTreeItem*>(parent()))
		return p->currentSortOrder();
	return d.sortOrder;
}

QStringList SessionTreeItem::manualSortOrder() const
{
	if (const SessionTreeItem* p = static_cast<const SessionTreeItem*>(parent()))
		return p->manualSortOrder();
	return d.manualOrder;
}

void SessionTreeItem::setManualSortOrder(const QStringList& order)
{
	if (d.manualOrder != order) {
		d.manualOrder = order;
		sortChildren(0, Qt::AscendingOrder);
	}
}

void SessionTreeItem::resetManualSortOrder()
{
	d.manualOrder.clear();
	for (int i = 0; i < childCount(); ++i)
		d.manualOrder += child(i)->text(0);
}

bool SessionTreeItem::operator<(const QTreeWidgetItem& other) const
{
	Q_ASSERT(parent() && other.parent());
	if (currentSortOrder() == Alphabetic) {
		const SessionTreeItem* otherItem = static_cast<const SessionTreeItem*>(&other);
		const bool a = d.view->viewType() == ViewInfo::Channel;
		const bool b = otherItem->d.view->viewType() == ViewInfo::Channel;
		if (a != b)
			return a;
		const IrcBuffer* ab = d.view->buffer();
		const IrcBuffer* bb = otherItem->view()->buffer();
		if (!ab || !bb)
			return QTreeWidgetItem::operator<(other);
		return ab->name().localeAwareCompare(bb->name()) < 0;
	} else {
		// manual sorting via dnd
		const QStringList sortOrder = manualSortOrder();
		const int a = sortOrder.indexOf(text(0));
		const int b = sortOrder.indexOf(other.text(0));
		if (a == -1 && b != -1)
			return false;
		if (a != -1 && b == -1)
			return true;
		return a < b;
	}
}
