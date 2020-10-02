#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QtNetworkAuth>

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

private slots:
    void replyFinished(QNetworkReply *reply);
};
#endif // MAINWINDOW_H
