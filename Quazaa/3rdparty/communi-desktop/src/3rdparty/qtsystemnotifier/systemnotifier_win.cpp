/*
* Copyright (C) 2013 J-P Nurmi <jpnurmi@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*/
#include "systemnotifier.h"
#ifndef Q_CC_GNU
#include "win/networknotifier.h"
#endif // !Q_CC_GNU

#include <qt_windows.h>
#include <qabstracteventdispatcher.h>

#if QT_VERSION >= 0x050000
#include <qabstractnativeeventfilter.h>
#endif // QT_VERSION

class SystemNotifierPrivate
#if QT_VERSION >= 0x050000
                            : public QAbstractNativeEventFilter
#endif
{
public:
#if QT_VERSION >= 0x050000
    bool nativeEventFilter(const QByteArray&, void* message, long*)
#else
    static bool nativeEventFilter(void* message)
#endif
    {
        MSG* msg = static_cast<MSG*>(message);
        if (msg && msg->message == WM_POWERBROADCAST) {
            switch (msg->wParam) {
            case PBT_APMSUSPEND:
                QMetaObject::invokeMethod(SystemNotifier::instance(), "sleep");
                break;
            case PBT_APMRESUMESUSPEND:
                QMetaObject::invokeMethod(SystemNotifier::instance(), "wake");
                break;
            default:
                break;
            }
        }
        return false;
    }
#if QT_VERSION < 0x050000
    QAbstractEventDispatcher::EventFilter prev;
#endif
#ifndef Q_CC_GNU
    NetworkNotifier network;
#endif // !Q_CC_GNU
};

void SystemNotifier::initialize()
{
    d = new SystemNotifierPrivate;
#if QT_VERSION >= 0x050000
    QAbstractEventDispatcher::instance()->installNativeEventFilter(d);
#else
    d->prev = QAbstractEventDispatcher::instance()->setEventFilter(SystemNotifierPrivate::nativeEventFilter);
#endif // QT_VERSION

#ifndef Q_CC_GNU
    connect(&d->network, SIGNAL(online()), this, SIGNAL(online()));
    connect(&d->network, SIGNAL(offline()), this, SIGNAL(offline()));
#endif // !Q_CC_GNU
}

void SystemNotifier::uninitialize()
{
    if (QAbstractEventDispatcher::instance())
#if QT_VERSION >= 0x050000
        QAbstractEventDispatcher::instance()->removeNativeEventFilter(d);
#else
        QAbstractEventDispatcher::instance()->setEventFilter(d->prev);
#endif // QT_VERSION
    delete d;
}
