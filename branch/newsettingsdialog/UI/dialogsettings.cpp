//
// dialogsettings.cpp
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

#include "dialogsettings.h"
#include "ui_dialogsettings.h"

#include "quazaasettings.h"
#include "QSkinDialog/qskinsettings.h"
#include "QSkinDialog/qskindialog.h"
#include "QSkinDialog/dialogskinpreview.h"
#include "dialogprofile.h"

DialogSettings::DialogSettings(QWidget *parent, SettingsPage::settingsPage page) :
	QDialog(parent),
	ui(new Ui::DialogSettings)
{
	ui->setupUi(this);
	switchSettingsPage(page);
	skinChangeEvent();

	newSkinSelected = false;

	// Load Basic Settings
	ui->checkBoxStartWithSystem->setChecked(quazaaSettings.Basic.StartWithSystem);
	ui->checkBoxConnectOnStart->setChecked(quazaaSettings.Basic.ConnectOnStartup);
	ui->comboBoxOnClose->setCurrentIndex(quazaaSettings.Basic.CloseMode);
	ui->checkBoxOnMinimize->setChecked(quazaaSettings.Basic.MinimizeToTray);
	ui->spinBoxDiskWarn->setValue(quazaaSettings.Basic.DiskSpaceWarning);
	ui->spinBoxDiskStop->setValue(quazaaSettings.Basic.DiskSpaceStop);

	// Load Parental Settings
	ui->checkBoxParentalChatFilter->setChecked(quazaaSettings.Parental.ChatAdultCensor);
	ui->checkBoxParentalSearchFilter->setChecked(quazaaSettings.Parental.FilterAdultSearchResults);
	ui->listWidgetParentalFilter->addItems(quazaaSettings.Parental.AdultFilter);

	// Load Library Settings
	ui->checkBoxLibraryRememberViews->setChecked(quazaaSettings.Library.RememberViews);
	ui->checkBoxLibraryWatchFolders->setChecked(quazaaSettings.Library.WatchFolders);
	ui->checkBoxQuickHashing->setChecked(quazaaSettings.Library.HighPriorityHashing);
	ui->checkBoxDisplayHashingProgress->setChecked(quazaaSettings.Library.HashWindow);
	ui->checkBoxLibraryGhostFiles->setChecked(quazaaSettings.Library.GhostFiles);
	ui->checkBoxVideoSeriesDetection->setChecked(quazaaSettings.Library.SmartSeriesDetection);
	ui->spinBoxFileHistoryRemember->setValue(quazaaSettings.Library.HistoryTotal);
	ui->spinBoxFileHistoryDays->setValue(quazaaSettings.Library.HistoryDays);
	ui->listWidgetFileTypesSafeOpen->addItems(quazaaSettings.Library.SafeExecuteTypes);
	ui->listWidgetFileTypesNeverShare->addItems(quazaaSettings.Library.NeverShareTypes);

	// File Types Settings

	// Load Media Player Settings

	// Load Search Settings
	ui->checkBoxSearchExpandMultiSorce->setChecked(quazaaSettings.Search.ExpandSearchMatches);
	ui->checkBoxSearchSwitchOnDownload->setChecked(quazaaSettings.Search.SwitchOnDownload);
	ui->checkBoxSearchHighlightNewMatches->setChecked(quazaaSettings.Search.HighlightNew);

	// Load Chat Settings
	ui->checkBoxPrivateMessagesGnutella->setChecked(quazaaSettings.Chat.GnutellaChatEnable);

	ui->checkBoxIrcConnectOnStart->setChecked(quazaaSettings.Chat.ConnectOnStartup);
	ui->checkBoxIrcEnableFileTransfers->setChecked(quazaaSettings.Chat.EnableFileTransfers);
	ui->checkBoxIrcShowTimestamp->setChecked(quazaaSettings.Chat.ShowTimestamp);
	ui->checkBoxIrcSSL->setChecked(quazaaSettings.Chat.IrcUseSSL);
	ui->spinBoxPrivateMessagesIdleMessage->setValue(quazaaSettings.Chat.AwayMessageIdleTime);
	ui->lineEditIrcServer->setText(quazaaSettings.Chat.IrcServerName);
	ui->spinBoxIrcPort->setValue(quazaaSettings.Chat.IrcServerPort);

	// Load Connection Settings
	ui->doubleSpinBoxInSpeed->setValue((quazaaSettings.Connection.InSpeed/1024)*8);
	ui->doubleSpinBoxOutSpeed->setValue((quazaaSettings.Connection.OutSpeed/1024)*8);
	ui->spinBoxNetworkPort->setValue(quazaaSettings.Connection.Port);
	ui->checkBoxRandomPort->setChecked(quazaaSettings.Connection.RandomPort);
	ui->spinBoxConnectionTimeout->setValue(quazaaSettings.Connection.TimeoutConnect);

	// Load Web Settings
	ui->checkBoxIntegrationMagnetLinks->setChecked(quazaaSettings.Web.Magnet);
	ui->checkBoxIntegrationGnutellaLinks->setChecked(quazaaSettings.Web.Gnutella);
	ui->checkBoxIntegrationAresLinks->setChecked(quazaaSettings.Web.Ares);
	ui->checkBoxIntegrationBitTorrentLinks->setChecked(quazaaSettings.Web.Torrent);
	ui->checkBoxIntegrationPioletLinks->setChecked(quazaaSettings.Web.Piolet);
	ui->checkBoxIntegrationEDonkeyLinks->setChecked(quazaaSettings.Web.ED2K);
	ui->checkBoxManageWebDownloads->setChecked(quazaaSettings.Web.BrowserIntegration);
	ui->listWidgetManageDownloadTypes->addItems(quazaaSettings.Web.ManageDownloadTypes);

	// Load Transfer Settings
	ui->checkBoxOnlyDownloadConnectedNetworks->setChecked(quazaaSettings.Transfers.RequireConnectedNetwork);
	ui->checkBoxSimpleProgress->setChecked(quazaaSettings.Transfers.SimpleProgressBar);

	// Load Download Settings
	ui->checkBoxExpandDownloads->setChecked(quazaaSettings.Downloads.ExpandDownloads);
	ui->lineEditSaveFolder->setText(quazaaSettings.Downloads.CompletePath);
	ui->lineEditTempFolder->setText(quazaaSettings.Downloads.IncompletePath);
	ui->comboBoxQueLength->setCurrentIndex(quazaaSettings.Downloads.QueueLimit);
	ui->spinBoxMaxFiles->setValue(quazaaSettings.Downloads.MaxFiles);
	ui->spinBoxMaxTransfers->setValue(quazaaSettings.Downloads.MaxTransfers);
	ui->spinBoxTransfersPerFile->setValue(quazaaSettings.Downloads.MaxTransfersPerFile);

	// Load Upload Settings
	ui->checkBoxSharePartials->setChecked(quazaaSettings.Uploads.SharePartials);
	ui->checkBoxSharingLimitHub->setChecked(quazaaSettings.Uploads.HubShareLimiting);
	ui->checkBoxSharePreviews->setChecked(quazaaSettings.Uploads.SharePreviews);
	ui->spinBoxUniqueHostLimit->setValue(quazaaSettings.Uploads.MaxPerHost);

	// Load Security Settings
	ui->checkBoxChatFilterSpam->setChecked(quazaaSettings.Security.ChatFilter);
	ui->checkBoxAllowBrowseProfile->setChecked(quazaaSettings.Security.AllowProfileBrowse);
	ui->checkBoxIrcFloodProtection->setChecked(quazaaSettings.Security.IrcFloodProtection);
	ui->spinBoxChatFloodLimit->setValue(quazaaSettings.Security.IrcFloodLimit);
	ui->checkBoxRemoteEnable->setChecked(quazaaSettings.Security.RemoteEnable);
	ui->lineEditRemoteUserName->setText(quazaaSettings.Security.RemoteUsername);
	ui->lineEditRemotePassword->setText(quazaaSettings.Security.RemotePassword);
	ui->checkBoxIgnoreLocalIP->setChecked(quazaaSettings.Security.SearchIgnoreLocalIP);
	ui->checkBoxEnableUPnP->setChecked(quazaaSettings.Security.EnableUPnP);
	ui->checkBoxAllowBrowseShares->setChecked(quazaaSettings.Security.AllowSharesBrowse);
	ui->listWidgetUserAgents->addItems(quazaaSettings.Security.BlockedAgentUploadFilter);

	// Load Gnutella 2 Settings
	ui->checkBoxConnectG2->setChecked(quazaaSettings.Gnutella2.Enable);
	ui->comboBoxG2Mode->setCurrentIndex(quazaaSettings.Gnutella2.ClientMode);
	ui->spinBoxG2LeafToHub->setValue(quazaaSettings.Gnutella2.NumHubs);
	ui->spinBoxG2HubToLeaf->setValue(quazaaSettings.Gnutella2.NumLeafs);
	ui->spinBoxG2HubToHub->setValue(quazaaSettings.Gnutella2.NumPeers);

	// Load Ares Settings
	ui->checkBoxConnectAres->setChecked(quazaaSettings.Ares.Enable);

	// Load eDonkey 2k Settings
	ui->checkBoxConnectEDonkey->setChecked(quazaaSettings.EDonkey.Enable);
	ui->checkBoxConnectKAD->setChecked(quazaaSettings.EDonkey.EnableKad);
	ui->checkBoxED2kSearchCahedServers->setChecked(quazaaSettings.EDonkey.SearchCachedServers);
	ui->spinBoxED2kMaxResults->setValue(quazaaSettings.EDonkey.MaxResults);
	ui->checkBoxED2kUpdateServerList->setChecked(quazaaSettings.EDonkey.LearnNewServers);
	ui->spinBoxED2kMaxClients->setValue(quazaaSettings.EDonkey.MaxClients);
	ui->checkBoxAutoQueryServerList->setChecked(quazaaSettings.EDonkey.MetAutoQuery);
	ui->lineEditEDonkeyServerListUrl->setText(quazaaSettings.EDonkey.ServerListURL);

	// Load BitTorrent Settings
	ui->checkBoxTorrentSaveDialog->setChecked(quazaaSettings.BitTorrent.UseSaveDialog);
	ui->checkBoxTorrentsStartPaused->setChecked(quazaaSettings.BitTorrent.StartPaused);
	ui->checkBoxTorrentsUseTemp->setChecked(quazaaSettings.BitTorrent.UseTemp);
	ui->checkBoxManagedTorrent->setChecked(quazaaSettings.BitTorrent.Managed);
	ui->checkBoxTorrentsEndgame->setChecked(quazaaSettings.BitTorrent.Endgame);
	ui->spinBoxTorrentsSimultaneous->setValue(quazaaSettings.BitTorrent.DownloadTorrents);
	ui->spinBoxTorrentsClientConnections->setValue(quazaaSettings.BitTorrent.DownloadConnections);
	ui->checkBoxTorrentsClearDownloaded->setChecked(quazaaSettings.BitTorrent.AutoClear);
	ui->spinBoxTorrentsRatioClear->setValue(quazaaSettings.BitTorrent.ClearRatio);
	ui->checkBoxTorrentsPreferTorrent->setChecked(quazaaSettings.BitTorrent.PreferBTSources);
	ui->checkBoxTorrentsUseKademlia->setChecked(quazaaSettings.BitTorrent.UseKademlia);
	ui->lineEditTorrentFolder->setText(quazaaSettings.BitTorrent.TorrentPath);

	// Set Generic Control States
	ui->groupBoxParentalFilter->setVisible(false);
	ui->pushButtonApply->setEnabled(false);

	// Load Skin Settings
	QDir dir(qApp->applicationDirPath() + "/Skin/");
	QFileInfoList skinDirs = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
	QListWidgetItem *itemToSet = new QListWidgetItem();
	foreach(QFileInfo i, skinDirs)
	{
		QListWidgetItem *item = new QListWidgetItem(i.fileName());
		ui->listWidgetSkins->addItem(item);
		if (item->text() == skinSettings.skinName)
		{
			itemToSet = item;
		}
	}
	ui->listWidgetSkins->setCurrentItem(itemToSet, QItemSelectionModel::SelectCurrent);

	connect(this, SIGNAL(skinChanged()), &skinSettings, SIGNAL(skinChanged()));
	connect(&skinSettings, SIGNAL(skinChanged()), this, SLOT(skinChangeEvent()));
}

DialogSettings::~DialogSettings()
{
	delete ui;
}

void DialogSettings::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void DialogSettings::switchSettingsPage(SettingsPage::settingsPage page)
{
	switch (page)
	{
	case SettingsPage::System:
		ui->stackedWidgetSettings->setCurrentIndex(0);
		ui->listWidgetGeneralTask->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Skins:
		ui->stackedWidgetSettings->setCurrentIndex(1);
		ui->listWidgetGeneralTask->setCurrentRow(1, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Library:
		ui->stackedWidgetSettings->setCurrentIndex(2);
		ui->listWidgetGeneralTask->setCurrentRow(2, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::FileTypes:
		ui->stackedWidgetSettings->setCurrentIndex(3);
		ui->listWidgetGeneralTask->setCurrentRow(3, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::MediaPlayer:
		ui->stackedWidgetSettings->setCurrentIndex(4);
		ui->listWidgetGeneralTask->setCurrentRow(4, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Search:
		ui->stackedWidgetSettings->setCurrentIndex(5);
		ui->listWidgetGeneralTask->setCurrentRow(5, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Integration:
		ui->stackedWidgetSettings->setCurrentIndex(6);
		ui->listWidgetGeneralTask->setCurrentRow(6, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Plugins:
		ui->stackedWidgetSettings->setCurrentIndex(7);
		ui->listWidgetGeneralTask->setCurrentRow(7, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::PrivateMessages:
		ui->stackedWidgetSettings->setCurrentIndex(8);
		ui->listWidgetCommunityTask->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(true);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Chat:
		ui->stackedWidgetSettings->setCurrentIndex(9);
		ui->listWidgetCommunityTask->setCurrentRow(1, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(true);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Security:
		ui->stackedWidgetSettings->setCurrentIndex(10);
		ui->listWidgetSecurityTask->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(true);
		break;
	case SettingsPage::Parental:
		ui->stackedWidgetSettings->setCurrentIndex(11);
		ui->listWidgetSecurityTask->setCurrentRow(1, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(true);
		break;
	case SettingsPage::Connection:
		ui->stackedWidgetSettings->setCurrentIndex(12);
		ui->listWidgetNetworkTask->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(true);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Transfers:
		ui->stackedWidgetSettings->setCurrentIndex(13);
		ui->listWidgetNetworkTask->setCurrentRow(1, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(true);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Downloads:
		ui->stackedWidgetSettings->setCurrentIndex(14);
		ui->listWidgetNetworkTask->setCurrentRow(2, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(true);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Uploads:
		ui->stackedWidgetSettings->setCurrentIndex(15);
		ui->listWidgetNetworkTask->setCurrentRow(3, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(true);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Gnutella2:
		ui->stackedWidgetSettings->setCurrentIndex(16);
		ui->listWidgetProtocolsTask->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(true);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Ares:
		ui->stackedWidgetSettings->setCurrentIndex(17);
		ui->listWidgetProtocolsTask->setCurrentRow(1, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(true);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::EDonkey:
		ui->stackedWidgetSettings->setCurrentIndex(18);
		ui->listWidgetProtocolsTask->setCurrentRow(2, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(true);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::BitTorrent:
		ui->stackedWidgetSettings->setCurrentIndex(19);
		ui->listWidgetProtocolsTask->setCurrentRow(3, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(true);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	case SettingsPage::Protocols:
		ui->stackedWidgetSettings->setCurrentIndex(20);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(false);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(true);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	default:
		ui->stackedWidgetSettings->setCurrentIndex(0);
		ui->listWidgetGeneralTask->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
		ui->toolButtonCommunityTask->setChecked(false);
		ui->toolButtonGeneralTask->setChecked(true);
		ui->toolButtonNetworkTask->setChecked(false);
		ui->toolButtonProtocolsTask->setChecked(false);
		ui->toolButtonSecurityTask->setChecked(false);
		break;
	}
}

void DialogSettings::on_listWidgetGeneralTask_clicked(QModelIndex index)
{
	ui->stackedWidgetSettings->setCurrentIndex(index.row());
}

void DialogSettings::on_listWidgetCommunityTask_clicked(QModelIndex index)
{
	ui->stackedWidgetSettings->setCurrentIndex(index.row() + 8);
}

void DialogSettings::on_listWidgetSecurityTask_clicked(QModelIndex index)
{
	ui->stackedWidgetSettings->setCurrentIndex(index.row() + 10);
}

void DialogSettings::on_listWidgetNetworkTask_clicked(QModelIndex index)
{
	ui->stackedWidgetSettings->setCurrentIndex(index.row() + 12);
}

void DialogSettings::on_listWidgetProtocolsTask_clicked(QModelIndex index)
{
	ui->stackedWidgetSettings->setCurrentIndex(index.row() + 16);
}

void DialogSettings::on_pushButtonOk_clicked()
{
	if(ui->pushButtonApply->isEnabled())
	{
		ui->pushButtonApply->click();
	}
	emit closed();
	close();
}

void DialogSettings::on_pushButtonCancel_clicked()
{
	emit closed();
	close();
}

void DialogSettings::on_pushButtonApply_clicked()
{
	// Save Basic Settings
	quazaaSettings.Basic.StartWithSystem = ui->checkBoxStartWithSystem->isChecked();
	quazaaSettings.Basic.ConnectOnStartup = ui->checkBoxConnectOnStart->isChecked();
	quazaaSettings.Basic.CloseMode = ui->comboBoxOnClose->currentIndex();
	quazaaSettings.Basic.MinimizeToTray = ui->checkBoxOnMinimize->isChecked();
	quazaaSettings.Basic.DiskSpaceWarning = ui->spinBoxDiskWarn->value();
	quazaaSettings.Basic.DiskSpaceStop = ui->spinBoxDiskStop->value();

	// Save Parental Settings
	quazaaSettings.Parental.ChatAdultCensor = ui->checkBoxParentalChatFilter->isChecked();
	quazaaSettings.Parental.FilterAdultSearchResults = ui->checkBoxParentalSearchFilter->isChecked();
	quazaaSettings.Parental.AdultFilter.clear();
	for (int m_iAdultFilterRow = 0; m_iAdultFilterRow < ui->listWidgetParentalFilter->count(); m_iAdultFilterRow++)
	{
		ui->listWidgetParentalFilter->setCurrentRow(m_iAdultFilterRow);
		quazaaSettings.Parental.AdultFilter.append(ui->listWidgetParentalFilter->currentItem()->text());
	}

	// Save Library Settings
	quazaaSettings.Library.RememberViews = ui->checkBoxLibraryRememberViews->isChecked();
	quazaaSettings.Library.WatchFolders = ui->checkBoxLibraryWatchFolders->isChecked();
	quazaaSettings.Library.HighPriorityHashing = ui->checkBoxQuickHashing->isChecked();
	quazaaSettings.Library.HashWindow = ui->checkBoxDisplayHashingProgress->isChecked();
	quazaaSettings.Library.GhostFiles = ui->checkBoxLibraryGhostFiles->isChecked();
	quazaaSettings.Library.SmartSeriesDetection = ui->checkBoxVideoSeriesDetection->isChecked();
	quazaaSettings.Library.HistoryTotal = ui->spinBoxFileHistoryRemember->value();
	quazaaSettings.Library.HistoryDays = ui->spinBoxFileHistoryDays->value();
	quazaaSettings.Library.SafeExecuteTypes.clear();
	for (int m_iSafeOpenRow = 0; m_iSafeOpenRow < ui->listWidgetFileTypesSafeOpen->count(); m_iSafeOpenRow++)
	{
		ui->listWidgetFileTypesSafeOpen->setCurrentRow(m_iSafeOpenRow);
		quazaaSettings.Library.SafeExecuteTypes.append(ui->listWidgetFileTypesSafeOpen->currentItem()->text());
	}
	quazaaSettings.Library.NeverShareTypes.clear();
	for (int m_iNeverShareRow = 0; m_iNeverShareRow < ui->listWidgetFileTypesNeverShare->count(); m_iNeverShareRow++)
	{
		ui->listWidgetFileTypesNeverShare->setCurrentRow(m_iNeverShareRow);
		quazaaSettings.Library.NeverShareTypes.append(ui->listWidgetFileTypesNeverShare->currentItem()->text());
	}

	// Save File Types Settings

	// Save Media Player Settings

	// Save Search Settings
	quazaaSettings.Search.ExpandSearchMatches = ui->checkBoxSearchExpandMultiSorce->isChecked();
	quazaaSettings.Search.SwitchOnDownload = ui->checkBoxSearchSwitchOnDownload->isChecked();
	quazaaSettings.Search.HighlightNew = ui->checkBoxSearchHighlightNewMatches->isChecked();

	// Save Chat Settings
	quazaaSettings.Chat.GnutellaChatEnable = ui->checkBoxPrivateMessagesGnutella->isChecked();
	quazaaSettings.Chat.ConnectOnStartup = ui->checkBoxIrcConnectOnStart->isChecked();
	quazaaSettings.Chat.EnableFileTransfers = ui->checkBoxIrcEnableFileTransfers->isChecked();
	quazaaSettings.Chat.ShowTimestamp = ui->checkBoxIrcShowTimestamp->isChecked();
	quazaaSettings.Chat.AwayMessageIdleTime = ui->spinBoxPrivateMessagesIdleMessage->value();
	quazaaSettings.Chat.IrcServerName = ui->lineEditIrcServer->text();
	quazaaSettings.Chat.IrcServerPort = ui->spinBoxIrcPort->value();
	quazaaSettings.Chat.IrcUseSSL = ui->checkBoxIrcSSL->isChecked();

	// Save Connection Settings
	quazaaSettings.Connection.InSpeed = (ui->doubleSpinBoxInSpeed->value()/8)*1024;
	quazaaSettings.Connection.OutSpeed = (ui->doubleSpinBoxOutSpeed->value()/8)*1024;
	quazaaSettings.Connection.Port = ui->spinBoxNetworkPort->value();
	quazaaSettings.Connection.RandomPort = ui->checkBoxRandomPort->isChecked();
	quazaaSettings.Connection.TimeoutConnect = ui->spinBoxConnectionTimeout->value();

	// Save Web Settings
	quazaaSettings.Web.Magnet = ui->checkBoxIntegrationMagnetLinks->isChecked();
	quazaaSettings.Web.Gnutella = ui->checkBoxIntegrationGnutellaLinks->isChecked();
	quazaaSettings.Web.Ares = ui->checkBoxIntegrationAresLinks->isChecked();
	quazaaSettings.Web.Torrent = ui->checkBoxIntegrationBitTorrentLinks->isChecked();
	quazaaSettings.Web.Piolet = ui->checkBoxIntegrationPioletLinks->isChecked();
	quazaaSettings.Web.ED2K = ui->checkBoxIntegrationEDonkeyLinks->isChecked();
	quazaaSettings.Web.BrowserIntegration = ui->checkBoxManageWebDownloads->isChecked();
	quazaaSettings.Web.ManageDownloadTypes.clear();
	for (int m_iDownloadTypesRow = 0; m_iDownloadTypesRow < ui->listWidgetManageDownloadTypes->count(); m_iDownloadTypesRow++)
	{
		ui->listWidgetManageDownloadTypes->setCurrentRow(m_iDownloadTypesRow);
		quazaaSettings.Web.ManageDownloadTypes.append(ui->listWidgetManageDownloadTypes->currentItem()->text());
	}

	// Save Transfer Settings
	quazaaSettings.Transfers.RequireConnectedNetwork = ui->checkBoxOnlyDownloadConnectedNetworks->isChecked();
	quazaaSettings.Transfers.SimpleProgressBar = ui->checkBoxSimpleProgress->isChecked();

	// Save Download Settings
	quazaaSettings.Downloads.ExpandDownloads = ui->checkBoxExpandDownloads->isChecked();
	quazaaSettings.Downloads.CompletePath = ui->lineEditSaveFolder->text();
	quazaaSettings.Downloads.IncompletePath = ui->lineEditTempFolder->text();
	quazaaSettings.Downloads.QueueLimit = ui->comboBoxQueLength->currentIndex();
	quazaaSettings.Downloads.MaxFiles = ui->spinBoxMaxFiles->value();
	quazaaSettings.Downloads.MaxTransfers = ui->spinBoxMaxTransfers->value();
	quazaaSettings.Downloads.MaxTransfersPerFile = ui->spinBoxTransfersPerFile->value();

	// Save Upload Settings
	quazaaSettings.Uploads.SharePartials = ui->checkBoxSharePartials->isChecked();
	quazaaSettings.Uploads.HubShareLimiting = ui->checkBoxSharingLimitHub->isChecked();
	quazaaSettings.Uploads.SharePreviews = ui->checkBoxSharePreviews->isChecked();
	quazaaSettings.Uploads.MaxPerHost = ui->spinBoxUniqueHostLimit->value();

	// Save Security Settings
	quazaaSettings.Security.ChatFilter = ui->checkBoxChatFilterSpam->isChecked();
	quazaaSettings.Security.AllowProfileBrowse = ui->checkBoxAllowBrowseProfile->isChecked();
	quazaaSettings.Security.IrcFloodProtection = ui->checkBoxIrcFloodProtection->isChecked();
	quazaaSettings.Security.IrcFloodLimit = ui->spinBoxChatFloodLimit->value();
	quazaaSettings.Security.RemoteEnable = ui->checkBoxRemoteEnable->isChecked();
	quazaaSettings.Security.RemoteUsername = ui->lineEditRemoteUserName->text();
	quazaaSettings.Security.RemotePassword = ui->lineEditRemotePassword->text();
	quazaaSettings.Security.SearchIgnoreLocalIP = ui->checkBoxIgnoreLocalIP->isChecked();
	quazaaSettings.Security.EnableUPnP = ui->checkBoxEnableUPnP->isChecked();
	quazaaSettings.Security.AllowSharesBrowse = ui->checkBoxAllowBrowseShares->isChecked();
	quazaaSettings.Security.BlockedAgentUploadFilter.clear();
	for (int m_iUserAgentsRow = 0; m_iUserAgentsRow < ui->listWidgetUserAgents->count(); m_iUserAgentsRow++)
	{
		ui->listWidgetUserAgents->setCurrentRow(m_iUserAgentsRow);
		quazaaSettings.Security.BlockedAgentUploadFilter.append(ui->listWidgetUserAgents->currentItem()->text());
	}

	// Save Gnutella 2 Settings
	quazaaSettings.Gnutella2.Enable = ui->checkBoxConnectG2->isChecked();
	quazaaSettings.Gnutella2.ClientMode = ui->comboBoxG2Mode->currentIndex();
	quazaaSettings.Gnutella2.NumHubs = ui->spinBoxG2LeafToHub->value();
	quazaaSettings.Gnutella2.NumLeafs = ui->spinBoxG2HubToLeaf->value();
	quazaaSettings.Gnutella2.NumPeers = ui->spinBoxG2HubToHub->value();

	// Save Ares Settings
	quazaaSettings.Ares.Enable = ui->checkBoxConnectAres->isChecked();

	// Save eDonkey 2k Settings
	quazaaSettings.EDonkey.Enable = ui->checkBoxConnectEDonkey->isChecked();
	quazaaSettings.EDonkey.EnableKad = ui->checkBoxConnectKAD->isChecked();
	quazaaSettings.EDonkey.SearchCachedServers = ui->checkBoxED2kSearchCahedServers->isChecked();
	quazaaSettings.EDonkey.MaxResults = ui->spinBoxED2kMaxResults->value();
	quazaaSettings.EDonkey.LearnNewServers = ui->checkBoxED2kUpdateServerList->isChecked();
	quazaaSettings.EDonkey.MaxClients = ui->spinBoxED2kMaxClients->value();
	quazaaSettings.EDonkey.MetAutoQuery = ui->checkBoxAutoQueryServerList->isChecked();
	quazaaSettings.EDonkey.ServerListURL = ui->lineEditEDonkeyServerListUrl->text();

	// Save BitTorrent Settings
	quazaaSettings.BitTorrent.UseSaveDialog = ui->checkBoxTorrentSaveDialog->isChecked();
	quazaaSettings.BitTorrent.StartPaused = ui->checkBoxTorrentsStartPaused->isChecked();
	quazaaSettings.BitTorrent.UseTemp = ui->checkBoxTorrentsUseTemp->isChecked();
	quazaaSettings.BitTorrent.Managed = ui->checkBoxManagedTorrent->isChecked();
	quazaaSettings.BitTorrent.Endgame = ui->checkBoxTorrentsEndgame->isChecked();
	quazaaSettings.BitTorrent.DownloadTorrents = ui->spinBoxTorrentsSimultaneous->value();
	quazaaSettings.BitTorrent.DownloadConnections = ui->spinBoxTorrentsClientConnections->value();
	quazaaSettings.BitTorrent.AutoClear = ui->checkBoxTorrentsClearDownloaded->isChecked();
	quazaaSettings.BitTorrent.ClearRatio = ui->spinBoxTorrentsRatioClear->value();
	quazaaSettings.BitTorrent.PreferBTSources = ui->checkBoxTorrentsPreferTorrent->isChecked();
	quazaaSettings.BitTorrent.UseKademlia = ui->checkBoxTorrentsUseKademlia->isChecked();
	quazaaSettings.BitTorrent.TorrentPath = ui->lineEditTorrentFolder->text();

	if (newSkinSelected)
	{
		// Save Skin Settings
		quazaaSettings.Skin.File = skinFile;
		skinSettings.loadSkin(skinFile);

		emit skinChanged();
		quazaaSettings.saveSkinSettings();
	}

	// Reset Apply Enabled To False
	quazaaSettings.saveSettings();
	ui->pushButtonApply->setEnabled(false);
}

void DialogSettings::skinChangeEvent()
{
	ui->frameCommonHeader->setStyleSheet(skinSettings.dialogHeader);
	ui->frameSidebarContents->setStyleSheet(skinSettings.sidebarBackground);
	ui->toolButtonCommunityTask->setStyleSheet(skinSettings.sidebarTaskHeader);
	ui->toolButtonGeneralTask->setStyleSheet(skinSettings.sidebarTaskHeader);
	ui->toolButtonNetworkTask->setStyleSheet(skinSettings.sidebarTaskHeader);
	ui->toolButtonProtocolsTask->setStyleSheet(skinSettings.sidebarTaskHeader);
	ui->toolButtonSecurityTask->setStyleSheet(skinSettings.sidebarTaskHeader);
}

void DialogSettings::on_pushButtonProfileEdit_clicked()
{
	QSkinDialog *dlgSkinProfile = new QSkinDialog(false, true, false, false, this);
	DialogProfile *dlgProfile = new DialogProfile(this);

	dlgSkinProfile->addChildWidget(dlgProfile);

	connect(dlgProfile, SIGNAL(closed()), dlgSkinProfile, SLOT(close()));
	dlgSkinProfile->exec();
}

void DialogSettings::on_pushButtonShowParentalFilter_clicked()
{
	ui->groupBoxParentalFilter->setVisible(true);
}

void DialogSettings::on_labelConfigureG2_linkActivated(QString link)
{
	Q_UNUSED(link);
	switchSettingsPage(SettingsPage::Gnutella2);
}

void DialogSettings::on_labelConfigureAres_linkActivated(QString link)
{
	Q_UNUSED(link);
	switchSettingsPage(SettingsPage::Ares);
}

void DialogSettings::on_labelConfigureEDonkey_linkActivated(QString link)
{
	Q_UNUSED(link);
	switchSettingsPage(SettingsPage::EDonkey);
}

void DialogSettings::on_labelConfigureBitTorrent_linkActivated(QString link)
{
	Q_UNUSED(link);
	switchSettingsPage(SettingsPage::BitTorrent);
}

void DialogSettings::on_listWidgetSkins_itemClicked(QListWidgetItem* item)
{
	newSkinSelected = true;
	ui->pushButtonApply->setEnabled(true);
	QSettings reader((qApp->applicationDirPath() + "/Skin/" + item->text() + "/" + item->text() + ".qsk"), QSettings::IniFormat);
	skinFile = (qApp->applicationDirPath() + "/Skin/" + item->text() + "/" + item->text() + ".qsk");
	tempSkinName = reader.value("skinName", "").toString();
	tempSkinAuthor = reader.value("skinAuthor", "").toString();
	tempSkinVersion = reader.value("skinVersion", "").toString();
	tempSkinDescription = reader.value("skinDescription", "").toString();

	ui->labelSkinAuthor->setText(tr("Author: %1").arg(tempSkinAuthor));
	ui->labelSkinVersion->setText(tr("Version: %1").arg(tempSkinVersion));
	ui->plainTextEditSkinDescription->setPlainText(tempSkinDescription);
}

void DialogSettings::on_pushButtonSkinPreview_clicked()
{
	if (ui->listWidgetSkins->currentRow() != -1)
	{
		QSkinDialog *dlgSkinPreviewFrame = new QSkinDialog(true, true, false, true);
		DialogSkinPreview *dlgSkinPreview = new DialogSkinPreview(this);

		dlgSkinPreviewFrame->addChildWidget(dlgSkinPreview);

		connect(dlgSkinPreview, SIGNAL(closed()), dlgSkinPreviewFrame, SLOT(close()));
		dlgSkinPreviewFrame->loadPreviewSkin(skinFile);
		dlgSkinPreview->loadSkin(skinFile);
		dlgSkinPreviewFrame->exec();
	}
}
