#ifndef BETTERYAO2_H_
#define BETTERYAO2_H_

//#include <mpi.h>
//
//#include "Env.h"
#include "YaoBase.h"
#include "GarbledCct.h"
//#include "NetIO.h"

//using std::vector;
//
//namespace comp
//{

class BetterYao2 : public YaoBase
{
public:
	BetterYao2(EnvParams &params);
	virtual ~BetterYao2() {}

	virtual void start();

//	uint64_t ot_ext();
	void oblivious_transfer();

	void cut_and_choose();
	void cut_and_choose2();
	void consistency_check();
	void circuit_evaluate();
//	void final_report();

private:
//	void init_cluster(EnvParams &params);
//	void init_network(EnvParams &params);
//	void init_environ(EnvParams &params);
//	void init_private(EnvParams &params);
//
//	Bytes  recv_data(int src);
//	void   send_data(int dst, const Bytes &data);
//
//	// subroutines for OT-extension implementation
	void ot_init();
//	uint64_t ot(uint32_t l);
	void ot_random(); // sender has m pairs of l-bit strings, and receiver has m bits
//	uint64_t ot_ext_random(const size_t sigma, const size_t k, const size_t l);
//	void ot_free();
//
//	uint64_t proc_evl_in();
//	uint64_t comb_evl_in();
//
//	void inv_proc_evl_in();
	void proc_gen_out();
	void proc_evl_out();

	// profiling subroutines
//	void step_init();
//	void step_report(uint64_t comm_sz, std::string step_name);

	// variables for IKNP03 OT-extension implementation
	G                               m_ot_g[2];
	G                               m_ot_h[2];

	size_t                          m_ot_bit_cnt;
	Bytes                           m_ot_recv_bits;
	vector<Bytes>                   m_ot_send_pairs;
	vector<Bytes>                   m_ot_out;
//
//	size_t                          m_ot_ext_bit_cnt;
//	Bytes                           m_ot_ext_recv_bits;
	vector<Bytes>                   m_ot_ext_out;

	vector<vector<Bytes> >          m_ot_keys; // ot output

	// variables for PSSW09 Random Combination Input technique
//	uint32_t                        m_evl_inp_rand_cnt;
//	Bytes                           m_evl_inp_rand;
//	vector<Bytes>                   m_evl_inp_selector;

	// variables for profiling
//	double                          m_timer_gen;
//	double                          m_timer_evl;
//	double                          m_timer_com; // inter-cluster communication
//	double                          m_timer_mpi; // intra-cluster communication
//
//	vector<double>                  m_timer_gen_vec;
//	vector<double>                  m_timer_evl_vec;
//	vector<double>                  m_timer_mpi_vec;
//	vector<double>                  m_timer_com_vec;
//
//	vector<std::string>             m_step_name_vec;
//	vector<uint64_t>                m_comm_sz_vec;

	// variables for cut-and-choose
	Bytes                           m_chks;
	Bytes                           m_all_chks;

	// variables for Yao protocol
	vector<Bytes>                   m_gen_inp_masks;
	vector<Bytes>                   m_coms;
	vector<Bytes>                   m_rnds;
	vector<GarbledCct>              m_ccts;
//
//	Bytes                           m_evl_inp;
//	Bytes                           m_gen_inp;
//	Bytes                           m_gen_out;
//	Bytes                           m_evl_out;
//
//	Prng                            m_prng;
//
//	// variables for MPI
//	MPI_Comm                        m_mpi_comm;
};

//}

#endif /* BETTERYAO2_H_ */
