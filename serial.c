#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "conio.h"
#include "serial.h"

st_serialConfiguration serialConfiguration;

void serial_initialiseTerminalUart(uart_inst_t *uartId) {

    assert (uartId != NULL);
    
    serialConfiguration.uartId = uartId;
    serialConfiguration.baudRate = UART_BAUD_RATE;
    uart_init(uartId, UART_BAUD_RATE);

    serialConfiguration.dataBits = UART_DATA_BITS;
    serialConfiguration.stopBits = UART_STOP_BITS;
    serialConfiguration.parity = UART_PARITY;
    uart_set_format(uartId, UART_DATA_BITS, UART_STOP_BITS, UART_PARITY_NONE);
    uart_set_hw_flow(uartId, false, false);

    // Set the TX and RX pins by using the function select on the GPIO
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(uartId, false);
    int UART_IRQ = (uartId == uart0) ? UART0_IRQ : UART1_IRQ;

    // Set up and enable the RX interrupt handler
    irq_set_exclusive_handler(UART_IRQ, serial_uartReceiveHandler);
    irq_set_enabled(UART_IRQ, true);

    // Enable the UART
    uart_set_irq_enables(uartId, true, false);
}

void serial_uartReceiveHandler(void) {
    while (uart_is_readable_within_us(serialConfiguration.uartId, UART_TIMEOUT_US)) {
        conio_printCharacter(uart_getc(serialConfiguration.uartId), PALETTE_COLOUR_AMBER_INDEX, PALETTE_COLOUR_BLACK_INDEX);
    }
}