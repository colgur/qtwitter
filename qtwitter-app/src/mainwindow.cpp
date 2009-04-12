/***************************************************************************
 *   Copyright (C) 2008-2009 by Dominik Kapusta       <d@ayoy.net>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <QMenu>
#include <QScrollBar>
#include <QMessageBox>
#include <QIcon>
#include <QMovie>
#include <QPalette>
#include <QShortcut>
#include <QDesktopWidget>
#include <QSignalMapper>
#include <QTreeView>
#include "mainwindow.h"
#include "tweet.h"
#include "ui_about.h"
#include "twitteraccountsmodel.h"
#include "twitteraccountsdelegate.h"
#include "settings.h"

extern ConfigFile settings;

const QString MainWindow::APP_VERSION = "0.6.0_pre1";

MainWindow::MainWindow( QWidget *parent ) :
    QWidget( parent ),
    resetUiWhenFinished( false )
{
  ui.setupUi( this );
  ui.accountsComboBox->setVisible( false );

  progressIcon = new QMovie( ":/icons/progress.gif", "gif", this );
  ui.countdownLabel->setMovie( progressIcon );
  ui.countdownLabel->setToolTip( ui.countdownLabel->text() + " " + tr( "characters left" ) );

  createConnections();
  createMenu();
  createTrayIcon();
}

MainWindow::~MainWindow() {
  settings.setValue( "TwitterAccounts/currentModel", ui.accountsComboBox->currentIndex() );
}

void MainWindow::createConnections()
{
  StatusFilter *filter = new StatusFilter( this );
  ui.statusEdit->installEventFilter( filter );

  connect( ui.updateButton, SIGNAL( clicked() ), this, SIGNAL( updateTweets() ) );
  connect( ui.settingsButton, SIGNAL( clicked() ), this, SIGNAL(settingsDialogRequested()) );
  connect( ui.statusEdit, SIGNAL( textChanged( QString ) ), this, SLOT( changeLabel() ) );
  connect( ui.statusEdit, SIGNAL( editingFinished() ), this, SLOT( resetStatus() ) );
  connect( ui.statusEdit, SIGNAL(errorMessage(QString)), this, SLOT(popupError(QString)) );
  connect( ui.accountsComboBox, SIGNAL(activated(int)), this, SLOT(configSaveCurrentModel(int)) );
  connect( filter, SIGNAL( enterPressed() ), this, SLOT( sendStatus() ) );
  connect( filter, SIGNAL( escPressed() ), ui.statusEdit, SLOT( cancelEditing() ) );
  connect( this, SIGNAL(addReplyString(QString,int)), ui.statusEdit, SLOT(addReplyString(QString,int)) );
  connect( this, SIGNAL(addRetweetString(QString)), ui.statusEdit, SLOT(addRetweetString(QString)) );

  QShortcut *hideShortcut = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_H ), this );
  connect( hideShortcut, SIGNAL(activated()), this, SLOT(hide()) );
  QShortcut *quitShortcut = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_Q ), this );
  connect( quitShortcut, SIGNAL(activated()), qApp, SLOT(quit()) );
#ifdef Q_WS_MAC
  ui.settingsButton->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Comma ) );
#else
  ui.settingsButton->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_S ) );
#endif
  ui.updateButton->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_R ) );
}

void MainWindow::createMenu()
{
  buttonMenu = new QMenu( this );
  newtweetAction = new QAction( tr( "New tweet" ), buttonMenu );
  newtwitpicAction = new QAction( tr( "Upload a photo to TwitPic" ), buttonMenu );
  gototwitterAction = new QAction( tr( "Go to Twitter" ), buttonMenu );
  gototwitpicAction = new QAction( tr( "Go to TwitPic" ), buttonMenu );
  newtweetAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
  newtwitpicAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_N ) );
  gototwitterAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_G ) );
  gototwitpicAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_G ) );

  QSignalMapper *mapper = new QSignalMapper( this );
  mapper->setMapping( gototwitterAction, "http://twitter.com/home" );
  mapper->setMapping( gototwitpicAction, "http://twitpic.com" );

  connect( newtweetAction, SIGNAL(triggered()), ui.statusEdit, SLOT(setFocus()) );
  connect( newtwitpicAction, SIGNAL(triggered()), this, SIGNAL(openTwitPicDialog()) );
  connect( gototwitterAction, SIGNAL(triggered()), mapper, SLOT(map()) );
  connect( gototwitpicAction, SIGNAL(triggered()), mapper, SLOT(map()) );
  connect( mapper, SIGNAL(mapped(QString)), this, SLOT(emitOpenBrowser(QString)) );

  buttonMenu->addAction( newtweetAction );
  buttonMenu->addAction( newtwitpicAction );
  buttonMenu->addSeparator();
  buttonMenu->addAction( gototwitterAction );
  buttonMenu->addAction( gototwitpicAction );
  ui.moreButton->setMenu( buttonMenu );
}

void MainWindow::createTrayIcon()
{
  trayIcon = new QSystemTrayIcon( this );
  trayIcon->setIcon( QIcon( ":/icons/twitter_48.png" ) );

  connect( trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)) );
  connect( trayIcon, SIGNAL(messageClicked()), this, SLOT(show()) );
#ifndef Q_WS_MAC
  QMenu *trayMenu = new QMenu( this );
  trayMenu = new QMenu( this );
  QAction *quitaction = new QAction( tr( "Quit" ), trayMenu);
  QAction *settingsaction = new QAction( tr( "Settings" ), trayMenu);
  settingsaction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_S ) );
  quitaction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Q ) );

  connect( quitaction, SIGNAL(triggered()), qApp, SLOT(quit()) );
  connect( settingsaction, SIGNAL(triggered()), this, SIGNAL(settingsDialogRequested()) );
  connect( settingsaction, SIGNAL(triggered()), this, SLOT(show()) );

  trayMenu->addAction(settingsaction);
  trayMenu->addSeparator();
  trayMenu->addAction(quitaction);
  trayIcon->setContextMenu( trayMenu );

  trayIcon->setToolTip( "qTwitter" );
#endif
  trayIcon->show();
}

StatusList* MainWindow::getListView()
{
  return ui.statusListView;
}

int MainWindow::getScrollBarWidth()
{
  return ui.statusListView->verticalScrollBar()->size().width();
}

void MainWindow::setupTwitterAccounts( const QList<TwitterAccount> &accounts, bool publicTimeline )
{
  if ( ( !publicTimeline && accounts.size() < 2 ) || accounts.isEmpty() ) {
    ui.accountsComboBox->setVisible( false );
    if ( !accounts.isEmpty() )
      emit switchModel( accounts.at(1).login );
    return;
  }

  ui.accountsComboBox->clear();
  foreach ( TwitterAccount account, accounts ) {
    if ( account.isEnabled )
      ui.accountsComboBox->addItem( account.login );
  }
  if ( publicTimeline )
    ui.accountsComboBox->addItem( tr( "public timeline" ) );
  if ( ui.accountsComboBox->count() <= 1 ) {
    ui.accountsComboBox->setVisible( false );
    emit switchModel( ui.accountsComboBox->currentText() );
    return;
  }
  ui.accountsComboBox->setVisible( true );
  int index = settings.value( "TwitterAccounts/currentModel", 0 ).toInt();
  if ( index >= ui.accountsComboBox->count() )
    ui.accountsComboBox->setCurrentIndex( ui.accountsComboBox->count() - 1 );
  else
    ui.accountsComboBox->setCurrentIndex( index );
  emit switchModel( ui.accountsComboBox->currentText() );
}

void MainWindow::setListViewModel( TweetModel *model )
{
  if ( !model )
    return;
  TweetModel *currentModel = qobject_cast<TweetModel*>( ui.statusListView->model() );
  if ( currentModel )
    currentModel->setVisible( false );
  ui.statusListView->setModel( model );
  model->display();
}

void MainWindow::closeEvent( QCloseEvent *e )
{
  if ( trayIcon->isVisible()) {
    hide();
    e->ignore();
    return;
  }
  QWidget::closeEvent( e );
}

void MainWindow::iconActivated( QSystemTrayIcon::ActivationReason reason )
{
  switch ( reason ) {
    case QSystemTrayIcon::Trigger:
#ifdef Q_WS_WIN
    if ( !isVisible() ) {
#else
    if ( !isVisible() || !QApplication::activeWindow() ) {
#endif
      show();
        raise();
        activateWindow();
      } else {
        hide();
      }
      break;
    default:
      break;
  }
}

void MainWindow::changeLabel()
{
  ui.countdownLabel->setText( ui.statusEdit->isStatusClean() ? QString::number( StatusEdit::STATUS_MAX_LENGTH ) : QString::number( StatusEdit::STATUS_MAX_LENGTH - ui.statusEdit->text().length() ) );
  ui.countdownLabel->setToolTip( ui.countdownLabel->text() + " " + tr( "characters left" ) );
}

void MainWindow::sendStatus()
{
  resetUiWhenFinished = true;
  emit post( ui.accountsComboBox->currentText(), ui.statusEdit->text(), ui.statusEdit->getInReplyTo() );
  showProgressIcon();
}

void MainWindow::resetStatusEdit()
{
  if ( resetUiWhenFinished ) {
    resetUiWhenFinished = false;
    ui.statusEdit->cancelEditing();
  }
  progressIcon->stop();
  changeLabel();
}

void MainWindow::showProgressIcon()
{
  ui.countdownLabel->clear();
  ui.countdownLabel->setMovie( progressIcon );
  progressIcon->start();
}

void MainWindow::configSaveCurrentModel( int index )
{
  if ( settings.value( "TwitterAccounts/currentModel", 0 ).toInt() != index ) {
    settings.setValue( "TwitterAccounts/currentModel", index );
    emit switchModel( ui.accountsComboBox->currentText() );
  }
}

void MainWindow::resetStatus()
{
  if ( ui.statusEdit->isStatusClean() ) {
    changeLabel();
  }
}

void MainWindow::resizeEvent( QResizeEvent *event )
{
  emit resizeView( event->size().width(), event->oldSize().width() );
}

void MainWindow::popupMessage( QString message )
{
  trayIcon->showMessage( tr( "New tweets" ), message, QSystemTrayIcon::Information );
}

void MainWindow::popupError( const QString &message )
{
  QMessageBox::information( this, tr("Error"), message );
}

void MainWindow::emitOpenBrowser( QString address )
{
  emit openBrowser( QUrl( address ) );
}

void MainWindow::changeListBackgroundColor(const QColor &newColor )
{
  QPalette palette( ui.statusListView->palette() );
  palette.setColor( QPalette::Base, newColor );
  ui.statusListView->setPalette( palette );
  ui.statusListView->update();
}

void MainWindow::about()
{
  QDialog *dlg = new QDialog( this );
  Ui::AboutDialog aboutUi;
  aboutUi.setupUi( dlg );
  dlg->adjustSize();
  aboutUi.textBrowser->setHtml( aboutUi.textBrowser->toHtml().arg( APP_VERSION ) );
  dlg->exec();
  dlg->deleteLater();
}

void MainWindow::retranslateUi()
{
  ui.moreButton->setToolTip( tr("More...") );
  ui.settingsButton->setToolTip( tr("Settings") );
  ui.updateButton->setToolTip( tr("Update tweets") );
  if ( ui.statusEdit->isStatusClean() ) {
    ui.statusEdit->initialize();
  }
  ui.statusEdit->setText( tr("What are you doing?") );
  newtweetAction->setText( tr( "New tweet" ) );
  newtwitpicAction->setText( tr( "Upload a photo to TwitPic" ) );
  gototwitterAction->setText( tr( "Go to Twitter" ) );
  gototwitpicAction->setText( tr( "Go to TwitPic" ) );
}

/*! \class MainWindow
    \brief A class defining the main window of the application.

    This class contains all the GUI elements of the main application window.
    It receives signals from Core and TweetModel classes and provides means
    of visualization for them.
*/

/*! \fn MainWindow::MainWindow( QWidget *parent = 0 )
    A default constructor. Creates a MainWindow instance with the given \a parent.
*/

/*! \fn MainWindow::~MainWindow()
    A default destructor.
*/

/*! \fn StatusList* MainWindow::getListView()
    A method for external access to the list view used for displaying Tweets.
    Used for initialization of TweetModel class's instance.
    \returns A pointer to the list view instance of MainWindow.
*/

/*! \fn int MainWindow::getScrollBarWidth()
    A method for accessing the list view scrollbar's width, needed for computing width
    of Tweet class instances.
    \returns List view scrollbar's width.
*/

/*! \fn void MainWindow::setListViewModel( TweetModel *model )
    Assigns the \a model to be a list view model.
    \param model The model for the list view.
*/

/*! \fn void MainWindow::changeListBackgroundColor( const QColor &newColor )
    Sets the background color of a list view. Used when changing color theme.
    \param newColor List view new background color.
*/

/*! \fn void MainWindow::popupMessage( int statusesCount, QStringList namesForStatuses, int messagesCount, QStringList namesForMessages )
    Pops up a tray icon notification message containing information about
    new Tweets and direct messages (if any). Displays total messages count and
    their authors' names for status updates and direct messages separately.
    \param statusesCount The amount of new status updates.
    \param namesForStatuses List of statuses senders' names.
    \param messagesCount The amount of new direct messages.
    \param namesForMessages List of direct messages senders' names.
*/

/*! \fn void MainWindow::popupError( const QString &message )
    Pops up a dialog with an error or information for User. This is an interface
    for all the classes that notify User about any problems (e.g. Core, TwitterAPI
    and ImageDownload).
    \param message An information to be shown to User.
*/

/*! \fn void MainWindow::retranslateUi()
    Retranslates all the translatable GUI elements of the class. Used when changing
    UI language on the fly.
*/

/*! \fn void MainWindow::resetStatusEdit()
    Resets status edit field if necessary. Invoked mainly when updating timeline finishes.
*/

/*! \fn void MainWindow::showProgressIcon()
    Displays progress icon when processing a request.
*/

/*! \fn void MainWindow::about()
    Pops up a small dialog with credits and short info on the application and its author.
*/

/*! \fn void MainWindow::updateTweets()
    Emitted to force timeline update, assigned to pressing the update button.
*/

/*! \fn void MainWindow::openTwitPicDialog()
    Emitted to open a dialog for uploading a photo to TwitPic.
*/

/*! \fn void MainWindow::post( const QByteArray& status, int inReplyTo = -1 )
    Emitted to post a status update. Assigned to pressing Enter inside the status edit field.
    \param status A status to be posted.
    \param inReplyTo In case the status is a reply - optional id of the existing status to which the reply is posted.
*/

/*! \fn void MainWindow::openBrowser( QUrl address )
    Emitted when "Go to..." action requested, asks to open a default browser.
    \param address Requested URL.
    \sa Core::openBrowser()
*/

/*! \fn void MainWindow::settingsDialogRequested()
    Emitted when settings button pressed, requests opening the settings dialog.
*/

/*! \fn void MainWindow::addReplyString( const QString& user, int inReplyTo )
    Works as a proxy between Tweet class instance and status edit field. Passes the request
    to initiate editing a reply.
    \param user Login of a User to whom the current User replies.
    \param inReplyTo Id of the existing status to which the reply is posted.
*/

/*! \fn void MainWindow::addRetweetString( QString message )
    Works as a proxy between Tweet class instance and status edit field. Passes the request
    to initiate editing a retweet.
    \param message A retweet message
*/

/*! \fn void MainWindow::resizeView( int width, int oldWidth )
    Emitted when resizing a window, to inform all the Tweets about the size change.
    \param width The width after resizing.
    \param oldWidth The width before resizing.
*/

/*! \fn void MainWindow::closeEvent( QCloseEvent *e )
    An event reimplemented in order to provide hiding instead of closing the application.
    Closing is provided only via a shortcut or tray icon menu option.
    \param e A QCloseEvent event's representation.
*/