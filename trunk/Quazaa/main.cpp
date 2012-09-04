/*
** $Id$
**
** Copyright © Quazaa Development Team, 2009-2012.
** This file is part of QUAZAA (quazaa.sourceforge.net)
**
** Quazaa is free software; this file may be used under the terms of the GNU
** General Public License version 3.0 or later as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Quazaa is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
** Please review the following information to ensure the GNU General Public
** License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** You should have received a copy of the GNU General Public License version
** 3.0 along with Quazaa; if not, write to the Free Software Foundation,
** Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <QtWidgets/QApplication>

#include "winmain.h"
#include "quazaaglobals.h"
#include "quazaasettings.h"
#include "dialoglanguage.h"
#include "timedsignalqueue.h"
#include <QtSingleApplication>

#include "geoiplist.h"
#include "network.h"
#include "queryhashmaster.h"
#include "dialogsplash.h"
#include "dialoglanguage.h"
#include "wizardquickstart.h"
#include "sharemanager.h"
#include "commonfunctions.h"
#include "transfers.h"
#include "hostcache.h"

#include "Security/security.h"

#ifdef Q_OS_LINUX
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif // Q_OS_LINUX

#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG
#include "debug_new.h"
#endif

#ifdef _SNAPSHOT_BUILD
	#include <QMessageBox>
	#include "version.h"
#endif

/*void myMessageOutput(QtMsgType type, const char *msg)
{
	switch (type) {
	case QtDebugMsg:
		fprintf(stderr, "Debug: %s\n", msg);
		break;
	case QtWarningMsg:
		fprintf(stderr, "Warning: %s\n", msg);
		break;
	case QtCriticalMsg:
		fprintf(stderr, "Critical: %s\n", msg);
		break;
	case QtFatalMsg:
		fprintf(stderr, "Fatal: %s\n", msg);
		abort();
	}
}*/

QuazaaGlobals quazaaGlobals;

int main(int argc, char *argv[])
{
	//qInstallMsgHandler(myMessageOutput);
	QtSingleApplication theApp( argc, argv );

	QStringList args = QApplication::arguments();

	// Check if the application is already running
	if( theApp.sendMessage( "Show App" ) )
	{
		return 0;
	}

// To enable this, run qmake with "DEFINES+=_SNAPSHOT_BUILD"
#ifdef _SNAPSHOT_BUILD
	QDate oExpire = QDate::fromString(Version::BUILD_DATE, Qt::ISODate).addDays(60);

	if( QDate::currentDate() > oExpire )
	{
		QMessageBox::information(NULL, QObject::tr("Cool Software, but..."), QObject::tr("This build is expired. If you wish to continue using this Cool Software you must download either latest stable release or latest snapshot build from http://quazaa.sf.net/.\n\nThe program will now terminate."));
		return 0;
	}

	if( !args.contains("--no-alpha-warning") )
	{
		int ret = QMessageBox::warning(NULL,
									   QObject::tr("Snapshot/Debug Build Warning"),
									   QObject::tr("WARNING: This is a SNAPSHOT BUILD of Quazaa. \n"
												   "It is NOT meant for GENERAL USE, and is only for testing specific features in a controlled environment.\n"
												   "It will frequently stop running, or will display debug information to assist testing.\n"
												   "This build will expire on %1.\n\n"
												   "Do you wish to continue?"
												   ).arg(oExpire.toString(Qt::SystemLocaleLongDate)),
									   QMessageBox::Yes | QMessageBox::No);
		if( ret == QMessageBox::No )
			return 0;
	}
#endif

	qsrand( time( 0 ) );

#ifdef Q_OS_LINUX

	rlimit sLimit;
	memset( &sLimit, 0, sizeof( rlimit ) );
	getrlimit( RLIMIT_NOFILE, &sLimit );

	sLimit.rlim_cur = sLimit.rlim_max;

	if( setrlimit( RLIMIT_NOFILE, &sLimit ) == 0 )
	{
		qDebug() << "Successfully raised resource limits";
	}
	else
	{
		qDebug() << "Cannot set resource limits";
	}

#endif // Q_OS_LINUX

	theApp.setApplicationName(    QuazaaGlobals::APPLICATION_NAME() );
	theApp.setApplicationVersion( QuazaaGlobals::APPLICATION_VERSION_STRING() );
	theApp.setOrganizationDomain( QuazaaGlobals::APPLICATION_ORGANIZATION_DOMAIN() );
	theApp.setOrganizationName(   QuazaaGlobals::APPLICATION_ORGANIZATION_NAME() );

	// Setup Qt elements of signal queue necessary for operation
	signalQueue.setup();

	//Initialize multilanguage support
	quazaaSettings.loadLanguageSettings();
	quazaaSettings.translator.load( quazaaSettings.Language.File );
	qApp->installTranslator( &quazaaSettings.translator );

	//Create splash window
	DialogSplash* dlgSplash = new DialogSplash();
	dlgSplash->show();
	qApp->processEvents();

	dlgSplash->updateProgress( 1, QObject::tr( "Loading settings..." ) );
	qApp->processEvents();

	//Initialize Settings
	quazaaSettings.loadSettings();

	//Check if this is Quazaa's first run
	dlgSplash->updateProgress( 5, QObject::tr( "Checking for first run..." ) );
	qApp->processEvents();
	if ( quazaaSettings.isFirstRun() )
	{
		DialogLanguage* dlgLanguage = new DialogLanguage();
		dlgLanguage->exec();

		dlgSplash->updateProgress( 10, QObject::tr( "Running first run wizard..." ) );
		quazaaSettings.saveFirstRun( false );
		quazaaSettings.saveSettings();
		quazaaSettings.saveProfile();

		WizardQuickStart* wzrdQuickStart = new WizardQuickStart();
		wzrdQuickStart->exec();
	}

	// Load Security Manager
	dlgSplash->updateProgress( 15, QObject::tr( "Loading Security Manager..." ) );
	qApp->processEvents();
	if ( !securityManager.start() )
		systemLog.postLog( LogSeverity::Information,
						   QObject::tr( "Security data file was not available." ) );

	//Load profile
	dlgSplash->updateProgress( 20, QObject::tr( "Loading Profile..." ) );
	qApp->processEvents();
	quazaaSettings.loadProfile();

	//Load Host Cache
	dlgSplash->updateProgress( 30, QObject::tr( "Loading Host Cache..." ) );
	qApp->processEvents();
	HostCache.m_pSection.lock();
	HostCache.Load();
	HostCache.m_pSection.unlock();

	//initialize geoip list
	GeoIP.loadGeoIP();

	//Load the library
	dlgSplash->updateProgress( 50, QObject::tr( "Loading Library..." ) );
	qApp->processEvents();
	QueryHashMaster.Create();
	ShareManager.Start();

	// Load Download Manager
	dlgSplash->updateProgress(60, QObject::tr("Loading Transfer Manager..."));
	qApp->processEvents();
	Transfers.start();

	dlgSplash->updateProgress( 80, QObject::tr( "Loading User Interface..." ) );
	qApp->processEvents();

	MainWindow = new CWinMain();
	if ( quazaaSettings.WinMain.Visible )
	{
		if(quazaaSettings.isFirstRun())
			MainWindow->showMaximized();
		else
			MainWindow->show();
	}

	dlgSplash->updateProgress( 90, QObject::tr( "Loading Tray Icon..." ) );
	qApp->processEvents();
	MainWindow->loadTrayIcon();

	// Make the main window show if the user tried to open another instance
	QObject::connect( &theApp, SIGNAL( messageReceived( const QString& ) ),
					  MainWindow, SLOT( showOnTop() ));
	theApp.setActivationWindow( MainWindow );

	dlgSplash->updateProgress( 100, QObject::tr( "Welcome to Quazaa!" ) );
	qApp->processEvents();

	dlgSplash->deleteLater();
	dlgSplash = 0;

	// Start networks if needed
	if(quazaaSettings.System.ConnectOnStartup)
	{
		if( quazaaSettings.Gnutella2.Enable )
		{
			Network.Connect();
		}
	}

	return theApp.exec();
}

