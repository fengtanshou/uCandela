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

#if 0
PROGMEM char usbHidReportDescriptor[36] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x27, 0xff, 0xff, 0x00, 0x00,  //   LOGICAL_MAXIMUM (65535)
    0x35, 0x00,                    //   PHYSICAL_MINIMUM (0)
    0x47, 0xff, 0xff, 0x00, 0x00,  //   PHYSICAL_MAXIMUM (65535)
    0x55, 0x0E,                    //   UNIT_EXPONENT (-2)
    0x67, 0xe1, 0x00, 0x00, 0x01,  //   UNIT (SI Lin:0x10000e1)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x95, 0x02,                    //   REPORT_COUNT (4)
    0x82, 0x62, 0x01,              // INPUT (Data,Var,Abs,NPrf,Null,Buf)
    0xc0                           // END_COLLECTION
};
#endif

/* we will use feature reports to transfer arbitrary data */
PROGMEM char usbHidReportDescriptor[24] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x27, 0xff, 0xff, 0x00, 0x00,  //   LOGICAL_MAXIMUM (65535)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x95, 0x02,                    //   REPORT_COUNT (4)
    0x09, 0x00,                    //   USAGE(Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE(Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};

typedef struct usbHidReport
{
	uint16_t value;
	uint16_t fp_value;
}report_type;

NOINIT static report_type report;

USB_PUBLIC usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;

	switch ( rq->bmRequestType & USBRQ_TYPE_MASK )
	{
	case USBRQ_TYPE_CLASS:
		switch ( rq->bRequest )
		{
		case USBRQ_HID_GET_REPORT:
			/* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* we only have one report type, so don't look at wValue */
			usbMsgPtr = (void *)&report;
			return sizeof(report);
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
	sampler_init();
	report.value = 0;
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

	sampler_start();
	for(;;)
	{
		usbPoll();
		if ( sampler_poll() )
		{
			const uint16_t fp_raw = sampler_get_sample();
			report.fp_value = fp_reciprocal(fp_raw);
			report.value = fp_to_uint16(report.fp_value);
			sampler_start();
		}
#if 0
		if ( usbInterruptIsReady() )
		{
			usbSetInterrupt(0/*TODO*/, 0/*TODO*/);
		}
#endif
	}
}
