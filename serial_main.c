#include "compiler.h"
#include "sampler.h"
#include "picofmt.h"
#include "uart.h"

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define BAUDRATE_38400 99
#define HZ_100

volatile uint16_t g_ticks NOINIT;
ISR(TIM0_COMPA_vect)
{
	++g_ticks;
}

void tick_init(void)
{
	g_ticks = 0;
	TCCR0A = _BV(WGM01); // wgm=2, CTC mode
#if defined HZ_100
	OCR0A = 117;
	TCCR0B = _BV(CS02) | _BV(CS00); // div-by-1024
#elif defined HZ_1000
	OCR0A = 175;
	TCCR0B = _BV(CS01) | _BV(CS00); // div-by-64
#else
#error no HZ_XXX defined
#endif
	TIMSK = _BV(OCIE0A);
}

void tick_wait(void)
{
	const uint8_t tck = g_ticks;
	while ( tck == g_ticks )
		sleep_cpu();
}

static NOINIT uint8_t mcusr_mirror;

INIT_FUNC_3 void early_init(void)
{
	mcusr_mirror = MCUSR;
	wdt_disable();
}

INIT_FUNC_8 void late_init(void)
{
	uart_init(BAUDRATE_38400);
	pfmt_out(PSTR("\r\nMCUSR: "));
	pfmt_print_bits(PSTR("PEBW"), mcusr_mirror);

	tick_init();
	sampler_init();
}

int main()
{
	sei();
	for(;;)
	{
		tick_wait();
		const uint16_t raw = sampler_get_next_sample();
		const uint16_t level = fp_reciprocal(raw);
		const uint32_t i_level = fp_to_uint32(level);

		/*
		  FOUT2("Data: $0 Lvl: $1", raw, level);
		*/
		FOUT2("Light: $0$1",i_level>>16,i_level&0xFFFF);
	}
}
