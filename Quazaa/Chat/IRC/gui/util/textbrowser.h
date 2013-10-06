/*
* Copyright (C) 2008-2013 J-P Nurmi <jpnurmi@gmail.com>
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

#ifndef TEXTBROWSER_H
#define TEXTBROWSER_H

#include <QTextBrowser>

class TextBrowser : public QTextBrowser
{
	Q_OBJECT
	Q_PROPERTY(int unseenBlock READ unseenBlock WRITE setUnseenBlock)

public:
	TextBrowser(QWidget* parent = 0);

	QWidget* buddy() const;
	void setBuddy(QWidget* buddy);

	int unseenBlock() const;
	void setUnseenBlock(int block);

	QColor unreadLineColor() const;
	void setUnreadLineColor(const QColor &color);

public slots:
	void append(const QString& text);
	void scrollToTop();
	void scrollToBottom();
	void scrollToNextPage();
	void scrollToPreviousPage();

protected:
	void keyPressEvent(QKeyEvent* event);
	void paintEvent(QPaintEvent* event);
	void resizeEvent(QResizeEvent* event);
	void wheelEvent(QWheelEvent* event);


private slots:
	void appendBuffer();

private:
	int ub;
	QWidget* bud;
	QBrush unreadLineBrush;
	QStringList buffer;
};

#endif // TEXTBROWSER_H
