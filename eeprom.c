#include "eeprom.h"
#include <avr/io.h>
#include <util/atomic.h>


#define EECR_PM ( (0<<EEPM1) | (0<<EEPM0) )

typedef volatile uint8_t intr_atomic_t;

enum eeprom_io_flags
{
	eif_default = 0,
	eif_inprogress = 1<<0,
};

static struct
{
	const void *src;
	void *dst;
	size_t length;
	intr_atomic_t flags;
}io;

inline static void __write_next_byte(void)
{
	EECR = EECR_PM | _BV(EERIE);

	EEAR =  (uint16_t)io.dst++;
	EEDR = *(uint8_t*)io.src++;
	--io.length;

	EECR |= _BV(EEMPE);
	EECR |= _BV(EEPE);
}

ISR(EE_RDY_vect)
{
	/* stop operation */
	if ( !io.length )
	{
		io.flags = eif_default;
		EECR = 0;
		return;
	}

	/* continue */
	__write_next_byte();
}

size_t eeprom_write_block_intr(const void *src,void *dst, size_t size)
{
	size_t r = -1U;
	if ( io.flags & eif_inprogress )
		goto exit;

	io.src = src;
	io.dst = dst;
	io.length = size;
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
