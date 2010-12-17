/*
 *
 * HID API definition for uCandela sensor v 1.0
 *
 */

#ifndef UC_API_H_INC
#define UC_API_H_INC

#include <stdint.h>

#define UCD_FEATURE_REPORT_COUNT 8

/* size MUST be less than8 bytes */
typedef struct
{
	uint8_t subrq_id;
}ucd_mux_request_type;

typedef struct
{
	uint8_t scale;
	/* some other parameters to follow */
}ucd_parameters_request_type;

typedef struct ucd_calibration_param
{
	char id[4];
	uint16_t param[2];
}ucd_calibration_request_type;

/*
 * 
 */
#define UCD_SUBRQ_MUX_REPORT_ID 1
#define UCD_SUBRQ_PARAMETERS 1
#define UCD_SUBRQ_CALIBRATION_SET_0 2
#define UCD_SUBRQ_CALIBRATION_SET_1 3
#define UCD_SUBRQ_CALIBRATION_SET_2 4
#define UCD_SUBRQ_CALIBRATION_SET_3 5

/*
 *
 */
#define UCD_SUBRQ_DATA_REPORT_ID 2

#endif /* UC_API_H_INC */

