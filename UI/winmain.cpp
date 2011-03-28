//
// winmain.cpp
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

#include "winmain.h"
#include "ui_winmain.h"
#include "QSkinDialog/qskindialog.h"
#include "dialogclosetype.h"
#include "dialogsplash.h"
#include "dialogwizard.h"
#include "dialogabout.h"
#include "dialogopentorrent.h"
#include "dialogcreatetorrent.h"
#include "dialogsettings.h"
#include "dialogeditshares.h"
#include "dialogadddownload.h"
#include "dialogdownloadsimport.h"
#include "dialoglanguage.h"
#include "dialogscheduler.h"
#include "dialogprofile.h"
#include "dialogaddrule.h"
#include "dialogsecuritysubscriptions.h"
#include "dialoglibrarysearch.h"
#include "dialogfiltersearch.h"
#include "dialogconnectto.h"

#include "quazaasettings.h"
#include "quazaaglobals.h"
#include "QSkinDialog/qskinsettings.h"
#include "commonfunctions.h"
#include "handshakes.h"
#include "network.h"
#include "neighbours.h"
#include "datagrams.h"
#include "geoiplist.h"
#include "ShareManager.h"

#include <QTimer>

void WinMain::quazaaStartup()
{
}

WinMain::WinMain(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::WinMain)
{
	ui->setupUi(this);

	//Initialize vaiables
	bypassCloseEvent = false;
	interfaceLoaded = false;

	skinChangeEvent();

	ui->actionAres->setChecked(quazaaSettings.Ares.Enable);
	ui->actionEDonkey->setChecked(quazaaSettings.EDonkey.Enable);
	ui->actionGnutella2->setChecked(quazaaSettings.Gnutella2.Enable);

	//Load And Set Up User Interface
	quazaaSettings.loadWindowSettings(this);
	restoreState(quazaaSettings.WinMain.MainToolbar);
	connect(&skinSettings, SIGNAL(skinChanged()), this, SLOT(skinChangeEvent()));

	//Set up the menu toolbar
	ui->toolBarMainMenu->addWidget(ui->menubarMain);

	//Set up the status bar
	tcpFirewalled = ":/Resource/Network/ShieldRed.png";
	udpFirewalled = ":/Resource/Network/ShieldRed.png";
	labelFirewallStatus = new QLabel(tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\"> <html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head><body style=\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\"><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">TCP: <img src=\":/Resource/Network/ShieldRed.png\" /> UDP: <img src=\":/Resource/Network/ShieldRed.png\" /></p></body></html>"));
	ui->statusbar->addPermanentWidget(labelFirewallStatus);
	labelBandwidthTotals = new QLabel();
	ui->statusbar->addPermanentWidget(labelBandwidthTotals);

	//Add the tabs
	pageHome = new WidgetHome();
	ui->stackedWidgetMain->addWidget(pageHome);
	pageLibrary = new WidgetLibrary();
	ui->stackedWidgetMain->addWidget(pageLibrary);
	pageMedia = new WidgetMedia();
	ui->stackedWidgetMain->addWidget(pageMedia);
	pageSearch = new WidgetSearch();
	ui->stackedWidgetMain->addWidget(pageSearch);
	pageTransfers = new WidgetTransfers();
	ui->stackedWidgetMain->addWidget(pageTransfers);
	pageSecurity = new WidgetSecurity();
	ui->stackedWidgetMain->addWidget(pageSecurity);
	pageActivity = new WidgetActivity();
	ui->stackedWidgetMain->addWidget(pageActivity);
	pageChat = new WidgetChat();
	ui->stackedWidgetMain->addWidget(pageChat);
	pageHostCache = new WidgetHostCache;
	ui->stackedWidgetMain->addWidget(pageHostCache);
	pageDiscovery = new WidgetDiscovery;
	ui->stackedWidgetMain->addWidget(pageDiscovery);
	pageScheduler = new WidgetScheduler;
	ui->stackedWidgetMain->addWidget(pageScheduler);
	pageGraph = new WidgetGraph;
	ui->stackedWidgetMain->addWidget(pageGraph);
	pagePacketDump = new WidgetPacketDump;
	ui->stackedWidgetMain->addWidget(pagePacketDump);
	pageSearchMonitor = new WidgetSearchMonitor;
	ui->stackedWidgetMain->addWidget(pageSearchMonitor);
	pageHitMonitor = new WidgetHitMonitor;
	ui->stackedWidgetMain->addWidget(pageHitMonitor);

	// Set up the navigation toolbar
	actionGroupMainNavigation = new QActionGroup(this);
	actionGroupMainNavigation->addAction(ui->actionHome);
	actionGroupMainNavigation->addAction(ui->actionLibrary);
	actionGroupMainNavigation->addAction(ui->actionMedia);
	actionGroupMainNavigation->addAction(ui->actionSearch);
	actionGroupMainNavigation->addAction(ui->actionTransfers);
	actionGroupMainNavigation->addAction(ui->actionSecurity);
	actionGroupMainNavigation->addAction(ui->actionActivity);
	actionGroupMainNavigation->addAction(ui->actionChat);
	actionGroupMainNavigation->addAction(ui->actionHostCache);
	actionGroupMainNavigation->addAction(ui->actionDiscovery);
	actionGroupMainNavigation->addAction(ui->actionScheduler);
	actionGroupMainNavigation->addAction(ui->actionGraph);
	actionGroupMainNavigation->addAction(ui->actionPacketDump);
	actionGroupMainNavigation->addAction(ui->actionSearchMonitor);
	actionGroupMainNavigation->addAction(ui->actionHitMonitor);
	ui->stackedWidgetMain->setCurrentIndex(quazaaSettings.WinMain.ActiveTab);
	switch(quazaaSettings.WinMain.ActiveTab)
	{
		case 0:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Home.png"));
			ui->labelMainHeaderText->setText(tr("Quazaa Home"));
			ui->frameMainHeader->setStyleSheet(skinSettings.homeHeader);
			ui->actionHome->setChecked(true);
			break;
		case 1:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Library/Library.png"));
			ui->labelMainHeaderText->setText(tr("Library"));
			ui->frameMainHeader->setStyleSheet(skinSettings.libraryHeader);
			ui->actionLibrary->setChecked(true);
			break;
		case 2:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Media/Media.png"));
			ui->labelMainHeaderText->setText(tr("Media"));
			ui->frameMainHeader->setStyleSheet(skinSettings.mediaHeader);
			ui->actionMedia->setChecked(true);
			break;
		case 3:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Search.png"));
			ui->labelMainHeaderText->setText(tr("Search"));
			ui->frameMainHeader->setStyleSheet(skinSettings.searchHeader);
			ui->actionSearch->setChecked(true);
			break;
		case 4:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Transfers.png"));
			ui->labelMainHeaderText->setText(tr("Transfers"));
			ui->frameMainHeader->setStyleSheet(skinSettings.transfersHeader);
			ui->actionTransfers->setChecked(true);
			break;
		case 5:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Security/Security.png"));
			ui->labelMainHeaderText->setText(tr("Security"));
			ui->frameMainHeader->setStyleSheet(skinSettings.securityHeader);
			ui->actionSecurity->setChecked(true);
			break;
		case 6:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Globe.png"));
			ui->labelMainHeaderText->setText(tr("Activity"));
			ui->frameMainHeader->setStyleSheet(skinSettings.activityHeader);
			ui->actionActivity->setChecked(true);
			break;
		case 7:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Chat/Chat.png"));
			ui->labelMainHeaderText->setText(tr("Chat"));
			ui->frameMainHeader->setStyleSheet(skinSettings.chatHeader);
			ui->actionChat->setChecked(true);
			break;
		case 8:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/HostCache.png"));
			ui->labelMainHeaderText->setText(tr("Host Cache"));
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			ui->actionHostCache->setChecked(true);
			break;
		case 9:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/Discovery.png"));
			ui->labelMainHeaderText->setText(tr("Discovery"));
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			ui->actionDiscovery->setChecked(true);
			break;
		case 10:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Scheduler.png"));
			ui->labelMainHeaderText->setText(tr("Scheduler"));
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			ui->actionScheduler->setChecked(true);
			break;
		case 11:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Graph.png"));
			ui->labelMainHeaderText->setText(tr("Graph"));
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			ui->actionGraph->setChecked(true);
			break;
		case 12:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/PacketDump.png"));
			ui->labelMainHeaderText->setText(tr("Packet Dump"));
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			ui->actionPacketDump->setChecked(true);
			break;
		case 13:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/SearchMonitor.png"));
			ui->labelMainHeaderText->setText(tr("Search Monitor"));
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			ui->actionSearchMonitor->setChecked(true);
			break;
		case 14:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/HitMonitor.png"));
			ui->labelMainHeaderText->setText(tr("Hit Monitor"));
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			ui->actionHitMonitor->setChecked(true);
			break;
		default:
			ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Home.png"));
			ui->labelMainHeaderText->setText(tr("Quazaa Home"));
			ui->frameMainHeader->setStyleSheet(skinSettings.homeHeader);
			ui->actionHome->setChecked(true);
			break;
	}
	connect(ui->actionNewSearch, SIGNAL(triggered()), pageSearch, SLOT(on_toolButtonNewSearch_clicked()));
	connect(pageHome, SIGNAL(requestSearch(QString*)), this, SLOT(startNewSearch(QString*)));
	connect(pageHome, SIGNAL(triggerLibrary()), this, SLOT(on_actionLibrary_triggered()));
	connect(pageHome, SIGNAL(triggerSecurity()), this, SLOT(on_actionSecurity_triggered()));
	connect(pageHome, SIGNAL(triggerTransfers()), this, SLOT(on_actionTransfers_triggered()));

	QSortFilterProxyModel* neighboursSortModel = new QSortFilterProxyModel(this);
	neighboursList = new CNeighboursTableModel(this, pageActivity->panelNeighbours->treeView());
	neighboursSortModel->setSourceModel(neighboursList);
	pageActivity->panelNeighbours->setModel(neighboursSortModel);
	//pageActivity->panelNeighbours->setModel(neighboursList);
	neighboursSortModel->setDynamicSortFilter(true);

	neighboursRefresher = new QTimer(this);
	connect(neighboursRefresher, SIGNAL(timeout()), neighboursList, SLOT(UpdateAll()));
	connect(neighboursRefresher, SIGNAL(timeout()), this, SLOT(updateStatusBar()));
	connect(neighboursRefresher, SIGNAL(timeout()), pageActivity->panelNeighbours, SLOT(updateG2()));

	update();
	qApp->processEvents();

	interfaceLoaded = true;
}

WinMain::~WinMain()
{
	delete ui;
}
void WinMain::loadTrayIcon()
{
	// Create the system tray right click menu.
	trayMenu = new QMenu(this);
	trayMenu->addAction(ui->actionShowOrHide);
	trayMenu->addSeparator();
	trayMenu->addAction(ui->actionNewSearch);
	trayMenu->addAction(ui->actionURLDownload);
	trayMenu->addSeparator();
	trayMenu->addAction(ui->actionMediaPlay);
	trayMenu->addAction(ui->actionMediaStop);
	trayMenu->addAction(ui->actionMediaOpen);
	trayMenu->addSeparator();
	trayMenu->addAction(ui->actionMediaRewind);
	trayMenu->addAction(ui->actionMediaNextTrack);
	trayMenu->addSeparator();
	trayMenu->addAction(ui->actionExitAfterTransfers);
	trayMenu->addAction(ui->actionExit);
	// Create the system tray icon
	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setIcon(QIcon(":/Resource/Quazaa.png"));
	trayIcon->setToolTip(tr("Quazaa"));
	trayIcon->setContextMenu(trayMenu);
	// Connect an event handler to the tray icon so we can handle mouse events
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	        this, SLOT(icon_activated(QSystemTrayIcon::ActivationReason)));
	trayIcon->show();
}

bool WinMain::event(QEvent* e)
{
	QMainWindow::event(e);
	switch(e->type())
	{
		case QEvent::Close:
			if(!bypassCloseEvent)
			{
				if(quazaaSettings.System.CloseMode == 0)
				{

					QSkinDialog* dlgSkinCloseType = new QSkinDialog(false, false, false);
					DialogCloseType* dlgCloseType = new DialogCloseType(this);

					dlgSkinCloseType->addChildWidget(dlgCloseType);

					connect(dlgCloseType, SIGNAL(closed()), dlgSkinCloseType, SLOT(close()));
					dlgSkinCloseType->exec();
				}

				switch(quazaaSettings.System.CloseMode)
				{
					case 1:
						quazaaShutdown();
						return false;
					case 2:
						emit hideMain();
						e->ignore();
						return true;
					case 3:
						quazaaShutdown();
						return false;
					default:
						quazaaShutdown();
						return false;
				}
			}
			else
			{
				quazaaShutdown();
				return false;
			}
		case QEvent::Show:
			neighboursRefresher->start(1000);
			break;
		case QEvent::Hide:
			if(neighboursRefresher)
			{
				neighboursRefresher->stop();
			}
			break;
		default:
			return false;
	}

	return false;
}

void WinMain::changeEvent(QEvent* e)
{
	QMainWindow::changeEvent(e);
	switch(e->type())
	{
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			switch(quazaaSettings.WinMain.ActiveTab)
			{
				case 0:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Home.png"));
					ui->labelMainHeaderText->setText(tr("Quazaa Home"));
					ui->frameMainHeader->setStyleSheet(skinSettings.homeHeader);
					ui->actionHome->setChecked(true);
					break;
				case 1:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Library/Library.png"));
					ui->labelMainHeaderText->setText(tr("Library"));
					ui->frameMainHeader->setStyleSheet(skinSettings.libraryHeader);
					ui->actionLibrary->setChecked(true);
					break;
				case 2:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Media/Media.png"));
					ui->labelMainHeaderText->setText(tr("Media"));
					ui->frameMainHeader->setStyleSheet(skinSettings.mediaHeader);
					ui->actionMedia->setChecked(true);
					break;
				case 3:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Search.png"));
					ui->labelMainHeaderText->setText(tr("Search"));
					ui->frameMainHeader->setStyleSheet(skinSettings.searchHeader);
					ui->actionSearch->setChecked(true);
					break;
				case 4:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Transfers.png"));
					ui->labelMainHeaderText->setText(tr("Transfers"));
					ui->frameMainHeader->setStyleSheet(skinSettings.transfersHeader);
					ui->actionTransfers->setChecked(true);
					break;
				case 5:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Security/Security.png"));
					ui->labelMainHeaderText->setText(tr("Security"));
					ui->frameMainHeader->setStyleSheet(skinSettings.securityHeader);
					ui->actionSecurity->setChecked(true);
					break;
				case 6:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Globe.png"));
					ui->labelMainHeaderText->setText(tr("Activity"));
					ui->frameMainHeader->setStyleSheet(skinSettings.activityHeader);
					ui->actionActivity->setChecked(true);
					break;
				case 7:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Chat/Chat.png"));
					ui->labelMainHeaderText->setText(tr("Chat"));
					ui->frameMainHeader->setStyleSheet(skinSettings.chatHeader);
					ui->actionChat->setChecked(true);
					break;
				case 8:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/HostCache.png"));
					ui->labelMainHeaderText->setText(tr("Host Cache"));
					ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
					ui->actionHostCache->setChecked(true);
					break;
				case 9:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/Discovery.png"));
					ui->labelMainHeaderText->setText(tr("Discovery"));
					ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
					ui->actionDiscovery->setChecked(true);
					break;
				case 10:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Scheduler.png"));
					ui->labelMainHeaderText->setText(tr("Scheduler"));
					ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
					ui->actionScheduler->setChecked(true);
					break;
				case 11:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Graph.png"));
					ui->labelMainHeaderText->setText(tr("Graph"));
					ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
					ui->actionGraph->setChecked(true);
					break;
				case 12:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/PacketDump.png"));
					ui->labelMainHeaderText->setText(tr("Packet Dump"));
					ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
					ui->actionPacketDump->setChecked(true);
					break;
				case 13:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/SearchMonitor.png"));
					ui->labelMainHeaderText->setText(tr("Search Monitor"));
					ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
					ui->actionSearchMonitor->setChecked(true);
					break;
				case 14:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/HitMonitor.png"));
					ui->labelMainHeaderText->setText(tr("Hit Monitor"));
					ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
					ui->actionHitMonitor->setChecked(true);
					break;
				default:
					ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Home.png"));
					ui->labelMainHeaderText->setText(tr("Quazaa Home"));
					ui->frameMainHeader->setStyleSheet(skinSettings.homeHeader);
					ui->actionHome->setChecked(true);
					break;
			}
			break;
		default:
			break;
	}
}

void WinMain::quazaaShutdown()
{
	DialogSplash* dlgSplash = new DialogSplash;
	dlgSplash->show();

	dlgSplash->updateProgress(5, tr("Closing Networks..."));
	neighboursRefresher->stop();
	delete neighboursRefresher;
	neighboursRefresher = 0;
	Network.Disconnect();
	ShareManager.Stop();

	dlgSplash->updateProgress(10, tr("Saving Settings..."));
	quazaaSettings.saveSettings();

	dlgSplash->updateProgress(15, tr("Saving UI..."));
	quazaaSettings.WinMain.MainToolbar = saveState();
	pageHome->saveWidget();
	pageLibrary->saveWidget();
	pageMedia->saveWidget();
	pageSearch->saveWidget();
	pageTransfers->saveWidget();
	pageSecurity->saveWidget();
	pageActivity->saveWidget();
	pageChat->saveWidget();
	pageHostCache->saveWidget();
	pageDiscovery->saveWidget();
	pageScheduler->saveWidget();
	pageGraph->saveWidget();
	pagePacketDump->saveWidget();
	pageSearchMonitor->saveWidget();
	pageHitMonitor->saveWidget();
	quazaaSettings.saveWindowSettings(this);

	dlgSplash->updateProgress(20, tr("Removing Tray Icon..."));
	delete trayIcon;

	dlgSplash->close();
	emit closed();
}

void WinMain::on_actionHome_triggered()
{
	ui->actionHome->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Home.png"));
	ui->labelMainHeaderText->setText(tr("Quazaa Home"));
	ui->frameMainHeader->setStyleSheet(skinSettings.homeHeader);
	ui->stackedWidgetMain->setCurrentIndex(0);
	quazaaSettings.WinMain.ActiveTab = 0;
}

void WinMain::on_actionLibrary_triggered()
{
	ui->actionLibrary->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Library/Library.png"));
	ui->labelMainHeaderText->setText(tr("Library"));
	ui->frameMainHeader->setStyleSheet(skinSettings.libraryHeader);
	ui->stackedWidgetMain->setCurrentIndex(1);
	quazaaSettings.WinMain.ActiveTab = 1;
}

void WinMain::on_actionMedia_triggered()
{
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Media/Media.png"));
	ui->labelMainHeaderText->setText(tr("Media"));
	ui->frameMainHeader->setStyleSheet(skinSettings.mediaHeader);
	ui->stackedWidgetMain->setCurrentIndex(2);
	quazaaSettings.WinMain.ActiveTab = 2;
}

void WinMain::on_actionSearch_triggered()
{
	ui->actionSearch->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Search.png"));
	ui->labelMainHeaderText->setText(tr("Search"));
	ui->frameMainHeader->setStyleSheet(skinSettings.searchHeader);
	ui->stackedWidgetMain->setCurrentIndex(3);
	quazaaSettings.WinMain.ActiveTab = 3;
}

void WinMain::on_actionTransfers_triggered()
{
	ui->actionTransfers->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Transfers.png"));
	ui->labelMainHeaderText->setText(tr("Transfers"));
	ui->frameMainHeader->setStyleSheet(skinSettings.transfersHeader);
	ui->stackedWidgetMain->setCurrentIndex(4);
	quazaaSettings.WinMain.ActiveTab = 4;
}

void WinMain::on_actionSecurity_triggered()
{
	ui->actionSecurity->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Security/Security.png"));
	ui->labelMainHeaderText->setText(tr("Security"));
	ui->frameMainHeader->setStyleSheet(skinSettings.securityHeader);
	ui->stackedWidgetMain->setCurrentIndex(5);
	quazaaSettings.WinMain.ActiveTab = 5;
}

void WinMain::on_actionActivity_triggered()
{
	ui->actionActivity->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Globe.png"));
	ui->labelMainHeaderText->setText(tr("Activity"));
	ui->frameMainHeader->setStyleSheet(skinSettings.activityHeader);
	ui->stackedWidgetMain->setCurrentIndex(6);
	quazaaSettings.WinMain.ActiveTab = 6;
}

void WinMain::on_actionChat_triggered()
{
	ui->actionChat->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Chat/Chat.png"));
	ui->labelMainHeaderText->setText(tr("Chat"));
	ui->frameMainHeader->setStyleSheet(skinSettings.chatHeader);
	ui->stackedWidgetMain->setCurrentIndex(7);
	quazaaSettings.WinMain.ActiveTab = 7;
}

void WinMain::on_actionHostCache_triggered()
{
	ui->actionHostCache->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/HostCache.png"));
	ui->labelMainHeaderText->setText(tr("Host Cache"));
	ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
	ui->stackedWidgetMain->setCurrentIndex(8);
	quazaaSettings.WinMain.ActiveTab = 8;
}

void WinMain::on_actionDiscovery_triggered()
{
	ui->actionDiscovery->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/Discovery.png"));
	ui->labelMainHeaderText->setText(tr("Discovery"));
	ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
	ui->stackedWidgetMain->setCurrentIndex(9);
	quazaaSettings.WinMain.ActiveTab = 9;
}

void WinMain::on_actionScheduler_triggered()
{
	ui->actionScheduler->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Scheduler.png"));
	ui->labelMainHeaderText->setText(tr("Scheduler"));
	ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
	ui->stackedWidgetMain->setCurrentIndex(10);
	quazaaSettings.WinMain.ActiveTab = 10;
}

void WinMain::on_actionGraph_triggered()
{
	ui->actionGraph->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Generic/Graph.png"));
	ui->labelMainHeaderText->setText(tr("Graph"));
	ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
	ui->stackedWidgetMain->setCurrentIndex(11);
	quazaaSettings.WinMain.ActiveTab = 11;
}

void WinMain::on_actionPacketDump_triggered()
{
	ui->actionPacketDump->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/PacketDump.png"));
	ui->labelMainHeaderText->setText(tr("Packet Dump"));
	ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
	ui->stackedWidgetMain->setCurrentIndex(12);
	quazaaSettings.WinMain.ActiveTab = 12;
}

void WinMain::on_actionSearchMonitor_triggered()
{
	ui->actionSearchMonitor->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/SearchMonitor.png"));
	ui->labelMainHeaderText->setText(tr("Search Monitor"));
	ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
	ui->stackedWidgetMain->setCurrentIndex(13);
	quazaaSettings.WinMain.ActiveTab = 13;
}

void WinMain::on_actionHitMonitor_triggered()
{
	ui->actionHitMonitor->setChecked(true);
	ui->labelMainHeaderLogo->setPixmap(QPixmap(":/Resource/Network/HitMonitor.png"));
	ui->labelMainHeaderText->setText(tr("Hit Monitor"));
	ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
	ui->stackedWidgetMain->setCurrentIndex(14);
	quazaaSettings.WinMain.ActiveTab = 14;
}

void WinMain::skinChangeEvent()
{
	setStyleSheet(skinSettings.standardItems);
	ui->toolBarNavigation->setStyleSheet(skinSettings.navigationToolbar);
	ui->toolBarMainMenu->setStyleSheet(skinSettings.mainMenuToolbar);
	switch(ui->stackedWidgetMain->currentIndex())
	{
		case 0:
			ui->frameMainHeader->setStyleSheet(skinSettings.homeHeader);
			break;
		case 1:
			ui->frameMainHeader->setStyleSheet(skinSettings.libraryHeader);
			break;
		case 2:
			ui->frameMainHeader->setStyleSheet(skinSettings.mediaHeader);
			break;
		case 3:
			ui->frameMainHeader->setStyleSheet(skinSettings.searchHeader);
			break;
		case 4:
			ui->frameMainHeader->setStyleSheet(skinSettings.transfersHeader);
			break;
		case 5:
			ui->frameMainHeader->setStyleSheet(skinSettings.securityHeader);
			break;
		case 6:
			ui->frameMainHeader->setStyleSheet(skinSettings.activityHeader);
			break;
		case 7:
			ui->frameMainHeader->setStyleSheet(skinSettings.chatHeader);
			break;
		case 8:
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			break;
		case 9:
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			break;
		case 10:
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			break;
		case 11:
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			break;
		case 12:
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			break;
		case 13:
			ui->frameMainHeader->setStyleSheet(skinSettings.genericHeader);
			break;
		default:
			ui->frameMainHeader->setStyleSheet(skinSettings.homeHeader);
			break;
	}
}

void WinMain::icon_activated(QSystemTrayIcon::ActivationReason reason)
{
	switch(reason)
	{
		case QSystemTrayIcon::Unknown:
			break;
		case QSystemTrayIcon::DoubleClick:
			ui->actionShowOrHide->trigger();
			break;
		case QSystemTrayIcon::Trigger:
			break;
		case QSystemTrayIcon::MiddleClick:
			break;
		default:
			break;
	}
}

void WinMain::on_actionShowOrHide_triggered()
{
	if(isVisible())
	{
		emit hideMain();
	}
	else
	{
		emit showMain();
	}
}

void WinMain::on_actionExit_triggered()
{
	bypassCloseEvent = true;
	close();
}

void WinMain::on_actionAbout_triggered()
{
	QSkinDialog* dlgSkinAbout = new QSkinDialog(false, true, false, false, this);
	DialogAbout* dlgAbout = new DialogAbout;

	dlgSkinAbout->addChildWidget(dlgAbout);

	connect(dlgAbout, SIGNAL(closed()), dlgSkinAbout, SLOT(close()));
	dlgSkinAbout->show();
}

void WinMain::on_actionSettings_triggered()
{
	QSkinDialog* dlgSkinSettings = new QSkinDialog(true, true, false, false, this);
	DialogSettings* dlgSettings = new DialogSettings(this);

	dlgSkinSettings->addChildWidget(dlgSettings);

	connect(dlgSettings, SIGNAL(closed()), dlgSkinSettings, SLOT(close()));
	dlgSkinSettings->show();
}

void WinMain::on_actionCreateTorrent_triggered()
{
	QSkinDialog* dlgSkinCreateTorrent = new QSkinDialog(false, true, false, false, this);
	DialogCreateTorrent* dlgCreateTorrent = new DialogCreateTorrent;

	dlgSkinCreateTorrent->addChildWidget(dlgCreateTorrent);

	connect(dlgCreateTorrent, SIGNAL(closed()), dlgSkinCreateTorrent, SLOT(close()));
	dlgSkinCreateTorrent->show();
}

void WinMain::on_actionSeedTorrent_triggered()
{

}

void WinMain::on_actionOpenTorrent_triggered()
{
	QSkinDialog* dlgSkinOpenTorrent = new QSkinDialog(false, true, false, false, this);
	DialogOpenTorrent* dlgOpenTorrent = new DialogOpenTorrent;

	dlgSkinOpenTorrent->addChildWidget(dlgOpenTorrent);

	connect(dlgOpenTorrent, SIGNAL(closed()), dlgSkinOpenTorrent, SLOT(close()));
	dlgSkinOpenTorrent->show();
}

void WinMain::on_actionShares_triggered()
{
	QSkinDialog* dlgSkinEditShares = new QSkinDialog(false, true, false, false, this);
	DialogEditShares* dlgEditShares = new DialogEditShares;

	dlgSkinEditShares->addChildWidget(dlgEditShares);

	connect(dlgEditShares, SIGNAL(closed()), dlgSkinEditShares, SLOT(close()));
	dlgSkinEditShares->show();
}

void WinMain::on_actionOpenDownloadFolder_triggered()
{
	Functions.FolderOpen(quazaaSettings.Downloads.CompletePath);
}

void WinMain::on_actionURLDownload_triggered()
{
	QSkinDialog* dlgSkinAddDownload = new QSkinDialog(false, true, false, false, this);
	DialogAddDownload* dlgAddDownload = new DialogAddDownload;

	dlgSkinAddDownload->addChildWidget(dlgAddDownload);

	connect(dlgAddDownload, SIGNAL(closed()), dlgSkinAddDownload, SLOT(close()));
	dlgSkinAddDownload->show();
}

void WinMain::on_actionImportPartials_triggered()
{
	QSkinDialog* dlgSkinDownloadsImport = new QSkinDialog(false, true, false, false, this);
	DialogDownloadsImport* dlgDownloadsImport = new DialogDownloadsImport;

	dlgSkinDownloadsImport->addChildWidget(dlgDownloadsImport);

	connect(dlgDownloadsImport, SIGNAL(closed()), dlgSkinDownloadsImport, SLOT(close()));
	dlgSkinDownloadsImport->show();
}

void WinMain::on_actionChooseSkin_triggered()
{
	QSkinDialog* dlgSkinSettings = new QSkinDialog(true, true, false, false, this);
	DialogSettings* dlgSettings = new DialogSettings(this, SettingsPage::Skins);

	dlgSkinSettings->addChildWidget(dlgSettings);

	connect(dlgSettings, SIGNAL(closed()), dlgSkinSettings, SLOT(close()));
	dlgSkinSettings->show();
}

void WinMain::on_actionChooseLanguage_triggered()
{
	QSkinDialog* dlgSkinLanguage = new QSkinDialog(false, true, false, false, this);
	DialogLanguage* dlgLanguage = new DialogLanguage;

	dlgSkinLanguage->addChildWidget(dlgLanguage);

	connect(dlgLanguage, SIGNAL(closed()), dlgSkinLanguage, SLOT(close()));
	dlgSkinLanguage->exec();
}

void WinMain::on_actionQuickstartWizard_triggered()
{
	QSkinDialog* dlgSkinWizard = new QSkinDialog(false, true, false, false, this);
	DialogWizard* dlgWizard = new DialogWizard();

	dlgSkinWizard->addChildWidget(dlgWizard);

	connect(dlgWizard, SIGNAL(closed()), dlgSkinWizard, SLOT(close()));
	dlgSkinWizard->show();
}

void WinMain::on_actionUsersGuide_triggered()
{
	QDesktopServices::openUrl(QUrl("http://quazaa.sourceforge.net/wiki/Manual", QUrl::TolerantMode));
}

void WinMain::on_actionFAQ_triggered()
{
	QDesktopServices::openUrl(QUrl("http://quazaa.sourceforge.net/wiki/FAQ", QUrl::TolerantMode));
}

void WinMain::on_actionConnectionTest_triggered()
{
	QDesktopServices::openUrl(QUrl(QString("http://jlh.no-ip.org/connectiontest/index.php?port=%1&lang=%2&test=1").arg(quazaaSettings.Connection.Port).arg("en"), QUrl::TolerantMode));
}

void WinMain::on_actionCheckForNewVersion_triggered()
{

}

void WinMain::on_actionDonate_triggered()
{
	QDesktopServices::openUrl(QUrl("https://sourceforge.net/donate/index.php?group_id=286623", QUrl::TolerantMode));
}

void WinMain::on_actionQuazaaForums_triggered()
{
	QDesktopServices::openUrl(QUrl("http://quazaa.sourceforge.net/forum/", QUrl::TolerantMode));
}

void WinMain::on_actionEditMyProfile_triggered()
{
	QSkinDialog* dlgSkinProfile = new QSkinDialog(true, true, false, false, this);
	DialogProfile* dlgProfile = new DialogProfile;

	dlgSkinProfile->addChildWidget(dlgProfile);

	connect(dlgProfile, SIGNAL(closed()), dlgSkinProfile, SLOT(close()));
	dlgSkinProfile->show();
}

void WinMain::on_actionNewSearch_triggered()
{
	ui->actionSearch->trigger();
}

void WinMain::on_actionConnect_triggered()
{
	ui->actionConnect->setEnabled(false);
	ui->actionDisconnect->setEnabled(true);
	Network.Connect();
}

void WinMain::on_actionDisconnect_triggered()
{
	ui->actionConnect->setEnabled(true);
	ui->actionDisconnect->setEnabled(false);
	Network.Disconnect();
}

void WinMain::on_actionEDonkey_triggered(bool checked)
{
	quazaaSettings.EDonkey.Enable = checked;
}

void WinMain::on_actionGnutella2_triggered(bool checked)
{
	quazaaSettings.Gnutella2.Enable = checked;
}

void WinMain::on_actionAres_triggered(bool checked)
{
	quazaaSettings.Ares.Enable = checked;
}

void WinMain::startNewSearch(QString* searchString)
{
	ui->stackedWidgetMain->setCurrentIndex(3);
	pageSearch->startNewSearch(searchString);
}


void WinMain::updateStatusBar()
{
	quint16 nTCPInSpeed = 0;
	quint16 nTCPOutSpeed = 0;
	quint16 nUDPInSpeed = 0;
	quint16 nUDPOutSpeed = 0;

	if(!Network.m_bActive)
	{
		return;
	}

	if(Handshakes.m_pSection.tryLock(50))
	{
		if(!Handshakes.IsFirewalled())
		{
			tcpFirewalled = ":/Resource/Network/CheckedShieldGreen.png";
		}
		else
		{
			tcpFirewalled = ":/Resource/Network/ShieldRed.png";
		}
		Handshakes.m_pSection.unlock();
	}

	if(Network.m_pSection.tryLock(50))
	{
		if(!Datagrams.IsFirewalled())
		{
			udpFirewalled = ":/Resource/Network/CheckedShieldGreen.png";
		}
		else
		{
			udpFirewalled = ":/Resource/Network/ShieldRed.png";
		}
		nTCPInSpeed = Neighbours.DownloadSpeed();
		nTCPOutSpeed = Neighbours.UploadSpeed();
		nUDPInSpeed = Datagrams.DownloadSpeed();
		nUDPOutSpeed = Datagrams.UploadSpeed();
		Network.m_pSection.unlock();
	}

	labelFirewallStatus->setText(tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\"> <html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head><body style=\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\"><p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">TCP: <img src=\"%1\" /> UDP: <img src=\"%2\" /></p></body></html>").arg(tcpFirewalled).arg(udpFirewalled));
	labelBandwidthTotals->setText(tr("%1/s In:%2/s Out [D:%3/U:%4]").arg(Functions.FormatBytes(nTCPInSpeed + nUDPInSpeed)).arg(Functions.FormatBytes(nTCPOutSpeed + nUDPOutSpeed)).arg("0").arg("0"));
}

void WinMain::on_actionConnectTo_triggered()
{
	QSkinDialog* dlgSkinConnectTo = new QSkinDialog(false, true, false, false, this);
	DialogConnectTo* dlgConnectTo = new DialogConnectTo;

	dlgSkinConnectTo->addChildWidget(dlgConnectTo);

	connect(dlgConnectTo, SIGNAL(closed()), dlgSkinConnectTo, SLOT(close()));
	dlgSkinConnectTo->show();
}
