#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtNetworkAuth>
#include <QtCore>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // create OAuth object to use Spotify API
    spotifyAuth = new QOAuth2AuthorizationCodeFlow;
    // Http server for Uri reply
    spotifyReplyHandler = new QOAuthHttpServerReplyHandler(1337, this);
    spotifyAuth->setReplyHandler(spotifyReplyHandler);
    spotifyAuth->setClientIdentifier("98b2841ba0d641e0807b567d06b02fd7"); // Client ID
    spotifyAuth->setAuthorizationUrl(QUrl("https://accounts.spotify.com/authorize"));
    spotifyAuth->setAccessTokenUrl(QUrl("https://accounts.spotify.com/api/token"));
    spotifyAuth->setClientIdentifierSharedKey("b4bdb54dd7354c52b69a0a2ac81d7b2b"); // Client Secret

    // Open Browser to grant authorization -- needed for search queries
    connect(spotifyAuth, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    // Test -- when authorized, try get Amianto - Supercombo information from API
    connect(spotifyAuth, &QOAuth2AuthorizationCodeFlow::statusChanged, [=](
                QAbstractOAuth::Status status) {
            if (status == QAbstractOAuth::Status::Granted){
                spotifyAuth->get(QUrl("https://api.spotify.com/v1/search?q=amianto&type=track"));
            }
        });

    // Test -- when receiving API response, show information in qDebug
    connect(spotifyAuth, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    // Initiate authorization process
    spotifyAuth->grant();

}

// Function to get QNetworkReply data when it finishes receiving
// and printing to qDebug
void MainWindow::replyFinished(QNetworkReply *reply) {
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }

    QString answer = reply->readAll();

    qDebug() << answer;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete spotifyAuth;
}

