/*
* Copyright 2018, 2020 NXP
*
* NXP Confidential. This software is owned or controlled by NXP and may only
* be used strictly in accordance with the applicable license terms.  By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*/

/**
 @file
 @brief Rational number handling functions
*/
#ifndef _RATIONAL_H_
#define _RATIONAL_H_

struct rational {
	/* Stores a positive rational number in the form: a = i + p/q, with p/q < 1 */
	unsigned int i; /* integer part */
	unsigned int p; /* fractional part numerator */
	unsigned int q; /* fractional part denominator */
};

void rational_init(struct rational *r, unsigned long long p, unsigned int q);
void rational_add(struct rational *r, struct rational *r1, struct rational *r2);
void rational_div(struct rational *r, struct rational *r1, struct rational *r2);
int rational_cmp(struct rational *r1, struct rational *r2);

/**
 * rational_int_mul() - multiplies an unsigned integer by a rational
 */
static inline unsigned int rational_int_mul(unsigned int i, struct rational *r)
{
	return (i * r->i) + (i * r->p) / r->q;
}

/**
 * rational_int_mul2() - multiplies a rational by an unsigned integer
 */
static inline void rational_int_mul2(struct rational *r, struct rational *r1, unsigned int i)
{
	rational_init(r, i * ((unsigned long long)r1->i * r1->q + r1->p), r1->q);
}

/**
 * rational_int_div() - divides a rational by an unsigned integer
 */
static inline void rational_int_div(struct rational *r, struct rational *r1, unsigned int i)
{
	rational_init(r, (unsigned long long)r1->i * r1->q + r1->p, i * r1->q);
}


/**
 * rational_int_cmp() - compares an unsigned integer and a rational
 */
static inline int rational_int_cmp(unsigned int i, struct rational *r)
{
	if (((int)i - (int)r->i) > 0)
		return 1;
	else if (((int)i - (int)r->i) < 0)
		return -1;
	else if (r->p == 0)
		return 0;
	else
		return -1;
}


/**
 * rational_int_add() - adds an unsigned integer with a rational
 */
static inline void rational_int_add(struct rational *r, unsigned int i, struct rational *r1)
{
	r->i = i + r1->i;
	r->p = r1->p;
	r->q = r1->q;
}


#endif /* _RATIONAL_H_ */
