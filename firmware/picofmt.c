/*
 * PicoFormatter library
 */

#include "picofmt.h"
#include <stdarg.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

extern void uart_putchar(char);

uint16_t g_picofmt_args[PICO_MAX_ARGS] __attribute__((section(".noinit")));

uint8_t to_hex_char(uint8_t ch) __attribute__((const, noinline));
uint8_t to_hex_char(uint8_t ch)
{
	ch &= 0xF;
	return ( (ch) >= 0xA ? (ch)+'A'- 0xA : (ch) + '0' );
}

uint16_t fmt_hex_byte(uint8_t ch) __attribute__((const, naked, noinline));
uint16_t fmt_hex_byte(uint8_t byte)
{
	__asm__ volatile (
		"mov r25, r24\n "\
		"rcall to_hex_char\n "\
		 /*swap r24, r25 */
		"eor r24, r25\n eor r25, r24\n eor r24, r25\n "\
		"swap r24\n "\
		"rcall to_hex_char\n "\
		"ret"
		: "=r" (byte)	      \
		:
		);
}

void pfmt_crlf(void)
{
	uart_putchar('\r');
	uart_putchar('\n');
}

void pfmt_out(const prog_char *fmt)
{
	char ch;
	while ( (ch=pgm_read_byte(fmt++)) !=0 )
	{
		if ( ch == '$' )
		{
			ch = pgm_read_byte(fmt++) - '0';
			if ( ch < PICO_MAX_ARGS && ch >= 0 )
			{
				const uint16_t arg = g_picofmt_args[(uint8_t)ch];
				const uint16_t rh = fmt_hex_byte(arg>>8);
				uart_putchar(rh);
				uart_putchar(rh>>8);
				const uint16_t rl = fmt_hex_byte(arg>>0);
				uart_putchar(rl);
				uart_putchar(rl>>8);

			}
		}
		else
		{
			uart_putchar(ch);
		}
	}
}

void pfmt_outln(const prog_char *fmt)
{
	pfmt_out(fmt);
	pfmt_crlf();
}

void pfmt_print_bits(const prog_char *bit_names, uint8_t value)
{
	do
	{
		const uint8_t c = pgm_read_byte(bit_names++);
		if ( c && (value & 1) )
			uart_putchar(c);
		value >>= 1;
	} while ( value );
	pfmt_crlf();
}
