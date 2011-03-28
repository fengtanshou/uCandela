#include <stdio.h>
#include "fplib.h"

#define PFP(value) printf("%s = %04x\n", #value, value);

fp16_t fpdiv(fp16_t fp1, fp16_t fp2)
{
	return fp_compose( 0x10*fp_extract_sig(fp1) / fp_extract_sig(fp2),
			   fp_extract_exp(fp1) - fp_extract_exp(fp2) - 1
		);
}

fp16_t fpmul(fp16_t fp1, fp16_t fp2)
{
	const uint32_t mul = fp_extract_sig(fp1) * fp_extract_sig(fp2);
	const uint8_t exp = fp_extract_exp(fp1) + fp_extract_exp(fp2);
	return fp_compose( (mul << (0x10-exp))>>16, exp);
}

void conversion(fp16_t value)
{
	fp16_t rv = fp_inverse(value, 0);
	fp16_t srv = fp_inverse(value, 10);
	unsigned int iv = fp_to_uint16(srv);
	printf("%04x - %04x - %04x - %04x\n", value, rv, srv, iv);
}


void test_division(void)
{
	PFP(FP16_MAX);
	fp16_t fp1 = fp_compose(0x100, 1);
	PFP(fp1);
	fp16_t fp2 = fp_compose(0x080, 0);
	PFP(fp2);
	fp16_t r = fpdiv(fp1, fp2);
	PFP(r);
	fp16_t test = fpdiv(fp1, r);
	PFP(test);
}

//#define IMAX ((FP16_MANTISSA_MAX+1UL)<<(FP16_EXPONENT_MAX))
#define IMAX 0x100000000ULL
void test_reciprocal_(fp16_t fp)
{
	printf("\nTesting %04x\n", fp);

	const fp16_t rec = fp_inverse(fp, 0);

	printf("\tin float: ONE / %04x = %04x int16=%04x ( %04x * %04x = %04x )\n",
	       fp, rec,
	       fp_to_uint16(rec),
	       fp, rec, fpmul(fp, rec));

	const uint32_t ifp = fp_to_uint32(fp+FP16_MANTISSA_MIN);
	printf("\tin int: %09llx / %08x = %08x ( %08x * %08x = %09lx )\n",
	       IMAX, ifp, fp_to_uint32(rec),
	       ifp, fp_to_uint32(rec), fp_to_uint32(fpmul(fp,rec)));

	const uint32_t irec = (IMAX / ifp);
	printf("\tverify: %09llx / %08x = %08x ( %08x * %08x = %09lx)\n",
	       IMAX, ifp, irec,
	       ifp, irec, ifp*irec);
}

void test_reciprocal(void)
{
#if 0
	test_reciprocal_(0x0020);
	test_reciprocal_(0x0100);
	test_reciprocal_(0x456);
#endif
#if 1
	int exp = 0x0;
	unsigned int i;
	for(i=0; i!=4096; ++i)
	{
		const fp16_t fp = fp_compose(i<<4, exp);
		test_reciprocal_(fp);
	}
#endif
}

void test_integer_print(fp16_t fp)
{
	printf("fp=%04x i32=%08x i16l=%04x\n",
	       fp,
	       fp_to_uint32(fp),
	       fp_to_uint16(fp));
}

void test_integer(void)
{
	int i;
	test_integer_print(0);
	for(i=0; i!=FP16_EXPONENT_MAX+1; ++i)
		test_integer_print( fp_compose(0xABC0, i) );
	test_integer_print(0xFFFF);
}


uint16_t idiv_r(uint16_t n, uint16_t d)
{
	uint32_t n32 = (uint32_t)n;
	uint32_t d32 = ((uint32_t)d)<<16;
	uint32_t r = 0;
	int i;
	for(i=31; i>=0; --i)
	{
		n32 <<= 1;
		printf("i=%02d n=%08x d=%08x", i, n32, d32);
		if ( n32 >= d32 )
		{
			n32 -= d32;
			r |= (1<<i);
		}
		printf(" --> n32=%08x r=%08x\n", n32, r);
	}
	return r>>16;
}

uint16_t idiv_nr(uint16_t n, uint16_t d)
{
	uint32_t res = 0;
	uint32_t n32 = n;
	uint32_t d32 = d<<16;
	int i;

	for(i=0; i!=32; ++i)
	{
		n32<<=1;
		res<<=1;
		printf("%02d: p=%08x", i, n32);
		if ( (n32 & 0x80000000) == 0 ) // positive
		{
			printf(" (+)");
			res++;
			n32 -= d32;
		}
		else
		{
			printf(" (-)");
			res--;
			n32 += d32;
		}
		printf(" --> p=%08x r=%08x\n", n32, res);
	}
	return res>>16;
}

uint16_t idiv_r_16(uint16_t n, uint16_t d)
{
	uint16_t tmp = 0;
	uint16_t r = 0;
	int i;
	for(i=0; i!=32; ++i)
	{
		printf("i=%02d Ni=%04x", i, n);
		// transfer bit to tmp
		tmp = (tmp<<1) | (n&0x8000?1:0);
		n<<=1;
		r<<=1;
		printf(" tmp=%04x n=%04x d=%04x", tmp, n, d);
		if ( tmp >= d )
		{
			tmp -= d;
			r |= 1;
		}
		printf(" --> tmp=%04x r=%04x\n", tmp, r);
		if ( r & 0x800 ) break;
	}
	printf("result=%r extra shift = %d\n", i - 16);
	return r;
}

uint16_t idiv_inverse(uint16_t d)
{
	uint16_t tmp = 1;
	uint16_t result = 0;

	uint8_t shift;
	d >>= 1;
	for(shift=0; shift!=31 && !(result&(1<<15)); ++shift)
	{
		tmp <<= 1;
		result <<= 1;
		printf("\ti=%d tmp=%04x D=%04x ", shift, tmp, d);
		if ( tmp >= d )
		{
			tmp -= d;
			result |= 1;
		}
		printf("newtmp=%04x result=%04x\n", tmp, result);
//		if ( result & (1<<15) ) break;
	}
	shift += 1;
	// account for divisor /2
//	result>>=1;
	printf("\tFINAL result=%04x shift = %d (%d) corrected result=%04x verify=%05x verify2=%05x\n",
	       result, shift-16, shift, result >> (shift-16),
	       (result >> (shift-16))* (d<<1),
	       (result * d)>>(shift-17));
	return result;
}

uint16_t idiv_inverse_8(uint8_t d)
{
	printf("Dividing 0x100 by %03x\n", d);
#if 1
	uint8_t tmp = 1;
	uint8_t D = d;
	if ( D >= 0x80 ) D >>= 1;
#else
	uint16_t tmp = 1;
	uint16_t D = d;
#endif
	uint8_t result = 0;
	uint8_t shift = 0;
	for(; shift!=16; ++shift)
	{
		tmp <<= 1;
		result <<= 1;
		printf("\ti=%d tmp=%04x D=%04x ", shift, tmp, D);
		if ( tmp >= D )
		{
			tmp -= D;
			result |= 1;
		}
		printf("newtmp=%04x result=%04x\n", tmp, result);
		if ( result & (1<<7) ) break;
	}
	printf("\tFINAL result=%04x shift = %d (%d)\n", result, shift-7, shift);
	return result;
}

void test_inverse_(uint16_t n)
{
	const uint16_t r = idiv_inverse(n);
	//const uint16_t r = idiv_inverse_8(n);
	printf("%05x / %04x = %04x\n\n", 0x10000UL, n, r);
}

void test_idiv()
{
	test_inverse_(0x0010);
	test_inverse_(0x0020);
	test_inverse_(0x0300);

	test_inverse_(0x0DE0);
	test_inverse_(0x2000);
	test_inverse_(0xBAC0);
	test_inverse_(0x1024);
	test_inverse_(0x7AFE);
}

void test_conversion()
{
	conversion(0x0100);
	conversion(0x1000);
	conversion(0x1001);
	conversion(0x2002);
	  conversion(0x2102);
	  conversion(0x2202);
	  conversion(0x2302);
	conversion(0x2004);
	conversion(0x2008);
	conversion(0x200a);
	conversion(0x200b);
	conversion(0x200c);
	conversion(0x200d);
	conversion(0x200e);
	conversion(0xff0e);
}

int main()
{
//	test_idiv();
//	test_division();
//	test_integer();

//	test_reciprocal();
	test_conversion();

#if 0
	int i;
	for(i=0; i!=16; ++i)
	{
		const fp16_t fp = fp_compose(0x7800, i);
		const uint32_t u32 = fp_to_uint32(fp);
		const uint16_t u16 = fp_to_uint16(fp);
		printf("%04x %08x %04x\r\n", fp, u32, u16);
	}
#endif
}
