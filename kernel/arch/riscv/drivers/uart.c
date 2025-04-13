#include <platform.h>
#include <common.h>
#include <riscv.h>
#include <trap/context.h>
#include <proc/proc.h>

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

#define FCR_FIFO_EN (1 << 0)
#define FCR_FIFO_CLR (3 << 1)

#define IER_TX_EN (1 << 1)
#define IER_RX_EN (1 << 0)

#define UART_REG(reg) ((volatile uint8 *)(VIRT_UART0 + reg))
#define uart_read_reg(reg) (*UART_REG(reg))
#define uart_write_reg(reg, v) (*UART_REG(reg) = v)

#define UART_TX_BUF_SIZE 32
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64 uart_tx_r = 0;   // number of bytes read from buf and transmit
uint64 uart_tx_w = 0;   // number of bytes written to buf

static inline int uart_buf_empty() {
    return (uart_tx_r == uart_tx_w);
}

static inline int uart_buf_full() {
    return (uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE);
}

static inline void uart_buf_write(char c) {
    uart_tx_buf[(uart_tx_w++) % UART_TX_BUF_SIZE] = c;
}

static inline char uart_buf_read() {
    return uart_tx_buf[(uart_tx_r++) % UART_TX_BUF_SIZE];
}


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

    // enable FIFO buffer and clear it
    // when buffer is full, interrupt
    uart_write_reg(FCR, FCR_FIFO_EN | FCR_FIFO_CLR);

    // enable RHR, THR interrupts
    uart_write_reg(IER, IER_RX_EN | IER_TX_EN);
}

// polling version, disable intr while output
// used by kernel printf
int
uart_putc_sync(char c)
{
    int intr_status = intr_get();
    intr_off();

    while ((uart_read_reg(LSR) & LSR_TX_IDLE) == 0)
        ;
    char ret = uart_write_reg(THR, c);

    if (intr_status)
        intr_on();

    return ret;
}

// transmit all characters in uart_buf
static void
uart_start()
{
    while (1) {
        if (uart_buf_empty()) return;
        if ((uart_read_reg(LSR) & LSR_TX_IDLE) == 0) return;

        char c = uart_buf_read();

        wakeup(&uart_tx_r);

        uart_write_reg(THR, c);
    }
}


void
uart_putc(char c)
{
    while (1) {
        if (uart_buf_full()) {
            sleep(&uart_tx_r);
        } else {
            uart_buf_write(c);
            uart_start();
            return;
        }
    }
}


void
uart_irq_handler()
{
    uart_start();
}