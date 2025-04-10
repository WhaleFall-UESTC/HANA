/* arch/riscv/drivers/uart.c */
void            uart_init(void);
int             uart_putc_sync(char c);
void            uart_putc(char c);
void            uart_irq_handler();

static inline int
putchar(int c) 
{
    return uart_putc_sync(c);
}