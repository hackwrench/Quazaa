//
// widgetmedia.cpp
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

#include "widgetmedia.h"
#include "ui_widgetmedia.h"

#include "quazaasettings.h"
#include "QSkinDialog/qskinsettings.h"

WidgetMedia::WidgetMedia(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::WidgetMedia)
{
	ui->setupUi(this);
	seekSlider = new QSlider();
	seekSlider->setOrientation(Qt::Horizontal);
	volumeSlider = new QSlider();
	volumeSlider->setOrientation(Qt::Horizontal);
	volumeSlider->setMaximumWidth(100);
	ui->toolBarPlayControls->addWidget(seekSlider);
	ui->toolBarPlayControls->addSeparator();
	ui->toolBarPlayControls->addAction(ui->actionMediaMute);
	ui->toolBarPlayControls->addWidget(volumeSlider);
	restoreState(quazaaSettings.WinMain.MediaToolbars);
	ui->splitterMedia->restoreState(quazaaSettings.WinMain.MediaSplitter);
	ui->actionMediaRepeat->setChecked(quazaaSettings.Media.Repeat);
	ui->actionMediaShuffle->setChecked(quazaaSettings.Media.Shuffle);
	connect(&skinSettings, SIGNAL(skinChanged()), this, SLOT(skinChangeEvent()));
	skinChangeEvent();
}

WidgetMedia::~WidgetMedia()
{
	delete ui;
}

void WidgetMedia::changeEvent(QEvent* e)
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

void WidgetMedia::skinChangeEvent()
{
	seekSlider->setStyleSheet(skinSettings.seekSlider);
	volumeSlider->setStyleSheet(skinSettings.volumeSlider);
	ui->toolButtonMediaPlaylistTaskHeader->setStyleSheet(skinSettings.sidebarUnclickableTaskHeader);
	ui->frameMediaPlaylistTask->setStyleSheet(skinSettings.sidebarTaskBackground);
	ui->toolBarPlayControls->setStyleSheet(skinSettings.mediaToolbar);
	ui->toolBarSettings->setStyleSheet(skinSettings.mediaToolbar);
	ui->tableWidgetMediaPlaylistTask->setStyleSheet(skinSettings.listViews);
}

void WidgetMedia::saveWidget()
{
	quazaaSettings.WinMain.MediaToolbars = saveState();
	quazaaSettings.WinMain.MediaSplitter = ui->splitterMedia->saveState();
}

/*void WidgetMedia::on_actionMediaOpen_triggered()
{
	mediaPlayer->openFile();
}

void WidgetMedia::on_volumeSlider_valueChanged(int value)
{
	quazaaSettings.Media.Volume = value;
}*/

void WidgetMedia::on_actionMediaRepeat_triggered(bool checked)
{
	quazaaSettings.Media.Repeat = checked;
}

void WidgetMedia::on_actionMediaShuffle_triggered(bool checked)
{
	quazaaSettings.Media.Shuffle = checked;
}

void WidgetMedia::on_splitterMedia_customContextMenuRequested(QPoint pos)
{
	Q_UNUSED(pos);

	if(ui->splitterMedia->handle(1)->underMouse())
	{
		if(ui->splitterMedia->sizes()[1] > 0)
		{
			quazaaSettings.WinMain.MediaSplitterRestoreLeft = ui->splitterMedia->sizes()[0];
			quazaaSettings.WinMain.MediaSplitterRestoreRight = ui->splitterMedia->sizes()[1];
			QList<int> newSizes;
			newSizes.append(ui->splitterMedia->sizes()[0] + ui->splitterMedia->sizes()[1]);
			newSizes.append(0);
			ui->splitterMedia->setSizes(newSizes);
		}
		else
		{
			QList<int> sizesList;
			sizesList.append(quazaaSettings.WinMain.MediaSplitterRestoreLeft);
			sizesList.append(quazaaSettings.WinMain.MediaSplitterRestoreRight);
			ui->splitterMedia->setSizes(sizesList);
		}
	}
}

void WidgetMedia::on_toolButtonMediaPlaylistTaskHeader_clicked()
{
	if(ui->splitterMedia->sizes()[0] > 0)
	{
		quazaaSettings.WinMain.MediaSplitterRestoreLeft = ui->splitterMedia->sizes()[0];
		quazaaSettings.WinMain.MediaSplitterRestoreRight = ui->splitterMedia->sizes()[1];
		QList<int> newSizes;
		newSizes.append(0);
		newSizes.append(ui->splitterMedia->sizes()[0] + ui->splitterMedia->sizes()[1]);
		ui->splitterMedia->setSizes(newSizes);
	}
	else
	{
		QList<int> sizesList;
		sizesList.append(quazaaSettings.WinMain.MediaSplitterRestoreLeft);
		sizesList.append(quazaaSettings.WinMain.MediaSplitterRestoreRight);
		ui->splitterMedia->setSizes(sizesList);
	}
}
