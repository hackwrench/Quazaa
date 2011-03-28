//
// widgetdownloads.cpp
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

#include "widgetdownloads.h"
#include "ui_widgetdownloads.h"

#include "quazaasettings.h"
#include "QSkinDialog/qskinsettings.h"

WidgetDownloads::WidgetDownloads(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::WidgetDownloads)
{
	ui->setupUi(this);
	restoreState(quazaaSettings.WinMain.DownloadsToolbar);
	connect(&skinSettings, SIGNAL(skinChanged()), this, SLOT(skinChangeEvent()));
	skinChangeEvent();
}

WidgetDownloads::~WidgetDownloads()
{
	delete ui;
}

void WidgetDownloads::changeEvent(QEvent* e)
{
	QMainWindow::changeEvent(e);
	switch(e->type())
	{
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}

void WidgetDownloads::skinChangeEvent()
{
	ui->toolBarControls->setStyleSheet(skinSettings.toolbars);
	ui->toolBarFileTypes->setStyleSheet(skinSettings.toolbars);
	ui->toolBarFilter->setStyleSheet(skinSettings.toolbars);
	ui->treeViewDownloads->setStyleSheet(skinSettings.listViews);
}

void WidgetDownloads::saveWidget()
{
	quazaaSettings.WinMain.DownloadsToolbar = saveState();
}
