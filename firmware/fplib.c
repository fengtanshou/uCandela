#include "fplib.h"
#include <stdio.h>

typedef int8_t smallint_t;
typedef uint8_t u_smallint_t;

static const uint8_t precision_bits = 16;
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
	const sig_type sig = (fp_extract_sig(f)+FP16_MANTISSA_MIN)>>1; /* deliberately lose 1 bit of precision */
	sig_type partial = 1;
	sig_type result = 0;
	smallint_t shift;

	for(shift = precision_bits;
	    shift != -precision_bits;
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

	/* left-align the result if full precision is not used */
	result <<= sizeof(sig_type)*8 - precision_bits;

	/* perform exponent computations */
	smallint_t exp = 1 /* +1 is for divisor shifted to the right */
		+ FP16_EXPONENT_MAX - fp_extract_exp(f) /* invert the exponent of the original number */
		+ shift /* take additional precision shift into account */
		+ extra_shift;

	/* normalize.
	 * we only shift to the right, as the result is always left alighned
	 * and shifting it left will saturate the register immediately
	 */
	if ( exp > FP16_EXPONENT_MAX )
		return fp_compose(FP16_MANTISSA_MAX, FP16_EXPONENT_MAX);
	for(; exp < 0; ++exp) result >>= 1;

	/* since the exp is trimmed to [0;16) and result is left aligned
	 * we directly mask-OR them instead of using fp_compose.
	 * this saves a couple of bytes
	 */
	return (result & FP16_MANTISSA_MASK) | (u_smallint_t)exp;
}
