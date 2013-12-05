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
#include <QtDBus>

class SystemNotifierPrivate : public QObject
{
    Q_OBJECT

private slots:
    void sleeping()
    {
        QMetaObject::invokeMethod(SystemNotifier::instance(), "sleep");
    }

    void resuming()
    {
        QMetaObject::invokeMethod(SystemNotifier::instance(), "wake");
    }

    void networkStateChanged(uint state)
    {
        static const uint NM_STATE_DISCONNECTED = 20;
        static const uint NM_STATE_CONNECTED_GLOBAL = 70;

        if (state == NM_STATE_DISCONNECTED)
            QMetaObject::invokeMethod(SystemNotifier::instance(), "offline");
        else if (state == NM_STATE_CONNECTED_GLOBAL)
            QMetaObject::invokeMethod(SystemNotifier::instance(), "online");
    }
};

void SystemNotifier::initialize()
{
    d = new SystemNotifierPrivate;

    QDBusConnection bus = QDBusConnection::systemBus();
    bus.connect("org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
                "org.freedesktop.NetworkManager", "StateChanged", d, SLOT(networkStateChanged(uint)));

    bus.connect("org.freedesktop.UPower", "/org/freedesktop/UPower",
                "org.freedesktop.UPower", "Sleeping", d, SLOT(sleeping()));
    bus.connect("org.freedesktop.UPower", "/org/freedesktop/UPower",
                "org.freedesktop.UPower", "Resuming", d, SLOT(resuming()));
}

void SystemNotifier::uninitialize()
{
    delete d;
}

#include "systemnotifier_dbus.moc"
