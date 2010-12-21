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

fp16_t fp_inverse(fp16_t f)
{
	/* constants */
	const sig_type precision_mask = 1<<(precision_bits-1);

	/* run a simplified restoring division algorithm
	 * with numerator=0x10000
	 * to obtain inverse of the significand
	 */
	sig_type sig = fp_extract_sig(f)+1;
	sig_type partial = 1;
	sig_type result = 0;
	u_smallint_t shift = 0;

	 /* HACK: this is necessary for number > 0x80 due to limited width of CPU regs */
	if ( sig > ( ((sig_type)-1) / 2 ) )
	{
		sig >>= 1;
		shift = 1;
	}

//	printf("\t --- divide by %02x\n", sig);
	for(; shift!=precision_bits*2; ++shift)
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
	/* shift is the (strictly positive) number of extra right shifts
	 * that must be applied to the result to position binary point correctly
	 */
	shift -= precision_bits-1;
	/* result is always in range 0 - 0xFFF due to precision limit */

	/* perform exponent computations */
	smallint_t exp = - fp_extract_exp(f) - shift + FP16_EXPONENT_MAX;
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
