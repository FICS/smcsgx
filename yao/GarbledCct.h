#ifndef GARBLEDCCT_H_
#define GARBLEDCCT_H_

//#include <emmintrin.h>

#include "Env.h"
#include "Prng.h"


class GarbledCct
{
public:
	GarbledCct()  {countrd=0;lastmwsize=0;}
	~GarbledCct() { /*delete [] m_w;*/ }

	void gen_init(const std::vector<Bytes> &keys, const Bytes &gen_inp_mask, const Bytes &seed);
	void gen_next_gate(const Gate &g);

	void com_init(const std::vector<Bytes> &keys, const Bytes &gen_inp_mask, const Bytes &seed);
	void com_next_gate(const Gate &g);

	void evl_init(const std::vector<Bytes> &keys, const Bytes &masked_gen_inp, const Bytes &evl_inp);
	void evl_next_gate(const Gate &g);

private:
	void update_hash(const Bytes &data);

	Bytes               m_hash;

	//__m128i             m_R;
	Bytes 		    m_R_b;


	const std::vector<Bytes>  *m_ot_keys;
	std::vector<Bytes>  m_C;

	Prng                m_prng;

	uint64_t            m_gate_ix;

	uint32_t            m_gen_inp_ix;
	uint32_t            m_evl_inp_ix;
	uint32_t            m_gen_out_ix;
	uint32_t            m_evl_out_ix;
	uint32_t            m_sgx_inp_ix;
	uint32_t            m_sgx_out_ix;
	
	//__m128i *           m_w;
	//__m128i             m_clear_mask;

	vector<Bytes>	    m_w_b;
	int **              m_w;
	int lastmwsize;

	Bytes		    m_clear_mask_b;

public:
	std::vector<Bytes>  *m_sgx_keys;
	std::vector<Bytes>  *sgx_keys_out;
	std::vector<char> *sgx_permutes;


	size_t            	m_max_map_size;

	Bytes               m_gen_inp_mask;
	Bytes               m_evl_inp;
	Bytes               m_gen_out;
	Bytes               m_evl_out;

	std::vector<Z>      m_m;
	std::vector<G>      m_M;

	std::vector<Bytes>      new_m_M;

	Bytes X_b[2], Y_b[2], Z_b[2];
	Bytes aes_key_b[2], aes_plaintext_b, aes_ciphertext_b;
	Bytes addtmp;
	Bytes current_key_b, a_b;

	Bytes aes_key_0,aes_key_1;

	char buffer[200000];
	unsigned int buffersize;


	long long countrd;


void createinput();

private:
	Bytes               m_o_bufr;
	Bytes               m_i_bufr;
	Bytes::iterator     m_i_bufr_ix;

public:
	const Bytes hash() const
	{
#ifdef RAND_SEED
		return m_hash.hash(Env::k());
#else
		return m_hash;
#endif
	}


	uint8_t * recvdataptr; 
	uint32_t count;

	void recv(Bytes &i_data)
	{
		//m_i_bufr.clear();
		//m_i_bufr += i_data;
		//m_i_bufr_ix = m_i_bufr.begin();
		//buffersize = i_data.size();
		//std::cout <<"recvnewdata\n";
		count = 0;
		recvdataptr = &( i_data[0]);
	}

	const Bytes send()
	{
		static Bytes o_data;
		//o_data.swap(m_o_bufr);
		//m_o_bufr.clear();
	
		buffersize = 0;
		return o_data;
	}
};

#define  _mm_extract_epi8(x, imm) \
	((((imm) & 0x1) == 0) ?   \
	_mm_extract_epi16((x), (imm) >> 1) & 0xff : \
	_mm_extract_epi16( _mm_srli_epi16((x), 8), (imm) >> 1))


Bytes KDF128(const Bytes &in, const Bytes &key);
Bytes KDF256(const Bytes &in, const Bytes &key);

void KDF128(const uint8_t *in, uint8_t *out, const uint8_t *key);
void KDF256(const uint8_t *in, uint8_t *out, const uint8_t *key);

#endif /* GARBLEDCCT_H_ */
