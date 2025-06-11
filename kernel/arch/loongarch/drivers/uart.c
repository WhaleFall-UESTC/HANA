#include <common.h>
#include <mm/memlayout.h>
#include <irq/interrupt.h>
#include <locking/spinlock.h>
#include <proc/proc.h>

#define RHR 0       // receive holding register
#define THR 0       // transmit holding register
#define DLL 0       // divisor latch low
#define DLM 1       // divisor latch high
#define IER 1       // interrupt enable register
#define FCR 2       // FIFO control register
#define ISR 2       // interrupt status register
#define IIR 2
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

#define IIR_NO_INT      (1 << 1)
#define IIR_ID_MASK      0x0f
#define IIR_TX_EMPTY    (1 << 1)
#define IIR_RX_READY    (1 << 3)
#define IIR_RX_TIMEOUT  (1 << 6)

#define UART_REG(reg) ((volatile uint8 *)(UART0 + reg))
#define uart_read_reg(reg) (*UART_REG(reg))
#define uart_write_reg(reg, v) (*UART_REG(reg) = v)

#define UART_TX_BUF_SIZE 32
spinlock_t uart_tx_lock;
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

typedef int (*putchar_t)(int);
putchar_t put_char;
typedef int (*getchar_t)();
getchar_t get_char;

int uart_putc_sync(int c);
int uart_getc();

void 
uart_init()
{
	put_char = uart_putc_sync;
	get_char = uart_getc;

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
	// uart_write_reg(IER, IER_RX_EN | IER_TX_EN);

	spinlock_init(&uart_tx_lock, "uart locking");
}

// if the UART is idle, and a character is waiting
// in the transmit buffer, send it.
// caller must hold uart_tx_lock.
// called from both the top- and bottom-half.
void
uart_start()
{
	while(1) {
		if(uart_tx_w == uart_tx_r) {
			// transmit buffer is empty.
			return;
		}

		if((uart_read_reg(LSR) & LSR_TX_IDLE) == 0) {
			// the UART transmit holding register is full,
			// so we cannot give it another byte.
			// it will interrupt when it's ready for a new byte.
			return;
		}

		int c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];
		uart_tx_r += 1;

		// maybe uart_putc() is waiting for space in the buffer.
		wakeup(&uart_tx_r);

		uart_write_reg(THR, c);
	}
}


// add a character to the output buffer and tell the
// UART to start sending if it isn't already.
// blocks if the output buffer is full.
// because it may block, it can't be called
// from interrupts; it's only suitable for use
// by write().
void
uart_putc(int c)
{
	spinlock_acquire(&uart_tx_lock);

	while(1) {
		if(uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE) {
			// buffer is full.
			// wait for uart_start() to open up space in the buffer.
			sleep(&uart_tx_r);
		} else {
			uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE] = c;
			uart_tx_w += 1;
			uart_start();
			spinlock_release(&uart_tx_lock);
			return;
		}
	}
}

// polling version, disable intr while output
// used by kernel printf
int
uart_putc_sync(int c)
{
	irq_pushoff();

	while ((uart_read_reg(LSR) & LSR_TX_IDLE) == 0)
		;
	char ret = uart_write_reg(THR, c);

	irq_popoff();

	return ret;
}

int
uart_getc()
{
	if(uart_read_reg(LSR) & 0x01){
		// input data is ready.
		return uart_read_reg(RHR);
	} else {
		return -1;
	}
}

// handle a uart interrupt, raised because input has
// arrived, or the uart is ready for more output, or
// both. called from trap.c.
int
uart_isr()
{
	// read and process incoming characters.
	while(1) {
		int c = uart_getc();
		if(c == -1)
			break;
		// consoleintr(c);
	}

	// send buffered characters.
	spinlock_acquire(&uart_tx_lock);
	uart_start();
	spinlock_release(&uart_tx_lock);
	return 0;
}