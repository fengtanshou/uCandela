#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/fuse.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdint.h>

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

#define SENSOR_PIN PB1
#define SENSOR_PORT PORTB
#define SENSOR_DDR DDRB

#define BAUDRATE_38400 99

#define ACIS_TOGGLE 0
#define ACIS_FALLING 2
#define ACIS_RISING 3

#define TIMER1_PRESCALER_VALUE(value) 0		\
	| ( (value)&8 ? _BV(CS13) : 0 )		\
	| ( (value)&4 ? _BV(CS12) : 0 )		\
	| ( (value)&2 ? _BV(CS11) : 0 )		\
	| ( (value)&1 ? _BV(CS10) : 0 )		\
	;

void sigma_init();
void sigma_reset();

static volatile enum {
	sigma_idle,
	sigma_ready,
	sigma_sampling,
	sigma_complete,
	sigma_complete_underflow,
	sigma_complete_overflow,
} sigma_state NOINIT;
static volatile uint16_t sigma_value NOINIT;

ISR(ANA_COMP_vect)
{
	if ( sigma_state != sigma_sampling )
		return;

	sigma_value = TCNT1;
	TCCR1 = 0;
	TIFR = _BV(TOV1);
	sigma_state = sigma_complete;

	SENSOR_PORT |= _BV(SENSOR_PIN);
	SENSOR_DDR |= _BV(SENSOR_PIN);

	/*
	if ( bit_is_set(ACSR,ACO) )
		FOUT1("ACMP: $0", sigma_value);
	else
		FOUT1("acmp: $0", sigma_value);
	*/
}

ISR(TIM1_OVF_vect)
{
	if( sigma_state != sigma_sampling )
		return;

	TCCR1 = 0;
	TIFR = _BV(TOV1);
	sigma_state = sigma_complete_overflow;
}

void sigma_init(void)
{
	DIDR0 |= _BV(AIN1D);

	const uint8_t amode = ACIS_RISING;
	const uint8_t acsr = _BV(ACBG)
		| ((amode&2) ? _BV(ACIS1) : 0)
		| ((amode&1) ? _BV(ACIS0) : 0)
		;
	ACSR = acsr;
	ACSR |= acsr | _BV(ACI) | _BV(ACIE);

	TCCR1 = 0;
	GTCCR = 0;
	TIFR = _BV(TOV1);
	TIMSK = _BV(TOIE1);
	
	sigma_state = sigma_idle;
}

void sigma_shutdown(void)
{
	ACSR = _BV(ACD) | _BV(ACI);
	TIMSK = 0;
	TCCR1 = 0;
	TIFR = _BV(TOV1);
}

void sigma_reset(void)
{
	// precharge
	SENSOR_PORT |= _BV(SENSOR_PIN);
	SENSOR_DDR |= _BV(SENSOR_PIN);
	TCNT1 = 0;
	GTCCR = _BV(PSR1);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		sigma_state = sigma_ready;
	}
	_delay_us(1);	
}

void sigma_start()
{
	// start discharging
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		const uint8_t ddr = SENSOR_DDR & (~_BV(SENSOR_PIN));
		const uint8_t port = SENSOR_PORT & (~_BV(SENSOR_PIN));

		sigma_state = sigma_sampling;

		// fast path
		TCCR1 = TIMER1_PRESCALER_VALUE(1); // ck
		SENSOR_DDR = ddr;
		SENSOR_PORT = port;
	}
}

uint8_t sigma_wait(void)
{
	uint8_t state;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		while ( sigma_state == sigma_sampling ) 
			NONATOMIC_BLOCK(NONATOMIC_FORCEOFF)
			{
				sleep_cpu();
			}
		state = sigma_state;
	}
	return state;
}

int main()
{
#ifndef NDEBUG
	uart_init(BAUDRATE_38400);
	pfmt_out(PSTR("\r\nMCUSR: "));
	pfmt_print_bits(PSTR("PEBW"), 0);
#endif

	FOUT0("init");
	sigma_init();
	sei();

	for(; 1 ;)
	{
		//FOUT0("start");
		sigma_start();
		//FOUT0("wait");
		const uint8_t state = sigma_wait();
		cli();
		//FOUT2("data: $0 state $1", sigma_value, state);
		if ( sigma_complete == state )
			FOUT1("D: $0", sigma_value);
		sei();
		sigma_reset();
	}

	return 0;
}
