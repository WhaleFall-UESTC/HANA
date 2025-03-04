#include <platform.h>
#include <common.h>

#define RHR 0       // receive holding register
#define THR 0       // transmit holding register
#define DLL 0       // divisor latch low
#define DLM 1       // divisor latch high
#define IER 1       // interrupt enable register
#define FCR 2       // FIFO control register
#define ISR 2       // interrupt status register
#define LCR 3       // line control register
#define MCR 4       // modem control register
#define LSR 5       // line status register
#define MSR 6       // modem status register
#define SPR 7       // scratchpad register

#define LSR_TX_IDLE (1 << 5)

#define UART_REG(reg) ((volatile uint8 *)(UART0 + reg))
#define uart_read_reg(reg) (*UART_REG(reg))
#define uart_write_reg(reg, v) (*UART_REG(reg) = v)

void 
uart_init()
{
    // disable interrupts
    uart_write_reg(IER, 0x00);

    // enable divcisor latch
    uart_write_reg(LCR, ((1 << 7) | uart_read_reg(LCR)));
    // baud rate 38.4k
    uart_write_reg(DLL, 0x03);
    uart_write_reg(DLM, 0x00);

    // leave set-baud mode,
    // and set word length to 8 bits, no parity.
    uart_write_reg(LCR, 0x03);

    // enable RHR, THR interrupts
    // uart_write_reg(IER, 0x03);
}

// polling
char 
uart_putc(char c)
{
    while ((uart_read_reg(LSR) & LSR_TX_IDLE) == 0)
        ;
    return uart_write_reg(THR, c);
}
