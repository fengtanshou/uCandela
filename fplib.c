#include "fplib.h"
#include <stdio.h>

typedef int8_t smallint_t;
typedef uint8_t u_smallint_t;

static const uint8_t precision_bits = 12;
typedef uint16_t sig_type;

#if 0
/* faster & smaller division with only 8-bit precision
 * does not work properly for now
 */
static const uint8_t precision_bits = 8;
typedef uint8_t sig_type;
#endif

fp16_t fp_inverse(const fp16_t f, int8_t extra_shift)
{
	/* constants */
	const sig_type precision_mask = 1<<(precision_bits-1);

	/* run a simplified restoring division algorithm
	 * with numerator=0x10000
	 * to obtain inverse of the significand
	 */
	const sig_type sig = fp_mask_sig(f)+1;
	sig_type partial = 1;
	sig_type result = 0;

	/* shift is the (strictly positive) number of extra left shifts
	 * that must be applied to the result to position binary point correctly
	 */
	smallint_t shift;

	/* total sweep is 2*precision_bits (double the precision of significand) */
	for(shift = FP16_EXPONENT_MAX - 1 + precision_bits;
	    shift != FP16_EXPONENT_MAX - 1 - precision_bits; 
	    --shift)
	{
		partial <<= 1;
		result <<= 1;
		if ( partial >= sig )
		{
			partial -= sig;
			result |= 1;
		}
		/* terminate if desired precision is achieved */
		if ( result & precision_mask ) break;
	}

	/* perform exponent computations */
	const smallint_t exp = shift - fp_extract_exp(f) + extra_shift;
	return fp_normalize(result, exp);
}

fp16_t fp_normalize(uint16_t sig, int8_t exp)
{
	for (; exp < 0; ++exp ) sig >>= 1;
	for (; exp > FP16_EXPONENT_MAX; --exp)
	{
		sig <<= 1;
		if ( sig >= FP16_MANTISSA_MAX ) /* saturate */
			return fp_compose(FP16_MANTISSA_MAX, FP16_EXPONENT_MAX);
	}
	return fp_compose(sig, exp);
}
