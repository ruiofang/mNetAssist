#include "mNetAssistWidget.h"
#include "ui_mNetAssistWidget.h"
#include "mdefine.h"
#include <QMessageBox>
#include <stdlib.h>
#include <QFileDialog>
#include <QFile>
#include <QString>
#include <QTimer>
#include <QInputDialog>
#include <QDateTime>
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QSettings>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QColor>

const char toHex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

mNetAssistWidget::mNetAssistWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mNetAssistWidget)
{
    ui->setupUi(this);

    // 设置窗口大小为屏幕的85%
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int width = static_cast<int>(screenGeometry.width() * 0.80);
        int height = static_cast<int>(screenGeometry.height() * 0.80);
        resize(width, height);
        
        // 居中显示窗口
        int x = (screenGeometry.width() - width) / 2;
        int y = (screenGeometry.height() - height) / 2;
        move(x, y);
    }

    //设置程序的开启默认画面
    setUdpGuiExt();

    //获取本机的IP地址,并初始化相应的控件属性和变量
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
       if (ipAddressesList.at(i) != QHostAddress::LocalHost && ipAddressesList.at(i).toIPv4Address()) {
           m_ip = ipAddressesList.at(i).toString();
           break;
       }
    }
     // 设置获取的IP到相应文本框
    ui->lEditUdpIP->setText(m_ip);
    ui->SndProgressBar->setVisible(false);

   //初始化全局变量
    rmtServerIP =new QHostAddress();
     rcvDataCnt  = 0;
     sndDataCnt = 0;
     TcpClientLinkCnt = 0;
     //NetState = false;
     loopSending = false;
     CurIPPort = "";
     CurPath = "";
     curFile = 0;
     
     // 初始化历史发送功能
     maxHistorySize = 20; // 最多保存20条历史记录
     sendHistory.clear();
     
     // 初始化连接历史功能
     maxConnectionHistorySize = 15; // 最多保存15条连接历史
     // 设置配置文件路径为程序同目录
     historyFilePath = QApplication::applicationDirPath() + "/mNetAssist_history.ini";
     ipHistory.clear();
     portHistory.clear();
     serverIpHistory.clear(); // TCP客户端服务器IP历史
     lastProtocolType = 0; // 默认UDP协议
     
     // 初始化日志显示相关设置
     showLogHeader = true;    // 默认显示日志头部
     showSentData = true;     // 默认显示发送数据
     showReceivedData = true; // 默认显示接收数据
     
     // 连接历史记录信号槽
     connect(ui->cBoxSendHistory, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
             this, &mNetAssistWidget::on_cBoxSendHistory_currentTextChanged);
     connect(ui->cBoxIpHistory, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
             this, &mNetAssistWidget::on_cBoxIpHistory_currentTextChanged);
     connect(ui->cBoxPortHistory, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
             this, &mNetAssistWidget::on_cBoxPortHistory_currentTextChanged);
             
     // 加载历史记录
     loadConnectionHistory();
     loadSendHistory();
     
     // 如果当前IP为空且不是TCP客户端模式，则使用本机IP作为默认值
     if (ui->lEditIpAddr->text().isEmpty() && lastProtocolType != 2) {
         ui->lEditIpAddr->setText(m_ip);
     }
}

mNetAssistWidget::~mNetAssistWidget()
{
    // 保存连接历史记录
    saveConnectionHistory();
    saveSendHistory();
    delete ui;
}

/**********************************************************/
//Function for connection button
void mNetAssistWidget::on_pBtnNetCnnt_clicked(bool checked)
{
    if(checked){      //切换到链接状态
        if(ui->cBoxNetType->currentIndex() == UDP_MODE){
            //建立UDP链接
            udpSocket = new QUdpSocket(this);
             connect(udpSocket,SIGNAL(readyRead()),this,SLOT(udpDataReceived()));
            lhAddr.setAddress(ui->lEditIpAddr->text());
            lhPort = ui->lEditIpPort->text().toInt();
            rmtAddr.setAddress(ui->lEditUdpIP->text());
            rmtPort = ui->lEditUdpPort->text().toInt();
            bool result=udpSocket->bind(lhPort);
            if(!result)
            {
                ui->pBtnNetCnnt->setChecked(0);
                QMessageBox::information(this,tr("错误"),tr("UDP绑定端口失败!"));
                return;
            }
            ui->CurState->setText(tr("建立UDP连接成功"));
            // 保存连接历史
            addToConnectionHistory(ui->lEditIpAddr->text(), ui->lEditIpPort->text());

        }else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){
             //建立TCP服务器链接
            lhAddr.setAddress(ui->lEditIpAddr->text());
            lhPort = ui->lEditIpPort->text().toInt();

            if(! slotTryCreateTcpServer())
            {
                ui->pBtnNetCnnt->setChecked(0);
                QMessageBox::information(this,tr("错误"),tr("尝试建立服务器失败! 请确认网络状态和端口。"));
                return;
            }
            ui->CurState->setText(tr("建立TCP服务器成功"));
            // 保存连接历史
            addToConnectionHistory(ui->lEditIpAddr->text(), ui->lEditIpPort->text());
        }else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE){
            //建立TCP客户端
            QString ip = ui->lEditIpAddr->text();
            if(!rmtServerIP->setAddress(ip))
            {
                QMessageBox::information(this,tr("错误"),tr("TCP服务器IP设置失败!"));
                return;
            }
            tcpClientSocket = new QTcpSocket(this);
            connect(tcpClientSocket,SIGNAL(readyRead()),this,SLOT(tcpClientDataReceived()));
            tcpClientSocket->connectToHost(*rmtServerIP,ui->lEditIpPort->text().toInt());

            if( !tcpClientSocket->waitForConnected(2000)){
                ui->lEditUdpPort->setText(QString::number(0,10));
                ui->pBtnNetCnnt->setChecked(0);
                QMessageBox::information(this,tr("错误"),tr("尝试连接服务器失败! 请确认服务器状态。"));

                return;
            }

            ui->lEditUdpPort->setText(QString::number(tcpClientSocket->localPort(),10));
            ui->CurState->setText(tr("连接TCP服务器成功"));
            // 保存连接历史
            addToConnectionHistory(ui->lEditIpAddr->text(), ui->lEditIpPort->text());
        }
        ui->pBtnNetCnnt->setText(tr(" 断开网络"));
        ui->pBtnSendData->setEnabled(true);
        //NetState = true;
    }else{ //切换到断开状态
        if(ui->cBoxNetType->currentIndex() == UDP_MODE){
            //断开UDP链接
            udpSocket->close();
            delete udpSocket;
        }else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){
            //断开TCP服务器链接
            slotDeleteTcpServer();
        }else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE){
            //断开TCP客户端链接
           tcpClientSocket->disconnectFromHost();
        }

        ui->pBtnNetCnnt->setText(tr(" 连接网络"));
        ui->pBtnSendData->setEnabled(false);
        ui->CurState->setText(tr(""));
       // NetState = false;
    }
}

/**********************************************************/
//funcions switch
void mNetAssistWidget::on_cBoxNetType_currentIndexChanged(int index)
{
    if(index == UDP_MODE){
        setUdpGuiExt();
        ui->label_Port->setText(tr("本地端口"));
        ui->label_IP->setText(tr("本地IP地址"));
        ui->labelUdp->setText(tr("目标IP地址"));
        ui->labelUdp1->setText(tr("目标端口"));
        ui->lEditIpAddr->setText(m_ip);
    }else if(index == TCP_SERVER_MODE){
        setTcpSvrGuiExt();
       // setTcpClientGuiExt();
        ui->label_Port->setText(tr("本地端口"));
        ui->label_IP->setText(tr("本地IP地址"));
        ui->lEditIpAddr->setText(m_ip);
    }else if(index == TCP_CLIENT_MODE){
        setTcpClientGuiExt();
        ui->label_Port->setText(tr("服务器端口"));
        ui->label_IP->setText(tr("服务器IP地址"));
        ui->labelUdp->setText(tr("本地IP地址"));
        ui->labelUdp1->setText(tr("本地端口"));
        // 使用历史服务器IP，如果没有则使用127.0.0.1
        if (!serverIpHistory.isEmpty()) {
            ui->lEditIpAddr->setText(serverIpHistory.first());
        } else {
            ui->lEditIpAddr->setText(tr("127.0.0.1"));
        }
    }
}

/**********************************************************/
//set UDP mode
void mNetAssistWidget::setUdpGuiExt()
{
    ui->labelSpaceUdp->setVisible(true);
    ui->lEditUdpIP->setVisible(true);
    ui->labelUdp->setVisible(true);
    ui->labelUdp1->setVisible(true);
    ui->lEditUdpPort->setVisible(true);

    ui->labelClients->setVisible(false);
    ui->cBoxClients->setVisible(false);
    ui->labelSpaceClients->setVisible(false);
    ui->cBox_chatMode->setVisible(false);
    ui->cBox_echoMode->setVisible(false);
}

/**********************************************************/
//set TCP server
void mNetAssistWidget::setTcpSvrGuiExt()
{
    ui->labelSpaceUdp->setVisible(false);
    ui->lEditUdpIP->setVisible(false);
    ui->labelUdp->setVisible(false);
    ui->labelUdp1->setVisible(false);
    ui->lEditUdpPort->setVisible(false);

    ui->labelClients->setVisible(true);
    ui->cBoxClients->setVisible(true);
    ui->labelSpaceClients->setVisible(true);
    ui->cBox_chatMode->setVisible(true);
    ui->cBox_echoMode->setVisible(true);
}

/**********************************************************/
void mNetAssistWidget::setTcpClientGuiExt()
{
    ui->labelSpaceUdp->setVisible(true);
    ui->lEditUdpIP->setVisible(true);
    ui->labelUdp->setVisible(true);
    ui->labelUdp1->setVisible(true);
    ui->lEditUdpPort->setVisible(true);

    ui->labelClients->setVisible(false);
    ui->cBoxClients->setVisible(false);
    ui->labelSpaceClients->setVisible(false);
    ui->cBox_chatMode->setVisible(false);
    ui->cBox_echoMode->setVisible(false);
}

/**********************************************************/
//send data
void mNetAssistWidget::on_pBtnSendData_clicked()
{
    if(ui->cBoxStartSndFile->checkState()){
        on_pBtnResetCnt_clicked();
        ui->pBtnSendData->setText(tr("正在发送"));
        ui->pBtnSendData->setEnabled(false);
        insertDateTimeInRcvDisp();
        ui->ReceiveTextEdit->appendPlainText(tr("开始发送..."));
        toSendFile();
        insertDateTimeInRcvDisp();
        ui->ReceiveTextEdit->appendPlainText(tr("发送完成！"));
        float hasSnd = sndDataCnt;
        hasSnd = hasSnd/1024/1024;
        QString hasSndSz = QString("%1").arg(hasSnd);
        ui->ReceiveTextEdit->appendPlainText(tr("共发送数据：")+hasSndSz+"MB");

        ui->pBtnSendData->setText(tr("发送"));
        ui->pBtnSendData->setEnabled(true);
        return;
    }

    if(ui->tEditSendText->toPlainText().size()==0) {
        QMessageBox::information(this,tr("提示"),tr("发送区为空，请输入内容。"));
        return;  //如果发送区为空则直接跳出
    }

    if(ui->cBoxLoopSnd->checkState())
    {
        if( !loopSending){
            ui->pBtnSendData->setText(tr("停止发送"));
            timer->start();
            loopSending=true;
        }else{
            timer->stop();
            ui->pBtnSendData->setText(tr("发送"));
            loopSending=false;
        }
    }else{
        // 保存到历史记录
        QString sendData = ui->tEditSendText->toPlainText();
        if (!sendData.isEmpty()) {
            addToSendHistory(sendData);
        }
        
        toSendData();
        if(ui->cBox_AntoClearSnd->checkState()){
            ui->tEditSendText->clear();
        }
    }
}

/**********************************************************/
//send data
void mNetAssistWidget::toSendData()
{
    QByteArray datagram;

    if(ui->cBox_SndHexDisp->checkState()){
        QStringList hexStr = ui->tEditSendText->toPlainText().split(" ",QString::SkipEmptyParts);
        int hexSize = hexStr.size();
        for(int i=0;i<hexSize;i++){
            QString hexSubStr = hexStr.at(i);
            datagram.append(ConvertHexStr(hexSubStr));
        }
        datagram.resize(hexSize);
    }else{
        datagram = ui->tEditSendText->toPlainText().toLocal8Bit();
    }

    if(datagram.size()==0)   return;

    // 记录发送数据到日志
    QString logData;
    if(ui->cBox_SndHexDisp->checkState()){
        // 十六进制模式显示
        logData = ui->tEditSendText->toPlainText();
    } else {
        // 文本模式显示
        logData = QString::fromLocal8Bit(datagram);
    }
    logSentData(logData);

    if(ui->cBoxNetType->currentIndex() == UDP_MODE){
        udpSocket->writeDatagram(datagram.data(), datagram.size(),rmtAddr, rmtPort);
    }else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){
        int idx = ui->cBoxClients->currentIndex() ;
        if(idx == 0){
             emit sendDataToClient((char *)datagram.data(), datagram.size(),0,0);
        }else{
             emit sendDataToClient((char *)datagram.data(), datagram.size(),tcpClientSocketDescriptorList.at(idx),0);
        }
    }else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE){
        tcpClientSocket->write(datagram.data(), datagram.size());
    }

    sndDataCnt+=datagram.size();
    ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
}

/**********************************************************/
void mNetAssistWidget::toSendFile()
{
    if(curFile==0)   return;
    char buf[1024];
    int rdLen=0;
   ui->SndProgressBar->setMaximum(curFile->bytesAvailable());
   ui->SndProgressBar->setVisible(true);
   ui->SndProgressBar->setValue(0);
    if(ui->cBoxNetType->currentIndex() == UDP_MODE){    //UDP 模式

        while(!curFile->atEnd())
        {              
              rdLen = curFile->read(buf,1024);
              udpSocket->writeDatagram(buf, rdLen,rmtAddr, rmtPort);
              sndDataCnt+=rdLen;
              ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
              ui->SndProgressBar->setValue(ui->SndProgressBar->value()+rdLen);
              msDelay(1);
        }
    }else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){ //TCP服务器模式
        int idx = ui->cBoxClients->currentIndex() ;
        if(idx == 0){
            while(!curFile->atEnd())
            {
                  //msDelay(2);
                  rdLen = curFile->read(buf,1024);
                  emit sendDataToClient(buf, rdLen,0,0);

                  sndDataCnt+=rdLen;
                  ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
                  ui->SndProgressBar->setValue(ui->SndProgressBar->value()+rdLen);
                  QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            }
        }else{
            while(!curFile->atEnd())
            {
                 // msDelay(2);
                  rdLen = curFile->read(buf,1024);
                  emit sendDataToClient(buf, rdLen,tcpClientSocketDescriptorList.at(idx),0);
                  sndDataCnt+=rdLen;
                  ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
                  ui->SndProgressBar->setValue(ui->SndProgressBar->value()+rdLen);
                  QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            }
        }
    }else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE){
        while(!curFile->atEnd())
        {          
              rdLen = curFile->read(buf,1024);
              tcpClientSocket->write(buf, rdLen);
              //更新发送数据计数器
              sndDataCnt+=rdLen;
              ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
              ui->SndProgressBar->setValue(ui->SndProgressBar->value()+rdLen);
              QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
    }

    ui->SndProgressBar->setVisible(false);
}
/**********************************************************/
//udp data received
void mNetAssistWidget::udpDataReceived()
{
    QHostAddress address;
    quint16 port;
    QString tmpIPPort="";

    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(),datagram.size(),&address,&port);

        if(ui->StartRcvFile->checkState()){
            char *buf;
            buf = datagram.data();
             if( curFile!=0) {
                  curFile->write(buf,datagram.size());
             }
        }else{
             if(!ui->cBox_PauseShowRcv->checkState()){
                 tmpIPPort = address.toString()+":"+QString::number(port,10);
                 
                 // 记录接收数据到日志
                 QString rcvMsg;
                 if(!ui->cBox_RcvHexDisp->checkState()){              //字符串模式
                     rcvMsg = QString::fromUtf8(datagram,datagram.size());
                 }else{                                               //十六进制模式
                     for(int i=0;i<datagram.size();i++){
                         char ch = datagram.at(i);
                         QString tmpStr="";
                         tmpStr.append(toHex[(ch&0xf0)/16]);
                         tmpStr.append(toHex[ch&0x0f]);
                         tmpStr.append(" ");
                         rcvMsg.append(tmpStr);
                     }
                 }
                 
                 QString direction = QString("接收自 %1").arg(tmpIPPort);
                 logReceivedData(rcvMsg, direction);
            }
        }
        rcvDataCnt+=datagram.size();
        ui->lEdit_RcvCnt->setText(QString::number(rcvDataCnt,10));
    }
}

/**********************************************************/
//clear the receive editor
void mNetAssistWidget::on_pBtnClearRcvDisp_clicked()
{
    ui->ReceiveTextEdit->clear();
}

/**********************************************************/
//reset the conters
void mNetAssistWidget::on_pBtnResetCnt_clicked()
{
    sndDataCnt = 0;
    rcvDataCnt = 0;

    ui->lEdit_RcvCnt->setText(QString::number(0,10));
    ui->lEdit_SndCnt->setText(QString::number(0,10));
}

/**********************************************************/
//UDP port changed PRC
void mNetAssistWidget::on_lEditUdpPort_textChanged(QString text)
 {
    rmtPort = text.toInt();
 }

/**********************************************************/
//UDP IP  Adderess changed PRC
void mNetAssistWidget::on_lEditUdpIP_textChanged(QString text)
 {
     rmtAddr.setAddress(text);
 }

/**********************************************************/
// TCP Client data received
void mNetAssistWidget::tcpClientDataReceived()
{
    while(tcpClientSocket->bytesAvailable()>0)
    {
        QByteArray datagram;
        datagram.resize(tcpClientSocket->bytesAvailable());
        tcpClientSocket->read(datagram.data(),datagram.size());
        if(ui->StartRcvFile->checkState()){
            char *buf;
            buf = datagram.data();
             if( curFile!=0) {
                  curFile->write(buf,datagram.size());
             }
        }else{
        if(!ui->cBox_PauseShowRcv->checkState()){
            QString tmpIPPort = ui->lEditIpAddr->text()+":"+ui->lEditIpPort->text();
            
            // 记录接收数据到日志
            QString rcvMsg;
            if(!ui->cBox_RcvHexDisp->checkState()){            //字符串模式
                rcvMsg = QString::fromUtf8(datagram,datagram.size());
            }else{                                             //十六进制模式
                for(int i=0;i<datagram.size();i++){
                    char ch = datagram.at(i);
                    QString tmpStr="";
                    tmpStr.append(toHex[(ch&0xf0)/16]);
                    tmpStr.append(toHex[ch&0x0f]);
                    tmpStr.append(" ");
                    rcvMsg.append(tmpStr);
                }
            }
            
            QString direction = QString("接收自 %1").arg(tmpIPPort);
            logReceivedData(rcvMsg, direction);
        }
        }

         rcvDataCnt+=datagram.size();
         ui->lEdit_RcvCnt->setText(QString::number(rcvDataCnt,10));
    }
}

void mNetAssistWidget::insertDateTimeInRcvDisp()
{
      int year,month,day;
      QDateTime::currentDateTime().date().getDate(&year,&month,&day);
      QString date = QString::number(year,10)+"-"+QString::number(month,10)+"-"+QString::number(day,10);
      ui->ReceiveTextEdit->appendPlainText(tr("【")+date+tr(" ")+QDateTime::currentDateTime().time().toString()+tr("】"));
}

/**********************************************************/
bool mNetAssistWidget::slotTryCreateTcpServer()
{
    mtcpServer = new mTcpServer(this);

    if(! mtcpServer->listen(lhAddr,lhPort))
    {
        return false;
        QMessageBox::information(this,tr("错误"),tr("尝试建立服务器失败! 请确认网络状态和端口。"));
    }

    connect(mtcpServer,SIGNAL(updateTcpServer(char*,int,int)),this,SLOT(tcpServerDataReceived(char*,int,int)));
    connect(this,SIGNAL(sendDataToClient(char*,int,int,int)),mtcpServer,SLOT(sendDataToClient(char*,int,int,int)));
    connect(mtcpServer,SIGNAL(addClientLink(QString,int)),this,SLOT(addClientLink(QString,int)));
    connect(mtcpServer,SIGNAL(removeClientLink(QString,int)),this,SLOT(removeClientLink(QString,int)));

    return true;
}

/**********************************************************/
void mNetAssistWidget::slotDeleteTcpServer()
{
    //disconnect(mtcpServer,SIGNAL(updateTcpServer(char*,int,int)),this,SLOT(tcpServerDataReceived(char*,int,int)));
    mtcpServer->disconnect();
    mtcpServer->close();
    delete  mtcpServer;
}

/**********************************************************/
void mNetAssistWidget::tcpServerDataReceived(char *msg,int length,int socketDescriptorEx)
{
    if(ui->StartRcvFile->checkState()){     //保存接收数据到文件
         if(curFile!=0) {
                curFile->write(msg,length);
          }
    }else{                                                  //显示接收数据
        if(!ui->cBox_PauseShowRcv->checkState()){
            int idx = tcpClientSocketDescriptorList.indexOf(socketDescriptorEx);
            QString tmpIPPort = ui->cBoxClients->itemText(idx);
            
            // 记录接收数据到日志
            QString rcvMsg;
            if(!ui->cBox_RcvHexDisp->checkState()){  //字符串模式
                rcvMsg = QString::fromUtf8(msg, length);
             }else{                                                            //十六进制模式
                 for(int i=0;i<length;i++){
                     char ch =*(msg+i);
                     QString tmpStr="";
                     tmpStr.append(toHex[(ch&0xf0)/16]);
                     tmpStr.append(toHex[ch&0x0f]);
                     tmpStr.append(" ");
                     rcvMsg.append(tmpStr);
                 }
             }
             
             QString direction = QString("接收自 %1").arg(tmpIPPort);
             logReceivedData(rcvMsg, direction);
         }
    }
    //群聊功能控制，转发数据
    if(ui->cBox_chatMode->checkState()){
        if(ui->cBox_echoMode->checkState()){
            emit sendDataToClient(msg,length,0,0);
        }else{
            emit sendDataToClient(msg,length,0,socketDescriptorEx);
        }
    }

    //数据接收计数器更新
    rcvDataCnt+=length;
    ui->lEdit_RcvCnt->setText(QString::number(rcvDataCnt,10));
}

/**********************************************************/
void mNetAssistWidget::addClientLink(QString clientAddrPort,int socketDescriptor)
{  
    if(TcpClientLinkCnt == 0){
        tcpClientSocketDescriptorList.clear();
        tcpClientSocketDescriptorList.append(0);
        ui->cBoxClients->addItem(tr("全部连接"));
    }
    TcpClientLinkCnt++;
    tcpClientSocketDescriptorList.append(socketDescriptor);
    ui->cBoxClients->addItem(clientAddrPort);
}

/**********************************************************/
void mNetAssistWidget::removeClientLink(QString clientAddrPort,int socketDescriptor)
{
    if(socketDescriptor!=-1) return;
    if(TcpClientLinkCnt <= 1){
        tcpClientSocketDescriptorList.clear();
        ui->cBoxClients->clear();
    }else{
        TcpClientLinkCnt--;
        int idx = ui->cBoxClients->findText(clientAddrPort);
        ui->cBoxClients->removeItem(idx);
        tcpClientSocketDescriptorList.removeAt(idx);
    }
}

/***********************************************/
//保存接收区的文本文件的内容到文件
void mNetAssistWidget::on_pBtnSaveRcvData_clicked()
{
    QString path = QFileDialog::getSaveFileName(this,tr("保存接收区内容到文本文件"),tr(""),tr("文本文件(*.txt)"));

    QFile saveFile(path);
    if (saveFile.open(QFile::WriteOnly | QIODevice::Truncate)) {
         QTextStream out(&saveFile);
         QString  str = ui->ReceiveTextEdit->toPlainText();
         out << str;
    }
    saveFile.close();
}

/***********************************************/
//开启定时自动发送功能
void mNetAssistWidget::on_cBoxLoopSnd_toggled(bool checked)
{
    if(checked){
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(toSendData()));
        int msInterval = ui->lEdit_Interval_ms->text().toInt();
        if(msInterval>0){
           timer->setInterval(ui->lEdit_Interval_ms->text().toInt());
        }else{
            ui->cBoxLoopSnd->setChecked(false);
            delete timer;
       }
    }else{
        timer->stop();
        delete timer;
        ui->pBtnSendData->setEnabled(true);
    }
}

/***********************************************/
//修改发送间隔时间
void mNetAssistWidget::on_lEdit_Interval_ms_editingFinished()
{
    int msInterval = ui->lEdit_Interval_ms->text().toInt();
    if(msInterval>0){
       timer->setInterval(ui->lEdit_Interval_ms->text().toInt());
    }
}

/***********************************************/
//清空发送区
void mNetAssistWidget::on_pBtnClearSndDisp_clicked()
{
    ui->tEditSendText->clear();
}

/***********************************************/
//载入文本文件到发送区
void mNetAssistWidget::on_pBtnLoadSndData_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,tr("载入文本文件到发送区"));
    QFile file(path);
    QByteArray str;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
         QMessageBox::information(this,tr("错误"),tr("打开文件失败。"));
         return ;
     } else {
         while (!file.atEnd()) {
              str = file.readLine();
              ui->tEditSendText->insertPlainText(str);
         }
    }
    file.close();
}

/***********************************************/
//接收数据转存到文件
void mNetAssistWidget::on_StartRcvFile_clicked(bool checked)
{
      if(checked){
           QFileDialog *qfd=new QFileDialog(this);
           qfd->setViewMode(QFileDialog::List);
           qfd->setFileMode(QFileDialog::AnyFile);
           qfd->setWindowTitle(tr("建立接收文件"));
          // qfd->setFilter(tr("所有文件(*.*)"));         //qt4.8
           qfd->setNameFilter(tr("所有文件(*.*)"));  //qt5

           if (qfd->exec() == QDialog::Accepted )   //如果成功的执行
           {
                  QStringList slist = qfd->selectedFiles();
                  CurPath = slist[0];
                  curFile = new QFile(CurPath);

                  if (!curFile->open(QFile::WriteOnly | QIODevice::Truncate)) {
                      //打开文件失败
                      ui->StartRcvFile->setChecked(false);
                      return;
                  }
                  ui->ReceiveTextEdit->setPlainText(tr("接收数据保存到文件：\n")+CurPath+tr("\n"));
                  on_pBtnResetCnt_clicked();
             }else{
                  ui->StartRcvFile->setChecked(false);
                  return;
             }
      }else{
           ui->ReceiveTextEdit->clear();
           if(curFile) curFile->close();
           if(curFile) delete curFile;
      }
}

/***********************************************/
//发送数据源为文件
void mNetAssistWidget::on_cBoxStartSndFile_clicked(bool checked)
{
    if(checked){      
        QFileDialog *qfd=new QFileDialog(this);
        qfd->setViewMode(QFileDialog::List);
        qfd->setFileMode(QFileDialog::AnyFile);
        qfd->setWindowTitle(tr("选择发送文件"));
       // qfd->setFilter(tr("所有文件(*.*)"));            //qt4.8
        qfd->setNameFilter(tr("所有文件(*.*)"));   //qt5

         if (qfd->exec() == QDialog::Accepted )   //如果成功的执行
         {
                QStringList slist = qfd->selectedFiles();
                CurPath = slist[0];
                curFile = new QFile(CurPath);

                if (!curFile->open(QFile::ReadOnly | QIODevice::Truncate)) {
                         //打开文件失败
                    ui->cBoxStartSndFile->setChecked(false);
                    return;
                 }
                ui->ReceiveTextEdit->setPlainText(tr("从文件发送数据：\n")+CurPath+tr("\n"));
          }else{
               ui->cBoxStartSndFile->setChecked(false);
               return;
         }
    }else{
        ui->ReceiveTextEdit->clear();
        if(curFile) curFile->close();
        if(curFile) delete curFile;
    }
}

/***********************************************/
//发送区以十六进制与字符串间切换
void mNetAssistWidget::on_cBox_SndHexDisp_clicked(bool checked)
{
    QByteArray datagram;
    if(checked){
        if(ui->tEditSendText->toPlainText().length()!=0){
             datagram = ui->tEditSendText->toPlainText().toLocal8Bit();
             ui->tEditSendText->clear();
             for(int i=0;i<datagram.size();i++){
                 char ch = datagram.at(i);
                 QString tmpStr = QString::number(ch,16);
                 ui->tEditSendText->insertPlainText(tmpStr+" ");
             }
        }
    }else{
         if(ui->tEditSendText->toPlainText().length()!=0){
             QStringList hexStr = ui->tEditSendText->toPlainText().split(" ",QString::SkipEmptyParts);
             int hexSize = hexStr.size();
             qDebug()<<QString::number(hexSize,10);
             for(int i=0;i<hexSize;i++){
                 QString hexSubStr = hexStr.at(i);
                 datagram.append(ConvertHexStr(hexSubStr));
             }
             ui->tEditSendText->clear();
             QString msg = datagram.data();
             ui->tEditSendText->setPlainText(msg);
         }
    }
}

/***********************************************/
//转化十六进制中的字符到ASCII
char mNetAssistWidget::ConvertHexChar(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else return (-1);
}

/***********************************************/
//转化十六进制字符到ASCII字母
char mNetAssistWidget::ConvertHexStr(QString hexSubStr)
{
    char ch = 0;
    if(hexSubStr.length()==2){
      // ch =  ConvertHexChar(hexSubStr.at(0).toAscii())*16+ ConvertHexChar(hexSubStr.at(1).toAscii()); //qt4.8
        ch =  ConvertHexChar(hexSubStr.at(0).toLatin1())*16+ ConvertHexChar(hexSubStr.at(1).toLatin1());
    }else if(hexSubStr.length()==1){
       //ch =  ConvertHexChar(hexSubStr.at(0).toAscii());  //qt4.8
       ch =  ConvertHexChar(hexSubStr.at(0).toLatin1());
    }
    return ch;
}

/***********************************************/
//毫秒级延时
void mNetAssistWidget::msDelay(unsigned int msec)
{
     QTime dieTime = QTime::currentTime().addMSecs(msec);
     while( QTime::currentTime() < dieTime )
      QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

/**********************************************************/
// 历史发送功能实现
void mNetAssistWidget::addToSendHistory(const QString &data)
{
    // 去除前后空白字符
    QString trimmedData = data.trimmed();
    if (trimmedData.isEmpty()) {
        return;
    }
    
    // 检查是否已存在相同的历史记录，如果存在则移除旧的
    int existingIndex = sendHistory.indexOf(trimmedData);
    if (existingIndex != -1) {
        sendHistory.removeAt(existingIndex);
        ui->cBoxSendHistory->removeItem(existingIndex + 1); // +1 因为第一项是提示文本
    }
    
    // 添加到历史记录开头
    sendHistory.prepend(trimmedData);
    
    // 限制历史记录数量
    if (sendHistory.size() > maxHistorySize) {
        sendHistory.removeLast();
        ui->cBoxSendHistory->removeItem(ui->cBoxSendHistory->count() - 1);
    }
    
    // 更新下拉列表（添加到第二项，第一项是提示文本）
    QString displayText = trimmedData;
    if (displayText.length() > 50) {
        displayText = displayText.left(50) + "...";
    }
    ui->cBoxSendHistory->insertItem(1, displayText);
    
    // 重置选择到提示文本
    ui->cBoxSendHistory->setCurrentIndex(0);
    
    // 立即保存发送历史
    saveSendHistory();
}

void mNetAssistWidget::on_cBoxSendHistory_currentTextChanged(const QString &text)
{
    if (text == "选择历史发送内容..." || text.isEmpty()) {
        return; // 忽略提示文本
    }
    
    // 找到对应的完整历史记录
    int index = ui->cBoxSendHistory->currentIndex() - 1; // -1 因为第一项是提示文本
    if (index >= 0 && index < sendHistory.size()) {
        ui->tEditSendText->setPlainText(sendHistory.at(index));
    }
}

void mNetAssistWidget::on_pBtnClearHistory_clicked()
{
    clearSendHistory();
}

void mNetAssistWidget::clearSendHistory()
{
    sendHistory.clear();
    ui->cBoxSendHistory->clear();
    ui->cBoxSendHistory->addItem("选择历史发送内容...");
}

/**********************************************************/
// 连接历史功能实现
void mNetAssistWidget::addToConnectionHistory(const QString &ip, const QString &port)
{
    QString trimmedIp = ip.trimmed();
    QString trimmedPort = port.trimmed();
    
    if (trimmedIp.isEmpty() || trimmedPort.isEmpty()) {
        return;
    }
    
    // 保存当前协议类型
    lastProtocolType = ui->cBoxNetType->currentIndex();
    
    // 根据协议类型处理不同的IP历史
    if (lastProtocolType == 2) { // TCP客户端模式，保存服务器IP
        int existingIndex = serverIpHistory.indexOf(trimmedIp);
        if (existingIndex != -1) {
            serverIpHistory.removeAt(existingIndex);
        }
        serverIpHistory.prepend(trimmedIp);
        if (serverIpHistory.size() > maxConnectionHistorySize) {
            serverIpHistory.removeLast();
        }
    } else { // UDP和TCP服务器模式，保存本地IP
        int existingIpIndex = ipHistory.indexOf(trimmedIp);
        if (existingIpIndex != -1) {
            ipHistory.removeAt(existingIpIndex);
            ui->cBoxIpHistory->removeItem(existingIpIndex + 1);
        }
        ipHistory.prepend(trimmedIp);
        if (ipHistory.size() > maxConnectionHistorySize) {
            ipHistory.removeLast();
            ui->cBoxIpHistory->removeItem(ui->cBoxIpHistory->count() - 1);
        }
        ui->cBoxIpHistory->insertItem(1, trimmedIp);
        ui->cBoxIpHistory->setCurrentIndex(0);
    }
    
    // 处理端口历史
    int existingPortIndex = portHistory.indexOf(trimmedPort);
    if (existingPortIndex != -1) {
        portHistory.removeAt(existingPortIndex);
        ui->cBoxPortHistory->removeItem(existingPortIndex + 1);
    }
    portHistory.prepend(trimmedPort);
    if (portHistory.size() > maxConnectionHistorySize) {
        portHistory.removeLast();
        ui->cBoxPortHistory->removeItem(ui->cBoxPortHistory->count() - 1);
    }
    ui->cBoxPortHistory->insertItem(1, trimmedPort);
    ui->cBoxPortHistory->setCurrentIndex(0);
}

void mNetAssistWidget::on_cBoxIpHistory_currentTextChanged(const QString &text)
{
    if (text == "▼" || text.isEmpty()) {
        return;
    }
    ui->lEditIpAddr->setText(text);
}

void mNetAssistWidget::on_cBoxPortHistory_currentTextChanged(const QString &text)
{
    if (text == "▼" || text.isEmpty()) {
        return;
    }
    ui->lEditIpPort->setText(text);
}

void mNetAssistWidget::saveConnectionHistory()
{
    QSettings settings(historyFilePath, QSettings::IniFormat);
    settings.beginGroup("ConnectionHistory");
    
    // 保存IP历史
    settings.beginWriteArray("IpHistory");
    for (int i = 0; i < ipHistory.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("ip", ipHistory.at(i));
    }
    settings.endArray();
    
    // 保存端口历史
    settings.beginWriteArray("PortHistory");
    for (int i = 0; i < portHistory.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("port", portHistory.at(i));
    }
    settings.endArray();
    
    // 保存TCP客户端服务器IP历史
    settings.beginWriteArray("ServerIpHistory");
    for (int i = 0; i < serverIpHistory.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("serverIp", serverIpHistory.at(i));
    }
    settings.endArray();
    
    // 保存最后使用的协议类型
    settings.setValue("LastProtocolType", lastProtocolType);
    
    settings.endGroup();
}

void mNetAssistWidget::saveSendHistory()
{
    QSettings settings(historyFilePath, QSettings::IniFormat);
    settings.beginGroup("SendHistory");
    
    // 保存发送历史
    settings.beginWriteArray("SendHistory");
    for (int i = 0; i < sendHistory.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("data", sendHistory.at(i));
    }
    settings.endArray();
    
    settings.endGroup();
}

void mNetAssistWidget::loadConnectionHistory()
{
    QSettings settings(historyFilePath, QSettings::IniFormat);
    settings.beginGroup("ConnectionHistory");
    
    // 加载IP历史
    int ipSize = settings.beginReadArray("IpHistory");
    for (int i = 0; i < ipSize && i < maxConnectionHistorySize; ++i) {
        settings.setArrayIndex(i);
        QString ip = settings.value("ip").toString();
        if (!ip.isEmpty()) {
            ipHistory.append(ip);
            ui->cBoxIpHistory->addItem(ip);
        }
    }
    settings.endArray();
    
    // 加载端口历史
    int portSize = settings.beginReadArray("PortHistory");
    for (int i = 0; i < portSize && i < maxConnectionHistorySize; ++i) {
        settings.setArrayIndex(i);
        QString port = settings.value("port").toString();
        if (!port.isEmpty()) {
            portHistory.append(port);
            ui->cBoxPortHistory->addItem(port);
        }
    }
    settings.endArray();
    
    // 加载TCP客户端服务器IP历史
    int serverIpSize = settings.beginReadArray("ServerIpHistory");
    for (int i = 0; i < serverIpSize && i < maxConnectionHistorySize; ++i) {
        settings.setArrayIndex(i);
        QString serverIp = settings.value("serverIp").toString();
        if (!serverIp.isEmpty()) {
            serverIpHistory.append(serverIp);
        }
    }
    settings.endArray();
    
    // 加载最后使用的协议类型
    lastProtocolType = settings.value("LastProtocolType", 0).toInt();
    
    settings.endGroup();
    
    // 设置最后使用的协议类型为默认值
    ui->cBoxNetType->setCurrentIndex(lastProtocolType);
    
    // 根据协议类型设置最后使用的IP
    if (lastProtocolType == 2) { // TCP客户端模式
        if (!serverIpHistory.isEmpty()) {
            ui->lEditIpAddr->setText(serverIpHistory.first());
        }
    } else { // UDP和TCP服务器模式
        if (!ipHistory.isEmpty()) {
            ui->lEditIpAddr->setText(ipHistory.first());
        }
    }
    
    // 设置最后使用的端口
    if (!portHistory.isEmpty()) {
        ui->lEditIpPort->setText(portHistory.first());
    }
}

void mNetAssistWidget::loadSendHistory()
{
    QSettings settings(historyFilePath, QSettings::IniFormat);
    settings.beginGroup("SendHistory");
    
    // 加载发送历史
    int sendSize = settings.beginReadArray("SendHistory");
    for (int i = 0; i < sendSize && i < maxHistorySize; ++i) {
        settings.setArrayIndex(i);
        QString data = settings.value("data").toString();
        if (!data.isEmpty()) {
            sendHistory.append(data);
            
            // 更新下拉列表
            QString displayText = data;
            if (displayText.length() > 50) {
                displayText = displayText.left(50) + "...";
            }
            ui->cBoxSendHistory->addItem(displayText);
        }
    }
    settings.endArray();
    
    settings.endGroup();
}

/**********************************************************/
// 日志功能实现
void mNetAssistWidget::addToLog(const QString &data, const QString &direction)
{
    if (data.isEmpty()) {
        return;
    }
    
    // 更新头部显示状态
    showLogHeader = ui->cBox_ShowLogHeader->isChecked();
    
    QString logEntry;
    
    if (showLogHeader) {
        // 获取当前时间
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        
        // 构建完整日志条目
        logEntry = QString("[%1] %2: %3")
                          .arg(timestamp)
                          .arg(direction)
                          .arg(data);
    } else {
        // 只显示数据内容
        logEntry = data;
    }
    
    // 添加到日志显示区
    if (ui->ReceiveTextEdit->toPlainText().size() != 0) {
        ui->ReceiveTextEdit->appendPlainText("");  // 添加空行分隔
    }
    
    // 设置文本颜色
    QTextCursor cursor = ui->ReceiveTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    
    QTextCharFormat format;
    if (direction.contains("发送到")) {
        format.setForeground(QColor(0, 128, 0)); // 绿色表示发送
    } else if (direction.contains("接收")) {
        format.setForeground(QColor(0, 0, 255)); // 蓝色表示接收
    } else {
        format.setForeground(QColor(0, 0, 0)); // 黑色默认
    }
    
    cursor.setCharFormat(format);
    cursor.insertText(logEntry + "\n");
    
    // 自动滚动到底部
    ui->ReceiveTextEdit->moveCursor(QTextCursor::End);
}

void mNetAssistWidget::logSentData(const QString &data)
{
    // 检查是否显示发送数据
    showSentData = ui->cBox_ShowSentData->isChecked();
    if (!showSentData) {
        return; // 如果不显示发送数据，直接返回
    }
    
    QString direction;
    int protocolType = ui->cBoxNetType->currentIndex();
    
    if (protocolType == 0) { // UDP
        QString target = ui->lEditUdpIP->text() + ":" + ui->lEditUdpPort->text();
        direction = QString("发送到 %1").arg(target);
    } else if (protocolType == 1) { // TCP服务器
        int clientIndex = ui->cBoxClients->currentIndex();
        if (clientIndex == 0) {
            direction = "发送到 全部客户端";
        } else {
            QString client = ui->cBoxClients->currentText();
            direction = QString("发送到 %1").arg(client);
        }
    } else if (protocolType == 2) { // TCP客户端
        QString server = ui->lEditIpAddr->text() + ":" + ui->lEditIpPort->text();
        direction = QString("发送到 %1").arg(server);
    }
    
    addToLog(data, direction);
}

void mNetAssistWidget::logReceivedData(const QString &data)
{
    logReceivedData(data, "接收");
}

void mNetAssistWidget::logReceivedData(const QString &data, const QString &source)
{
    // 检查是否显示接收数据
    showReceivedData = ui->cBox_ShowReceivedData->isChecked();
    if (!showReceivedData) {
        return; // 如果不显示接收数据，直接返回
    }
    
    addToLog(data, source);
}
