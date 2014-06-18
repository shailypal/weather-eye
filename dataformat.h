#ifndef _DATA_FORMAT_H_
#define _DATA_FORMAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UC_SOURCE_ID        (0)
#define PC_SOURCE_ID        (1)
#define MOBILE_SOURCE_ID    (2)

#define HEADER_BYTE_START   (0xAA)
#define HEADER_BYTE_END     (0xBB)
#define FOOTER_BYTE         (0xCC)

#define COLD_AIR_FAN        (0x01)
#define HOT_AIR_FAN         (0x02)

#define MAX_TEMP_VAL        (0x2D)

typedef struct _pkt_data
{
    unsigned char header[2];
    unsigned char sourceID;
    unsigned char command;
    unsigned char operation;
    unsigned char datalength;
    unsigned char parameter[12];
    unsigned char checksum;
    unsigned char footer;
}pkt_data;

typedef enum _uartReadState
{
    uartRead_headerByteStart = 0,
    uartRead_headerByteEnd,
    uartRead_sourceID,
    uartRead_command,
    uartRead_operation,
    uartRead_datalength,
    uartRead_parameter,
    uartRead_checksum,
    uartRead_footer,
    uartRead_Done
}uartReadState;

typedef enum _commandValue
{
    commandVal_Temp = 0,
    commandVal_Humidity,
    commandVal_hello,
    commandVal_bye,
    commandVal_ping,
    commandVal_fan,
    commandVal_TempRange,
    commandVal_Max
}commandValue;

typedef enum _operations
{
    Op_set = 0,
    Op_get,
    Op_status,
    Op_error,
    Op_Max
}operations;

#ifdef __cplusplus
}
#endif

#endif

