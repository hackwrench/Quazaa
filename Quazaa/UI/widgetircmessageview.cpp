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

#include "widgetircmessageview.h"
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

WidgetIrcMessageView::WidgetIrcMessageView(WidgetIrcMessageView::ViewType type, Session* session, QWidget* parent) :
    QWidget(parent)
{
    d.setupUi(this);
    d.viewType = type;

    closeButton = new QPushButton(this);
    closeButton->setIcon(QIcon(":/Resource/Generic/Exit.png"));
    closeButton->setFixedSize(20,20);
    closeButton->setStyleSheet("QPushButton { border: 0px solid transparent; }"
                               "QPushButton:hover { border: 3px ridge cornflowerblue; border-radius: 6px; }"
                               "QPushButton:pressed { border: 3px groove cornflowerblue; border-radius: 6px; }");
    connect(closeButton, SIGNAL(clicked()), this, SLOT(closePressed()));

    d.chatInput = new WidgetChatInput(0, true);
    d.horizontalLayoutChatInput->addWidget(d.chatInput);
    d.chatInput->setFixedHeight(80);

	connect(d.splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(onSplitterMoved()));

    setFocusProxy(d.chatInput->textEdit());
    d.textBrowser->setBuddy(d.chatInput->textEdit());
	d.textBrowser->viewport()->installEventFilter(this);
	connect(d.textBrowser, SIGNAL(anchorClicked(QUrl)), SLOT(onAnchorClicked(QUrl)));

	d.formatter = new MessageFormatter(this);
	d.formatter->setHighlights(QStringList(session->nickName()));
	d.formatter->setMessageFormat("class='message'");
	d.formatter->setEventFormat("class='event'");
	d.formatter->setNoticeFormat("class='notice'");
	d.formatter->setActionFormat("class='action'");
	d.formatter->setUnknownFormat("class='unknown'");
	d.formatter->setHighlightFormat("class='highlight'");
	d.formatter->setTimeStampFormat("class='timestamp'");

	d.session = session;
	connect(&d.parser, SIGNAL(customCommand(QString, QStringList)), this, SLOT(onCustomCommand(QString, QStringList)));

	d.topicLabel->setVisible(type == ChannelView);
	d.listView->setVisible(type == ChannelView);
	if (type == ChannelView) {
		d.listView->setSession(session);
		connect(d.listView, SIGNAL(queried(QString)), this, SIGNAL(queried(QString)));
		connect(d.listView, SIGNAL(doubleClicked(QString)), this, SIGNAL(queried(QString)));
		connect(d.listView, SIGNAL(commandRequested(IrcCommand*)), d.session, SLOT(sendCommand(IrcCommand*)));
	}

	if (!command_model) {
		CommandParser::addCustomCommand("QUERY", "<user> (<message...>)");
        CommandParser::addCustomCommand("M", "<user> (<message...>)");
		CommandParser::addCustomCommand("MSG", "<user> (<message...>)");
		CommandParser::addCustomCommand("TELL", "<user> (<message...>)");
		CommandParser::addCustomCommand("J", "<channel> (<key>)");
        CommandParser::addCustomCommand("P", "(<reason...>)");
        CommandParser::addCustomCommand("LEAVE", "(<reason...>)");
		CommandParser::addCustomCommand("NS", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("NICKSERV", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("CS", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("CHANSERV", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("HS", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("HOSTSERV", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("MS", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("MEMOSERV", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("BS", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("BOTSERV", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("OS", "(<command>) (<params...>)");
		CommandParser::addCustomCommand("OPERSERV", "(<command>) (<params...>)");
        CommandParser::addCustomCommand("SYSINFO", "");
        CommandParser::addCustomCommand("CLEAR", "");
        CommandParser::addCustomCommand("CTCP", "<target> <command> (<params...>)");

		QStringList prefixedCommands;
		foreach(const QString & command, CommandParser::availableCommands())
		prefixedCommands += "/" + command;

		command_model = new QStringListModel(qApp);
		command_model->setStringList(prefixedCommands);
	}

    d.chatInput->textEdit()->completer()->setDefaultModel(d.listView->userModel());
    d.chatInput->textEdit()->completer()->setSlashModel(command_model);

    connect(d.chatInput, SIGNAL(messageSent(QTextDocument*)), this, SLOT(onSend(QTextDocument*)));
    connect(d.chatInput, SIGNAL(messageSent(QString)), this, SLOT(onSend(QString)));
    connect(d.chatInput->textEdit(), SIGNAL(textChanged(QString)), this, SLOT(showHelp(QString)));

    d.chatInput->helpLabel()->hide();
	d.searchEditor->setTextEdit(d.textBrowser);

	QShortcut* shortcut = new QShortcut(Qt::Key_Escape, this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(onEscPressed()));

	applySettings();
}

WidgetIrcMessageView::~WidgetIrcMessageView()
{
}

bool WidgetIrcMessageView::isActive() const
{
    if (!d.session->isActive())
        return false;
    if (d.viewType == ChannelView)
        return d.joined;
    return true;
}

WidgetIrcMessageView::ViewType WidgetIrcMessageView::viewType() const
{
    return d.viewType;
}

Session* WidgetIrcMessageView::session() const
{
    return d.session;
}

IrcUserListModel* WidgetIrcMessageView::userModel() const
{
    return d.listView->userModel();
}

QTextBrowser* WidgetIrcMessageView::textBrowser() const
{
    return d.textBrowser;
}

MessageFormatter* WidgetIrcMessageView::messageFormatter() const
{
    return d.formatter;
}

QString WidgetIrcMessageView::receiver() const
{
    return d.receiver;
}

void WidgetIrcMessageView::setReceiver(const QString& receiver)
{
    if (d.receiver != receiver) {
        d.receiver = receiver;
        if (d.viewType == ChannelView)
            d.listView->setChannel(receiver);
        emit receiverChanged(receiver);
    }
}

MenuFactory* WidgetIrcMessageView::menuFactory() const
{
    return d.listView->menuFactory();
}

void WidgetIrcMessageView::setMenuFactory(MenuFactory* factory)
{
    d.listView->setMenuFactory(factory);
}

QByteArray WidgetIrcMessageView::saveSplitter() const
{
    if (d.viewType != ServerView)
        return d.splitter->saveState();
    return QByteArray();
}

void WidgetIrcMessageView::restoreSplitter(const QByteArray& state)
{
    d.splitter->restoreState(state);
}

void WidgetIrcMessageView::showHelp(const QString& text, bool error)
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

void WidgetIrcMessageView::appendMessage(const QString& message)
{
    if (!message.isEmpty()) {
        // workaround the link activation merge char format bug
        QString copy = message;
        if (copy.endsWith("</a>"))
            copy += " ";

        d.textBrowser->append(copy);
        if (!isVisible() && d.textBrowser->unseenBlock() == -1)
            d.textBrowser->setUnseenBlock(d.textBrowser->document()->blockCount() - 1);

#if QT_VERSION >= 0x040800
        QTextBlock block = d.textBrowser->document()->lastBlock();
        QTextBlockFormat format = block.blockFormat();
        format.setLineHeight(120, QTextBlockFormat::ProportionalHeight);
        QTextCursor cursor(block);
        cursor.setBlockFormat(format);
#endif // QT_VERSION
    }
}

void WidgetIrcMessageView::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    d.textBrowser->setUnseenBlock(-1);
}

bool WidgetIrcMessageView::eventFilter(QObject* object, QEvent* event)
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

void WidgetIrcMessageView::onEscPressed()
{
    d.chatInput->helpLabel()->hide();
    d.searchEditor->hide();
    setFocus(Qt::OtherFocusReason);
}

void WidgetIrcMessageView::onSplitterMoved()
{
    emit splitterChanged(d.splitter->saveState());
}

void WidgetIrcMessageView::onSend(const QString& text)
{
	QStringList lines = text.split(QRegExp("[\\r\\n]"), QString::SkipEmptyParts);
	foreach (const QString& line, lines) {
        if ((viewType() == ServerView) && !line.startsWith("/"))
        {
            QString modify = line;
            modify.prepend(tr("/quote "));
            onSend(modify);
        } else {
            IrcCommand* cmd = d.parser.parseCommand(receiver(), line);
            if (cmd) {
                if (cmd->type() == IrcCommand::Quote)
                {
                    QString rawMessage = cmd->toString();
                    if(viewType() == ServerView)
                        appendMessage(d.formatter->formatRaw(rawMessage));
                    else
                        emit appendRawMessage(d.formatter->formatRaw(rawMessage));
                }
                d.session->sendCommand(cmd);

                if (cmd->type() == IrcCommand::Message || cmd->type() == IrcCommand::CtcpAction) {
                    IrcMessage* msg = IrcMessage::fromData(":" + d.session->nickName().toUtf8() + " " + cmd->toString().toUtf8(), d.session);
                    receiveMessage(msg);
                    delete msg;
                }
            } else if (d.parser.hasError()) {
                showHelp(line, true);
            }
        }
    }
}

void WidgetIrcMessageView::onSend(QTextDocument *message)
{
    CChatConverter *converter = new CChatConverter(message);
    onSend(converter->toIrc());
}

void WidgetIrcMessageView::onAnchorClicked(const QUrl& link)
{
    if (link.scheme() == "nick")
        emit queried(link.toString(QUrl::RemoveScheme));
    else
        QDesktopServices::openUrl(link);
}

void WidgetIrcMessageView::closePressed()
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

void WidgetIrcMessageView::applySettings()
{
	d.formatter->setTimeStamp(quazaaSettings.Chat.ShowTimestamp);
	d.formatter->setStripNicks(quazaaSettings.Chat.StripNicks);

	d.textBrowser->document()->setMaximumBlockCount(quazaaSettings.Chat.MaxBlockCount);
	d.topicLabel->setProperty("gradient", quazaaSettings.Chat.Layout == "tree");

	QTextDocument* doc = d.topicLabel->findChild<QTextDocument*>();
	if (doc)
		doc->setDefaultStyleSheet(QString("a { color: %1 }").arg(quazaaSettings.Chat.Colors.value(IrcColorType::Link)));

    QString foregroundColor = quazaaSettings.Chat.Colors.value(IrcColorType::Default);
	QString backgroundColor = quazaaSettings.Chat.Colors.value(IrcColorType::Background);
    d.topicLabel->setStyleSheet(QString("QLabel { color: %1; background-color: %2; }").arg(foregroundColor).arg(backgroundColor));
	d.textBrowser->setStyleSheet(QString("QTextBrowser { color: %1; background-color: %2; }").arg(foregroundColor).arg(backgroundColor));
    d.listView->setStyleSheet(QString("ListViewIrcUsers { color: %1; background-color: %2; }").arg(foregroundColor).arg(backgroundColor));

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

void WidgetIrcMessageView::receiveMessage(IrcMessage* message)
{
    if (d.viewType == ChannelView)
        d.listView->processMessage(message);

    bool hilite = false;
    bool matches = false;

    switch (message->type()) {
        case IrcMessage::Notice:
            matches = static_cast<IrcNoticeMessage*>(message)->message().contains(d.session->nickName());
            hilite = true;
            break;
        case IrcMessage::Private:
            matches = d.viewType != ChannelView || static_cast<IrcPrivateMessage*>(message)->message().contains(d.session->nickName());
            hilite = true;
            break;
        case IrcMessage::Topic:
            d.topicLabel->setText(d.formatter->formatHtml(static_cast<IrcTopicMessage*>(message)->topic()));
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
        case IrcMessage::Invite:
        case IrcMessage::Ping:
        case IrcMessage::Pong:
        case IrcMessage::Error:
            break;
        case IrcMessage::Numeric:
            switch (static_cast<IrcNumericMessage*>(message)->code()) {
                case Irc::RPL_NOTOPIC:
                    d.topicLabel->setText(tr("-"));
                    break;
                case Irc::RPL_TOPIC:
                    d.topicLabel->setText(d.formatter->formatHtml(message->parameters().value(2)));
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

    QString formatted = d.formatter->formatMessage(message, d.listView->userModel());
    if (formatted.length()) {
        if (matches)
            emit alerted(message);
        else if (hilite) // TODO: || (!d.receivedCodes.contains(Irc::RPL_ENDOFMOTD) && d.viewType == ServerView))
            emit highlighted(message);

        appendMessage(formatted);
    }
}

bool WidgetIrcMessageView::hasUser(const QString& user) const
{
    return (!d.session->nickName().compare(user, Qt::CaseInsensitive)) ||
           (d.viewType == QueryView && !d.receiver.compare(user, Qt::CaseInsensitive)) ||
           (d.viewType == ChannelView && d.listView->hasUser(user));
}

void WidgetIrcMessageView::onCustomCommand(const QString& command, const QStringList& params)
{
    if (command == "QUERY" || command == "M" || command == "MSG" || command == "TELL")
    {
        if (!params.value(1).isEmpty())
		{
            QStringList message = params.mid(1);
            emit appendQueryMessage(params.value(0), message.join(" "));
		}
		else
        {
            emit queried(params.value(0));
        }
    }
    else if (command == "J" || command == "JOIN")
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
    else if (command == "P" || command == "LEAVE")
    {
        QStringList reason = params.mid(1);
        IrcCommand* command = IrcCommand::createPart(receiver(), reason.join(" "));
        session()->sendCommand(command);
    }
	else if (command == "NS" || command == "NICKSERV")
	{
		emit appendQueryMessage("NickServ", params.join(" "));
	}
	else if (command == "CS" || command == "CHANSERV")
	{
		emit appendQueryMessage("ChanServ", params.join(" "));
	}
	else if (command == "HS" || command == "HOSTSERV")
	{
		emit appendQueryMessage("HostServ", params.join(" "));
	}
	else if (command == "MS" || command == "MEMOSERV")
	{
		emit appendQueryMessage("MemoServ", params.join(" "));
	}
	else if (command == "BS" || command == "BOTSERV")
	{
		emit appendQueryMessage("BotServ", params.join(" "));
	}
	else if (command == "OS" || command == "OPERSERV")
	{
		emit appendQueryMessage("OperServ", params.join(" "));
	}
	else if (command == "SYSINFO")
	{
		QuazaaSysInfo *sysInfo = new QuazaaSysInfo();
		onSend(tr("Application:%1 %2 OS:%3 Qt Version:%4").arg(QApplication::applicationName(), QuazaaGlobals::APPLICATION_VERSION_STRING(), sysInfo->osVersionToString(), qVersion()));
		//onSend(tr("CPU:%1 Cores:%2 Memory:%3").arg(QApplication::applicationName(), QuazaaGlobals::APPLICATION_VERSION_STRING()));
    }
    else if (command == "CLEAR")
    {
        d.textBrowser->clear();
    }
    else if (command == "CTCP")
    {
        QString args = params.at(1).toUpper().append(" ");
        QStringList command = params.mid(2);
        args.append(command.join(" "));
        d.session->sendCommand(IrcCommand::createCtcpRequest(params.at(0), args));
    }
	else if (command == "WHOIS")
	{
        d.session->sendCommand(IrcCommand::createWhois(params.value(0)));
    }
}
