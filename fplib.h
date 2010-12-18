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
inline fp16_t fp_inverse(fp16_t f);

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

inline fp16_t fp_inverse(fp16_t f)
{
	/* constants */
	const uint8_t precision_bits = 12;
	const uint16_t sign_mask = 1<<(sizeof(uint16_t)*8-1);
	const uint16_t precision_mask = 1<<(precision_bits-1);

	/* run a simplified restoring division algorithm
	 * with numerator=0x10000
	 * to obrain inverse of the significand
	 */
	const uint16_t sig = fp_extract_sig(f)+1;
	uint16_t partial = 1;
	uint16_t result = 0;
	uint8_t shift;
	for(shift=0; shift!=precision_bits*2; ++shift)
	{
		partial <<= 1;
		result <<= 1;
		const uint16_t next = partial - sig;
		if ( ! (next & sign_mask) ) /* less than zero */
		{
			partial = next;
			result |= 1;
		}
		/* terminate if desired precision is achieved */
		if ( result & precision_mask ) break;
	}
	/* shift is the (strictly positive) number of extra right shifts
	 * that must be applied to the result to position binary point correctly
	 */
	shift -= precision_bits-1;
	/* result is always in range 0 - 0xFFF due to precision limit */

	/* perform exponent computations */
	int8_t exp = - fp_extract_exp(f) - shift + /*precision_bits*/FP16_EXPONENT_MAX;
	/* normalize */
	for (; exp < 0; ++exp )
		result >>= 1;
	return fp_compose(result, exp & 0xF);
}

inline fp16_t fp_shift(fp16_t fp, int8_t shift)
{
	uint16_t sig = fp_extract_sig(fp);
	uint16_t exp = fp_extract_exp(fp);

	exp += shift;
	for (; exp < 0; ++exp ) sig >>= 1;
	for (; exp > FP16_EXPONENT_MAX; --exp)
	{
		sig <<= 1;
		if ( sig >= FP16_MANTISSA_MAX ) /* saturate */
			return fp_compose(FP16_MANTISSA_MAX, FP16_EXPONENT_MAX);
	}
	return fp_compose(sig, exp);
}

#endif /* FPLIB_H_INC */
