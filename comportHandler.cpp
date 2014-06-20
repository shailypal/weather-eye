
#include "comPortHandler.hpp"
#include "threadHandler.hpp"
#include "dataFormat.h"
#include <Windows.h>
#include <cstdio>

class comPortHandlerPrivate
{
public:
    msgQueueHandler *parentQ;
    HANDLE          hndl;
    threadHandler   rxThread;
    uartReadState   readState;
    bool runMore;
};

comPortHandler::comPortHandler (const char *portName)
{
    d = new comPortHandlerPrivate;
    d->hndl = CreateFile((LPCTSTR)portName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (d->hndl != INVALID_HANDLE_VALUE)
    {
        DCB dcb;
        memset(&dcb,0,sizeof(dcb));
        dcb.DCBlength = sizeof(dcb);
        dcb.BaudRate = CBR_9600;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity = NOPARITY;
        dcb.ByteSize = 8;

        SetCommState(d->hndl, &dcb);
        SetCommMask(d->hndl, EV_RXCHAR);

        COMMTIMEOUTS    UartTimeOut;
        UartTimeOut.ReadIntervalTimeout = 3;
        UartTimeOut.ReadTotalTimeoutMultiplier = 3;
        UartTimeOut.ReadTotalTimeoutConstant = 2;
        UartTimeOut.WriteTotalTimeoutMultiplier = 3;
        UartTimeOut.WriteTotalTimeoutConstant = 2;
        SetCommTimeouts(d->hndl, &UartTimeOut);
    }
}

comPortHandler::~comPortHandler ()
{
    delete d;
}

void comPortHandler::setParentMsgQueue (msgQueueHandler *mq)
{
    d->parentQ = mq;
}

void comPortHandler::start ()
{
    Slot<void, void*> fp;
    fp = new Callback <comPortHandler, void, void*>(this, &comPortHandler::rxHandler);
    d->rxThread.create (fp, this);
}

int comPortHandler::receive (unsigned char *buff, const unsigned int buffSize, unsigned int &recvSize)
{
    if (d->hndl == INVALID_HANDLE_VALUE)
        Sleep (INFINITE);

    d->readState = uartRead_headerByteStart;
    int index = 0;
    DWORD read = 0;
    int numberOfDataByte = 0;
    memset (buff, 0, buffSize);
    do
    {
        unsigned char readChar;
        ReadFile (d->hndl, &readChar, 1, &read, NULL);
        Sleep (10);
        if (read > 0)
        {
            printf ("%x ", readChar);
            if (readChar == HEADER_BYTE_START)
            {
                memset (buff, 0, buffSize);
                buff[0] = HEADER_BYTE_START;
                index = 1;
                d->readState = uartRead_headerByteEnd;
            }
            if ((d->readState != uartRead_headerByteEnd) && (readChar == HEADER_BYTE_END))
            {
                memset (buff, 0, buffSize);
                index = 0;
                d->readState = uartRead_headerByteStart;
            }
            if ((d->readState != uartRead_footer) && (readChar == FOOTER_BYTE))
            {
                memset (buff, 0, buffSize);
                index = 0;
                d->readState = uartRead_headerByteStart;
            }
            switch (d->readState)
            {
                case uartRead_headerByteEnd:
                    if (readChar == HEADER_BYTE_END)
                    {
                        buff[1] = HEADER_BYTE_END;
                        d->readState = uartRead_sourceID;
                        index = 2;
                    }
                    break;
                case uartRead_sourceID:
                    buff[index] = readChar;
                    d->readState = uartRead_command;
                    index++;
                    break;
                case uartRead_command:
                    buff[index] = readChar;
                    d->readState = uartRead_operation;
                    index++;
                    break;
                case uartRead_operation:
                    buff[index] = readChar;
                    d->readState = uartRead_datalength;
                    index++;
                    break;
                case uartRead_datalength:
                    buff[index] = readChar;
                    d->readState = uartRead_parameter;
                    numberOfDataByte = readChar;
                    if (numberOfDataByte == 0)
                        d->readState = uartRead_checksum;
                    index++;
                    break;
                case uartRead_parameter:
                    buff[index] = readChar;
                    numberOfDataByte--;
                    if (numberOfDataByte <= 0)
                        d->readState = uartRead_checksum;
                    index++;
                    break;
                case uartRead_checksum:
                    index = 18;
                    buff[index] = readChar;
                    d->readState = uartRead_footer;
                    index++;
                    break;
                case uartRead_footer:
                    if (readChar == FOOTER_BYTE)
                    {
                        buff[index] = readChar;
                        d->readState = uartRead_Done;
                        index++;
                        recvSize = index;
                    }
                    break;
                default:
                    break;
            }
        }
    } while (d->readState != uartRead_Done);
    return 0;
}

int comPortHandler::send (const unsigned char *buff, const unsigned int buffSize)
{
    if (d->hndl == INVALID_HANDLE_VALUE)
        return -1;
    DWORD ret = 0;
    for (unsigned int i = 0; i < buffSize; i++)
    {
        WriteFile(d->hndl, &buff[i], 1, &ret, NULL);
        Sleep (5);
    }
    return (int)ret;
}

void comPortHandler::rxHandler (void *p)
{
    if (static_cast<comPortHandler*>(p) != this)
    {
        return;
    }

    unsigned char *buffer = new unsigned char[20];
    unsigned int recvSize = 0;
    while (d->runMore)
    {
        receive (buffer, 20, recvSize);
        printf ("\n");
        d->parentQ->write (buffer, recvSize);
    }
    delete [] buffer;
}
