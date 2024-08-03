#ifndef SERIAL_H
#define	SERIAL_H

#include "pico.h"
#include "pico/stdlib.h"
#include "bsp/board.h"

#define UART_TX_PIN             20
#define UART_RX_PIN             21
#define UART_BAUD_RATE          115200
#define UART_DATA_BITS          8
#define UART_PARITY             UART_PARITY_NONE
#define UART_STOP_BITS          1
#define UART_TIMEOUT_US         500

#define UART_BUFFER_SIZE        TEXT_COLUMNS_VISIBLE

typedef struct {
    uart_inst_t         *uartId;
    uint32_t            baudRate;
    uint8_t             dataBits;
    uint8_t             stopBits;
    uart_parity_t       parity;
} st_serialConfiguration;

typedef struct {
    uint8_t             buffer[UART_BUFFER_SIZE];
    volatile uint8_t    headIndex;
    volatile uint8_t    tailIndex;
    bool                full;
} st_serialBuffer;

typedef struct {
    void                (* characterHandler)(uint8_t character);
} st_receivedCharacterHandler;

void serial_initialiseTerminalUart(uart_inst_t *uartId);
st_serialConfiguration *serial_getSerialConfiguration(void);
void serial_bufferInitialise(st_serialBuffer *serialBuffer);
bool serial_bufferPutCharacter(st_serialBuffer *serialBuffer, uint8_t character);
bool serial_bufferGetCharacter(st_serialBuffer *serialBuffer, uint8_t *character);
bool serial_uartSendCharacter(uint8_t character);
bool serial_attachReceivedCharacterHandler(void (*characterHandler)(uint8_t character));

#endif // SERIAL_H