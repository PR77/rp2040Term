#include "pico.h"
#include "pico/stdlib.h"
#include "string.h"
#include "time.h"
#include "serial.h"

static st_serialConfiguration serialConfiguration;
static st_serialBuffer serialTransmitBuffer;
static st_receivedCharacterHandler receivedCharacterHandler;

static void serial_uartInterruptHandler(void);

/**
    Initialise serial UART point and support data structures.

    @param[in]     uartId pointer to UART device.
*/
void serial_initialiseTerminalUart(uart_inst_t *uartId) {

    assert (uartId != NULL);

    serial_bufferInitialise(&serialTransmitBuffer);

    memset(&receivedCharacterHandler, 0, sizeof(st_receivedCharacterHandler));
    
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
    irq_set_exclusive_handler(UART_IRQ, serial_uartInterruptHandler);
    irq_set_enabled(UART_IRQ, true);

    // Enable the UART RX interrupt by default, TX enabled on when data needs to be sent.
    uart_set_irq_enables(uartId, true, false);
}

/**
    Get a pointer to the serial port (UART) configuration.

    @returns[out]  st_serialConfiguration pointer to serial port configuration structure.
*/
st_serialConfiguration *serial_getSerialConfiguration(void) {

    return (&serialConfiguration);
}

void serial_bufferInitialise(st_serialBuffer *serialBuffer) {

    assert (serialBuffer != NULL);

    memset(serialBuffer, 0, sizeof(st_serialBuffer));
}

bool serial_bufferPutCharacter(st_serialBuffer *serialBuffer, uint8_t character) {
    
    bool insertSuccess = false;

    assert (serialBuffer != NULL);

    // Some of the buffer code taken from
    // https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/

    if (serialBuffer->full == false) {
        serialBuffer->buffer[serialBuffer->headIndex++] = character;

        if (serialBuffer->headIndex == UART_BUFFER_SIZE) {
            serialBuffer->headIndex = 0;    
        }

        insertSuccess = true;
    }

    serialBuffer->full = (serialBuffer->headIndex == serialBuffer->tailIndex);

    return (insertSuccess);
}

bool serial_bufferGetCharacter(st_serialBuffer *serialBuffer, uint8_t *character) {

    bool removedSuccess = false;

    assert (serialBuffer != NULL);

    // Some of the buffer code taken from
    // https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/

    if ((serialBuffer->full == false) && (serialBuffer->headIndex != serialBuffer->tailIndex)) {
        *character = serialBuffer->buffer[serialBuffer->tailIndex++];

        serialBuffer->full = false;

        if (serialBuffer->tailIndex == UART_BUFFER_SIZE) {
            serialBuffer->tailIndex = 0;    
        }

        removedSuccess = true;
    }

    return (removedSuccess);
}

/**
    Sends a character out the configured UART. This is non blocking as the character
    is pushed directly into a buffer and transmission is started.

    @param[in]     character character to send via UART device.
    @returns[out]  bool to indicate transmit was successfully queued in buffer.

*/
bool serial_uartSendCharacter(uint8_t character) {

    bool insertSuccess = false;

    insertSuccess = serial_bufferPutCharacter(&serialTransmitBuffer, character);

    if ((insertSuccess == true) && (uart_is_writable(serialConfiguration.uartId) == true)) {
        uint8_t characterToSend = 0;

        uart_set_irq_enables(serialConfiguration.uartId, true, true);

        // Now trigger the transmit interrupts by pushing a character out. Lets just call
        // the interrupt handler as this will do the transmission.
        serial_uartInterruptHandler();
    }
 
    return (insertSuccess);
}

/**
    Attach a custom handler which is called whenever a character is received from
    the UART interface.
    
    @param[in]     characterHandler function pointer to character handler. Can be NULL if not required.
    @return[out]   bool true if attach was successful, otherwise false.
*/
bool serial_attachReceivedCharacterHandler(void (*characterHandler)(uint8_t character)) {

    bool attachedSuccess = false;

    if (*characterHandler != NULL) {
        receivedCharacterHandler.characterHandler = characterHandler;
        attachedSuccess = true;
    }

    return (attachedSuccess);
}

/**
    UART interrupt handler - handles both receiving and transmitting.
*/
static void serial_uartInterruptHandler(void) {

    // Handle receive interrupts
    if (uart_is_readable(serialConfiguration.uartId) == true) {

        uint8_t character = uart_getc(serialConfiguration.uartId);

        if (NULL != receivedCharacterHandler.characterHandler) {
            receivedCharacterHandler.characterHandler(character);
        }
    }

    // Handle transmit interrupts
    if (uart_is_writable(serialConfiguration.uartId) == true) {
        uint8_t characterToSend = 0;

        if (serial_bufferGetCharacter(&serialTransmitBuffer, &characterToSend) == true) {
            uart_putc_raw(serialConfiguration.uartId, characterToSend);
        } else {
            // Done transmitting all data in buffer. TX interrupts are now disabled.
            uart_set_irq_enables(serialConfiguration.uartId, true, false);    
        }
    }
}