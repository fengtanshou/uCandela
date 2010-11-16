/**
 *  basic sampling routines
 */

#ifndef SAMPLER_H_INC
#define SAMPLER_H_INC

#define INPUT_PIN PB1
#define INPUT_DB  DDB1
#define INPUT_PORT PORTB
#define INPUT_DDR  DDRB

#include "fplib.h"

void sampler_init(void);
void sampler_start(void);
fp16_t sampler_get_next_sample(void);
uint8_t sampler_poll(void);
fp16_t sampler_get_sample(void);
fp16_t sampler_get_next_sample(void);

#endif /* SAMPLER_H_INC */
