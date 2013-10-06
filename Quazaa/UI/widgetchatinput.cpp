/*
** $Id$
**
** Copyright © Quazaa Development Team, 2009-2013.
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

#include "widgetchatinput.h"
#include "ui_widgetchatinput.h"
#include "dialogconnectto.h"
#include "dialogirccolordialog.h"
#include "skinsettings.h"

#include "chatsessiong2.h"

#include <QColorDialog>
#include <QPalette>

#include "debug_new.h"

CWidgetChatInput::CWidgetChatInput(QWidget *parent, bool isIrc) :
    QMainWindow(parent),
	ui(new Ui::CWidgetChatInput)
{
    ui->setupUi(this);
    bIsIrc = isIrc;

    defaultColor = ui->textEditInput->textColor();

	QTextCharFormat format;
    format.setFontStyleHint(QFont::TypeWriter);
    ui->textEditInput->setCurrentCharFormat(format);

    connect(ui->textEditInput, SIGNAL(cursorPositionChanged()), this, SLOT(updateToolbar()));

    connect(ui->textEditInput, SIGNAL(returnPressed()), ui->toolButtonSend, SLOT(click()));
    connect(ui->textEditInput, SIGNAL(currentCharFormatChanged(QTextCharFormat)), this, SLOT(onTextFormatChange(QTextCharFormat)));

    toolButtonSmilies = new QToolButton();
	toolButtonSmilies->setPopupMode(QToolButton::InstantPopup);
	toolButtonSmilies->setToolTip(tr("Smilies"));
	toolButtonSmilies->setIcon(QIcon(":/Resource/Smileys/0.png"));
	widgetSmileyList = new CWidgetSmileyList(this);
	toolButtonSmilies->setMenu(widgetSmileyList);

	toolButtonPickColor = new QToolButton(this);
	toolButtonPickColor->setIconSize(QSize(24,24));
    if(bIsIrc) {
		toolButtonPickColor->setIcon(QIcon(":/Resource/Generic/Skin.png"));
        toolButtonPickColor->setStyleSheet("");
    } else {
        toolButtonPickColor->setIcon(QIcon());
        toolButtonPickColor->setStyleSheet(QString("QToolButton { background-color: %1; border-style: outset; border-width: 2px;	border-radius: 6px; border-color: lightgrey; }").arg(ui->textEditInput->textColor().name()));
    }
	toolButtonPickColor->setToolTip(tr("Font Color"));
	connect(toolButtonPickColor, SIGNAL(clicked()), this, SLOT(pickColor()));

	toolButtonPrivateMessage = new QToolButton(this);
	toolButtonPrivateMessage->setText(tr("New Private Message"));
	toolButtonPrivateMessage->setToolTip(tr("New Private Message"));
    toolButtonPrivateMessage->setIcon(QIcon(":/Resource/Chat/Chat.png"));
	ui->toolBarTextTools->insertWidget(ui->actionBold, toolButtonPickColor);
	ui->toolBarTextTools->addSeparator();
    ui->toolBarTextTools->addWidget(toolButtonSmilies);
    ui->actionBold->setChecked(ui->textEditInput->fontWeight() == QFont::Bold);
    ui->actionItalic->setChecked(ui->textEditInput->fontItalic());
    ui->actionUnderline->setChecked(ui->textEditInput->fontUnderline());
	ui->toolBarTextTools->addWidget(toolButtonPrivateMessage);
    toolButtonPrivateMessage->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(ui->actionItalic, SIGNAL(toggled(bool)), ui->textEditInput, SLOT(setFontItalic(bool)));
    connect(ui->actionUnderline, SIGNAL(toggled(bool)), ui->textEditInput, SLOT(setFontUnderline(bool)));
	connect(toolButtonPrivateMessage, SIGNAL(clicked()), this, SLOT(addPrivateMessage()));
    setSkin();
    updateToolbar();
}

CWidgetChatInput::~CWidgetChatInput()
{
	delete widgetSmileyList;
	delete ui;
}

void CWidgetChatInput::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void CWidgetChatInput::on_toolButtonSend_clicked()
{
    if (!ui->textEditInput->document()->isEmpty())
	{
        if (ui->textEditInput->document()->lineCount() > 1)
		{
            QStringList lineList = ui->textEditInput->document()->toHtml().split("\n");
			for(int i = 4; i < lineList.size(); i++)
			{
				QTextDocument *line = new QTextDocument();
				line->setHtml(lineList.at(i));
				if(line->toPlainText().startsWith("/"))
					emit messageSent(line->toPlainText());
				else
					emit messageSent(line);
			}
		} else {
            if(ui->textEditInput->document()->toPlainText().startsWith("/"))
                emit messageSent(ui->textEditInput->document()->toPlainText());
			else
                emit messageSent(ui->textEditInput->document());
		}
        ui->textEditInput->addHistory(ui->textEditInput->document());
        ui->textEditInput->resetHistoryIndex();
        QTextCharFormat oldFormat = ui->textEditInput->currentCharFormat();
        ui->textEditInput->document()->clear();
        ui->textEditInput->setCurrentCharFormat(oldFormat);
	}
}

void CWidgetChatInput::setText(QString text)
{
    ui->textEditInput->setHtml(text);
}

void CWidgetChatInput::onTextFormatChange(QTextCharFormat newFormat)
{
	if( newFormat.fontWeight() == QFont::Normal )
		ui->actionBold->setChecked(false);
	else if( newFormat.fontWeight() == QFont::Bold )
		ui->actionBold->setChecked(true);

	ui->actionItalic->setChecked(newFormat.fontItalic());
	ui->actionUnderline->setChecked(newFormat.fontUnderline());
}

void CWidgetChatInput::on_actionBold_toggled(bool checked)
{
    ui->textEditInput->setFontWeight((checked ? QFont::Bold : QFont::Normal));
}

void CWidgetChatInput::on_actionItalic_toggled(bool checked)
{
    ui->textEditInput->setFontItalic(checked);
}

void CWidgetChatInput::on_actionUnderline_toggled(bool checked)
{
    ui->textEditInput->setFontUnderline(checked);
}

void CWidgetChatInput::pickColor()
{
	QColor fontColor;
	if (bIsIrc)
	{
        CDialogIrcColorDialog *dlgIrcColor = new CDialogIrcColorDialog(ui->textEditInput->textColor(), this);
		bool accepted = dlgIrcColor->exec();
		if (accepted)
		{
			if (!dlgIrcColor->isDefaultColor())
			{
				fontColor = dlgIrcColor->color();
                ui->textEditInput->setTextColor(fontColor);
                }
			else
			{
                fontColor = defaultColor;
                ui->textEditInput->setTextColor(fontColor);
            }
		}
	}
	else
	{
        fontColor = QColorDialog::getColor(ui->textEditInput->textColor(), this, tr("Select Font Color"));

		if (fontColor.isValid())
		{
            ui->textEditInput->setTextColor(fontColor);
        }
	}

    updateToolbar();
}

void CWidgetChatInput::addPrivateMessage()
{
	CDialogConnectTo* dlgConnectTo = new CDialogConnectTo(this);
	bool accepted = dlgConnectTo->exec();

	if (accepted)
	{
		CEndPoint ip(dlgConnectTo->getAddressAndPort());

		switch (dlgConnectTo->getConnectNetwork())
		{
		case CDialogConnectTo::G2:
		{
			CChatSessionG2* pS = new CChatSessionG2(ip);
			pS->Connect();
			break;
		}
		case CDialogConnectTo::eDonkey:
			break;
		case CDialogConnectTo::Ares:
			break;
		default:
			break;
		}
	}
}

void CWidgetChatInput::updateToolbar()
{
    if(bIsIrc)
	{
        if((ui->textEditInput->textColor() == defaultColor))
        {
            toolButtonPickColor->setIcon(QIcon(":/Resource/Generic/Skin.png"));
            toolButtonPickColor->setStyleSheet("");
        } else {
            toolButtonPickColor->setIcon(QIcon());
            toolButtonPickColor->setStyleSheet(QString("QToolButton { background-color: %1; border-style: outset; border-width: 2px;	border-radius: 6px; border-color: lightgrey; }").arg(ui->textEditInput->textColor().name()));
        }
	} else {
        toolButtonPickColor->setStyleSheet(QString("QToolButton { background-color: %1; border-style: outset; border-width: 2px;	border-radius: 6px; border-color: lightgrey; }").arg(ui->textEditInput->textColor().name()));
	}
    ui->actionBold->setChecked(ui->textEditInput->fontWeight() == QFont::Bold);
    ui->actionItalic->setChecked(ui->textEditInput->fontItalic());
    ui->actionUnderline->setChecked(ui->textEditInput->fontUnderline());
}

CWidgetReturnEmitTextEdit *CWidgetChatInput::textEdit()
{
    return ui->textEditInput;
}

QLabel *CWidgetChatInput::helpLabel()
{
	return ui->helpLabel;
}

void CWidgetChatInput::setSkin()
{

}
