/* arch/riscv/drivers/uart.c */
void            uart_init(void);
int             uart_putc(char c);

static inline int
putchar(int c) 
{
    return uart_putc(c);
}