/* peripherials */
#include "compiler.h"
#include "sampler.h"

/* include proper usb driver headers */
#ifndef USBDRV
#error no USB driver selected
#endif

#if USBDRV == vusb
#include "usbdrv/usbdrv.h"
#else /* USBDRV */
#error USB driver not supported
#endif /* USBDRV */

/* library headers */
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

NOINIT uint8_t mcusr_mirror;

INIT_FUNC_3 void early_init(void)
{
	mcusr_mirror = MCUSR;
	wdt_disable();
}

INIT_FUNC_8 void late_init(void)
{
	usbInit();
}

USB_PUBLIC usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	/* TODO: use USB_NO_MSG to implement data writing */
	return 0;
}

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len)
{
	/* TODO: return 1 if all data consumed */
	return 0;
}

USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len)
{
	return 0;
}

#if defined(__GNUC__) && defined(__AVR__)
int main(void) __attribute__((OS_main));
#endif
int main(void)
{
	usbDeviceDisconnect();
	_delay_ms(250);
	usbDeviceConnect();

	sei();

	for(;;)
	{
		usbPoll();
		if ( usbInterruptIsReady() )
		{
			usbSetInterrupt(0/*TODO*/, 0/*TODO*/);
		}
	}
}
