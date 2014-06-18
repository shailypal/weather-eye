#include "reg_c51.h"
#include "dataFormat.h"

#define ON                  (0)
#define OFF                 (1)

#define TRUE                (1)
#define FALSE               (0)

#define COOL_FAN_ON         (0)
#define COOL_FAN_OFF        (1)
#define HOT_FAN_ON          (2)
#define HOT_FAN_OFF         (3)

sbit c_fan = P1^5;
sbit h_fan = P1^6;

uartReadState   readState;

unsigned char uart_data;
unsigned char TxOK=0;

unsigned char Temp=22;
unsigned char Humidity=45;

unsigned char rxBuffer[20];
unsigned char txBuffer[20];

unsigned char uartByteQueue[20];
unsigned char byteCount = 0;
unsigned char readIndex = 0;
unsigned char writeIndex = 0;

void data_recv(unsigned char dat);
void send_data(unsigned char *data, unsigned char size);
void main_loop();
void processData (const pkt_data *pkt);
unsigned char formatPkt (pkt_data *pkt, unsigned char *buff);
void sendHello ();
void sendPingReply ();
void send_Temp();
void send_Humidity();
void output(unsigned char mode);
unsigned char ReadUart ();
unsigned char comPortReceive (unsigned char *buff);
void senseTemp ();
void senseHumidity ();

void main (void) 
{
    SCON = 0x50;
    TMOD = 0x20;
    TH1  = 0xFD;
    TL1  = 0xFD;
    ES = 1;
    EA = 1;
    TR1 = 1;

    output(COOL_FAN_OFF);
    output(HOT_FAN_OFF);
    sendHello ();

    while(1)
    {
        main_loop();
    }
}

void senseTemp ()
{
    Temp=22;
}

void senseHumidity ()
{
    Humidity=45;
}

void serial(void) unsigned charerrupt 4 
{

    if (TI == 1) 
    {
      TI=0;
      TxOK=0;
   }
    if (RI == 1) 
    {
      RI = 0;
      uart_data = SBUF;
      data_recv(uart_data);
    }
}

void data_recv(unsigned char dat)
{
    uartByteQueue [writeIndex++] = dat;
    byteCount++;
    if (writeIndex == 20)
        writeIndex = 0;
}

void send_data(unsigned char *data, unsigned char size)
{
    unsigned char index;
    for (index = 0; index < size; index++)
    {
        TxOK=1;
        SBUF = data[index];
        while(TxOK);
    }
}

void main_loop()
{
    pkt_data *pkt = (pkt_data*)rxBuffer;
    comPortReceive (rxBuffer);
    processData (pkt);
}

void processData (const pkt_data *pkt)
{
    switch (pkt->command)
    {
        case commandVal_ping:
            if (pkt->operation == Op_get)
                sendPingReply ();
            break;

        case commandVal_fan:
            if (pkt->operation == Op_set)
            {
                if ((pkt->parameter[0] == COLD_AIR_FAN) && (pkt->parameter[1] == TRUE))
                {
                    output (COOL_FAN_ON);
                }
                if ((pkt->parameter[0] == HOT_AIR_FAN) && (pkt->parameter[1] == TRUE))
                {
                    output (HOT_FAN_ON);
                }
                if ((pkt->parameter[0] == COLD_AIR_FAN) && (pkt->parameter[1] == FALSE))
                {
                    output (COOL_FAN_OFF);
                }
                if ((pkt->parameter[0] == HOT_AIR_FAN) && (pkt->parameter[1] == FALSE))
                {
                    output (HOT_FAN_OFF);
                }
            }
            break;

        case commandVal_Temp:
            if (pkt->operation == Op_get)
            {
                senseTemp ();
                send_Temp ();
            }
            break;

        case commandVal_Humidity:
            if (pkt->operation == Op_get)
            {
                senseHumidity ();
                send_Humidity ();
            }
            break;

        default:
            break;
    }
}

unsigned char formatPkt (pkt_data *pkt, unsigned char *buff)
{
    unsigned char index = 0;
    unsigned char pos = 0;
    unsigned char *ptr = (unsigned char*)pkt;

    pkt->header[0] = HEADER_BYTE_START;
    pkt->header[1] = HEADER_BYTE_END;
    pkt->sourceID = UC_SOURCE_ID;
    pkt->footer = FOOTER_BYTE;
    pkt->checksum = 0;

    for (pos = 0; pos < (pkt->datalength + 6); pos++)
        buff[pos] = ptr[pos];
    index = (pkt->datalength + 6);
    buff[index++] = pkt->checksum;
    buff[(pkt->datalength + 7)] = pkt->footer;

    return (pkt->datalength + 8);
}

void sendHello ()
{
    pkt_data pkt;
    unsigned char size = 0;

    pkt.command = (unsigned char)commandVal_hello;
    pkt.operation = (unsigned char)Op_status;
    pkt.datalength = 0;

    size = formatPkt (&pkt, sendBuff);
    send_data (sendBuff, size);
}

void sendPingReply ()
{
    pkt_data pkt;
    unsigned char size = 0;

    pkt.command = (unsigned char)commandVal_ping;
    pkt.operation = (unsigned char)Op_status;
    pkt.datalength = 0;

    size = formatPkt (&pkt, sendBuff);
    send_data (sendBuff, size);
}

void send_Temp()
{
    pkt_data pkt;
    unsigned char size = 0;

    pkt.command = (unsigned char)commandVal_Temp;
    pkt.operation = (unsigned char)Op_status;
    pkt.datalength = 1;
    pkt.parameter[0] = gTemp;

    size = formatPkt (&pkt, sendBuff);
    send_data (sendBuff, size);
}

void send_Humidity()
{
    pkt_data pkt;
    unsigned char size = 0;

    pkt.command = (unsigned char)commandVal_Humidity;
    pkt.operation = (unsigned char)Op_status;
    pkt.datalength = 1;
    pkt.parameter[0] = gHum;

    size = formatPkt (&pkt, sendBuff);
    send_data (sendBuff, size);
}

void output(unsigned char mode)
{
    switch(mode)
        {
        case COOL_FAN_ON:
            c_fan = ON;
            break;
        case COOL_FAN_OFF:
            c_fan = OFF;
            break;
        case HOT_FAN_ON:
            h_fan = ON;
            break;
        case HOT_FAN_OFF:
            h_fan = OFF;
            break;
        }
}

unsigned char ReadUart ()
{
    unsigned char ret = 0;
    while (byteCount == 0);
    ret = uartByteQueue [readIndex++];
    byteCount--;
    if (readIndex == 20)
        readIndex = 0;
    return ret;
}

unsigned char comPortReceive (unsigned char *buff)
{
    unsigned char index = 0;
    unsigned char numberOfDataByte = 0;
    unsigned char readChar;

    readState = uartRead_headerByteStart;
    do
    {
        readChar = ReadUart ();
        if (readChar == HEADER_BYTE_START)
        {
            buff[0] = HEADER_BYTE_START;
            index = 1;
            readState = uartRead_headerByteEnd;
        }
        if ((readState != uartRead_headerByteEnd) && (readChar == HEADER_BYTE_END))
        {
            index = 0;
            readState = uartRead_headerByteStart;
        }
        if ((readState != uartRead_footer) && (readChar == FOOTER_BYTE))
        {
            index = 0;
            readState = uartRead_headerByteStart;
        }
        switch (readState)
        {
            case uartRead_headerByteEnd:
                if (readChar == HEADER_BYTE_END)
                {
                    buff[1] = HEADER_BYTE_END;
                    readState = uartRead_sourceID;
                    index = 2;
                }
                break;
            case uartRead_sourceID:
                buff[index] = readChar;
                readState = uartRead_command;
                index++;
                break;
            case uartRead_command:
                buff[index] = readChar;
                readState = uartRead_operation;
                index++;
                break;
            case uartRead_operation:
                buff[index] = readChar;
                readState = uartRead_datalength;
                index++;
                break;
            case uartRead_datalength:
                buff[index] = readChar;
                readState = uartRead_parameter;
                numberOfDataByte = readChar;
                if (numberOfDataByte == 0)
                    readState = uartRead_checksum;
                index++;
                break;
            case uartRead_parameter:
                buff[index] = readChar;
                numberOfDataByte--;
                if (numberOfDataByte <= 0)
                    readState = uartRead_checksum;
                index++;
                break;
            case uartRead_checksum:
                index = 18;
                buff[index] = readChar;
                readState = uartRead_footer;
                index++;
                break;
            case uartRead_footer:
                if (readChar == FOOTER_BYTE)
                {
                    buff[index] = readChar;
                    readState = uartRead_Done;
                    index++;
                }
                break;
            default:
                break;
        }
    } while (readState != uartRead_Done);
    return 0;
}
