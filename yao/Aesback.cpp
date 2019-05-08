#include <stdint.h>
#include <cassert>

#include "Bytes.h"

#if !defined (ALIGN16)
# if defined (__GNUC__)
#  define ALIGN16  __attribute__  ( (aligned (16)))
# else
#  define ALIGN16 __declspec (align (16))
# endif
#endif

extern "C"
{
void AES_128_Key_Expansion(const uint8_t *userkey, uint8_t *key_schedule);
void AES_192_Key_Expansion(const uint8_t *userkey, uint8_t *key_schedule);
void AES_256_Key_Expansion(const uint8_t *userkey, uint8_t *key_schedule);

void AES_ECB_decrypt (const uint8_t *in, uint8_t *out, unsigned long length, const uint8_t *KS, int nr);
void AES_ECB_encrypt (const uint8_t *in, uint8_t *out, unsigned long length, const uint8_t *KS, int nr);
};

#if defined AESNI

//
// When AES-NI is available, KDF(key, in) = AES_key (in), where
// in = m_gate_ix and key = X||Y.
//

#include <wmmintrin.h>

void KDF128(const uint8_t *in, uint8_t *out, const uint8_t *key)
{
    ALIGN16 uint8_t KEY[16*11];
    ALIGN16 uint8_t PLAINTEXT[64];
    ALIGN16 uint8_t CIPHERTEXT[64];

    AES_128_Key_Expansion(key, KEY);
    _mm_storeu_si128(&((__m128i*)PLAINTEXT)[0],*(__m128i*)in);
    AES_ECB_encrypt(PLAINTEXT, CIPHERTEXT, 64, KEY, 10);
    _mm_storeu_si128((__m128i*)out,((__m128i*)CIPHERTEXT)[0]);
}

void KDF256(const uint8_t *in, uint8_t *out, const uint8_t *key)
{
    ALIGN16 uint8_t KEY[16*15];
    ALIGN16 uint8_t PLAINTEXT[64];
    ALIGN16 uint8_t CIPHERTEXT[64];

    AES_256_Key_Expansion(key, KEY);
    _mm_storeu_si128(&((__m128i*)PLAINTEXT)[0],*(__m128i*)in);
    AES_ECB_encrypt(PLAINTEXT, CIPHERTEXT, 64, KEY, 14);
    _mm_storeu_si128((__m128i*)out,((__m128i*)CIPHERTEXT)[0]);
}

#else

//
// If none of the above is available, KDF(key, in) = H(key) = SHA-256(key),
// where key = X||Y.
//

#include <openssl/evp.h>
#include <openssl/sha.h>

void KDF128(const uint8_t *in, uint8_t *out, const uint8_t *key)
{
	SHA256_CTX sha256;

	SHA256_Init(&sha256);
	SHA256_Update(&sha256, key, 16);
	SHA256_Final(out, &sha256);
}

void KDF256(const uint8_t *in, uint8_t *out, const uint8_t *key)
{
	SHA256_CTX sha256;

	SHA256_Init(&sha256);
	SHA256_Update(&sha256, key, 32);
	SHA256_Final(out, &sha256);
}
#endif

Bytes KDF128(const Bytes &msg, const Bytes &key)
{
	assert(msg.size() == 16);
	assert(key.size() == 16);

	static Bytes out(32);
	KDF128(&msg[0], &out[0], &key[0]);
	return out;
}

Bytes KDF256(const Bytes &msg, const Bytes &key)
{
	assert(msg.size() == 16);
	assert(key.size() == 32);

	static Bytes out(32);
	KDF256(&msg[0], &out[0], &key[0]);
	return out;
}
