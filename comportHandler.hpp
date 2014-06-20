#ifndef _COM_PORT_HANDLER_HPP_
#define _COM_PORT_HANDLER_HPP_

#include "msgQueueHandler.hpp"

class comPortHandlerPrivate;
class comPortHandler
{
public:
    comPortHandler (const char *portName);
    virtual ~comPortHandler ();

    void setParentMsgQueue (msgQueueHandler *mq);

    void start ();
    int receive (unsigned char *buff, const unsigned int buffSize, unsigned int &recvSize);
    int send (const unsigned char *buff, const unsigned int buffSize);

private:
    comPortHandlerPrivate   *d;
    void rxHandler (void *p);
};

#endif
