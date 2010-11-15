#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/fuse.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdint.h>
#include "sampler.h"

/* include debug features */
#ifndef NDEBUG
#include "picofmt.h"
#include "uart.h"
#endif

/* include proper usb driver headers */
#ifndef USBDRV
#error no USB driver selected
#endif

#if USBDRV == vusb
#include "usbdrv/usbdrv.h"
#else /* USBDRV */
#error USB driver not supported
#endif /* USBDRV */



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

USB_PUBLIC usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	return 0;
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
		const uint16_t raw = sampler_get_next_sample();
		const uint16_t level = fp_reciprocal(raw);
		const uint32_t i_level = fp_to_uint32(level);

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			/*
			FOUT2("Data: $0 Lvl: $1", raw, level);
			*/
			FOUT2("Light: $0$1",i_level>>16,i_level&0xFFFF);
		}
	}
	return 0;
}
