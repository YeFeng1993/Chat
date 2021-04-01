#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDialog>
#include <QDebug>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QTextBlock>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QIcon>
#include <QMap>
#include <QString>
#include <QDebug>
#include "chat.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void init();
    void start();

private:
    Ui::Widget *ui;
    QListWidget *list;
    QGroupBox *GBox;
    QLineEdit *IpEdit;
    QTextEdit *textread;
    QTextEdit *textwrite;
    QPushButton *send;
    QPushButton *AddButton;
    QPushButton *DelButton;
    QVBoxLayout *vlayout;
    QVBoxLayout *vlayout1;
    QHBoxLayout *hlayout;
    QHBoxLayout *hlayout1;
    QHBoxLayout *hlayout2;

    const int Maxblockcount = 1000;
    bool visible = false;
    int epfd;
    int chatfd;
    int s;
    struct sockaddr_in servaddr;
    socklen_t len;
    struct epoll_event events[EPOLL_SIZE];
    char message[BUF_SIZE];
    QMap<int, QString>connectmap;
    QMap<int, QStringList>chatlog;
    QMap<int, bool>isunread;

    void Addfd(int epollfd, int fd, bool enable_et);
    void wait();
    void respond(QString ip);
    void SendMessage();
    void ShowDialog();
    void Connect(QLineEdit *iptext);
    void Delete();

signals:
    void ShowRecvMessage(char *news);
    void RemoteClose(int closefd);

private slots:
    void slot_ShowRecvMessage(char *news);
    void slot_RemoteClose(int closefd);
};

#endif // WIDGET_H
