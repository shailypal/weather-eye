#include "tcpHandler.hpp"
#include <iostream>
#include <winsock2.h>
#include "dataFormat.h"

class channelHandle
{
public:
    channelHandle ()
    {
        recvSocket = INVALID_SOCKET;
        sendSocket = INVALID_SOCKET;
    }
    SOCKET recvSocket;
    SOCKET sendSocket;
};

class tcpClientPrvate
{
public:
    channelHandle *handle;
    msgQueueHandler *parentQ;
    threadHandler rxThread;

    int id;
    bool runMore;
};


//-----------------------------------------------------------------
tcpServer::tcpServer ()
{
}

tcpServer::~tcpServer ()
{
}

void tcpServer::setParentMsgQueue (msgQueueHandler *mq)
{
    parentQ = mq;
}

void tcpServer::start ()
{
    Slot<void, void*> fp;
    fp = new Callback <tcpServer, void, void*>(this, &tcpServer::connectHandler);
    connectThread.create (fp, this);
}

tcpClient* tcpServer::getClient (unsigned int id)
{
    if (clientList.find(id) != clientList.end ())
        return clientList[id];
    else
        return 0;
}

void tcpServer::deleteClient (unsigned int id)
{
    if (clientList.find(id) != clientList.end ())
    {
        tcpClient *c = clientList[id];
        clientList.erase (id);
        delete c;
    }
}

void tcpServer::connectHandler (void *p)
{
    if (static_cast<tcpServer*>(p) != this)
    {
        std::cout << "FATAL ERROR:: Pointer mismatch for TCP Connect handler" << std::endl;
        return;
    }

    SOCKET RxServer;
    SOCKET TxServer;
    SOCKADDR_IN local;
    WSADATA wsaData;
    SOCKADDR_IN from;
    int fromlen=sizeof(from);

    WSAStartup(MAKEWORD(2,2),&wsaData);

    local.sin_family=AF_INET; //Address family
    local.sin_addr.s_addr=INADDR_ANY; //Wild card IP address
    local.sin_port = htons((u_short)TCP_SERVER_SEND_PORT); //port to use
    TxServer = ::socket(AF_INET,SOCK_STREAM, 0);
    ::bind(TxServer,(struct sockaddr*)&local,sizeof(local));

    local.sin_family=AF_INET; //Address family
    local.sin_addr.s_addr= INADDR_ANY; // Specific IP of the client who connected to Tx socket
    local.sin_port = htons((u_short)TCP_SERVER_RECV_PORT); //port to use
    RxServer = ::socket(AF_INET,SOCK_STREAM, 0);
    ::bind(RxServer,(struct sockaddr*)&local,sizeof(local));

    unsigned int clientCount = 1;

    while (1)
    {
        channelHandle *client = new channelHandle;

        ::listen(TxServer,10);
        std::cout << "TCP Server: Waiting for connection of Send Socket" << std::endl;
        client->sendSocket = ::accept(TxServer, (struct sockaddr*)&from,&fromlen);
        std::cout << "TCP Server: Send Socket connection, socket ID: " << client->sendSocket << std::endl;

        ::listen(RxServer,10);
        std::cout << "TCP Server: Waiting for connection of Recv Socket" << std::endl;
        client->recvSocket = ::accept(RxServer, (struct sockaddr*)&from,&fromlen);
        std::cout << "TCP Server: Recv Socket connection, socket ID: " << client->recvSocket << std::endl;

        tcpClient *handler = new tcpClient (clientList.size(), client);
        clientList[clientCount++] = handler;
        handler->setParentMsgQueue (parentQ);
        handler->start ();
    }
}

//-----------------------------------------------------------------
tcpClient* tcpClient::connectRemote (const char *ip)
{
    channelHandle *client = new channelHandle;
    client->recvSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    client->sendSocket = ::socket(AF_INET, SOCK_STREAM, 0);

    SOCKADDR_IN RxService;
    RxService.sin_family = AF_INET;
    RxService.sin_addr.s_addr = inet_addr(ip);
    RxService.sin_port = htons(TCP_CLIENT_RECV_PORT);

    SOCKADDR_IN TxService;
    TxService.sin_family = AF_INET;
    TxService.sin_addr.s_addr = inet_addr(ip);
    TxService.sin_port = htons(TCP_CLIENT_SEND_PORT);

    int ret = ::connect (client->recvSocket, (SOCKADDR*) &RxService, sizeof(RxService));
    if (ret == SOCKET_ERROR)
    {
        ::closesocket (client->recvSocket);
        client->recvSocket = INVALID_SOCKET;
    }

    Sleep (100);

    ret = ::connect (client->sendSocket, (SOCKADDR*) &TxService, sizeof(TxService));
    if (ret == SOCKET_ERROR)
    {
        ::closesocket (client->sendSocket);
        client->sendSocket = INVALID_SOCKET;
    }

    tcpClient *c = new tcpClient (0, client);
    return c;
}

tcpClient::tcpClient (int id, void *hndl)
{
    d = new tcpClientPrvate;
    d->handle = static_cast<channelHandle*>(hndl);
    d->runMore = true;
    d->id = id;
}

tcpClient::~tcpClient ()
{
    ::closesocket (d->handle->recvSocket);
    ::closesocket (d->handle->sendSocket);
    delete d->handle;
    delete d;
}

void tcpClient::setParentMsgQueue (msgQueueHandler *mq)
{
    d->parentQ = mq;
}

void tcpClient::start ()
{
    Slot<void, void*> fp;
    fp = new Callback <tcpClient, void, void*>(this, &tcpClient::rxHandler);
    d->rxThread.create (fp, this);
}

int tcpClient::receive (unsigned char *buff, const unsigned int buffSize, int &recvSize)
{
    int ret = 0;
    if (d->handle->recvSocket != INVALID_SOCKET)
    {
        ret = ::recv(d->handle->recvSocket, (char*)buff, buffSize, 0);
        recvSize = ret;
        return ret;
    }
    else
        Sleep (INFINITE);
    return 0;
}

int tcpClient::send (const unsigned char *buff, const unsigned int buffSize)
{
    int ret = 0;
    if (d->handle->sendSocket != INVALID_SOCKET)
    {
        ret = ::send(d->handle->sendSocket, (char*)buff, buffSize, 0);
    }
    return ret;
}

void tcpClient::rxHandler (void *p)
{
    if (static_cast<tcpClient*>(p) != this)
    {
        std::cout << "FATAL ERROR:: Pointer mismatch for TCP Rx handler" << std::endl;
        return;
    }

    unsigned char *buffer = new unsigned char[MAX_RECV_SIZE];

    int recvSize = 0;
    while (d->runMore)
    {
        this->receive (&buffer[sizeof (unsigned int)], MAX_RECV_SIZE, recvSize);
        if (recvSize > 0)
        {
            pkt_data msg;
            msg.header[0] = buffer[0];
            msg.header[1] = buffer[1];
            msg.sourceID = buffer[2];
            msg.command = buffer[3];
            msg.operation = buffer[4];
            msg.datalength = buffer[5];
            if (msg.datalength > 0)
                memcpy (msg.parameter, &buffer[6], msg.datalength);
            msg.checksum = buffer[(6 + msg.datalength)];
            msg.footer = buffer[(7 + msg.datalength)];
            d->parentQ->write (buffer, (recvSize + sizeof (unsigned int)));
        }
        else
        {
            pkt_data msg;
            msg.header[0] = HEADER_BYTE_START;
            msg.header[1] = HEADER_BYTE_END;
            msg.sourceID = MOBILE_SOURCE_ID;
            msg.command = commandVal_bye;
            msg.operation = Op_status;
            msg.datalength = 4;
            msg.parameter [0] = (char)(d->id & 0x000000FF);
            msg.parameter [1] = (char)((d->id & 0x0000FF00) >> 8);
            msg.parameter [2] = (char)((d->id & 0x00FF0000) >> 16);
            msg.parameter [3] = (char)((d->id & 0xFF000000) >> 24);
            msg.checksum = 0;
            msg.footer = FOOTER_BYTE;

            d->parentQ->write (&msg, sizeof (msg));
            d->runMore = false;
        }
    }
    delete [] buffer;
}
