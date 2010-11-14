#include "eeprom.h"
#include <avr/io.h>
#include <util/atomic.h>


typedef uint8_t intr_atomic_t;

enum eeprom_io_flags
{
	eif_default = 0,
	eif_inprogress = 1<<0,
};

static volatile struct
{
	const void *src;
	void *dst;
	eesize_t length;
	eesize_t pos;
	intr_atomic_t flags;
}io = 
{
	.pos = 0,
	.length = 0,
	.flags = eif_default,
};

inline static void __write_next_byte(void)
{
	const uint8_t eepm = (0<<EEPM1) | (0<<EEPM0); // erase-then-write
	const uint8_t eecr = eepm | _BV(EERIE);
	EEAR =   (uint16_t)(io.dst + io.pos);
	EEDR = *((uint8_t*)(io.src + io.pos));
	EECR = eecr | _BV(EEMPE);
	EECR = eecr | _BV(EEMPE) | _BV(EEPE);
}

ISR(EE_RDY_vect)
{
	/* disable eeprom */
	if ( io.pos == io.length )
	{
		io.flags = eif_default;
		EECR = 0;
		return;
	}

	__write_next_byte();
	++io.pos;
}

eesize_t eeprom_write_block_intr(const void *src,void *dst, eesize_t size)
{
	size_t r = -1U;
	if ( io.flags & eif_inprogress )
		goto exit;

	io.src = src;
	io.dst = dst;
	io.length = size;
	io.pos = 0;
	io.flags = eif_default | eif_inprogress;

	EECR = _BV(EERIE);
exit:
	return r;
}

uint8_t eeprom_is_complete()
{
	if ( io.flags & eif_inprogress )
		return 0;
	return !0;
}
