#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/fuse.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdint.h>
#include "sampler.h"

#ifndef NDEBUG
#include "picofmt.h"
#include "uart.h"
#endif

/*
 * debug facilities
 */
#ifndef NDEBUG
#define DPRINT(msg) pfmt_outln(PSTR(msg))
#else // NDEBUG
#define DPRINT(msg) ((void)0)
#endif // NDEBUG

#ifndef NOINIT
#define NOINIT __attribute__((section(".noinit")))
#endif

#define BAUDRATE_38400 99

volatile uint8_t g_ticks NOINIT;
ISR(TIM0_COMPA_vect)
{
	++g_ticks;
}

void timer_init(void)
{
	g_ticks = 0;
	OCR0A = 117;
	TCCR0A = _BV(WGM01); // wgm=2, CTC mode
	TCCR0B = _BV(CS02) | _BV(CS00); // div-by-1024
	TIMSK = _BV(OCIE0A);
}

void tick_wait(void)
{
	const uint8_t tck = g_ticks;
	while ( tck == g_ticks )
		sleep_cpu();
}

int main()
{
	DDRB = 0;

#ifndef NDEBUG
	uart_init(BAUDRATE_38400);
	pfmt_out(PSTR("\r\nMCUSR: "));
	pfmt_print_bits(PSTR("PEBW"), 0);
#endif

	timer_init();
	sei();

	sampler_init();
	for(;;)
	{
		tick_wait();
		uint16_t value = sampler_get_next_sample();

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			FOUT1("Data: $0", value);
		}
	}
	return 0;
}
