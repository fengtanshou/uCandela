#ifndef UART_H_INC
#define UART_H_INC

void uart_init(uint8_t divisor);
void uart_putchar(char ch);
int uart_getchar(void);

#endif // UART_H_INC
