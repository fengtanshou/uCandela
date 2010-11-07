#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
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

#define SENSOR_PIN PB1
#define SENSOR_PORT PORTB
#define SENSOR_DDR DDRB

#define BAUDRATE_38400 98

#define ACIS_TOGGLE 0
#define ACIS_FALLING 2
#define ACIS_RISING 3

void sigma_init();
void sigma_reset();

ISR(ANA_COMP_vect)
{
	pfmt_out(PSTR("acmp!"));
}

void sigma_init(void)
{
	DIDR0 |= _BV(AIN1D);

	const uint8_t amode = ACIS_FALLING;
	const uint8_t acsr = _BV(ACBG)
		| (amode&2) ? _BV(ACIS1) : 0 
		| (amode&1) ? _BV(ACIS0) : 0
		;
	ACSR = acsr;
	ACSR |= acsr | _BV(ACI) | _BV(ACIE);

	sigma_reset();
}

void sigma_reset(void)
{
	// precharge
	SENSOR_PORT |= _BV(SENSOR_PIN);
	SENSOR_DDR |= _BV(SENSOR_PIN);
	_delay_us(1);	
}

void sigma_start()
{
}

int main()
{
#ifndef NDEBUG
	uart_init(BAUDRATE_38400);
	pfmt_out(PSTR("\r\nMCUSR: "));
	pfmt_print_bits(PSTR("PEBW"), 0);
#endif

	

	return 0;
}
