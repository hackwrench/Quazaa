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

#include "lineeditor.h"
#include "chatcompleter.h"
#include <QStyleFactory>
#include <QKeyEvent>
#include <QShortcut>
#include <QPainter>
#include <QStyle>

LineEditor::LineEditor(QWidget* parent) : HistoryLineEdit(parent)
{
	d.lag = -1;

	d.completer = new ChatCompleter(this);
	d.completer->setWidget(this);
	d.completer->setLineEdit(this);

	setAttribute(Qt::WA_MacShowFocusRect, false);

	// a workaround for a bug in the Oxygen style (style animation eats up all cpu)
	if (style()->objectName() == "oxygen") {
		QStringList keys = QStringList() << "fusion" << "plastique" << "cleanlooks" << "windows";
		while (!keys.isEmpty()) {
			QString key = keys.takeFirst();
			if (QStyleFactory::keys().contains(key)) {
				setStyle(QStyleFactory::create(key));
				break;
			}
		}
	}

	QShortcut* shortcut = new QShortcut(Qt::Key_Tab, this);
	connect(shortcut, SIGNAL(activated()), d.completer, SLOT(onTabPressed()));

	setButtonVisible(Left, true);
	setAutoHideButton(Left, true);
	connect(this, SIGNAL(leftButtonClicked()), d.completer, SLOT(onTabPressed()));

	setButtonVisible(Right, true);
	setAutoHideButton(Right, true);
	connect(this, SIGNAL(rightButtonClicked()), this, SLOT(onSend()));

	connect(this, SIGNAL(returnPressed()), this, SLOT(onSend()));
	connect(this, SIGNAL(textEdited(QString)), this, SIGNAL(typed(QString)));
}

ChatCompleter* LineEditor::completer() const
{
	return d.completer;
}

qint64 LineEditor::lag() const
{
	return d.lag;
}

void LineEditor::setLag(qint64 lag)
{
	if (d.lag != lag) {
		d.lag = lag;
		update();
	}
}

bool LineEditor::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride) {
		QKeyEvent* ke = static_cast<QKeyEvent*>(event);
		if (ke == QKeySequence::MoveToStartOfDocument) {
			emit scrollToTop();
			event->accept();
			return true;
		} else if (ke == QKeySequence::MoveToEndOfDocument) {
			emit scrollToBottom();
			event->accept();
			return true;
		} else if (ke == QKeySequence::MoveToNextPage) {
			emit scrollToNextPage();
			event->accept();
			return true;
		} else if (ke == QKeySequence::MoveToPreviousPage) {
			emit scrollToPreviousPage();
			event->accept();
			return true;
		}
	}
	return HistoryLineEdit::event(event);
}

void LineEditor::paintEvent(QPaintEvent* event)
{
	HistoryLineEdit::paintEvent(event);

	if (d.lag > 0 && text().isEmpty()) {
		QPainter painter(this);
		QColor color = palette().text().color();
		color.setAlpha(128);
		painter.setPen(color);
		QFont font;
		if (font.pointSize() != -1)
			font.setPointSizeF(0.8 * font.pointSizeF());
		painter.setFont(font);
		int l, t, r, b;
		getTextMargins(&l, &t, &r, &b);
		painter.drawText(rect().adjusted(l, t, -r, -b), Qt::AlignVCenter | Qt::AlignRight, tr("%1ms").arg(d.lag));
	}
}

void LineEditor::onSend()
{
	bool ignore = true;
	const QStringList lines = text().split(QRegExp("[\\r\\n]"), QString::SkipEmptyParts);
	foreach (const QString& line, lines) {
		if (!line.trimmed().isEmpty()) {
			ignore = false;
			emit send(line);
		}
	}
	if (!ignore)
		clear();
}
