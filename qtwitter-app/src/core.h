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


#ifndef CORE_H
#define CORE_H

#include <QStandardItemModel>
#include <QAuthenticator>
#include <QTimer>
#include <QMap>
#include <QCache>
#include "imagedownload.h"
#include "mainwindow.h"

class TwitPicEngine;
class TweetModel;
class QAbstractItemModel;
class TwitterAccountsModel;
class StatusList;
class TwitterAPI;

class Core : public QObject
{
  Q_OBJECT

public:
  enum AuthDialogState {
    STATE_ACCEPTED,
    STATE_REJECTED,
    STATE_DIALOG_OPEN,
    STATE_DISABLE_ACCOUNT,
    STATE_REMOVE_ACCOUNT
  };

  Core( MainWindow *parent = 0 );
  virtual ~Core();

  void applySettings();
  bool setTimerInterval( int msecs );
#ifdef Q_WS_X11
  void setBrowserPath( const QString& path );
#endif

  void setModelTheme( const ThemeData &theme );
  QAbstractItemModel* getTwitterAccountsModel();
  TweetModel* getModel( const QString &login );

public slots:
  void forceGet();
  void get();
  void get( const QString &login, const QString &password );
  void post( const QString &login, const QString &status, int inReplyTo );
  void destroyTweet( const QString &login, int id );

  void uploadPhoto( const QString &login, QString photoPath, QString status );
  void abortUploadPhoto();
  void twitPicResponse( bool responseStatus, QString message, bool newStatus );

  void downloadImage( const QString &login, Entry *entry );
  void openBrowser( QUrl address );
  AuthDialogState authDataDialog( TwitterAccount *account );

  void retranslateUi();

signals:
  void setupTwitterAccounts( const QList<TwitterAccount> &accounts, bool isPublicTimelineRequested );
  void errorMessage( const QString &message );
  void twitPicResponseReceived();
  void twitPicDataSendProgress(int,int);
  void setImageForUrl( const QString& url, QImage *image );
  void requestListRefresh( bool isPublicTimeline, bool isSwitchUser);
  void requestStarted();
  void allRequestsFinished();
  void resetUi();
  void timelineUpdated();
  void directMessagesSyncChanged( bool b );
  void modelChanged( TweetModel *model );
  void addReplyString( const QString &user, int id );
  void addRetweetString( QString message );
  void about();
  void sendNewsReport( QString message );
  void resizeData( int width, int oldWidth );
  void newRequest();

private slots:
  void setImageInHash( const QString&, QImage );
  void addEntry( const QString &login, Entry* entry );
  void deleteEntry( const QString &login, int id );
  void slotUnauthorized( const QString &login, const QString &password );
  void slotUnauthorized( const QString &login, const QString &password, const QString &status, int inReplyToId );
  void slotUnauthorized( const QString &login, const QString &password, int destroyId );
  void slotNewRequest();
  void slotRequestDone( const QString &login, int role );
  void storeNewTweets( const QString &login );

private:
  void sendNewsInfo();
  void setupTweetModels();
  void createConnectionsWithModel( TweetModel *model );
  bool retryAuthorizing( TwitterAccount *account, int role );
  bool authDialogOpen;
  bool publicTimeline;
  int requestCount;
  int tempModelCount;
  QStringList newTweets;

  TwitPicEngine *twitpicUpload;
  QMap<QString,ImageDownload*> imageDownloader;
  TwitterAccountsModel *accountsModel;
  TwitterAPI *twitterapi;
  QMap<QString,TweetModel*> tweetModels;
  QCache<QString,QImage> imageCache;
  QTimer *timer;

  StatusList *listViewForModels;
  int margin;

#ifdef Q_WS_X11
  QString browserPath;
#endif
};

#endif //CORE_H