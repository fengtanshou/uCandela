/* library of small inline functions to simulate
 * simple floating point functionality
 */

#ifndef FPLIB_H_INC
#define FPLIB_H_INC

#include <stdint.h>
#include <assert.h>

typedef uint16_t fp16_t;

#define FP16_MANTISSA_OFFSET 0
#define FP16_MANTISSA_MASK   0xFFF
#define FP16_MANTISSA_MAX    0xFFF
#define FP16_EXPONENT_OFFSET 12
#define FP16_EXPONENT_MASK   0x0F
#define FP16_EXPONENT_MAX    0x0F

#define FP16_MAX (FP16_MANTISSA_MAX << FP16_EXPONENT_MAX)
#define FP16_MIN 0x0

#ifndef FORCEINLINE
#define FORCEINLINE __attribute__((always_inline))
#endif

inline fp16_t fp_compose(uint16_t mantissa, uint8_t exponent) FORCEINLINE;
inline uint16_t fp_extract_sig(fp16_t f) FORCEINLINE;
inline uint8_t fp_extract_exp(fp16_t f) FORCEINLINE;
inline uint32_t fp_to_uint32(fp16_t f) FORCEINLINE;
inline uint32_t fp_to_uint32_high(fp16_t f) FORCEINLINE;
inline uint16_t fp_to_uint16(fp16_t f) FORCEINLINE;
inline uint16_t fp_to_uint16_high(fp16_t f) FORCEINLINE;
inline fp16_t fp_shift(fp16_t fp, int8_t shift);

/* computes 2**extra_shift/f */
fp16_t fp_inverse(fp16_t f, int8_t extra_shift);
fp16_t fp_normalize(uint16_t sig, int8_t exp);

FORCEINLINE inline fp16_t fp_mask_sig(fp16_t f)
{
	return f & (FP16_MANTISSA_MASK<<FP16_MANTISSA_OFFSET);
}

FORCEINLINE inline fp16_t fp_mask_exp(fp16_t f)
{
	return f & (FP16_EXPONENT_MASK<<FP16_EXPONENT_OFFSET);
}

FORCEINLINE inline fp16_t fp_compose_masked(fp16_t mantissa, fp16_t exponent)
{
	return mantissa | exponent;
}

inline fp16_t fp_compose(uint16_t mantissa, uint8_t exponent)
{
	return  0
		| ((mantissa & FP16_MANTISSA_MASK) << FP16_MANTISSA_OFFSET)
		| ((exponent & FP16_EXPONENT_MASK) << FP16_EXPONENT_OFFSET);
}

inline uint16_t fp_extract_sig(fp16_t f)
{
	return (f >> FP16_MANTISSA_OFFSET) & FP16_MANTISSA_MASK;
}

inline uint8_t fp_extract_exp(fp16_t f)
{
	return (f >> FP16_EXPONENT_OFFSET) & FP16_EXPONENT_MASK;
}


inline uint32_t fp_to_uint32_high(fp16_t f)
{
	return (((uint32_t)fp_extract_sig(f))) << (fp_extract_exp(f)+4+1);
}

inline uint32_t fp_to_uint32(fp16_t f)
{
	return (((uint32_t)fp_extract_sig(f))) << fp_extract_exp(f);
}

inline uint16_t fp_to_uint16_high(fp16_t f)
{
	return (fp_extract_sig(f)<<4) >> (FP16_EXPONENT_MAX-fp_extract_exp(f));
}

inline uint16_t fp_to_uint16(fp16_t f)
{
	return fp_extract_sig(f) << fp_extract_exp(f);
}


inline fp16_t fp_shift(fp16_t fp, int8_t shift)
{
	const uint16_t sig = fp_extract_sig(fp);
	const int8_t exp = fp_extract_exp(fp) + shift;

	return fp_normalize(sig, exp);
}

#endif /* FPLIB_H_INC */
