#include "fplib.h"

fp16_t fp_inverse(fp16_t f)
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

fp16_t fp_shift(fp16_t fp, int8_t shift)
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
