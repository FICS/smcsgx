#ifndef PRNG_H_
#define PRNG_H_

#include <openssl/aes.h>

#include "Bytes.h"

class Prng {

	Bytes   m_state;
	AES_KEY m_key;

public:
	static int cnt;
	static const char *RANDOM_FILE;

	Prng() { srand(); }
	Prng(const Bytes &seed) { srand(seed); }
	virtual ~Prng() {}

	void srand();
	void srand(const Bytes &seed);
	Bytes rand();
	Bytes rand(size_t bits);
	uint64_t rand_range(uint64_t n);  // sample a number from { 0, 1, ..., n-1 }
};

#endif /* PRNG_H_ */
