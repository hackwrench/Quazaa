/*
* Copyright (C) 2008-2013 Communi authors
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

#include "widgetircmessageview.h"
#include "commandparser.h"
#include "menufactory.h"
#include "completer.h"
#include "ircuserlistmodel.h"
#include "session.h"
#include "chatconverter.h"
#include "quazaaglobals.h"
#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QStringListModel>
#include <QTextBlock>
#include <QShortcut>
#include <QKeyEvent>
#include <QDateTime>
#include <QDebug>
#include <QUrl>
#include <QIcon>
#include <ircmessage.h>
#include <irccommand.h>
#include <irc.h>
#include "quazaasysinfo.h"

static QStringListModel* command_model = 0;
static const int VERTICAL_MARGIN = 1; // matches qlineedit_p.cpp

CWidgetIrcMessageView::CWidgetIrcMessageView(CWidgetIrcMessageView::ViewType type, Session* session, QWidget* parent) :
	QWidget(parent)
{
	d.setupUi(this);
	d.viewType = type;
	d.joined = false;

	closeButton = new QPushButton(this);
	closeButton->setIcon(QIcon(":/Resource/Generic/Exit.png"));
	closeButton->setFixedSize(20,20);
	closeButton->setStyleSheet("QPushButton { border: 0px solid transparent; }"
							   "QPushButton:hover { border: 3px ridge cornflowerblue; border-radius: 6px; }"
							   "QPushButton:pressed { border: 3px groove cornflowerblue; border-radius: 6px; }");
	connect(closeButton, SIGNAL(clicked()), this, SLOT(closePressed()));

	d.chatInput = new CWidgetChatInput(this, true);
	d.horizontalLayoutChatInput->addWidget(d.chatInput);
	d.chatInput->setFixedHeight(80);

	connect(d.splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(onSplitterMoved()));

	setFocusProxy(d.chatInput->textEdit());
	d.textBrowser->setBuddy(d.chatInput->textEdit());
	d.textBrowser->viewport()->installEventFilter(this);
	connect(d.textBrowser, SIGNAL(anchorClicked(QUrl)), SLOT(onAnchorClicked(QUrl)));

	d.formatter = new MessageFormatter(this);

	d.session = session;
	connect(d.session, SIGNAL(activeChanged(bool)), this, SIGNAL(activeChanged()));

	d.topicLabel->setVisible(type == ChannelView);
	d.listView->setVisible(type == ChannelView);
	if (type == ChannelView) {
		d.listView->setSession(session);
		connect(d.listView, SIGNAL(queried(QString)), this, SIGNAL(queried(QString)));
		connect(d.listView, SIGNAL(doubleClicked(QString)), this, SIGNAL(queried(QString)));
		connect(d.listView, SIGNAL(commandRequested(IrcCommand*)), d.session, SLOT(sendCommand(IrcCommand*)));
	}

	if (!command_model) {
		command_model = new QStringListModel(qApp);

		CommandParser::addCustomCommand("QUERY", "<user>");
		CommandParser::addCustomCommand("MSG", "<user/channel> (<message...>)");
		CommandParser::addCustomCommand("SYSINFO", "");
		CommandParser::addCustomCommand("CLEAR", "");
		CommandParser::addCustomCommand("CTCP", "<target> <command> (<params...>)");
	}

	d.chatInput->textEdit()->completer()->setUserModel(d.listView->userModel());
	d.chatInput->textEdit()->completer()->setCommandModel(command_model);
	connect(d.chatInput->textEdit()->completer(), SIGNAL(commandCompletion(QString)), this, SLOT(completeCommand(QString)));

	connect(d.chatInput, SIGNAL(messageSent(QTextDocument*)), this, SLOT(sendMessage(QTextDocument*)));
	connect(d.chatInput, SIGNAL(messageSent(QString)), this, SLOT(sendMessage(QString)));
	connect(d.chatInput->textEdit(), SIGNAL(textChanged(QString)), this, SLOT(showHelp(QString)));

	connect(d.chatInput->textEdit(), SIGNAL(scrollToTop()), d.textBrowser, SLOT(scrollToTop()));
	connect(d.chatInput->textEdit(), SIGNAL(scrollToBottom()), d.textBrowser, SLOT(scrollToBottom()));
	connect(d.chatInput->textEdit(), SIGNAL(scrollToNextPage()), d.textBrowser, SLOT(scrollToNextPage()));
	connect(d.chatInput->textEdit(), SIGNAL(scrollToPreviousPage()), d.textBrowser, SLOT(scrollToPreviousPage()));

	d.chatInput->helpLabel()->hide();
	d.searchEditor->setTextEdit(d.textBrowser);

	QShortcut* shortcut = new QShortcut(Qt::Key_Escape, this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(onEscPressed()));
}

CWidgetIrcMessageView::~CWidgetIrcMessageView()
{
}

bool CWidgetIrcMessageView::isActive() const
{
	if (!d.session->isActive())
		return false;
	if (d.viewType == ChannelView)
		return d.joined;
	return true;
}

CWidgetIrcMessageView::ViewType CWidgetIrcMessageView::viewType() const
{
	return d.viewType;
}

Session* CWidgetIrcMessageView::session() const
{
	return d.session;
}

IrcUserListModel* CWidgetIrcMessageView::userModel() const
{
	return d.listView->userModel();
}

QTextBrowser* CWidgetIrcMessageView::textBrowser() const
{
	return d.textBrowser;
}

MessageFormatter* CWidgetIrcMessageView::messageFormatter() const
{
	return d.formatter;
}

QString CWidgetIrcMessageView::receiver() const
{
	return d.receiver;
}

void CWidgetIrcMessageView::setReceiver(const QString& receiver)
{
	if (d.receiver != receiver) {
		d.receiver = receiver;
		if (d.viewType == ChannelView)
			d.listView->setChannel(receiver);
		emit receiverChanged(receiver);
	}
}

MenuFactory* CWidgetIrcMessageView::menuFactory() const
{
	return d.listView->menuFactory();
}

void CWidgetIrcMessageView::setMenuFactory(MenuFactory* factory)
{
	d.listView->setMenuFactory(factory);
}

QByteArray CWidgetIrcMessageView::saveSplitter() const
{
	if (d.viewType != ServerView)
		return d.splitter->saveState();
	return QByteArray();
}

void CWidgetIrcMessageView::restoreSplitter(const QByteArray& state)
{
	d.splitter->restoreState(state);
}

void CWidgetIrcMessageView::showHelp(const QString& text, bool error)
{
	QString syntax;
	if (text == "/") {
		QStringList commands = CommandParser::availableCommands();
		syntax = commands.join(" ");
	} else if (text.startsWith('/')) {
		QStringList words = text.mid(1).split(' ');
		QString command = words.value(0);
		QStringList suggestions = CommandParser::suggestedCommands(command, words.mid(1));
		if (suggestions.count() == 1)
			syntax = CommandParser::syntax(suggestions.first());
		else
			syntax = suggestions.join(" ");

		if (syntax.isEmpty() && error)
			syntax = tr("Unknown command '%1'").arg(command.toUpper());
	}

	d.chatInput->helpLabel()->setVisible(!syntax.isEmpty());
	QPalette pal;
	if (error)
		pal.setColor(QPalette::WindowText, quazaaSettings.Chat.Colors[IrcColorType::Highlight]);
	d.chatInput->helpLabel()->setPalette(pal);
	d.chatInput->helpLabel()->setText(syntax);
}

void CWidgetIrcMessageView::sendMessage(const QString& text)
{
	QStringList lines = text.split(QRegExp("[\\r\\n]"), QString::SkipEmptyParts);
	foreach (const QString& line, lines) {
		if ((viewType() == ServerView) && !line.startsWith("/"))
		{
			QString modify = line;
			modify.prepend(tr("/quote "));
			sendMessage(modify);
		} else {
			IrcCommand* cmd = CommandParser::parseCommand(d.receiver, line);
			if (cmd) {
				if (cmd->type() == IrcCommand::Quote)
				{
					if(viewType() == ServerView)
						d.textBrowser->append( d.formatter->formatMessage( common::getDateTimeUTC(), tr("[RAW] %1").arg(cmd->parameters().join(" ")) ) );
					else
						emit appendRawMessage(d.formatter->formatMessage( common::getDateTimeUTC(), tr("[RAW] %1").arg(cmd->parameters().join(" ")) ) );
				}
				if (cmd->type() == IrcCommand::Custom) {
					QString command = cmd->parameters().value(0);
					QStringList params = cmd->parameters().mid(1);
					if (command == "CLEAR") {
						d.textBrowser->clear();
					} else if (command == "MSG") {
						// support "/msg foo /cmd" without executing /cmd
						QString msg = QStringList(params.mid(1)).join(" ");
						if (msg.startsWith('/') && !msg.startsWith("//") && !msg.startsWith("/ "))
							msg.prepend('/');
						emit messaged(params.value(0), msg);
					} else if (command == "QUERY") {
						emit queried(params.value(0));
					}
					else if (command == "JOIN")
					{
						if (params.count() == 1 || params.count() == 2)
						{
							QString channel = params.value(0);
							QString password = params.value(1);
							if (!d.session->isChannel(channel))
								channel = channel.prepend("#");
							if (!d.session->isChannel(channel))
							{
								channel = channel.remove(0,1);
								channel = channel.prepend(d.session->info().channelTypes().at(0));
							}
							if (d.session->isChannel(channel))
								d.session->sendCommand(IrcCommand::createJoin(QStringList() << channel, QStringList() << password));
							emit openView(channel);
						}
					}
					else if (command == "SYSINFO")
					{
						CQuazaaSysInfo *sysInfo = new CQuazaaSysInfo();
						sendMessage(tr("Application:%1 %2 OS:%3 Qt Version:%4").arg(QApplication::applicationName(), CQuazaaGlobals::APPLICATION_VERSION_STRING(), sysInfo->osVersionToString(), qVersion()));
						//onSend(tr("CPU:%1 Cores:%2 Memory:%3").arg(QApplication::applicationName(), QuazaaGlobals::APPLICATION_VERSION_STRING()));
					}
					else if (command == "CTCP")
					{
						QString args = params.at(1).toUpper().append(" ");
						QStringList command = params.mid(2);
						args.append(command.join(" "));
						d.session->sendCommand(IrcCommand::createCtcpRequest(params.at(0), args));
					}
					delete cmd;
				} else {
					d.session->sendCommand(cmd);

					if (cmd->type() == IrcCommand::Message || cmd->type() == IrcCommand::CtcpAction || cmd->type() == IrcCommand::Notice) {
						IrcMessage* msg = IrcMessage::fromData(":" + d.session->nickName().toUtf8() + " " + cmd->toString().toUtf8(), d.session);
						receiveMessage(msg);
						delete msg;
					}
				}
			} else {
				showHelp(line, true);
			}
		}
	}
}

void CWidgetIrcMessageView::sendMessage(QTextDocument *message)
{
	CChatConverter *converter = new CChatConverter(message);
	sendMessage(converter->toIrc());
}

void CWidgetIrcMessageView::hideEvent(QHideEvent* event)
{
	QWidget::hideEvent(event);
	d.textBrowser->setUnseenBlock(-1);
}

bool CWidgetIrcMessageView::eventFilter(QObject* object, QEvent* event)
{
	if (object == d.textBrowser->viewport() && event->type() == QEvent::ContextMenu) {
		QContextMenuEvent* menuEvent = static_cast<QContextMenuEvent*>(event);
		QAbstractTextDocumentLayout* layout = d.textBrowser->document()->documentLayout();
		QUrl link(layout->anchorAt(menuEvent->pos()));
		if (link.scheme() == "nick") {
			QMenu* standardMenu = d.textBrowser->createStandardContextMenu(menuEvent->pos());
			QMenu* customMenu = d.listView->menuFactory()->createUserViewMenu(link.toString(QUrl::RemoveScheme), this);
			customMenu->addSeparator();
			customMenu->insertActions(0, standardMenu->actions());
			customMenu->exec(menuEvent->globalPos());
			customMenu->deleteLater();
			delete standardMenu;
			return true;
		}
	}
	return QWidget::eventFilter(object, event);
}

void CWidgetIrcMessageView::onEscPressed()
{
	d.chatInput->helpLabel()->hide();
	d.searchEditor->hide();
	setFocus(Qt::OtherFocusReason);
}

void CWidgetIrcMessageView::onSplitterMoved()
{
	emit splitterChanged(d.splitter->saveState());
}

void CWidgetIrcMessageView::onAnchorClicked(const QUrl& link)
{
	if (link.scheme() == "nick")
		emit queried(link.toString(QUrl::RemoveScheme));
	else
		QDesktopServices::openUrl(link);
}

void CWidgetIrcMessageView::completeCommand(const QString &command)
{
	if (command == "TOPIC")
		d.chatInput->textEdit()->insertPlainText(d.topic);
}

void CWidgetIrcMessageView::closePressed()
{
	switch (viewType())
	{
	case ServerView:
		d.session->quit();
	case ChannelView:
		emit closeView(receiver());
	case QueryView:
		emit closeView(receiver());
	default:
		break;
	}
}

void CWidgetIrcMessageView::applySettings()
{
	CommandParser::setAliases(quazaaSettings.Chat.Aliases);

	QStringList commands;
	foreach (const QString& command, CommandParser::availableCommands())
		commands += "/" + command;
	command_model->setStringList(commands);

	d.formatter->setTimeStamp(quazaaSettings.Chat.ShowTimestamp);
	d.formatter->setTimeStampFormat(quazaaSettings.Chat.TimestampFormat);
	d.formatter->setStripNicks(quazaaSettings.Chat.StripNicks);

	d.textBrowser->document()->setMaximumBlockCount(quazaaSettings.Chat.MaxBlockCount);

	QTextDocument* doc = d.topicLabel->findChild<QTextDocument*>();
	if (doc)
		doc->setDefaultStyleSheet(QString("a { color: %1 }").arg(quazaaSettings.Chat.Colors.value(IrcColorType::Link)));

	QString foregroundColor = quazaaSettings.Chat.Colors.value(IrcColorType::Default);
	QString backgroundColor = quazaaSettings.Chat.Colors.value(IrcColorType::Background);
	d.topicLabel->setStyleSheet(QString("QLabel { color: %1; background-color: %2; }").arg(foregroundColor).arg(backgroundColor));
	d.textBrowser->setStyleSheet(QString("QTextBrowser { color: %1; background-color: %2; }").arg(foregroundColor).arg(backgroundColor));
	d.textBrowser->setUnreadLineColor(QColor(quazaaSettings.Chat.Colors.value(IrcColorType::Alert)));
	d.listView->setStyleSheet(QString("CListViewIrcUsers { color: %1; background-color: %2; }").arg(foregroundColor).arg(backgroundColor));

	d.textBrowser->document()->setDefaultStyleSheet(
		QString(
			".highlight { color: %1; }"
			".message   { color: %2; }"
			".notice    { color: %3; }"
			".action    { color: %4; }"
			".event     { color: %5; }"
			".timestamp { color: %6; font-size: small }"
			"a { color: %7 }"
		).arg(quazaaSettings.Chat.Colors.value(IrcColorType::Highlight))
		.arg(quazaaSettings.Chat.Colors.value(IrcColorType::Default))
		.arg(quazaaSettings.Chat.Colors.value(IrcColorType::Notice))
		.arg(quazaaSettings.Chat.Colors.value(IrcColorType::Action))
		.arg(quazaaSettings.Chat.Colors.value(IrcColorType::Event))
		.arg(quazaaSettings.Chat.Colors.value(IrcColorType::TimeStamp))
		.arg(quazaaSettings.Chat.Colors.value(IrcColorType::Link)));
}

void CWidgetIrcMessageView::receiveMessage(IrcMessage* message)
{
	if (d.viewType == ChannelView)
		d.listView->processMessage(message);

	bool ignore = false;
	switch (message->type()) {
		case IrcMessage::Private: {
			IrcSender sender = message->sender();
			if (sender.name() == QLatin1String("***") && sender.user() == QLatin1String("znc")) {
				QString content = static_cast<IrcPrivateMessage*>(message)->message();
				if (content == QLatin1String("Buffer Playback...")) {
					d.formatter->setZncPlaybackMode(true);
					ignore = true;
				} else if (content == QLatin1String("Playback Complete.")) {
					d.formatter->setZncPlaybackMode(false);
					ignore = true;
				}
			}
			break;
		}
		case IrcMessage::Topic:
			d.topic = static_cast<IrcTopicMessage*>(message)->topic();
			d.topicLabel->setText(d.formatter->formatHtml(d.topic));
			if (d.topicLabel->text().isEmpty())
				d.topicLabel->setText(tr("-"));
			break;
		case IrcMessage::Unknown:
			qWarning() << "unknown:" << message;
			break;
		case IrcMessage::Join:
			if (message->flags() & IrcMessage::Own) {
				d.joined = true;
				emit activeChanged();
			}
			break;
		case IrcMessage::Part:
			if (message->flags() & IrcMessage::Own) {
				d.joined = false;
				emit activeChanged();
			}
			break;
		case IrcMessage::Numeric:
			switch (static_cast<IrcNumericMessage*>(message)->code()) {
				case Irc::RPL_NOTOPIC:
					d.topic.clear();
					d.topicLabel->setText(tr("-"));
					break;
				case Irc::RPL_TOPIC:
					d.topic = message->parameters().value(2);
					d.topicLabel->setText(d.formatter->formatHtml(d.topic));
					break;
				case Irc::RPL_TOPICWHOTIME: {
					QDateTime dateTime = QDateTime::fromTime_t(message->parameters().value(3).toInt());
					d.topicLabel->setToolTip(tr("Set %1 by %2").arg(dateTime.toString(), message->parameters().value(2)));
					break;
				}
				default:
					break;
			}
			break;
		default:
			break;
	}

	d.formatter->setUsers(d.listView->userModel()->users());
	d.formatter->setHighlights(QStringList() << d.session->nickName());
	QString formatted = d.formatter->formatMessage(message);
	if (!ignore && (!isVisible() || !isActiveWindow())) {
		IrcMessage::Type type = d.formatter->effectiveMessageType();
		if (d.formatter->hasHighlight() || (type == IrcMessage::Private && d.viewType != ChannelView))
			emit highlighted(message);
		else if (type == IrcMessage::Notice || type == IrcMessage::Private) // TODO: || (!d.receivedCodes.contains(Irc::RPL_ENDOFMOTD) && d.viewType == ServerView))
			emit missed(message);
	}

	d.textBrowser->append(formatted);
}

bool CWidgetIrcMessageView::hasUser(const QString& user) const
{
	return (!d.session->nickName().compare(user, Qt::CaseInsensitive)) ||
		(d.viewType == QueryView && !d.receiver.compare(user, Qt::CaseInsensitive)) ||
		(d.viewType == ChannelView && d.listView->hasUser(user));
}
