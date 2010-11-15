#include "sampler.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define COUNT_MAX 0xFF

static const uint8_t prescaler_min = 1;
static const uint8_t prescaler_max = 15;
static const uint8_t acsr_acis_falling = _BV(ACIS1);
static const uint8_t acsr_acis_rising = (_BV(ACIS1) | _BV(ACIS0));
static const uint8_t acsr_acis_toggle = 0;
static const uint8_t underflow_threshold = 0x10;

enum capture_flags
{
	cf_ready = 0,
	cf_running = 1<<0,
	cf_overflow = 1<<1,
};

enum capture_mode {
	capture_mode_rising,
	capture_mode_falling,
};

static struct {
	uint8_t value;
	volatile uint8_t flags;
	uint8_t prescaler;
} capture_state;

/* timer control */

static inline void
timer1_disable(void)
{
	TCCR1 = 0; // stop clock
	TIFR = _BV(TOV1); // clear pending interrupts
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


/* capture control */

static inline void
capture_stop(void)
{
	ACSR &= ~_BV(ACIE);
	ACSR |= _BV(ACI);
}

static void
capture_start(uint8_t speed_index)
{
	capture_state.flags = cf_running;

	ACSR |= _BV(ACI) | _BV(ACIE);
	timer1_run(speed_index);
	input_discharge();
}

static void
capture_reset(enum capture_mode mode)
{
	input_precharge_high();

	ACSR = 0
		| _BV(ACBG)
		| ( capture_mode_rising == mode ? acsr_acis_rising : acsr_acis_falling )
		;
	timer1_enable(0);

	capture_state.flags = cf_ready;
}

/* interrupts */
ISR(ANA_COMP_vect)
{
	capture_state.value = TCNT1;

	timer1_disable();
	capture_stop();
	input_precharge_high();

	capture_state.flags = cf_ready;
}

ISR(TIM1_OVF_vect)
{
	capture_state.value = COUNT_MAX;

	timer1_disable();
	capture_stop();
	input_precharge_high();

	capture_state.flags = cf_overflow;
}


/* sampler api */
void
sampler_init(void)
{
	capture_state.prescaler = prescaler_max;
	capture_reset(capture_mode_rising);
}

fp16_t
sampler_get_next_sample(void)
{
	const uint8_t ps = capture_state.prescaler;

	capture_start(ps);
	/* wait for the operation to complete */
	uint8_t flags;
	do { sleep_cpu(); } while ( cf_running == (flags=capture_state.flags) );

	/* process result */
	if ( cf_overflow == flags )
	{
		capture_state.prescaler = ( ps >= prescaler_max ? prescaler_max : ps+1 );
		return UINT16_MAX;
	}
	if ( capture_state.value < underflow_threshold )
	{
		capture_state.prescaler = ( ps <= prescaler_min ? prescaler_min : ps-1 );
	}

	/* return adjusted result based on prescaler */
	return fp_compose( ((uint16_t)capture_state.value)<<8, ps-1 );
}
