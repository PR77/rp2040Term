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

typedef struct {
    uart_inst_t     *uartId;
    uint32_t        baudRate;
    uint8_t         dataBits;
    uint8_t         stopBits;
    uart_parity_t   parity;
} st_serialConfiguration;

void serial_initialiseTerminalUart(uart_inst_t *uartId);
void serial_uartReceiveHandler(void);

#endif // SERIAL_H