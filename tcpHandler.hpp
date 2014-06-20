#ifndef _TCP_HANDLER_HPP_
#define _TCP_HANDLER_HPP_
#include "threadHandler.hpp"
#include "msgQueueHandler.hpp"
#include <map>

#define MAX_RECV_SIZE               (20)

#define TCP_SERVER_SEND_PORT        (20250)
#define TCP_SERVER_RECV_PORT        (20249)

#define TCP_CLIENT_SEND_PORT        TCP_SERVER_RECV_PORT
#define TCP_CLIENT_RECV_PORT        TCP_SERVER_SEND_PORT

class tcpClientPrvate;
class tcpClient;

class tcpServer
{
public:
    tcpServer ();
    virtual ~tcpServer ();
    void start ();

    void setParentMsgQueue (msgQueueHandler *mq);

    tcpClient* getClient (unsigned int id);
    void deleteClient (unsigned int id);

private:
    std::map <unsigned int, tcpClient*> clientList;
    threadHandler connectThread;
    msgQueueHandler *parentQ;
    void connectHandler (void *p);
};

class tcpClient
{
public:
    static tcpClient* connectRemote (const char *ip);
    tcpClient (int id, void *hndl);
    virtual ~tcpClient ();

    void setParentMsgQueue (msgQueueHandler *mq);

    void start ();
    int receive (unsigned char *buff, const unsigned int buffSize, int &recvSize);
    int send (const unsigned char *buff, const unsigned int buffSize);

private:
    tcpClientPrvate *d;
    void rxHandler (void *p);
};

#endif
