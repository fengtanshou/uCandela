/*
 *
 * HID API definition for uCandela sensor v 1.0
 *
 */

#ifndef UC_API_H_INC
#define UC_API_H_INC

#include <stdint.h>

#define token_paste__(a,b) a##b
#define CASSERT_impl__(cond, loc_line)			\
	typedef char token_paste__(assertion_failed_line_,loc_line)[2*(!!(cond))-1];
#define CASSERT(cond) CASSERT_impl__(cond, __LINE__)

#define UCD_FEATURE_REPORT_COUNT 16

typedef struct
{
	uint8_t subrq_id;
}ucd_mux_request_type;
CASSERT(sizeof(ucd_mux_request_type) == 1);

typedef struct
{
	int8_t sensitivity;
	uint16_t flags;
	uint8_t padding[13];
}ucd_parameters_request_type;
CASSERT(sizeof(ucd_parameters_request_type) == 16);

typedef struct ucd_calibration_param
{
	char id[4];
	uint16_t param[6];
}ucd_calibration_request_type;
CASSERT(sizeof(ucd_calibration_request_type) == 16);

/*
 * 
 */
#define UCD_SUBRQ_MUX_REPORT_ID 1
#define UCD_SUBRQ_PARAMETERS 1
#define UCD_SUBRQ_CALIBRATION_SET_0 0x10
#define UCD_SUBRQ_CALIBRATION_SET_1 0x11
#define UCD_SUBRQ_CALIBRATION_SET_2 0x12
#define UCD_SUBRQ_CALIBRATION_SET_3 0x13
#define UCD_SUBRQ_CALIBRATION_SET_4 0x14
#define UCD_SUBRQ_CALIBRATION_SET_5 0x15
#define UCD_SUBRQ_CALIBRATION_SET_6 0x16
#define UCD_SUBRQ_CALIBRATION_SET_7 0x17

/*
 *
 */
#define UCD_SUBRQ_DATA_REPORT_ID 2

#endif /* UC_API_H_INC */

