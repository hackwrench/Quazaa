#ifndef WIDGETCHAT_H
#define WIDGETCHAT_H

#include <QWidget>
#include "widgetchatcenter.h"

namespace Ui {
    class WidgetChat;
}

class WidgetChat : public QWidget {
    Q_OBJECT
public:
    WidgetChat(QWidget *parent = 0);
    ~WidgetChat();
	WidgetChatCenter *panelChatCenter;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::WidgetChat *ui;
};

#endif // WIDGETCHAT_H
