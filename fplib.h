/* library of small inline functions to simulate
 * simple floating point functionality
 */

#ifndef FPLIB_H_INC
#define FPLIB_H_INC

#include <stdint.h>

typedef uint16_t fp16_t;

#define FP16_EXPONENT_MAX 0xF
#define FP16_MAX 0x10000000UL
#define FP16_MIN 0x0

#ifndef FORCEINLINE
#define FORCEINLINE __attribute__((always_inline))
#endif

inline fp16_t fp_compose(uint16_t mantissa, uint8_t exponent) FORCEINLINE;
inline uint16_t fp_extract_sig(fp16_t f) FORCEINLINE;
inline uint8_t fp_extract_exp(fp16_t f) FORCEINLINE;
inline fp16_t fp_reciprocal(fp16_t f) FORCEINLINE;
inline uint32_t fp_to_uint32(fp16_t f) FORCEINLINE;
inline uint16_t fp_to_uint16(fp16_t f) FORCEINLINE;

inline fp16_t fp_compose(uint16_t mantissa, uint8_t exponent)
{
	exponent &= 0xF;
	return (mantissa&0xFFF0) | exponent;
}

inline uint16_t fp_extract_sig(fp16_t f)
{
	return (f&0xFFF0);
}

inline uint8_t fp_extract_exp(fp16_t f)
{
	return (f&0xF);
}

inline fp16_t fp_reciprocal(fp16_t f)
{
	const uint16_t sig = fp_extract_sig(f);
	const uint8_t exp = fp_extract_exp(f);
	return fp_compose(FP16_MAX/sig, 0x10-exp-1);
}

inline uint32_t fp_to_uint32(fp16_t f)
{
	return (((uint32_t)fp_extract_sig(f))>>4) << fp_extract_exp(f);
}

/* saves only top 16 bits out of 24 full precision */
/*
 * fp: sssE
 * u32: 0xxxZZZZ (Z - can be zero depending on exponent)
 * u16: xxxZ
 */
inline uint16_t fp_to_uint16(fp16_t f)
{
	return ((uint32_t)fp_extract_sig(f)) >> (0x10-fp_extract_exp(f));
}

#endif /* FPLIB_H_INC */
