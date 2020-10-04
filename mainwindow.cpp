#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtNetworkAuth>
#include <QtCore>
#include <QtGui>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // create OAuth object to use Spotify API
    this->authorized = FALSE;
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

    // When authorized, set bool 'authorized' as True and change color of button
    connect(spotifyAuth, &QOAuth2AuthorizationCodeFlow::statusChanged, [=](
                QAbstractOAuth::Status status) {
            if (status == QAbstractOAuth::Status::Granted){
                ui->connectToSpotify->setStyleSheet("background-color: rgb(36,223,7)");
                ui->connectToSpotify->setText("Connected to Spotify");
                ui->connectToSpotify->setDisabled(TRUE);
                this->authorized = TRUE;
            }
        });

    // Timer to wait 500ms after last change in lineEdit to perform query
    timerToGet = new QTimer(this);
    timerToGet->setSingleShot(true);
    connect(timerToGet, &QTimer::timeout, this, &MainWindow::performQuery);

    // When receiving API response treat response
    connect(spotifyAuth, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

}

// Function to get QNetworkReply data when it finishes receiving
// and printing results in list
void MainWindow::replyFinished(QNetworkReply *reply) {
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }

    QJsonDocument jsonReply = QJsonDocument::fromJson(reply->readAll());
    QJsonObject tracks = jsonReply.object().value("tracks").toObject();
    jsonReplyArray = tracks.value("items").toArray(); // Array with query reply items

    // Create a list with the song name and artis name for each item
    QStringList queryResult;
    foreach (const QJsonValue &value, jsonReplyArray){
        QString song = "";
        song += (QString) value.toObject()["name"].toString();
        song += " - ";
        song += value.toObject()["artists"].toArray()[0].toObject()["name"].toString();
        queryResult.append(song);
    }

    // Write in ui list
    ui->listQueryResult->clear();
    ui->listQueryResult->addItems(queryResult);

    qDebug() << queryResult;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete spotifyAuth;
    delete spotifyReplyHandler;
}

// Connect to Spotify when button clicked
void MainWindow::on_connectToSpotify_clicked()
{
    // Initiate authorization process
    spotifyAuth->grant();
}

// Timer start when editing lineEdit, will only finish after stop editing
void MainWindow::on_spotifySearch_textEdited()
{
    timerToGet->start(500);
}

// When timer timeout, search for string in lineEdit
// using Spotify API
void MainWindow::performQuery()
{
    if (this->authorized){
        QString searchString = ui->spotifySearch->text();
        if (!searchString.isEmpty())
            spotifyAuth->get(QUrl("https://api.spotify.com/v1/search?q="+searchString+"&type=track"));
    }
}
