#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtNetwork>
 #include <QDataStream>
 #include <QTextStream>
#include  "mTcpServer.h"

class QUdpSocket;

namespace Ui {
  class mNetAssistWidget;
}

class mNetAssistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit mNetAssistWidget(QWidget *parent = 0);
    ~mNetAssistWidget();
    void setUdpGuiExt();
    void setTcpSvrGuiExt();
    void setTcpClientGuiExt();

    QList<int> tcpClientSocketDescriptorList;
    int TcpClientLinkCnt;

signals:
    void sendDataToClient(char *msg,int length,int socketDescriptor,int socketDescriptorEx);

private slots:
    void on_pBtnNetCnnt_clicked(bool checked);
    void on_cBoxNetType_currentIndexChanged(int index);

    void on_pBtnSendData_clicked();
    void on_pBtnClearRcvDisp_clicked();
    void on_lEditUdpPort_textChanged(QString text);
    void on_lEditUdpIP_textChanged(QString text);
    void on_pBtnResetCnt_clicked();
    char ConvertHexChar(char ch);
    char ConvertHexStr(QString hexSubStr);

    //=======UDP========
    void udpDataReceived();

    //=====TCP Client=====
    void tcpClientDataReceived();

    //=====TCP Server=====
    bool slotTryCreateTcpServer();
    void slotDeleteTcpServer();
    void tcpServerDataReceived(char* msg,int length,int socketDescriptorEx);

    void addClientLink(QString clientAddrPort,int socketDescriptor);
    void removeClientLink(QString clientAddrPort,int socketDescriptor);
    void toSendData();
    void toSendFile();
    void insertDateTimeInRcvDisp();

   void msDelay(unsigned int msec);

    void on_pBtnSaveRcvData_clicked();
    void on_cBoxLoopSnd_toggled(bool checked);
    void on_lEdit_Interval_ms_editingFinished();

    void on_pBtnClearSndDisp_clicked();

    void on_pBtnLoadSndData_clicked();

    void on_StartRcvFile_clicked(bool checked);

    void on_cBoxStartSndFile_clicked(bool checked);

    void on_cBox_SndHexDisp_clicked(bool checked);

    // 历史发送功能相关槽函数
    void on_cBoxSendHistory_currentTextChanged(const QString &text);
    void on_pBtnClearHistory_clicked();
    void addToSendHistory(const QString &data);
    void clearSendHistory();
    
    // 连接历史功能相关槽函数
    void on_cBoxIpHistory_currentTextChanged(const QString &text);
    void on_cBoxPortHistory_currentTextChanged(const QString &text);
    void addToConnectionHistory(const QString &ip, const QString &port);
    void saveConnectionHistory();
    void loadConnectionHistory();
    void saveSendHistory();
    void loadSendHistory();
    
    // 日志功能相关方法
    void addToLog(const QString &data, const QString &direction);
    void logSentData(const QString &data);
    void logReceivedData(const QString &data);
    void logReceivedData(const QString &data, const QString &source);

private:
    Ui::mNetAssistWidget *ui;

    QString m_ip;
    //-----UDP-----
    QUdpSocket *udpSocket;
    QHostAddress lhAddr;
    int lhPort;
    QHostAddress rmtAddr;
    int rmtPort;

    //-----TCP Client-----
    QHostAddress *rmtServerIP;
    QTcpSocket *tcpClientSocket;
    mTcpServer *mtcpServer;

    //Global state
    unsigned int rcvDataCnt;
    unsigned int sndDataCnt;
    //bool NetState;
    QTimer *timer;
    bool loopSending;

    QString CurIPPort;

    QString CurPath;
    QFile *curFile;
    
    // 历史发送记录相关
    QStringList sendHistory;
    int maxHistorySize;
    
    // 连接历史记录相关
    QStringList ipHistory;
    QStringList portHistory;
    QStringList serverIpHistory; // TCP客户端模式的服务器IP历史
    int maxConnectionHistorySize;
    QString historyFilePath;
    int lastProtocolType; // 最后使用的协议类型
    
    // 日志显示相关
    bool showLogHeader;    // 是否显示日志头部
    bool showSentData;     // 是否显示发送数据
    bool showReceivedData; // 是否显示接收数据

    bool isSelectingHistory;
};

#endif // WIDGET_H
