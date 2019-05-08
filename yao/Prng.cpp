#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <openssl/sha.h>

#include "Prng.h"

const char *Prng::RANDOM_FILE = "/dev/urandom";

static Bytes shen_sha256(const Bytes &data, const size_t bits)
{
	static const byte MASK[8] =
		{ 0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };

	byte buf[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, &data[0], data.size());
	SHA256_Final(buf, &sha256);

	Bytes hash(buf, buf + (bits+7)/8);
	hash.back() &= MASK[bits % 8]; // clear the extra bits

	return hash;
}


const int AES_BLOCK_SIZE_IN_BITS = AES_BLOCK_SIZE*8;

void Prng::srand()
{
	Bytes seed(AES_BLOCK_SIZE);

	FILE *fp = fopen(RANDOM_FILE, "r");
	fread(&seed[0], 1, seed.size(), fp);
	fclose(fp);

	srand(seed);
}

void Prng::srand(const Bytes &seed)
{
	Bytes hashed_seed = shen_sha256(seed, AES_BLOCK_SIZE_IN_BITS);
	memset(&m_key, 0, sizeof(AES_KEY));
	AES_set_encrypt_key(&hashed_seed[0], AES_BLOCK_SIZE_IN_BITS, &m_key);
	m_state = shen_sha256(seed, AES_BLOCK_SIZE_IN_BITS);
}


uint64_t Prng::rand_range(uint64_t n) // sample a number from { 0, 1, ..., n-1 }
{
	int bit_length = 0;
	while ((1<<bit_length)<n) { bit_length++; }

	Bytes rnd;
	std::string hex_rnd;
	char *endptr;
	uint64_t ret = 0;
	do
	{ // repeat if the sampled number is >= n
		rnd = rand(bit_length);
		hex_rnd = "0x" + rnd.to_hex();
		ret = strtoll(hex_rnd.c_str(), &endptr, 16);

	} while (ret >= n);

	return ret;
}

Bytes Prng::rand(size_t bits)
{
	static const byte MASK[8] =
		{ 0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };

	Bytes rnd;

	while (bits > 128)
	{
		rnd += rand();
		bits -= 128;
	}

	Bytes last = rand();

	last.resize((bits+7)/8);
	last.back() &= MASK[bits%8]; // clear the extra bits

	return rnd + last;
}

int Prng::cnt = 0;

Bytes Prng::rand()
{
	Bytes rnd(AES_BLOCK_SIZE);
	AES_ecb_encrypt(&m_state[0], &rnd[0], &m_key, AES_ENCRYPT);

	cnt++;

	// update state
	long long cnt = *(long long*)(&m_state[m_state.size()-1-sizeof(cnt)]);
	cnt++;
	*(long long*)(&m_state[m_state.size()-1-sizeof(cnt)]) = cnt;

	return rnd;
}
