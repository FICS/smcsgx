#ifndef BETTERYAO_H_
#define BETTERYAO_H_

#include "YaoBase.h"
#include "GarbledCct.h"

class BetterYao : public YaoBase
{
public:
	BetterYao(EnvParams &params);
	virtual ~BetterYao() {}
	virtual void start();

	void initSGXProgram();
	void runSGXProgram(string filename,vector<Bytes> * input, vector<Bytes> * output, vector<char> * permutes);



private:
	void oblivious_transfer();
	void cut_and_choose();
	void circuit_commit();
	void consistency_check();
	void circuit_evaluate();

	void proc_gen_out();
	void proc_evl_out();

	// variables for IKNP03 OT-extension implementation
	G                      m_ot_g[2];
	G                      m_ot_h[2];

	vector<vector<Bytes> > m_ot_keys; // ot output

//	vector<Bytes> sgx_keys_in;
//	vector<Bytes> sgx_keys_out;

	// variables for cut-and-choose
	Bytes                  m_chks;
	Bytes                  m_all_chks;

	// variables for Yao protocol
	vector<Bytes>          m_gen_inp_masks;
	vector<Bytes>          m_coms;
	vector<Bytes>          m_rnds;
	vector<GarbledCct>     m_ccts;

	vector<vector<Bytes> > m_gen_inp_com;

	void BM_OT_ext_with_third(const size_t k, const size_t l);
	void oblivious_transfer_with_inputs_gen_third_SINGLE(int lengthOfInOuts);

	size_t                          m_ot_bit_cnt;
	vector<Bytes>                   m_ot_send_pairs;

	 vector<Bytes> CircuitInput0CircuitBase;	

	// subroutines for OT-extension implementation
//	uint64_t ot_init();
//	uint64_t ot(uint32_t l);
//	uint64_t ot_random(); // sender has m pairs of l-bit strings, and receiver has m bits
//	uint64_t ot_ext_random(const size_t sigma, const size_t k, const size_t l);
//	void ot_free();
//
//	uint64_t proc_evl_in();
//	uint64_t comb_evl_in();
//
//	void inv_proc_evl_in();
//
//	size_t                          m_ot_bit_cnt;
//	Bytes                           m_ot_recv_bits;
//	vector<Bytes>                   m_ot_send_pairs;
//	vector<Bytes>                   m_ot_out;
//
//	size_t                          m_ot_ext_bit_cnt;
//	Bytes                           m_ot_ext_recv_bits;
//	vector<Bytes>                   m_ot_ext_out;
//	// variables for PSSW09 Random Combination Input technique
//	uint32_t                        m_evl_inp_rand_cnt;
//	Bytes                           m_evl_inp_rand;
//	vector<Bytes>                   m_evl_inp_selector;
};
#endif
