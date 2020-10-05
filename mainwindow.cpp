#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtNetworkAuth>
#include <QtCore>
#include <QtGui>
#include <QJsonDocument>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>

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

    // Read config file, or create it if non-existant
    if (QFileInfo::exists(".\\config\\config.json") && QFileInfo(".\\config\\config.json").exists()) {
        config = new QFile(".\\config\\config.json");
        config->open(QFile::ReadOnly | QFile::Text);
        configJson = QJsonDocument::fromJson(config->readAll());
        config->close();
    }
    else {
        QDir dir;
        dir.mkdir("config");

        QJsonObject initialObj = configJson.object();
        initialObj.insert("playlists", QJsonObject());
        configJson.setObject(initialObj);
    }

    loadPlaylists();
    loadPlaylistSongs();

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
    // save configurations as json file
    config = new QFile(".\\config\\config.json");
    config->open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
    config->write(configJson.toJson());
    config->close();

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

// load saved playlists to drop-down menu
void MainWindow::loadPlaylists()
{
    // Load playlists dropdown
    QStringList playlists;
    foreach(const QString &value, configJson.object()["playlists"].toObject().keys()){
        playlists.append(value);
    }
    ui->playlistSelection->clear();
    ui->playlistSelection->addItems(playlists);
}

// Create new playlist
void MainWindow::on_createPlaylist_clicked()
{
    // Pop-up input to get playlist name
    bool OK;
    QString newPlaylist = QInputDialog::getText(this, "New Playlist", "Playlist Name: ",
                                                QLineEdit::Normal, QString(), &OK);
    if(!OK){ // if cancel, do nothing
        return;
    }

    if (newPlaylist.isEmpty()) {
        QMessageBox::warning(this, "Playlist Error", "Empty playlist name");
    }
    else if (configJson.object()["playlists"].toArray().contains(newPlaylist)) {
        QMessageBox::warning(this, "Playlist Error", "Duplicate playlist name");
    }
    else { // add new playlist name to configs
        QJsonObject rootObject = configJson.object();
        QJsonObject appended = rootObject["playlists"].toObject();
        appended.insert(newPlaylist, QJsonArray());
        rootObject.insert("playlists", appended);
        configJson.setObject(rootObject);
    }

    // reload playlists
    loadPlaylists();

    // set new as active
    ui->playlistSelection->setCurrentText(newPlaylist);

}

// push a selected song to the current playlist
void MainWindow::on_pushToPlaylist_clicked()
{
    if (ui->listQueryResult->selectedItems().isEmpty()) {
        return;
    }

    foreach(const QListWidgetItem *selected, ui->listQueryResult->selectedItems()) {
        QJsonObject root = configJson.object();
        QJsonObject playlists = root["playlists"].toObject();
        QJsonArray addSong = playlists[ui->playlistSelection->currentText()].toArray();
        addSong.append(selected->text());
        playlists.insert(ui->playlistSelection->currentText(), addSong);
        root.insert("playlists", playlists);

        configJson.setObject(root);
    }

    loadPlaylistSongs();
}

int MainWindow::findJsonArray(const QString string, const QJsonArray arr)
{
    for(int i=0; i<arr.size(); i++){
        if (arr[i].toString() == string)
            return i;
    }
    return -1;
}

// load the song list to the current playlist
void MainWindow::loadPlaylistSongs()
{
    QJsonArray songs = configJson.object()["playlists"].toObject()[ui->playlistSelection->currentText()].toArray();
    ui->playlistSongs->clear();
    foreach(const QJsonValue &song, songs){
        ui->playlistSongs->addItem(song.toString());
    }
}

// load songs when playlist change
void MainWindow::on_playlistSelection_currentTextChanged()
{
    loadPlaylistSongs();
}
