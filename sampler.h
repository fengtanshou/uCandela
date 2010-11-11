/**
 *  basic sampling routines
 */

#ifndef SAMPLER_H_INC
#define SAMPLER_H_INC

#define INPUT_PIN PB1
#define INPUT_DB  DDB1
#define INPUT_PORT PORTB
#define INPUT_DDR  DDRB

#include <stdint.h>

void sampler_init(void);
uint16_t sampler_get_next_sample(void);

#endif /* SAMPLER_H_INC */
