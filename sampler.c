#include "sampler.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define COUNT_MAX 0xFF

enum capture_mode {
	capture_mode_rising,
	capture_mode_falling,
};

static const uint8_t prescaler_min = 1;
static const uint8_t prescaler_max = 15;
static const uint8_t acsr_acis_falling = _BV(ACIS1);
static const uint8_t acsr_acis_rising = (_BV(ACIS1) | _BV(ACIS0));
static const uint8_t acsr_acis_toggle = 0;
static const uint8_t underflow_threshold = 0x10;
static const uint8_t overflow_threshold  = 0xF0;

extern uint8_t sampler_value;
static uint8_t s_prescaler __attribute__((section(".noinit")));


/* timer control */

static inline void
timer1_disable(void)
{
	TCCR1 = 0; // stop clock
	TIFR = _BV(TOV1); // clear pending interrupts
}

static inline uint8_t
timer1_is_running(void)
{
	return TCCR1 & (_BV(CS13)|_BV(CS12)|_BV(CS11)|_BV(CS10));
}

static inline void
timer1_run(uint8_t prescaler)
{
	TCNT1 = 0;
	TCCR1 = 0
		| ( prescaler & 8 ? _BV(CS13) : 0 )
		| ( prescaler & 4 ? _BV(CS12) : 0 )
		| ( prescaler & 2 ? _BV(CS11) : 0 )
		| ( prescaler & 1 ? _BV(CS10) : 0 )
		;
}

static inline void
timer1_enable(uint8_t prescaler)
{
	GTCCR |= _BV(PSR1);
	TIFR = _BV(TOV1);
	TIMSK |= _BV(TOIE1);

	timer1_run(prescaler);
}

/* input pin control */

// hard pull input pin up
static inline void
input_precharge_high(void)
{
	INPUT_PORT |= _BV(INPUT_PIN);
	INPUT_DDR |= _BV(INPUT_DB);
}

// hard pull input pin down
static inline void
input_precharge_low(void)
{
	INPUT_PORT &= ~_BV(INPUT_PIN);
	INPUT_DDR |= _BV(INPUT_DB);
}

// let input pin float
static inline void
input_discharge(void)
{
	INPUT_DDR &= ~_BV(INPUT_DB);
	INPUT_PORT &= ~_BV(INPUT_PIN);
}


/* comparator control */

static inline void
comp_enable(void)
{
	ACSR |= _BV(ACI) | _BV(ACIE);
}

static inline void
comp_disable(void)
{
	ACSR &= ~_BV(ACIE);
	ACSR |= _BV(ACI);
}

static inline void
comp_set_mode(enum capture_mode mode)
{
	ACSR = 0
		| _BV(ACBG)
		| ( capture_mode_rising == mode ? acsr_acis_rising : acsr_acis_falling )
		;
}

/* capture control */

static inline void
capture_stop(void)
{
	comp_disable();
	timer1_disable();
	input_precharge_high();
}

static void
capture_start(uint8_t speed_index)
{
	comp_enable();
	timer1_run(speed_index);
	input_discharge();
}

static void
capture_reset(enum capture_mode mode)
{
	input_precharge_high();

	comp_set_mode(mode);
	timer1_enable(0);
}

/* sampler api */
void
sampler_init(void)
{
	s_prescaler = prescaler_max;
	capture_reset(capture_mode_rising);
}

void sampler_start(void)
{
	capture_start(s_prescaler);
}

uint8_t  sampler_poll(void)
{
	/* check if it is still running */
	if ( timer1_is_running() )
		return 0;

	/* completed, stop and recharge */
	capture_stop();

	/* adjust for next measurement */
	const uint8_t ps = s_prescaler;
	if ( sampler_value >= overflow_threshold )
	{
		s_prescaler = ( ps >= prescaler_max ? prescaler_max : ps+1 );
	}
	if ( sampler_value < underflow_threshold )
	{
		s_prescaler = ( ps <= prescaler_min ? prescaler_min : ps-1 );
	}
	return 1;
}

fp16_t sampler_get_sample(void)
{
	/* return adjusted result based on prescaler */
	return fp_compose( ((uint16_t)sampler_value)<<8, s_prescaler - 1 );
}

fp16_t
sampler_get_next_sample(void)
{
	capture_start(s_prescaler);

	while ( !sampler_poll() ) sleep_cpu();

	return sampler_get_sample();
}
