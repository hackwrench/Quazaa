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

#include "homepage.h"
#include "application.h"
#include "dialogircsettings.h"
#include <QCommandLinkButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QLabel>
#include <QMenu>
#include <QFile>

static QString readHtmlFile(const QString& filePath)
{
	QFile file(filePath);
	file.open(QIODevice::ReadOnly);
	return QString::fromUtf8(file.readAll());
}

WidgetIrcHomePage::WidgetIrcHomePage(QWidget* parent) : QWidget(parent)
{
	bg.load(":/Resource/background.png");

	header = new QLabel(this);
	header->setMargin(2);
	header->setObjectName("headerLabel");
	header->setOpenExternalLinks(true);
	header->setWordWrap(true);
	header->setText(readHtmlFile(":/Resource/welcome_header.html").arg(CApplication::applicationName()).arg(CApplication::applicationSlogan()));

	slogan = new QLabel(this);
	slogan->setMargin(2);
	slogan->setObjectName("sloganLabel");
	slogan->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	slogan->setText("<small><b>" + tr("Please respect others while using this service.") + "</b></small>");

	footer = new QLabel(this);
	footer->setObjectName("footerLabel");
	footer->setMargin(2);
	footer->setOpenExternalLinks(true);
	footer->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	footer->setStyleSheet("");
	footer->setText("<small>Copyright (C) 2008-2013 Quazaa Development Team &lt;<a href='mailto:quazaa-users@lists.sourceforge.net'>quazaa-users@lists.sourceforge.net</a>&gt;</small>");

	QLineEdit lineEdit;
	lineEdit.setStyleSheet("QLineEdit { border: 1px solid transparent; }");
	slogan->setFixedHeight(lineEdit.sizeHint().height());
	footer->setFixedHeight(lineEdit.sizeHint().height());

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(header);
	layout->addWidget(slogan);
	layout->addWidget(createBody(this), 1);
	layout->addWidget(footer);
	setLayout(layout);
}

void WidgetIrcHomePage::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);
	QPainter painter(this);
	painter.setOpacity(0.4);
	painter.drawPixmap(width() - bg.width(), height() - footer->height() - bg.height(), bg);
}

QWidget* WidgetIrcHomePage::createBody(QWidget* parent) const
{
	QWidget* body = new QWidget(parent);

	QCommandLinkButton* connectButton = new QCommandLinkButton(tr("Connect"), body);
	connectButton->setDescription(tr("New IRC connection"));
	QCommandLinkButton* settingsButton = new QCommandLinkButton(tr("Settings"), body);
	settingsButton->setDescription(tr("Configure %1").arg(CApplication::applicationName()));

	connect(connectButton, SIGNAL(clicked()), this, SIGNAL(connectRequested()));
	connect(settingsButton, SIGNAL(clicked()), this, SLOT(showSettings()));

	QGridLayout* layout = new QGridLayout(body);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(3, 1);
	layout->setRowStretch(0, 1);
	layout->addWidget(connectButton, 1, 1);
	layout->addWidget(settingsButton, 1, 2);
	layout->setRowStretch(7, 1);
	body->setLayout(layout);

	return body;
}

void WidgetIrcHomePage::showSettings()
{
	CDialogIrcSettings* dlgIrcSettings = new CDialogIrcSettings(this);

	dlgIrcSettings->exec();
}
