#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    vlayout = new QVBoxLayout(this);
    vlayout->addWidget(GBox=new QGroupBox("请在下方输入ip地址",this),1);

    hlayout = new QHBoxLayout();
    GBox->setLayout(hlayout);
    hlayout->addWidget(IpEdit=new QLineEdit(this), 3);
    hlayout->addStretch(4);
    hlayout->addWidget(AddButton=new QPushButton("添加",this),1);
    hlayout->addWidget(DelButton=new QPushButton("刪除",this),1);

    list = new QListWidget(this);
    list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    hlayout1 = new QHBoxLayout();
    vlayout->addLayout(hlayout1,9);
    hlayout1->addWidget(list,1);
    vlayout1 = new QVBoxLayout();
    hlayout1->addLayout(vlayout1,2);

    vlayout1->addWidget(textread=new QTextEdit(this),6);
    vlayout1->addWidget(textwrite=new QTextEdit(this),3);
    textread->setReadOnly(true);
    textread->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textread->document()->setMaximumBlockCount(Maxblockcount);
    textwrite->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textwrite->document()->setMaximumBlockCount(Maxblockcount);
    hlayout2 = new QHBoxLayout();
    vlayout1->addLayout(hlayout2,1);
    hlayout2->addStretch(8);
    hlayout2->addWidget(send=new QPushButton("发送"), 2);
    textread->setVisible(visible);
    textwrite->setVisible(visible);
    send->setVisible(visible);

    connect(AddButton, &QPushButton::clicked, [this](){Connect(IpEdit);});
    connect(DelButton, &QPushButton::clicked, [this](){Delete();});
    connect(list, &QListWidget::itemDoubleClicked, [this](){ShowDialog();});
    connect(send, &QPushButton::clicked, [this](){SendMessage();});
    connect(this, SIGNAL(ShowRecvMessage(char *)), this, SLOT(slot_ShowRecvMessage(char *)), Qt::QueuedConnection);
    connect(this, SIGNAL(RemoteClose(int)), this, SLOT(slot_RemoteClose(int)), Qt::QueuedConnection);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::init()
{
    epfd = epoll_create(EPOLL_SIZE);
    if(epfd<0)
    {
        qDebug("Create epoll failed!");
        return;
    }

    s = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    int reuse =1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))<0)
    {
        qDebug("setreuse failed!");
        return;
    }
    if(bind(s, (struct sockaddr* ) &servaddr, sizeof(servaddr))==-1)
    {
        qDebug("bind failed!");
        return;
    }
    if(listen(s, 20) == -1)
    {
        qDebug("listen failed!");
        return;
    }

    Addfd(epfd, s, true);
}

void Widget::Addfd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if( enable_et )
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)| O_NONBLOCK);
    qDebug("fd=%d added to epoll!",fd);
}

void Widget::wait()
{
    while(1)
    {
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count<0)
        {
            qDebug("event count failed!\n");
            break;
        }
        for(int i = 0; i < epoll_events_count; ++i)
        {
            bzero(&message, BUF_SIZE);
            int sockfd = events[i].data.fd;
            if(sockfd==s)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept(s, (struct sockaddr* )&client_address, &client_addrLength);
                QString clientip = QString(QLatin1String(inet_ntoa(client_address.sin_addr)));
                Addfd(epfd, clientfd, false);

                connectmap.insert(clientfd, clientip);
                list->addItem(new QListWidgetItem(QIcon("./icon/boy.png"), clientip+":"+QString::number(clientfd)));
            }
            else if(connectmap.find(sockfd)!=connectmap.end())
            {
                int ret = recv(sockfd, message, BUF_SIZE, 0);
                if(ret==0)
                {
                    qDebug("remote connect close!");
                    emit RemoteClose(sockfd);
                }
                else
                {
                    if(sockfd==chatfd)
                        emit ShowRecvMessage(message);
                    else
                    {
                        for(int i=0; i<list->count(); i++)
                        {
                            int unreadfd = list->item(i)->text().split(":")[1].toInt();
                            if(unreadfd == sockfd)
                                list->item(i)->setTextColor(Qt::red);
                        }
                        if(chatlog.find(sockfd)==chatlog.end())
                        {
                            QStringList templog;
                            templog.append(QString(message));
                            chatlog.insert(sockfd, templog);
                        }
                        else
                            chatlog.find(sockfd).value().append(QString(message));
                        if(isunread.find(sockfd)==isunread.end())
                        {
                            bool tempflag = true;
                            isunread.insert(sockfd, tempflag);
                        }
                        else
                            isunread.find(sockfd).value() = true;
                    }
                }
            }
        }
    }
}

void Widget::respond(QString ip)
{
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0)
    {
        qDebug("create socket failed!\n");
        return ;
    }
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0)|O_NONBLOCK);
    if(::connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr))<0)
    {
        if(errno!=EINPROGRESS)
        {
            qDebug("connect failed!\n");
            ::close(sock);
            return;
        }
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(sock,&wset);
        int ret = select(sock+1,NULL,&wset,NULL,&tv);
        if(ret<0)
        {
            qDebug("select failed!\n");
            ::close(sock);
            return;
        }
        else if(ret==0)
        {
            qDebug("connect time out!\n");
            ::close(sock);
            return;
        }
        else
        {
            socklen_t length = sizeof(int);
            int error = 0;
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &length);
            if(error!=0)
            {
                if(error==ECONNREFUSED)
                    qDebug("connect refused!\n");
                else
                    qDebug("get connect socket error is %d\n",error);
                ::close(sock);
                return;
            }
            else
            {
                Addfd(epfd, sock, true);
                connectmap.insert(sock, ip);
                list->addItem(new QListWidgetItem(QIcon("./icon/boy.png"), ip+":"+QString::number(sock)));
            }
        }
    }
    else
    {
        Addfd(epfd, sock, true);
        connectmap.insert(sock, ip);
        list->addItem(new QListWidgetItem(QIcon("./icon/boy.png"), ip+":"+QString::number(sock)));
    }
}

void Widget::start()
{
    std::thread t1(&Widget::wait, this);
    t1.detach();
}

void Widget::Connect(QLineEdit *iptext)
{
    QString ip = iptext->text();
    QStringList iplist = ip.split(".");
    if(iplist.size()!=4)
    {
        QMessageBox::information(this, "Error", "ip地址格式不正确，请重新输入! " );
        return;
    }

    bool ok1, ok2, ok3, ok4, ok5, ok6, ok7, ok8;
    int a1 = iplist[0].toInt(&ok1);
    int a2 = iplist[1].toInt(&ok2);
    int a3 = iplist[2].toInt(&ok3);
    int a4 = iplist[3].toInt(&ok4);
    ok5 = (a1>=0&&a1<=255)?true:false;
    ok6 = (a2>=0&&a2<=255)?true:false;
    ok7 = (a3>=0&&a3<=255)?true:false;
    ok8 = (a4>=0&&a4<=255)?true:false;

    if(ok1&&ok2&&ok3&&ok4)
    {
        if(!(ok5&&ok6&&ok7&&ok8))
        {
            QMessageBox::information(this, "Error", "范围为0～255，请重新输入！");
            return;
        }
        QString regroup_ip = QString::number(a1)+QString(".")+QString::number(a2)+QString(".")+QString::number(a3)+QString(".")+QString::number(a4);
        qDebug()<<regroup_ip;

        for(int i=0; i<list->count(); i++)
        {
            QString ip = list->item(i)->text().split(":")[0];
            if(ip==regroup_ip)
            {
                QMessageBox::information(this, "Error", "该ip地址已添加，请输入其它ip地址！" );
                return;
            }
        }

        std::thread t2(&Widget::respond, this, regroup_ip);
        t2.detach();
    }
    else
    {
        QMessageBox::information(this, "Error", "ip地址格式不正确，请重新输入！" );
        return;
    }
}

void Widget::SendMessage()
{
    std::string context = textwrite->toPlainText().toStdString();
    int bfsize = context.size();
    const char* ch = context.c_str();
    if(bfsize>BUF_SIZE)
    {
        qDebug()<<"send size exceed buff size!\n";
        return;
    }
    bzero(&message, BUF_SIZE);
    strcpy(message, ch);
    int sendnum = ::send(chatfd, message, bfsize, 0);
    if(sendnum == bfsize)
    {
        textread->setTextColor(Qt::green);
        textread->append(QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz ddd"));
        textread->setAlignment(Qt::AlignRight);
        textread->append(textwrite->toPlainText());
        textread->setAlignment(Qt::AlignRight);
    }
    else
        qDebug()<<"send num error!\n";
    textwrite->clear();
}

void Widget::ShowDialog()
{
    if(visible)
    {
        textread->clear();
        textwrite->clear();
    }
    else
    {
        visible = true;
        textread->setVisible(visible);
        textwrite->setVisible(visible);
        send->setVisible(visible);
    }
    chatfd = list->currentItem()->text().split(":")[1].toInt();
    if(isunread.find(chatfd)!=isunread.end())
    {
        if(isunread.find(chatfd).value()==true)
        {
            if(chatlog.find(chatfd)!=chatlog.end())
            {
                QStringList logs = chatlog.find(chatfd).value();
                textread->setTextColor(Qt::red);
                textread->append(QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz ddd"));
                textread->setAlignment(Qt::AlignLeft);
                for(int i=0; i<logs.size(); i++)
                {
                    QString log = logs[i];
                    textread->setTextColor(Qt::red);
                    textread->append(log);
                    textread->setAlignment(Qt::AlignLeft);
                }
                chatlog.find(chatfd).value().clear();
            }
            isunread.find(chatfd).value() = false;
            for(int i=0; i<list->count(); i++)
            {
                int unreadfd = list->item(i)->text().split(":")[1].toInt();
                if(unreadfd == chatfd)
                    list->item(i)->setTextColor(Qt::black);
            }
        }
    }
}

void Widget::slot_ShowRecvMessage(char *news)
{
    textread->setTextColor(Qt::red);
    textread->append(QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz ddd"));
    textread->setAlignment(Qt::AlignLeft);
    textread->setTextColor(Qt::red);
    textread->append(QString(news));
    textread->setAlignment(Qt::AlignLeft);
}

void Widget::Delete()
{
    QListWidgetItem *item = list->currentItem();
    if(item==NULL)
    {
        QMessageBox::information(this, "Error", "请先选择要删除的对象！" );
        return;
    }
    int deletesocket = item->text().split(":")[1].toInt();
    ::close(deletesocket);
    if(connectmap.find(deletesocket)!=connectmap.end())
        connectmap.erase(connectmap.find(deletesocket));
    if(chatlog.find(deletesocket)!=chatlog.end())
        chatlog.erase(chatlog.find(deletesocket));
    if(isunread.find(deletesocket)!=isunread.end())
        isunread.erase(isunread.find(deletesocket));
    delete item;
    if(deletesocket == chatfd)
    {
        visible = false;
        textread->clear();
        textwrite->clear();
        textread->setVisible(visible);
        textwrite->setVisible(visible);
        send->setVisible(visible);
    }
}

void Widget::slot_RemoteClose(int closefd)
{
    ::close(closefd);
    connectmap.erase(connectmap.find(closefd));
    if(chatlog.find(closefd)!=chatlog.end())
        chatlog.erase(chatlog.find(closefd));
    if(isunread.find(closefd)!=isunread.end())
        isunread.erase(isunread.find(closefd));
    for(int i=0; i<list->count(); i++)
    {
        int fd = list->item(i)->text().split(":")[1].toInt();
        if(fd == closefd)
        {
            delete list->item(i);
            if(closefd==chatfd)
            {
                visible = false;
                textread->clear();
                textwrite->clear();
                textread->setVisible(visible);
                textwrite->setVisible(visible);
                send->setVisible(visible);
            }
        }
    }
}
