
#include "GarbledCct.h"


void GarbledCct::gen_init(const vector<Bytes> &ot_keys, const Bytes &gen_inp_mask, const Bytes &seed)
{
	m_ot_keys = &ot_keys;
	m_gen_inp_mask = gen_inp_mask;
	m_prng.srand(seed);

	// R is a random k-bit string whose 0-th bit has to be 1
	static Bytes tmp;

	tmp = m_prng.rand(Env::k());
	tmp.set_ith_bit(0, 1);
	tmp.resize(16, 0);
	m_R = tmp;//= _mm_loadu_si128(reinterpret_cast<const __m128i*>(&tmp[0]));

	m_gate_ix = 0;

	m_gen_inp_ix = 0;
	m_evl_inp_ix = 0;
	m_gen_out_ix = 0;
	m_evl_out_ix = 0;

	m_o_bufr.clear();

	/*if (m_w == 0)
	{
		m_w = new //vec//__m128i[Env::circuit().m_cnt];
	}*/
    m_w.resize(Env::circuit().m_cnt);

	tmp.assign(16, 0);
	for (size_t ix = 0; ix < Env::k(); ix++) tmp.set_ith_bit(ix, 1);
	m_clear_mask = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));

    m_m.resize(Env::circuit().gen_inp_cnt()*2);
	m_M.resize(Env::circuit().gen_inp_cnt()*2);

	Z m0, m1;

	// init group elements associated with the generator's input bits
	for (size_t ix = 0; ix < Env::circuit().gen_inp_cnt(); ix++)
	{
		m0.random(m_prng);
		m1.random(m_prng);

		m_m[2*ix+0] = m0;
		m_m[2*ix+1] = m1;

		m_M[2*ix+0] = Env::clawfree().F(0, m0);
		m_M[2*ix+1] = Env::clawfree().F(1, m1);
	}
}


const int CIRCUIT_HASH_BUFFER_SIZE = 10*1024*1024;


void GarbledCct::com_init(const vector<Bytes> &ot_keys, const Bytes &gen_inp_mask, const Bytes &seed)
{
	gen_init(ot_keys, gen_inp_mask, seed);
	m_hash.reserve(CIRCUIT_HASH_BUFFER_SIZE+64);
	m_hash.clear();
}


void GarbledCct::evl_init(const vector<Bytes> &ot_keys, const Bytes &masked_gen_inp, const Bytes &evl_inp)
{
	m_ot_keys = &ot_keys;
	m_gen_inp_mask = masked_gen_inp;
	m_evl_inp = evl_inp;

	m_C.resize(Env::circuit().gen_out_cnt()*2);

	m_evl_out.resize((Env::circuit().evl_out_cnt()+7)/8);
	m_gen_out.resize((Env::circuit().gen_out_cnt()+7)/8);

	m_gate_ix = 0;

	m_gen_inp_ix = 0;
	m_evl_inp_ix = 0;
	m_gen_out_ix = 0;
	m_evl_out_ix = 0;

	m_i_bufr.clear();

	/*if (m_w == 0)
	{
		m_w = new __m128i[Env::circuit().m_cnt];
	}*/
    	m_w.resize(Env::circuit().m_cnt);
    
	static Bytes tmp;

	tmp.assign(16, 0);
	for (size_t ix = 0; ix < Env::k(); ix++) tmp.set_ith_bit(ix, 1);
	m_clear_mask = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));

	m_hash.clear();
}


void GarbledCct::gen_next_gate(const Gate &current_gate)
{
	Bytes current_zero_key;

	if (current_gate.m_tag == Circuit::GEN_INP)
	{
		Bytes a[2];

        	current_zero_key = m_prng.rand(Env::k());
        	current_zero_key.resize(16,0);
		static Bytes tmp;

		/*tmp = m_prng.rand(Env::k());
		tmp.resize(16, 0);
		current_zero_key = _mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));*/

		a[0] = m_M[2*m_gen_inp_ix+0].to_bytes().hash(Env::k());
		a[0].resize(16,0);
		/*tmp = m_M[2*m_gen_inp_ix+0].to_bytes().hash(Env::k());
		tmp.resize(16, 0);
		a[0] = _mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));*/

		a[1] = m_M[2*m_gen_inp_ix+1].to_bytes().hash(Env::k());
		a[1].resize(16,0);
		/*tmp = m_M[2*m_gen_inp_ix+1].to_bytes().hash(Env::k());
		tmp.resize(16, 0);
		a[1] = _mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));*/

		a[0] ^= current_zero_key;
        	a[1] ^= current_zero_key ^ m_R;
		//a[0] = _mm_xor_si128(a[0], current_zero_key);
		//a[1] = _mm_xor_si128(a[1], _mm_xor_si128(current_zero_key, m_R));

		uint8_t bit = m_gen_inp_mask.get_ith_bit(m_gen_inp_ix);

		//m_o_bufr += a[bit];
		//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), a[bit]);
        	tmp = a[bit];
		m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());

        //m_o_bufr += a[1-bit];
		//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), a[1-bit]);
		tmp = a[1-bit];
        	m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());

		m_gen_inp_ix++;
	}
	else if (current_gate.m_tag == Circuit::EVL_INP)
	{
		Bytes a[2];

        	current_zero_key = m_prng.rand(Env::k());
        	current_zero_key.resize(16,0);
		static Bytes tmp;

		/*tmp = m_prng.rand(Env::k());
		tmp.resize(16, 0);
		current_zero_key = _mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));*/

        	a[0] = (*m_ot_keys)[2*m_evl_inp_ix+0];
		a[0].resize(16,0);
		/*tmp = (*m_ot_keys)[2*m_evl_inp_ix+0];
		tmp.resize(16, 0);
		a[0] = _mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));*/

        	a[1] = (*m_ot_keys)[2*m_evl_inp_ix+1];
		a[1].resize(16,0);
		/*tmp = (*m_ot_keys)[2*m_evl_inp_ix+1];
		tmp.resize(16, 0);
		a[1] = _mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));*/

        	a[0] ^= current_zero_key; 
        	a[1] ^= current_zero_key ^ m_R;
		/*a[0] = _mm_xor_si128(a[0], current_zero_key);
		a[1] = _mm_xor_si128(a[1], _mm_xor_si128(current_zero_key, m_R));*/

        	//m_o_bufr += a[0];
		//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), a[0]);
       		tmp = a[0];
		m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());

		//m_o_bufr += a[1];
		//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), a[1]);
        	tmp = a[1];
		m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());

		m_evl_inp_ix++;
	}
	else
	{
		const vector<uint64_t> &inputs = current_gate.m_input_idx;
		assert(inputs.size() == 1 || inputs.size() == 2);

#ifdef FREE_XOR
		if (is_xor(current_gate))
		{
			/*current_zero_key = inputs.size() == 2?
				_mm_xor_si128(m_w[inputs[0]], m_w[inputs[1]]) : _mm_load_si128(m_w+inputs[0]);*/
            		current_zero_key = inputs.size() == 2? (m_w[inputs[0]] ^ m_w[inputs[1]]) : (m_w[inputs[0]]);
		}
		else
#endif
		if (inputs.size() == 2) // 2-arity gates
		{
			uint8_t bit;
			Bytes aes_key[2], aes_ciphertext;
			Bytes X[2], Y[2], Z[2];
			static Bytes tmp(16, 0);

			Bytes aes_plaintext;
			aes_plaintext.setBytes(m_gate_ix);

			X[0] = m_w[inputs[0]];
			Y[0] = m_w[inputs[1]];

			//X[1] = _mm_xor_si128(X[0], m_R); // X[1] = X[0] ^ R
			//Y[1] = _mm_xor_si128(Y[0], m_R); // Y[1] = Y[0] ^ R
            	X[1] = X[0] ^ m_R;
            	Y[1] = Y[0] ^ m_R;

			const uint8_t perm_x = (X[0].get_ith_bit(0)) & 0x01; // permutation bit for X
			const uint8_t perm_y = (Y[0].get_ith_bit(0)) & 0x01; // permutation bit for Y
			const uint8_t de_garbled_ix = (perm_y<<1)|perm_x;

			// encrypt the 0-th entry : (X[x], Y[y])
			aes_key[0] = X[perm_x];
			aes_key[1] = Y[perm_y];

			//KDF256((uint8_t*)&aes_plaintext, (uint8_t*)&aes_ciphertext, (uint8_t*)aes_key);
            		aes_ciphertext = KDF256(aes_plaintext,aes_key[0]+aes_key[1]);
            
			//aes_ciphertext = _mm_and_si128(aes_ciphertext, m_clear_mask); // clear extra bits so that only k bits left
			aes_ciphertext = aes_ciphertext && m_clear_mask;
            
            		bit = current_gate.m_table[de_garbled_ix];

//GRR
			// GRR technique: using zero entry's key as one of the output keys
           		Z[bit] = aes_ciphertext;
			//_mm_store_si128(Z+bit, aes_ciphertext);
            
            //Z[1-bit] = _mm_xor_si128(Z[bit], m_R);
            		Z[1-bit] = Z[bit] ^ m_R;
            
			current_zero_key = Z[0];//_mm_load_si128(Z);
//end GRR
            

			// encrypt the 1st entry : (X[1-x], Y[y])
			//aes_key[0] = _mm_xor_si128(aes_key[0], m_R);
            		aes_key[0] = aes_key[0] ^ m_R;

			aes_ciphertext = KDF256(aes_plaintext,  aes_key[0]+aes_key[1]);
            
			//aes_ciphertext = _mm_and_si128(aes_ciphertext, m_clear_mask);
            		aes_ciphertext = aes_ciphertext && m_clear_mask;
			bit = current_gate.m_table[0x01^de_garbled_ix];
            
			aes_ciphertext = aes_ciphertext ^ Z[bit];//_mm_xor_si128(aes_ciphertext, Z[bit]);
            
			//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), aes_ciphertext);
            		tmp = aes_ciphertext;
			m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());

			// encrypt the 2nd entry : (X[x], Y[1-y])
			aes_key[0] = aes_key[0] ^ m_R;//_mm_xor_si128(aes_key[0], m_R);
			aes_key[1] = aes_key[1] ^ m_R;//_mm_xor_si128(aes_key[1], m_R);

			//KDF256((uint8_t*)&aes_plaintext, (uint8_t*)&aes_ciphertext, (uint8_t*)aes_key);
            		aes_ciphertext = KDF256(aes_plaintext,  aes_key[0]+aes_key[1]);
            
			aes_ciphertext = aes_ciphertext && m_clear_mask;//_mm_and_si128(aes_ciphertext, m_clear_mask);
			bit = current_gate.m_table[0x02^de_garbled_ix];
			aes_ciphertext = aes_ciphertext ^ Z[bit];//_mm_xor_si128(aes_ciphertext, Z[bit]);
            
			tmp = aes_ciphertext;//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), aes_ciphertext);
            
			m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());

			// encrypt the 3rd entry : (X[1-x], Y[1-y])
			aes_key[0] = aes_key[0] ^ m_R;//_mm_xor_si128(aes_key[0], m_R);

			aes_ciphertext = KDF256(aes_plaintext,  aes_key[0]+aes_key[1]);//KDF256((uint8_t*)&aes_plaintext, (uint8_t*)&aes_ciphertext, (uint8_t*)aes_key);
            
			aes_ciphertext = aes_ciphertext && m_clear_mask;//_mm_and_si128(aes_ciphertext, m_clear_mask);
			bit = current_gate.m_table[0x03^de_garbled_ix];
			aes_ciphertext = aes_ciphertext ^ Z[bit];//_mm_xor_si128(aes_ciphertext, Z[bit]);
			tmp = aes_ciphertext;//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), aes_ciphertext);
			m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());
		}
		else // 1-arity gates
		{
			uint8_t bit;
			Bytes aes_key,  aes_ciphertext;
			Bytes X[2], Z[2];
			static Bytes tmp;

			tmp.assign(16, 0);

			Bytes aes_plaintext;
			aes_plaintext.setBytes(m_gate_ix);// = _mm_set1_epi64x(m_gate_ix);

			X[0] = m_w[inputs[0]];//_mm_load_si128(m_w+inputs[0]);
			X[1] = X[0] ^ m_R;//_mm_xor_si128(X[0], m_R);

			const uint8_t perm_x = X[0].get_ith_bit(0) & 0x01;

			// 0-th entry : X[x]
			aes_key = X[perm_x];//_mm_load_si128(X+perm_x);
			//KDF128((uint8_t*)&aes_plaintext, (uint8_t*)&aes_ciphertext, (uint8_t*)&aes_key);
            		aes_ciphertext = KDF128(aes_plaintext,aes_key);
			aes_ciphertext = aes_ciphertext && m_clear_mask;//_mm_and_si128(aes_ciphertext, m_clear_mask);
			bit = current_gate.m_table[perm_x];

#ifdef GRR
			Z[bit] = aes_ciphertext;//_mm_store_si128(Z+bit, aes_ciphertext);
			Z[1-bit] = Z[bit]^m_R;//_mm_xor_si128(Z[bit], m_R);
			current_zero_key = Z[0];//_mm_load_si128(Z);
#else
			tmp = m_prng.rand(Env::k());
			tmp.resize(16, 0);
			Z[0] = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));
			Z[1] = Z[0]^m_R;//_mm_xor_si128(Z[0], m_R);

			aes_ciphertext = aes_ciphertext ^ Z[bit];//_mm_xor_si128(aes_ciphertext, Z[bit]);
			tmp = aes_ciphertext;//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), aes_ciphertext);
			m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());
#endif

			// 1-st entry : X[1-x]
			aes_key = aes_key^m_R;//_mm_xor_si128(aes_key, m_R);

			//KDF128((uint8_t*)&aes_plaintext, (uint8_t*)&aes_ciphertext, (uint8_t*)&aes_key);
            		aes_ciphertext = KDF128(aes_plaintext,aes_key);
			aes_ciphertext = aes_ciphertext && m_clear_mask;//_mm_and_si128(aes_ciphertext, m_clear_mask);
			bit = current_gate.m_table[0x01^perm_x];
			aes_ciphertext = aes_ciphertext ^ Z[bit];//_mm_xor_si128(aes_ciphertext, Z[bit]);
            
            		tmp = aes_ciphertext;
			//_mm_storeu_si128(reinterpret_cast<__m128i*>(&tmp[0]), aes_ciphertext);
			m_o_bufr.insert(m_o_bufr.end(), tmp.begin(), tmp.begin()+Env::key_size_in_bytes());
		}

		if (current_gate.m_tag == Circuit::EVL_OUT)
		{
			m_o_bufr.push_back( current_zero_key.get_ith_bit(0) & 0x01); // permutation bit
		}
		else if (current_gate.m_tag == Circuit::GEN_OUT)
		{
			m_o_bufr.push_back(current_zero_key.get_ith_bit(0) & 0x01); // permutation bit

//			// TODO: C[ix_0] = w[ix0] || randomness, C[ix_1] = w[ix1] || randomness
//			m_o_bufr += (key_pair[0] + m_prng.rand(Env::k())).hash(Env::k());
//			m_o_bufr += (key_pair[1] + m_prng.rand(Env::k())).hash(Env::k());
		}
	}

	//_mm_store_si128(m_w+current_gate.m_idx, current_zero_key);
    	m_w[current_gate.m_idx] = current_zero_key;
    
	m_gate_ix++;
}

void GarbledCct::update_hash(const Bytes &data)
{
	m_hash += data;

#ifdef RAND_SEED
	if (m_hash.size() > CIRCUIT_HASH_BUFFER_SIZE) // hash the circuit by chunks
	{
		Bytes temperary_hash = m_hash.hash(Env::k());
		m_hash.clear();
		m_hash += temperary_hash;
	}
#endif
}

void GarbledCct::com_next_gate(const Gate &current_gate)
{
	gen_next_gate(current_gate);
	update_hash(m_o_bufr);
	m_o_bufr.clear(); // flush the output buffer
}


void GarbledCct::evl_next_gate(const Gate &current_gate)
{
	Bytes current_key, a;
	Bytes::const_iterator it;
	static Bytes tmp;

	if (current_gate.m_tag == Circuit::GEN_INP)
	{
		uint8_t bit = m_gen_inp_mask.get_ith_bit(m_gen_inp_ix);
		Bytes::iterator it = m_i_bufr_ix + bit*Env::key_size_in_bytes();

		tmp = m_M[m_gen_inp_ix].to_bytes().hash(Env::k());
		tmp.resize(16, 0);
		current_key = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));

		tmp.assign(it, it+Env::key_size_in_bytes());
		tmp.resize(16, 0);
		a = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));

		m_i_bufr_ix += Env::key_size_in_bytes()*2;

		current_key = current_key^a;//_mm_xor_si128(current_key, a);

		m_gen_inp_ix++;
	}
	else if (current_gate.m_tag == Circuit::EVL_INP)
	{
		uint8_t bit = m_evl_inp.get_ith_bit(m_evl_inp_ix);
		Bytes::iterator it = m_i_bufr_ix + bit*Env::key_size_in_bytes();

		tmp = (*m_ot_keys)[m_evl_inp_ix];
		tmp.resize(16, 0);
		current_key = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));

		tmp.assign(it, it+Env::key_size_in_bytes());
		tmp.resize(16, 0);
		a = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));

		m_i_bufr_ix += Env::key_size_in_bytes()*2;

		current_key = current_key^a;//_mm_xor_si128(current_key, a);

		m_evl_inp_ix++;
	}
	else
	{
        const vector<uint64_t> &inputs = current_gate.m_input_idx;

#ifdef FREE_XOR
		if (is_xor(current_gate))
		{
			//current_key = inputs.size() == 2?
			//	_mm_xor_si128(m_w[inputs[0]], m_w[inputs[1]]) : _mm_load_si128(m_w+inputs[0]);
            
            		current_key = inputs.size() == 2? m_w[inputs[0]]^m_w[inputs[1]] : m_w[inputs[0]];
		}
		else
#endif
        if (inputs.size() == 2) // 2-arity gates
		{
        		Bytes aes_key[2], aes_ciphertext;

			Bytes aes_plaintext;
			aes_plaintext.setBytes(m_gate_ix); //_mm_set1_epi64x(m_gate_ix);

			aes_key[0] =  m_w[inputs[0]];//_mm_load_si128(m_w+inputs[0]);
			aes_key[1] =  m_w[inputs[1]];//_mm_load_si128(m_w+inputs[1]);

			const uint8_t perm_x = aes_key[0].get_ith_bit(0)&0x01;//_mm_extract_epi8(aes_key[0], 0) & 0x01;
			const uint8_t perm_y = aes_key[1].get_ith_bit(0)&0x01;//_mm_extract_epi8(aes_key[1], 0) & 0x01;

			//KDF256((uint8_t*)&aes_plaintext, (uint8_t*)&aes_ciphertext, (uint8_t*)aes_key);
            		aes_ciphertext = KDF256(aes_plaintext,aes_key[0]+aes_key[1]);
			aes_ciphertext = aes_ciphertext^m_clear_mask;//_mm_and_si128(aes_ciphertext, m_clear_mask);
			uint8_t garbled_ix = (perm_y<<1) | (perm_x<<0);


			if (garbled_ix == 0)
			{
				current_key = aes_ciphertext;//_mm_load_si128(&aes_ciphertext);
			}
			else
			{
				it = m_i_bufr_ix+(garbled_ix-1)*Env::key_size_in_bytes();
				tmp.assign(it, it+Env::key_size_in_bytes());
				tmp.resize(16, 0);
				a = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));
				current_key = aes_ciphertext^a;//_mm_xor_si128(aes_ciphertext, a);
			}
			m_i_bufr_ix += 3*Env::key_size_in_bytes();

		}
		else // 1-arity gates
		{
        		Bytes aes_key,  aes_ciphertext;

			Bytes aes_plaintext;
			aes_plaintext.setBytes(m_gate_ix);

			aes_key = m_w[inputs[0]];//_mm_load_si128(m_w+inputs[0]);
			//KDF128((uint8_t*)&aes_plaintext, (uint8_t*)&aes_ciphertext, (uint8_t*)&aes_key);
           		aes_ciphertext = KDF128(aes_plaintext, aes_key);
            
			aes_ciphertext = aes_ciphertext && m_clear_mask;//_mm_and_si128(aes_ciphertext, m_clear_mask);

			const uint8_t perm_x = aes_key.get_ith_bit(0)&0x01;//_mm_extract_epi8(aes_key, 0) & 0x01;


			if (perm_x == 0)
			{
				current_key = aes_ciphertext;//_mm_load_si128(&aes_ciphertext);
			}
			else
			{
				tmp.assign(m_i_bufr_ix, m_i_bufr_ix+Env::key_size_in_bytes());
				tmp.resize(16, 0);
				a = tmp;//_mm_loadu_si128(reinterpret_cast<__m128i*>(&tmp[0]));
				current_key = aes_ciphertext^a;//_mm_xor_si128(aes_ciphertext, a);
			}
			m_i_bufr_ix += Env::key_size_in_bytes();

		}

		if (current_gate.m_tag == Circuit::EVL_OUT)
		{
			uint8_t out_bit = current_key.get_ith_bit(0) & 0x01;
			out_bit ^= *m_i_bufr_ix;
			m_evl_out.set_ith_bit(m_evl_out_ix, out_bit);
			m_i_bufr_ix++;

			m_evl_out_ix++;
		}
		else if (current_gate.m_tag == Circuit::GEN_OUT)
		{
			// TODO: Ki08 implementation
			uint8_t out_bit = current_key.get_ith_bit(0) & 0x01;
			out_bit ^= *m_i_bufr_ix;
			m_gen_out.set_ith_bit(m_gen_out_ix, out_bit);
			m_i_bufr_ix++;

//			m_C[2*m_gen_out_ix+0] = Bytes(m_i_bufr_ix, m_i_bufr_ix+Env::key_size_in_bytes());
//			m_i_bufr_ix += Env::key_size_in_bytes();
//
//			m_C[2*m_gen_out_ix+1] = Bytes(m_i_bufr_ix, m_i_bufr_ix+Env::key_size_in_bytes());
//			m_i_bufr_ix += Env::key_size_in_bytes();

			m_gen_out_ix++;
		}
	}

	m_w[current_gate.m_idx]=current_key;//_mm_store_si128(m_w+current_gate.m_idx, current_key);

	update_hash(m_i_bufr);
	m_gate_ix++;
}
