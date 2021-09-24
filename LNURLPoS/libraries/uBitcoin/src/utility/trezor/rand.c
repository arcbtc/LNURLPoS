/**
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "rand.h"
#include "sha2.h"
#include <string.h>

// #ifndef RAND_PLATFORM_INDEPENDENT

static uint32_t seed = 0;
static uint8_t hash[32];

/* 
 * On boot there is random some device-dependent junk in the RAM
 * It may depend on the platform, but in most cases it can be used
 * to get some device-specific randomness.
 * This is a junky-code that may be not perfect
 * but works much better than normal non-cryptographic PRNGs
 * 
 * Replace the random32() function with your own secure code.
 * There is also a possibility to replace the random_buffer() function 
 * as it is defined as a weak symbol.
 */

static void init_ram_seed(){
	uint8_t * arr = (uint8_t *)malloc(1000); // just allocate some memory
	memcpy(arr, hash, 32); // to maintain previous entropy, kinda
	sha256_Raw(arr, 1000, hash);
	free(arr);
	seed++;
}

void random_reseed(const uint32_t value)
{
	seed = value;
}

uint32_t __attribute__((weak)) random32(void){
	if(seed == 0){
		init_ram_seed();
	}
	SHA256_CTX	context;
	sha256_Init(&context);
	sha256_Update(&context, hash, 32);
	sha256_Update(&context, (uint8_t *)&seed, 4);
	sha256_Final(&context, hash);
	uint32_t * results = (uint32_t *)hash;
	seed = results[0];
	return results[1];
}

// #endif /* RAND_PLATFORM_INDEPENDENT */

//
// The following code is platform independent
//

void __attribute__((weak)) random_buffer(uint8_t *buf, size_t len)
{
	uint32_t r = 0;
	for (size_t i = 0; i < len; i++) {
		if (i % 4 == 0) {
			r = random32();
		}
		buf[i] = (r >> ((i % 4) * 8)) & 0xFF;
	}
}

uint32_t random_uniform(uint32_t n)
{
	uint32_t x, max = 0xFFFFFFFF - (0xFFFFFFFF % n);
	while ((x = random32()) >= max);
	return x / (max / n);
}

void random_permute(char *str, size_t len)
{
	for (int i = len - 1; i >= 1; i--) {
		int j = random_uniform(i + 1);
		char t = str[j];
		str[j] = str[i];
		str[i] = t;
	}
}
