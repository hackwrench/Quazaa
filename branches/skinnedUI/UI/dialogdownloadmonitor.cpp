//
// dialogdownloadmonitor.cpp
//
// Copyright � Quazaa Development Team, 2009-2010.
// This file is part of QUAZAA (quazaa.sourceforge.net)
//
// Quazaa is free software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of
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

#include "dialogdownloadmonitor.h"
#include "ui_dialogdownloadmonitor.h"
#include "QSkinDialog/qskinsettings.h"

DialogDownloadMonitor::DialogDownloadMonitor(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::DialogDownloadMonitor)
{
	ui->setupUi(this);
	connect(&skinSettings, SIGNAL(skinChanged()), this, SLOT(skinChangeEvent()));
	skinChangeEvent();
}

DialogDownloadMonitor::~DialogDownloadMonitor()
{
	delete ui;
}

void DialogDownloadMonitor::changeEvent(QEvent* e)
{
	QDialog::changeEvent(e);
	switch(e->type())
	{
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}

void DialogDownloadMonitor::on_pushButtonHide_clicked()
{
	emit closed();
	close();
}

void DialogDownloadMonitor::on_pushButton_clicked()
{
	emit closed();
	close();
}

void DialogDownloadMonitor::updateProgress(int percent, QString transferSpeed, QString timeRemaining,
        QString volumeDownloaded, QString numberSources, QPixmap icon,
        QString status, QString file)
{
	ui->progressBarStatus->setValue(percent);
	ui->labelEstimatedTimeRemaining->setText(timeRemaining);
	ui->labelTransferSpeed->setText(transferSpeed);
	ui->labelVolumeDownloaded->setText(volumeDownloaded);
	ui->labelNumberSources->setText(numberSources);
	ui->labelFileIcon->setPixmap(icon);
	ui->labelStatus->setText(status);
	ui->labelFileName->setText(file);
}

void DialogDownloadMonitor::skinChangeEvent()
{
	ui->frameCommonHeader->setStyleSheet(skinSettings.dialogHeader);
}
