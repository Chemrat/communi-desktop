/*
 * Copyright (C) 2008-2013 The Communi Project
 *
 * This example is free, and not covered by the LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "mainwindow.h"
#include "connectpage.h"
#include "soundnotification.h"
#include "systemnotifier.h"
#include "sharedtimer.h"
#include "qtdocktile.h"
#include "chatpage.h"
#include <QDesktopServices>
#include <IrcBufferModel>
#include <IrcConnection>
#include <QApplication>
#include <QCloseEvent>
#include <IrcChannel>
#include <QSettings>
#include <IrcBuffer>
#include <QShortcut>
#include <QTimer>
#include <QDir>

MainWindow::MainWindow(QWidget* parent) : QStackedWidget(parent)
{
    d.normalIcon.addFile(":/icons/16x16/communi2.png");
    d.normalIcon.addFile(":/icons/24x24/communi2.png");
    d.normalIcon.addFile(":/icons/32x32/communi2.png");
    d.normalIcon.addFile(":/icons/48x48/communi2.png");
    d.normalIcon.addFile(":/icons/64x64/communi2.png");
    d.normalIcon.addFile(":/icons/128x128/communi2.png");

    d.alertIcon.addFile(":/icons/16x16/communi2-alert.png");
    d.alertIcon.addFile(":/icons/24x24/communi2-alert.png");
    d.alertIcon.addFile(":/icons/32x32/communi2-alert.png");
    d.alertIcon.addFile(":/icons/48x48/communi2-alert.png");
    d.alertIcon.addFile(":/icons/64x64/communi2-alert.png");
    d.alertIcon.addFile(":/icons/128x128/communi2-alert.png");

#ifndef Q_OS_MAC
    setWindowIcon(d.normalIcon);
    qApp->setWindowIcon(d.normalIcon);
#endif // Q_OS_MAC

    QSettings settings;
    settings.beginGroup("Widgets/MainWindow");
    if (settings.contains("geometry"))
        restoreGeometry(settings.value("geometry").toByteArray());

    d.connectPage = new ConnectPage(this);
    connect(d.connectPage, SIGNAL(accepted()), this, SLOT(onAccepted()));
    connect(d.connectPage, SIGNAL(rejected()), this, SLOT(onRejected()));
    addWidget(d.connectPage);

    d.chatPage = new ChatPage(this);
    connect(d.chatPage, SIGNAL(currentBufferChanged(IrcBuffer*)), this, SLOT(setCurrentBuffer(IrcBuffer*)));
    addWidget(d.chatPage);

    d.dockTile = 0;
    if (QtDockTile::isAvailable())
        d.dockTile = new QtDockTile(this);

    d.trayIcon = 0;
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        d.trayIcon = new QSystemTrayIcon(this);
        d.trayIcon->setIcon(d.normalIcon);
        d.trayIcon->setVisible(true);
        connect(d.trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this, SLOT(onTrayIconActivated(QSystemTrayIcon::ActivationReason)));
    }

    d.sound = 0;
    if (SoundNotification::isAvailable()) {
        d.sound = new SoundNotification(this);

        QDir dataDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
        if (dataDir.exists() || dataDir.mkpath(".")) {
            QString filePath = dataDir.filePath("notify.mp3");
            if (!QFile::exists(filePath))
                QFile::copy(":/notify.mp3", filePath);
            d.sound->setFilePath(filePath);
        }
    }

    setCurrentBuffer(0);

    QShortcut* shortcut = new QShortcut(QKeySequence::Quit, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));

    shortcut = new QShortcut(QKeySequence::New, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(doConnect()));

    shortcut = new QShortcut(QKeySequence::Close, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(closeBuffer()));

    restoreSessions();
}

MainWindow::~MainWindow()
{
}

QSize MainWindow::sizeHint() const
{
    return QSize(800, 600);
}

void MainWindow::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
        unalert();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (isVisible()) {
        QSettings settings;
        settings.beginGroup("Widgets/MainWindow");
        settings.setValue("geometry", saveGeometry());

        QList<IrcConnection*> connections = d.chatPage->connections();
        foreach (IrcConnection* connection, connections) {
            connection->quit(tr("Communi %1").arg(IRC_VERSION_STR));
            connection->close();
        }

        // let the sessions close in the background
        hide();
        event->ignore();
        QTimer::singleShot(1000, qApp, SLOT(quit()));
    }
}

void MainWindow::alert()
{
    if (!isActiveWindow()) {
        QApplication::alert(this);
        if (d.dockTile)
            d.dockTile->setBadge(d.dockTile->badge() + 1);
        if (d.sound)
            d.sound->play();
        if (d.trayIcon) {
            d.trayIcon->setIcon(d.alertIcon);
            SharedTimer::instance()->registerReceiver(this, "doAlert");
        }
    }
}

void MainWindow::unalert()
{
    if (isActiveWindow()) {
        if (d.dockTile)
            d.dockTile->setBadge(0);
        if (d.trayIcon) {
            d.trayIcon->setIcon(d.normalIcon);
            SharedTimer::instance()->unregisterReceiver(this, "doAlert");
        }
    }
}

void MainWindow::doAlert()
{
    QIcon current = d.trayIcon->icon();
    if (current.cacheKey() == d.normalIcon.cacheKey())
        d.trayIcon->setIcon(d.alertIcon);
    else
        d.trayIcon->setIcon(d.normalIcon);
}

void MainWindow::doConnect()
{
    setCurrentIndex(0);
}

void MainWindow::onAccepted()
{
    IrcConnection* connection = new IrcConnection(this);
    connection->setHost(d.connectPage->host());
    connection->setPort(d.connectPage->port());
    connection->setSecure(d.connectPage->isSecure());
    connection->setNickName(d.connectPage->nickName());
    connection->setRealName(d.connectPage->realName());
    connection->setUserName(d.connectPage->userName());
    connection->setDisplayName(d.connectPage->displayName());
    connection->setPassword(d.connectPage->password());
    connection->open();

    connect(SystemNotifier::instance(), SIGNAL(sleep()), connection, SLOT(quit()));
    connect(SystemNotifier::instance(), SIGNAL(wake()), connection, SLOT(open()));
    connect(SystemNotifier::instance(), SIGNAL(online()), connection, SLOT(open()));
    connect(SystemNotifier::instance(), SIGNAL(offline()), connection, SLOT(quit()));

    d.chatPage->addConnection(connection);
    setCurrentWidget(d.chatPage);
}

void MainWindow::onRejected()
{
    setCurrentWidget(d.chatPage);
}

void MainWindow::closeBuffer()
{
    IrcBuffer* buffer = 0; //d.chatPage->currentBuffer();
    IrcChannel* channel = qobject_cast<IrcChannel*>(buffer);
    if (channel)
        channel->part(tr("Communi %1").arg(IRC_VERSION_STR));
    IrcBufferModel* model = buffer->model();

    if (buffer == model->get(0)) {
        IrcConnection* connection = buffer->connection();
        connection->quit(tr("Communi %1").arg(IRC_VERSION_STR));
        connection->close();
        d.chatPage->removeConnection(connection);
    } else {
        delete buffer;
    }

    if (d.chatPage->connections().isEmpty())
        setCurrentWidget(d.connectPage);
}

void MainWindow::setCurrentBuffer(IrcBuffer* buffer)
{
    if (buffer)
        setWindowTitle(tr("%2 - Communi %1").arg(IRC_VERSION_STR).arg(buffer->title()));
    else
        setWindowTitle(tr("Communi %1").arg(IRC_VERSION_STR));
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::DoubleClick:
            setVisible(!isVisible());
            break;
        case QSystemTrayIcon::Trigger:
            raise();
            activateWindow();
            break;
        default:
            break;
    }
}

void MainWindow::restoreSessions()
{
//    QSettings settings;
//    settings.beginGroup("Sessions");
//    QStringList sessions = settings.childGroups();

//    foreach (const QString& uuid, sessions) {
//        IrcSession* session = new IrcSession(this);
//        SessionManager* manager = new SessionManager(session);
//        manager->restore(uuid);
//        if (manager->enabled())
//            session->open();

//        // TODO:
//        connect(SystemNotifier::instance(), SIGNAL(sleep()), manager, SLOT(sleep()));
//        connect(SystemNotifier::instance(), SIGNAL(wake()), manager, SLOT(wake()));
//        connect(SystemNotifier::instance(), SIGNAL(online()), manager, SLOT(wake()));
//        connect(SystemNotifier::instance(), SIGNAL(offline()), session, SLOT(close()));

//        d.chatPage->addSession(manager);
//    }

//    if (!sessions.isEmpty())
//        setCurrentWidget(d.chatPage);
}