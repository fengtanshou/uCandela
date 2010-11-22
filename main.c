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

PROGMEM char usbHidReportDescriptor[31] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x27, 0xff, 0xff, 0xff, 0x7f,  //   LOGICAL_MAXIMUM (2147483647)
    0x35, 0x00,                    //   PHYSICAL_MINIMUM (0)
    0x47, 0x40, 0x42, 0x0f, 0x00,  //   PHYSICAL_MAXIMUM (1000000)
    0x67, 0xe1, 0x00, 0x00, 0x01,  //   UNIT (SI Lin:0x10000e1)
    0x75, 0x20,                    //   REPORT_SIZE (32)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0xc0                           // END_COLLECTION
};


typedef struct usbHidReport
{
	uint32_t value;
}report_type;

NOINIT static report_type report;
NOINIT static uint8_t idleRate;

USB_PUBLIC usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;

	/* The following requests are never used. But since they are required by
	 * the specification, we implement them in this example.
	 */
	switch ( rq->bmRequestType & USBRQ_TYPE_MASK )
	{
	case USBRQ_TYPE_CLASS:
		//DBG1(0x50, &rq->bRequest, 1);   /* debug output: print our request */
		switch ( rq->bRequest )
		{
		case USBRQ_HID_GET_REPORT:
			/* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* we only have one report type, so don't look at wValue */
			usbMsgPtr = (void *)&report;
			report.value++; /* simply count requests for now */
			return sizeof(report);
		case USBRQ_HID_GET_IDLE:
			usbMsgPtr = &idleRate;
			return 1;
		case USBRQ_HID_SET_IDLE:
			idleRate = rq->wValue.bytes[1];
			return 0;
		}
		break;
	default: /* no vendor specific requests implemented */
		break;
	}
	/* default for not implemented requests: return no data back to host */
	return 0;
#if 0
	/* TODO: use USB_NO_MSG to implement data writing */
	return 0;
#endif
}

#ifdef USB_CFG_IMPLEMENT_FN_WRITE
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len)
{
	/* TODO: return 1 if all data consumed */
	return 0;
}
#endif

#ifdef USB_CFG_IMPLEMENT_FN_READ
USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len)
{
	return 0;
}
#endif


INIT_FUNC_3 void early_init(void)
{
	mcusr_mirror = MCUSR;
	wdt_disable();
}

INIT_FUNC_8 void late_init(void)
{
	usbInit();
}

#if defined(__GNUC__) && defined(__AVR__)
int main(void) __attribute__((OS_main));
#endif
int main(void)
{
	report.value = 0;

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
