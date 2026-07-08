/*
* Copyright 2024, 2026 NXP
*
* SPDX-License-Identifier: 0BSD
*/

#ifndef _PRNG_32_H_
#define _PRNG_32_H_

#include <stdint.h>

void xoroshiro64starstar_init_state(uint64_t seed);
uint32_t xoroshiro64starstar_next(void);

#endif /* _PRNG_32_H_ */
