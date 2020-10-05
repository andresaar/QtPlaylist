#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QtNetworkAuth>
#include <QJsonDocument>
#include <QTimer>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QOAuth2AuthorizationCodeFlow* spotifyAuth;
    QOAuthHttpServerReplyHandler* spotifyReplyHandler;
    QJsonArray jsonReplyArray;
    QTimer* timerToGet;
    QFile* config;
    QJsonDocument configJson;
    bool authorized;

    void loadPlaylists();
    int findJsonArray(const QString string, const QJsonArray arr);
    void loadPlaylistSongs();

private slots:
    void replyFinished(QNetworkReply *reply);
    void performQuery();
    void on_connectToSpotify_clicked();
    void on_spotifySearch_textEdited();
    void on_createPlaylist_clicked();
    void on_pushToPlaylist_clicked();
    void on_playlistSelection_currentTextChanged();
};
#endif // MAINWINDOW_H
