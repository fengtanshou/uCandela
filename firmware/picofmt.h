#ifndef PICOFMT_H_INC
#define PICOFMT_H_INC

#include <avr/pgmspace.h>
#include <stdint.h>

#define PICO_MAX_ARGS 4

extern uint16_t g_picofmt_args[PICO_MAX_ARGS];

/*
 * Output a CR-LF sequence
 *
 */
void pfmt_crlf(void);

/*
 * Output a string replacing $0..$9 with g_picofmt_args
 *
 */
extern void pfmt_out(const prog_char *fmt);

/*
 * Output a newline-terminated string replacing $0..$9 with g_picofmt_args
 *
 */
extern void pfmt_outln(const prog_char *fmt);

/*
 * Output a string representing individual bits states
 *
 * - bit_names are in LSB to MSB order
 */
extern void pfmt_print_bits(const prog_char *bit_names, uint8_t value);

#define FOUT0(msg) pfmt_outln(PSTR(msg))

#define FOUT1(msg, arg0) do {			\
		g_picofmt_args[0] = arg0;	\
		FOUT0(msg);			\
	} while ( 0 )				\

#define FOUT2(msg, arg0, arg1) do {		\
		g_picofmt_args[1] = arg1;	\
		FOUT1(msg, arg0);		\
	} while ( 0 )				\

#define FOUT3(msg, arg0, arg1, arg2) do {	\
		g_picofmt_args[2] = arg2;	\
		FOUT2(msg, arg0, arg1);		\
	} while ( 0 )				\

#define FOUT4(msg, arg0, arg1, arg2, arg3) do {	\
		g_picofmt_args[3] = arg3;	\
		FOUT3(msg, arg0, arg1, arg2);	\
	} while ( 0 )				\

#endif // PICOFMT_H_INC
