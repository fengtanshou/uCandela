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

/* definitions */
#define MAX_FEATURE_DATA 5
#define MAX_FEATURES 2

#define USBRQ_HID_REPORT_TYPE_INPUT 1
#define USBRQ_HID_REPORT_TYPE_OUTPUT 2
#define USBRQ_HID_REPORT_TYPE_FEATURE 3

/* globals */
typedef struct usbHidFeatures
{
	uint8_t data[MAX_FEATURE_DATA];
}feature_type;

NOINIT static uint16_t report;
NOINIT static feature_type features[MAX_FEATURES];
NOINIT uint8_t mcusr_mirror;

/* we use combined report, INPUT returns data, FEATURE allows for controlling of parameters */
PROGMEM char usbHidReportDescriptor[63] = {
	/* input part */
	0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined Page 1)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x27, 0xff, 0xff, 0x00, 0x00,  //   LOGICAL_MAXIMUM (65535)
	0x35, 0x00,                    //   PHYSICAL_MINIMUM (0)
	0x47, 0xff, 0xff, 0x00, 0x00,  //   PHYSICAL_MAXIMUM (65535)
	0x55, 0x0E,                    //   UNIT_EXPONENT (-2)
	0x67, 0xe1, 0x00, 0x00, 0x01,  //   UNIT (SI Lin:0x10000e1)
	0x75, 0x10,                    //   REPORT_SIZE (16)
	0x95, 0x01,                    //   REPORT_COUNT (1)
	0x09, 0x01,                    //   USAGE(vendor usage 1)
	0x82, 0x62, 0x01,              // INPUT (Data,Var,Abs,NPrf,Null,Buf)

	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x85, 0x01,                    //   REPORT_ID(0)
	0x95, 0x04,                    //   REPORT_COUNT (4)
	0x09, 0x00,                    //   USAGE(Undefined)
	0xb2, 0x02, 0x01,              //   FEATURE(Data,Var,Abs,Buf)

	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x85, 0x02,                    //   REPORT_ID(1)
	0x95, 0x04,                    //   REPORT_COUNT (4)
	0x09, 0x00,                    //   USAGE(Undefined)
	0xb2, 0x02, 0x01,              //   FEATURE(Data,Var,Abs,Buf)

	0xc0,

#if 0
	/* feature part */
	0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined Page 1)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x95, 0x04,                    //   REPORT_COUNT (4)
	0x09, 0x00,                    //   USAGE(Undefined)
	0xb2, 0x02, 0x01,              //   FEATURE(Data,Var,Abs,Buf)
	0xc0,                          // END_COLLECTION
#endif
//	0xc0,                           // END_COLLECTION
};


/*
 * USB part
 */
USB_PUBLIC usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *)data;
	const uint8_t report_type = rq->wValue.bytes[1]; /* wValue: ReportType (highbyte) */
	const uint8_t report_id = rq->wValue.bytes[0]; /* ReportID (lowbyte) */

	switch ( rq->bmRequestType & USBRQ_TYPE_MASK )
	{
	case USBRQ_TYPE_CLASS:
		switch ( rq->bRequest )
		{
		case USBRQ_HID_GET_REPORT:
			switch ( report_type ) 
			{
			case USBRQ_HID_REPORT_TYPE_INPUT:
				usbMsgPtr = (void *)&report;
				return sizeof(report);
			case USBRQ_HID_REPORT_TYPE_FEATURE:
				if ( report_id-1 < MAX_FEATURES )
				{
					features[report_id-1].data[0] = report_id;
					usbMsgPtr = (void*)&(features[report_id-1].data);
					return MAX_FEATURE_DATA;
				}
				break;
			}
			break;
		case USBRQ_HID_SET_REPORT:
			if ( USBRQ_HID_REPORT_TYPE_FEATURE == report_type ) /* wValue: ReportType (highbyte) */
			{
			}
			break;
		}
		break;
	default: /* no vendor specific requests implemented */
		break;
	}
	/* default for not implemented requests: return no data back to host */
	return 0;
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



/*
 * Initialization and entry point
 */

INIT_FUNC_3 void early_init(void)
{
	mcusr_mirror = MCUSR;
	wdt_disable();
}

INIT_FUNC_8 void late_init(void)
{
	usbInit();
	sampler_init();
	report = 0;
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

	sampler_start();
	for(;;)
	{
		usbPoll();
		if ( usbInterruptIsReady() )
			usbSetInterrupt((void*)&report, sizeof(report));

		if ( sampler_poll() )
		{
			const uint16_t fp_raw = sampler_get_sample();
			const uint16_t fp_value = fp_reciprocal(fp_raw);
			report = fp_to_uint16(fp_value);
			sampler_start();
		}
	}
}
