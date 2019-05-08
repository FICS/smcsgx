#include "BetterYao2.h"

#include <log4cxx/logger.h>
static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("BetterYao2.cpp"));


BetterYao2::BetterYao2(EnvParams &params) : YaoBase(params), m_ot_bit_cnt(0)
{
	// Init variables
	m_coms.resize(Env::node_load());
	m_rnds.resize(Env::node_load());
	m_ccts.resize(Env::node_load());
	m_gen_inp_masks.resize(Env::node_load());
}


void BetterYao2::start()
{
	oblivious_transfer();
	cut_and_choose();
	cut_and_choose2();
	circuit_evaluate();
	consistency_check();
	final_report();
}

void BetterYao2::oblivious_transfer()
{
	step_init();

	double start; // time marker

	Bytes send, recv, bufr(Env::elm_size_in_bytes()*4);
	std::vector<Bytes> bufr_chunks, recv_chunks;

	G X[2], Y[2], gr, hr;
    Z s[2], t[2],  y,  a,  r;

	// step 1: generating the CRS: g[0], h[0], g[1], h[1]
	if (Env::is_root())
	{
		EVL_BEGIN
			start = MPI_Wtime();
				y.random();
				a.random();

				m_ot_g[0].random();
				m_ot_g[1] = m_ot_g[0]^y;          // g[1] = g[0]^y

				m_ot_h[0] = m_ot_g[0]^a;          // h[0] = g[0]^a
				m_ot_h[1] = m_ot_g[1]^(a + Z(1)); // h[1] = g[1]^(a+1)

				bufr.clear();
				bufr += m_ot_g[0].to_bytes();
				bufr += m_ot_g[1].to_bytes();
				bufr += m_ot_h[0].to_bytes();
				bufr += m_ot_h[1].to_bytes();
			m_timer_evl += MPI_Wtime() - start;

			start = MPI_Wtime(); // send to Gen's root process
				EVL_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
		EVL_END

		GEN_BEGIN
			start = MPI_Wtime();
				bufr = GEN_RECV();
			m_timer_com += MPI_Wtime() - start;
		GEN_END

	    m_comm_sz += bufr.size();
	}

	// send g[0], g[1], h[0], h[1] to slave processes
	start = MPI_Wtime();
		MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm);
	m_timer_mpi += MPI_Wtime() - start;

	start = MPI_Wtime();
		bufr_chunks = bufr.split(Env::elm_size_in_bytes());

		m_ot_g[0].from_bytes(bufr_chunks[0]);
		m_ot_g[1].from_bytes(bufr_chunks[1]);
		m_ot_h[0].from_bytes(bufr_chunks[2]);
		m_ot_h[1].from_bytes(bufr_chunks[3]);

		// pre-processing
		m_ot_g[0].fast_exp();
		m_ot_g[1].fast_exp();
		m_ot_h[0].fast_exp();
		m_ot_h[1].fast_exp();

		// allocate memory for m_keys
		m_ot_keys.resize(Env::node_load());
		for (size_t ix = 0; ix < m_ot_keys.size(); ix++)
		{
			m_ot_keys[ix].reserve(Env::circuit().evl_inp_cnt()*2);
		}
	m_timer_evl += MPI_Wtime() - start;
	m_timer_gen += MPI_Wtime() - start;

	// Step 2: ZKPoK of (g[0], g[1], h[0], h[1])
	// TODO

	// Step 3: gr=g[b]^r, hr=h[b]^r, where b is the evaluator's bit
	if (Env::is_root())
	{
		EVL_BEGIN
			start = MPI_Wtime();
				bufr.clear(); bufr.reserve(Env::exp_size_in_bytes()*Env::circuit().evl_inp_cnt());
				send.clear(); send.reserve(Env::elm_size_in_bytes()*Env::circuit().evl_inp_cnt()*2);
				for (size_t bix = 0; bix < Env::circuit().evl_inp_cnt(); bix++)
				{
					r.random();
					bufr += r.to_bytes();  // to be shared with slave evaluators

					byte bit_value = m_evl_inp.get_ith_bit(bix);
					send += (m_ot_g[bit_value]^r).to_bytes(); // gr
					send += (m_ot_h[bit_value]^r).to_bytes(); // hr
				}
			m_timer_evl += MPI_Wtime() - start;

			start = MPI_Wtime();
				EVL_SEND(send); // send (gr, hr)'s
			m_timer_com += MPI_Wtime() - start;

			m_comm_sz += send.size();
		EVL_END

		GEN_BEGIN
			start = MPI_Wtime();
				bufr = GEN_RECV(); // receive (gr, hr)'s
			m_timer_com += MPI_Wtime() - start;

			m_comm_sz += bufr.size();
		GEN_END
	}

	EVL_BEGIN // forward rs to slave evaluators
		start = MPI_Wtime();
			bufr.resize(Env::exp_size_in_bytes()*Env::circuit().evl_inp_cnt());
		m_timer_evl += MPI_Wtime() - start;

		start = MPI_Wtime();
			MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm); // now every evaluator has r's
		m_timer_mpi += MPI_Wtime() - start;

		start = MPI_Wtime();
			bufr_chunks = bufr.split(Env::exp_size_in_bytes());
		m_timer_evl += MPI_Wtime() - start;
	EVL_END

	GEN_BEGIN // forward (gr, hr)s to slave generators
		start = MPI_Wtime();
			bufr.resize(Env::elm_size_in_bytes()*Env::circuit().evl_inp_cnt()*2);
		m_timer_gen += MPI_Wtime() - start;

		start = MPI_Wtime();
			MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm); // now every Bob has bufr
		m_timer_mpi += MPI_Wtime() - start;

		start = MPI_Wtime();
			bufr_chunks = bufr.split(Env::elm_size_in_bytes());
		m_timer_gen += MPI_Wtime() - start;
	GEN_END

	// Step 4: the generator computes X[0], Y[0], X[1], Y[1]
	GEN_BEGIN
		for (size_t bix = 0; bix < Env::circuit().evl_inp_cnt(); bix++)
		{
			start = MPI_Wtime();
				gr.from_bytes(bufr_chunks[2*bix+0]);
				hr.from_bytes(bufr_chunks[2*bix+1]);

				if (m_ot_keys.size() > 2)
				{
					gr.fast_exp();
					hr.fast_exp();
				}
			m_timer_gen += MPI_Wtime() - start;

			for (size_t cix = 0; cix < m_ot_keys.size(); cix++)
			{
				start = MPI_Wtime();
					Y[0].random(); // K[0]
					Y[1].random(); // K[1]

					m_ot_keys[cix].push_back(Y[0].to_bytes().hash(Env::k()));
					m_ot_keys[cix].push_back(Y[1].to_bytes().hash(Env::k()));

					s[0].random(); s[1].random();
					t[0].random(); t[1].random();

					// X[b] = ( g[b]^s[b] ) * ( h[b]^t[b] ), where b = 0, 1
					X[0] = m_ot_g[0]^s[0]; X[0] *= m_ot_h[0]^t[0];
					X[1] = m_ot_g[1]^s[1]; X[1] *= m_ot_h[1]^t[1];

					// Y[b] = ( gr^s[b] ) * ( hr^t[b] ) * K[b], where b = 0, 1
					Y[0] *= gr^s[0]; Y[0] *= hr^t[0];
					Y[1] *= gr^s[1]; Y[1] *= hr^t[1];

					send.clear();
					send += X[0].to_bytes(); send += X[1].to_bytes();
					send += Y[0].to_bytes(); send += Y[1].to_bytes();
				m_timer_gen += MPI_Wtime() - start;

				start = MPI_Wtime();
					GEN_SEND(send);
				m_timer_com += MPI_Wtime() - start;

				m_comm_sz += send.size();
			}
		}

		for (size_t ix = 0; ix < m_ot_keys.size(); ix++)
		{
			assert(m_ot_keys[ix].size() == Env::circuit().evl_inp_cnt()*2);
		}
	GEN_END

	// Step 5: the evaluator computes K = Y[b]/X[b]^r
	EVL_BEGIN
		for (size_t bix = 0; bix < Env::circuit().evl_inp_cnt(); bix++)
		{
			start = MPI_Wtime();
				int bit_value = m_evl_inp.get_ith_bit(bix);
				r.from_bytes(bufr_chunks[bix]);
			m_timer_evl += MPI_Wtime() - start;

			for (size_t cix = 0; cix < m_ot_keys.size(); cix++)
			{
				start = MPI_Wtime();
					recv = EVL_RECV(); // receive X[0], X[1], Y[0], Y[1]
				m_timer_com += MPI_Wtime() - start;

				m_comm_sz += recv.size();

				start = MPI_Wtime();
					recv_chunks = recv.split(Env::elm_size_in_bytes());

					X[bit_value].from_bytes(recv_chunks[    bit_value]); // X[b]
					Y[bit_value].from_bytes(recv_chunks[2 + bit_value]); // Y[b]

					// K = Y[b]/(X[b]^r)
					Y[bit_value] /= X[bit_value]^r;
					m_ot_keys[cix].push_back(Y[bit_value].to_bytes().hash(Env::k()));
				m_timer_evl += MPI_Wtime() - start;
			}
		}

		for (size_t ix = 0; ix < m_ot_keys.size(); ix++)
		{
			assert(m_ot_keys[ix].size() == Env::circuit().evl_inp_cnt());
		}
	EVL_END

	step_report("ob-transfer");
}

void BetterYao2::cut_and_choose()
{
	step_init();

	double start;

	if (Env::is_root())
	{
		Bytes coins = m_prng.rand(Env::k());          // Step 0: flip coins
		Bytes remote_coins, comm, open;

		EVL_BEGIN
			start = MPI_Wtime();
				comm = EVL_RECV();                    // Step 1: receive bob's commitment
				EVL_SEND(coins);                      // Step 2: send coins to bob
				open = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;

			start = MPI_Wtime();
				if (!(open.hash(Env::s()) == comm))   // Step 3: check bob's decommitment
				{
					LOG4CXX_FATAL(logger, "commitment to coins can't be properly opened");
					MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				}
				remote_coins = Bytes(open.begin()+Env::k()/8, open.end());
			m_timer_evl += MPI_Wtime() - start;
		EVL_END

		GEN_BEGIN
			start = MPI_Wtime();
				open = m_prng.rand(Env::k()) + coins; // Step 1: commit to coins
				comm = open.hash(Env::s());
			m_timer_gen += MPI_Wtime() - start;

			start = MPI_Wtime();
				GEN_SEND(comm);
				remote_coins = GEN_RECV();            // Step 2: receive alice's coins
				GEN_SEND(open);                       // Step 3: decommit to the coins
			m_timer_com += MPI_Wtime() - start;
		GEN_END

		m_comm_sz = comm.size() + remote_coins.size() + open.size();

		start = MPI_Wtime();
			coins ^= remote_coins;
			Prng prng;
			prng.srand(coins); // use the coins to generate more random bits

			// make 60-40 check-vs-evaluateion circuit ratio
			m_all_chks.assign(Env::s(), 1);

			// FisherÐYates shuffle
			std::vector<uint16_t> indices(m_all_chks.size());
			for (size_t ix = 0; ix < indices.size(); ix++) { indices[ix] = ix; }

			// starting from 1 since the 0-th circuit is always evaluation-circuit
			for (size_t ix = 1; ix < indices.size(); ix++)
			{
				int rand_ix = prng.rand_range(indices.size()-ix);
				std::swap(indices[ix], indices[ix+rand_ix]);
			}

			int num_of_evls;
			switch(m_all_chks.size())
			{
			case 0: case 1:
				LOG4CXX_FATAL(logger, "there isn't enough circuits for cut-and-choose");
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				break;

			case 2: case 3:
				num_of_evls = 1;
				break;

			case 4:
				num_of_evls = 2;
				break;

			default:
				num_of_evls = m_all_chks.size()*2/5;
				break;
			}

			for (size_t ix = 0; ix < num_of_evls; ix++) { m_all_chks[indices[ix]] = 0; }
		m_timer_evl += MPI_Wtime() - start;
		m_timer_gen += MPI_Wtime() - start;
	}

	start = MPI_Wtime();
		m_chks.resize(Env::node_load());
	m_timer_evl += MPI_Wtime() - start;
	m_timer_gen += MPI_Wtime() - start;

	start = MPI_Wtime();
		MPI_Scatter(&m_all_chks[0], m_chks.size(), MPI_BYTE, &m_chks[0], m_chks.size(), MPI_BYTE, 0, m_mpi_comm);
	m_timer_mpi += MPI_Wtime() - start;

	step_report("cut-&-check");
}

void BetterYao2::cut_and_choose2()
{
	step_init();
	double start;

	Bytes bufr;
	vector<Bytes> bufr_chunks;

	m_ot_bit_cnt = Env::node_load();

	EVL_BEGIN
		start = MPI_Wtime();
			m_ot_recv_bits.resize((m_ot_bit_cnt+7)/8);
			for (size_t ix = 0; ix < m_chks.size(); ix++)
			{
				m_ot_recv_bits.set_ith_bit(ix, m_chks[ix]);
			}
		m_timer_evl += MPI_Wtime() - start;
	EVL_END

	ot_init();
	ot_random();

	GEN_BEGIN
		start = MPI_Wtime();
			for (size_t ix = 0; ix < m_ccts.size(); ix++)
			{
				m_rnds[ix] = m_prng.rand(Env::k());
				m_gen_inp_masks[ix] = m_prng.rand(Env::circuit().gen_inp_cnt());
				m_ccts[ix].gen_init(m_ot_keys[ix], m_gen_inp_masks[ix], m_rnds[ix]);
			}
		m_timer_gen += MPI_Wtime() - start;
	GEN_END

	Prng prng;
	G M;

	for (size_t ix = 0; ix < m_ccts.size(); ix++)
	{
		// evaluation-circuit branch of OT
		GEN_BEGIN
			start = MPI_Wtime(); // send group elements representing m_gen_inp
				bufr.clear();
				prng.srand(m_ot_out[2*ix+0]);
				for (size_t bix = 0; bix < Env::circuit().gen_inp_cnt(); bix++)
				{
					byte bit_value = m_gen_inp.get_ith_bit(bix);
					bufr += m_ccts[ix].m_M[2*bix+bit_value].to_bytes();
				}
				bufr ^= prng.rand(bufr.size()*8);
			m_timer_gen += MPI_Wtime() - start;

			start = MPI_Wtime();
				GEN_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
		GEN_END

		EVL_BEGIN
			start = MPI_Wtime();
				bufr = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;

			start = MPI_Wtime();
				if (m_chks[ix] == 0)
				{
					prng.srand(m_ot_out[ix]);
					bufr ^= prng.rand(bufr.size()*8);
					bufr_chunks = bufr.split(Env::elm_size_in_bytes());
					for (size_t bix = 0; bix < bufr_chunks.size(); bix++)
					{
						M.from_bytes(bufr_chunks[bix]);
						m_ccts[ix].m_M.push_back(M);
					}
				}
			m_timer_evl += MPI_Wtime() - start;
		EVL_END

		m_comm_sz += bufr.size();

		GEN_BEGIN
			start = MPI_Wtime(); // send the masked m_gen_inp
				bufr = m_gen_inp_masks[ix] ^ m_gen_inp;
				bufr ^= prng.rand(bufr.size()*8);
			m_timer_gen += MPI_Wtime() - start;

			start = MPI_Wtime();
				GEN_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
		GEN_END

		EVL_BEGIN
			start = MPI_Wtime();
				bufr = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;

			start = MPI_Wtime();
				if (m_chks[ix] == 0)
				{
					bufr ^= prng.rand(bufr.size()*8);
					m_gen_inp_masks[ix] = bufr;
				}
			m_timer_evl += MPI_Wtime() - start;
		EVL_END

		m_comm_sz += bufr.size();

		// check-circuit branch of OT
		GEN_BEGIN
			start = MPI_Wtime(); // send the m_gen_inp_masks[ix]
				prng.srand(m_ot_out[2*ix+1]);
				bufr = m_gen_inp_masks[ix];
				bufr ^= prng.rand(bufr.size()*8);
			m_timer_gen += MPI_Wtime() - start;

			start = MPI_Wtime();
				GEN_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
		GEN_END

		EVL_BEGIN
			start = MPI_Wtime();
				bufr = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;

			start = MPI_Wtime();
				if (m_chks[ix])
				{
					prng.srand(m_ot_out[ix]);
					bufr ^= prng.rand(bufr.size()*8);
					m_gen_inp_masks[ix] = bufr;
				}
			m_timer_evl += MPI_Wtime() - start;
		EVL_END

		m_comm_sz += bufr.size();

		GEN_BEGIN
			start = MPI_Wtime(); // send the masked m_gen_inp
				bufr = m_rnds[ix];
				bufr ^= prng.rand(bufr.size()*8);
			m_timer_gen += MPI_Wtime() - start;

			start = MPI_Wtime();
				GEN_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
		GEN_END

		EVL_BEGIN
			start = MPI_Wtime();
				bufr = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;

			start = MPI_Wtime();
				if (m_chks[ix])
				{
					bufr ^= prng.rand(bufr.size()*8);
					m_rnds[ix] = bufr;
				}
			m_timer_evl += MPI_Wtime() - start;
		EVL_END

		m_comm_sz += bufr.size();

		GEN_BEGIN
			start = MPI_Wtime(); // send the m_ot_keys
				bufr = Bytes(m_ot_keys[ix]);
				bufr ^= prng.rand(bufr.size()*8);
			m_timer_gen += MPI_Wtime() - start;

			start = MPI_Wtime();
				GEN_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
		GEN_END

		EVL_BEGIN
			start = MPI_Wtime();
				bufr = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;

			start = MPI_Wtime();
				if (m_chks[ix])
				{
					bufr ^= prng.rand(bufr.size()*8);
					m_ot_keys[ix] = bufr.split(Env::key_size_in_bytes());
				}
			m_timer_evl += MPI_Wtime() - start;
		EVL_END

		m_comm_sz += bufr.size();
	}

	step_report("cut-'n-chk2");
}


void BetterYao2::circuit_evaluate()
{
	step_init();

	double start;

	int verify = 1;
	Bytes bufr;

	EVL_BEGIN
		start = MPI_Wtime();
			for (size_t ix = 0; ix < m_ccts.size(); ix++)
			{
				if (m_chks[ix]) // check-circuits
				{
					m_ccts[ix].gen_init(m_ot_keys[ix], m_gen_inp_masks[ix], m_rnds[ix]);
				}
				else // evaluation-circuits
				{
					m_ccts[ix].evl_init(m_ot_keys[ix], m_gen_inp_masks[ix], m_evl_inp);
				}
			}
		m_timer_evl += MPI_Wtime() - start;
	EVL_END

	step_report("pre-cir-evl");
	step_init();

	GEN_BEGIN // start generating circuits gate-by-gate
		start = MPI_Wtime();
			while (Env::circuit().more_gate_binary())
			{
				const Gate &g = Env::circuit().next_gate_binary();

				for (size_t ix = 0; ix < m_ccts.size(); ix++)
				{
						m_ccts[ix].gen_next_gate(g);
						bufr = m_ccts[ix].send();
					m_timer_gen += MPI_Wtime() - start;

					start = MPI_Wtime();
						GEN_SEND(bufr);
					m_timer_com += MPI_Wtime() - start;

					m_comm_sz += bufr.size();

					start = MPI_Wtime(); // start m_timer_gen
				}
			}
		m_timer_gen += MPI_Wtime() - start;
	GEN_END

	EVL_BEGIN
		start = MPI_Wtime();
			int gate_ix = 0;
			while (Env::circuit().more_gate_binary())
			{
				const Gate &g = Env::circuit().next_gate_binary();

				for (size_t ix = 0; ix < m_ccts.size(); ix++)
				{
					m_timer_evl += MPI_Wtime() - start;

					start = MPI_Wtime();
						bufr = EVL_RECV();
					m_timer_com += MPI_Wtime() - start;

					m_comm_sz += bufr.size();

					start = MPI_Wtime();
						if (m_chks[ix]) // check-circuits
						{
							m_ccts[ix].gen_next_gate(g);
							verify &= (m_ccts[ix].send() == bufr);
						}
						else // evaluation-circuits
						{
							m_ccts[ix].recv(bufr);
							m_ccts[ix].evl_next_gate(g);
						}
				}
				gate_ix ++;
			}
		m_timer_evl += MPI_Wtime() - start;
	EVL_END

	EVL_BEGIN // check the hash of all the garbled circuits
		int all_verify = 0;

		start = MPI_Wtime();
			MPI_Reduce(&verify, &all_verify, 1, MPI_INT, MPI_LAND, 0, m_mpi_comm);
		m_timer_mpi += MPI_Wtime() - start;

		start = MPI_Wtime();
			if (Env::is_root() && !all_verify)
			{
				LOG4CXX_FATAL(logger, "Verification failed");
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			}
		m_timer_evl += MPI_Wtime() - start;
	EVL_END

	step_report("circuit-evl");

	if (Env::circuit().evl_out_cnt() != 0)
		proc_evl_out();

    if (Env::circuit().gen_out_cnt() != 0)
        proc_gen_out();
}


void BetterYao2::consistency_check()
{
	step_init();

	Bytes send, recv, bufr;
	std::vector<Bytes> recv_chunks, bufr_chunks;

	double start;

	Z m;

	if (Env::is_root())
	{
		GEN_BEGIN // give away the generator's input M_{0,j}
			start = MPI_Wtime();
				bufr.clear();
				for (int jx = 0; jx < Env::circuit().gen_inp_cnt(); jx++)
				{
					bool bit_value = m_gen_inp.get_ith_bit(jx);
					bufr += m_ccts[0].m_M[2*jx+bit_value].to_bytes();
				}
			m_timer_gen += MPI_Wtime() - start;

			start = MPI_Wtime();
				GEN_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
		GEN_END

		EVL_BEGIN
			start = MPI_Wtime();
				bufr = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;
		EVL_END

		m_comm_sz += bufr.size();
	}

	EVL_BEGIN // forward the generator's input M_{0,j} to slave evaluators
		start = MPI_Wtime();
			bufr.resize(Env::elm_size_in_bytes()*Env::circuit().gen_inp_cnt());
		m_timer_evl += MPI_Wtime() - start;

		start = MPI_Wtime();
			MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm);
		m_timer_mpi += MPI_Wtime() - start;

		start = MPI_Wtime();
			bufr_chunks = bufr.split(Env::elm_size_in_bytes());
		m_timer_evl += MPI_Wtime() - start;
	EVL_END

	GEN_BEGIN // give away m_{i,j}-m_{0,j} so that M_{i,j} can be reconstructed
		start = MPI_Wtime();
			if (Env::is_root()) // forward m_{0,j} to slave generators
			{
				bufr.clear();
				for (size_t jx = 0; jx < Env::circuit().gen_inp_cnt(); jx++)
				{
					bool bit_value = m_gen_inp.get_ith_bit(jx);
					bufr += m_ccts[0].m_m[2*jx+bit_value].to_bytes();
				}
			}
			bufr.resize(Env::exp_size_in_bytes()*Env::circuit().gen_inp_cnt());
		m_timer_gen += MPI_Wtime() - start;

		start = MPI_Wtime();
			MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm);
		m_timer_mpi += MPI_Wtime() - start;

		start = MPI_Wtime();
			bufr_chunks = bufr.split(Env::exp_size_in_bytes());

			send.clear();
			send.reserve(Env::exp_size_in_bytes()*Env::circuit().gen_inp_cnt()*m_ccts.size());
			for (size_t jx = 0; jx < Env::circuit().gen_inp_cnt(); jx++)
			{
				m.from_bytes(bufr_chunks[jx]); // retrieve m_{0,j}
				bool bit_value = m_gen_inp.get_ith_bit(jx);

				for (size_t ix = 0; ix < m_ccts.size(); ix++)
				{
					if (m_chks[ix]) // no data for check-circuits
						continue;
					send += (m_ccts[ix].m_m[2*jx+bit_value]-m).to_bytes(); // m_{i,j}-m_{0,j}
				}
			}
		m_timer_gen += MPI_Wtime() - start;

		start = MPI_Wtime();
			GEN_SEND(send);
		m_timer_com += MPI_Wtime() - start;

		m_comm_sz += send.size();
	GEN_END

	int verify = 1;
	G M;

	EVL_BEGIN
		start = MPI_Wtime();
			recv = EVL_RECV();
		m_timer_com += MPI_Wtime() - start;

		m_comm_sz += recv.size();

		start = MPI_Wtime();
			recv_chunks = recv.split(Env::exp_size_in_bytes());
			for (size_t ix = 0, kx = 0; ix < Env::circuit().gen_inp_cnt(); ix++)
			{
				M.from_bytes(bufr_chunks[ix]);

				for (size_t jx = 0; jx < m_ccts.size(); jx++)
				{
					if (m_chks[jx])
						continue;

					// reconstruct M_{i,j}
					m.from_bytes(recv_chunks[kx++]);                // m = m_{i,j}-m_{0,j}
					M *= Env::clawfree().R(m);                      // M_{i,j} = h^m * M_{0,j}
					verify &= (m_ccts[jx].m_M[ix] == M);
				}
			}
		m_timer_evl += MPI_Wtime() - start;

		int all_verify = 0;

		start = MPI_Wtime();
			MPI_Reduce(&verify, &all_verify, 1, MPI_INT, MPI_LAND, 0, m_mpi_comm);
		m_timer_mpi += MPI_Wtime() - start;

		start = MPI_Wtime();
			if (Env::is_root() && !all_verify)
			{
				LOG4CXX_FATAL(logger, "consistency check failed");
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			}
		m_timer_evl += MPI_Wtime() - start;
	EVL_END

	step_report("const-check");
}

void BetterYao2::proc_evl_out()
{
	EVL_BEGIN
		step_init();

		double start;
		Bytes send, recv;

		start = MPI_Wtime();
			const Bytes ZEROS((Env::circuit().evl_out_cnt()+7)/8, 0);

			for (size_t ix = 0; ix < m_ccts.size(); ix++) // fill zeros for uniformity (convenient to MPIs)
			{
				send += (m_chks[ix])? ZEROS : m_ccts[ix].m_evl_out;
			}

			if (Env::group_rank() == 0)
			{
				recv.resize(send.size()*Env::node_amnt());
			}
		m_timer_evl += MPI_Wtime() - start;

		start = MPI_Wtime();
			MPI_Gather(&send[0], send.size(), MPI_BYTE, &recv[0], send.size(), MPI_BYTE, 0, m_mpi_comm);
		m_timer_mpi += MPI_Wtime() - start;

		start = MPI_Wtime();
			if (Env::is_root())
			{
				size_t chks_total = 0;
				for (size_t ix = 0; ix < m_all_chks.size(); ix++)
					chks_total += m_all_chks[ix];

				// find majority by locating the median of output from evaluation-circuits
				std::vector<Bytes> vec = recv.split((Env::circuit().evl_out_cnt()+7)/8);
				size_t median_ix = (chks_total+vec.size())/2;
				std::nth_element(vec.begin(), vec.begin()+median_ix, vec.end());

				m_evl_out = *(vec.begin()+median_ix);
			}
		m_timer_evl += MPI_Wtime() - start;

		step_report("chk-evl-out");
	EVL_END
}


void BetterYao2::proc_gen_out()
{
	step_init();

	// TODO: implement Ki08
	m_gen_out = m_ccts[0].m_gen_out;

	EVL_BEGIN
		send_data(Env::world_rank()-1, m_gen_out);
	EVL_END

	GEN_BEGIN
		m_gen_out = recv_data(Env::world_rank()+1);
	GEN_END

	step_report("chk-gen-out");
}


//
// Implementation of "Two-Output Secure Computation with Malicious Adversaries"
// by abhi shelat and Chih-hao Shen from EUROCRYPT'11 (Protocol 2)
//
// The evaluator (sender) generates m_ot_bit_cnt pairs of k-bit random strings, and
// the generator (receiver) has input m_ot_bits and will receive output m_ot_out.
//
void BetterYao2::ot_init()
{
	double start;

	start = MPI_Wtime();
		std::vector<Bytes> bufr_chunks;
		Bytes bufr(Env::elm_size_in_bytes()*4);

		Z y, a;
	m_timer_gen += MPI_Wtime() - start;
	m_timer_evl += MPI_Wtime() - start;

	// step 1: ZKPoK of the CRS: g[0], h[0], g[1], h[1]
	if (Env::is_root())
	{
		EVL_BEGIN // evaluator (OT receiver)
			start = MPI_Wtime();
				y.random();

				a.random();

				m_ot_g[0].random();
				m_ot_g[1] = m_ot_g[0]^y;          // g[1] = g[0]^y

				m_ot_h[0] = m_ot_g[0]^a;          // h[0] = g[0]^a
				m_ot_h[1] = m_ot_g[1]^(a + Z(1)); // h[1] = g[1]^(a+1)

				bufr.clear();
				bufr += m_ot_g[0].to_bytes();
				bufr += m_ot_g[1].to_bytes();
				bufr += m_ot_h[0].to_bytes();
				bufr += m_ot_h[1].to_bytes();
			m_timer_evl += MPI_Wtime() - start;

			start = MPI_Wtime();
				EVL_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
		EVL_END

		GEN_BEGIN // generator (OT sender)
			start = MPI_Wtime();
				bufr = GEN_RECV();
			m_timer_com += MPI_Wtime() - start;
		GEN_END

	    m_comm_sz += bufr.size();
	}

	// send g[0], g[1], h[0], h[1] to slave processes
	start = MPI_Wtime();
		MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm);
	m_timer_mpi += MPI_Wtime() - start;

	start = MPI_Wtime();
		bufr_chunks = bufr.split(Env::elm_size_in_bytes());

		m_ot_g[0].from_bytes(bufr_chunks[0]);
		m_ot_g[1].from_bytes(bufr_chunks[1]);
		m_ot_h[0].from_bytes(bufr_chunks[2]);
		m_ot_h[1].from_bytes(bufr_chunks[3]);

		// group element pre-processing
		m_ot_g[0].fast_exp();
		m_ot_g[1].fast_exp();
		m_ot_h[0].fast_exp();
		m_ot_h[1].fast_exp();
	m_timer_gen += MPI_Wtime() - start;
	m_timer_evl += MPI_Wtime() - start;
}


void BetterYao2::ot_random()
{
	double start;

	start = MPI_Wtime();
		Bytes send, recv;
		std::vector<Bytes> recv_chunks;

		Z r, s[2], t[2];
		G gr, hr, X[2], Y[2];

		m_ot_out.clear();
		m_ot_out.reserve(2*m_ot_bit_cnt); // the receiver only uses half of it
	m_timer_gen += MPI_Wtime() - start;
	m_timer_evl += MPI_Wtime() - start;

	EVL_BEGIN // evaluator (OT receiver)
		assert(m_ot_recv_bits.size() >= ((m_ot_bit_cnt+7)/8));

		for (size_t bix = 0; bix < m_ot_bit_cnt; bix++)
		{
			// Step 1: gr=g[b]^r, hr=h[b]^r, where b is the receiver's bit
			start = MPI_Wtime();
				int bit_value = m_ot_recv_bits.get_ith_bit(bix);

				r.random();

				gr = m_ot_g[bit_value]^r;
				hr = m_ot_h[bit_value]^r;

				send.clear();
				send += gr.to_bytes();
				send += hr.to_bytes();
			m_timer_evl += MPI_Wtime() - start;

			start = MPI_Wtime();
				EVL_SEND(send);

				// Step 2: the evaluator computes X[0], Y[0], X[1], Y[1]
				recv.clear();
				recv += EVL_RECV(); // receive X[0], Y[0], X[1], Y[1]
			m_timer_com += MPI_Wtime() - start;

			m_comm_sz += send.size() + recv.size();

			// Step 3: the evaluator computes K = Y[b]/X[b]^r
			start = MPI_Wtime();
				recv_chunks = recv.split(Env::elm_size_in_bytes());

				X[bit_value].from_bytes(recv_chunks[    bit_value]); // X[b]
				Y[bit_value].from_bytes(recv_chunks[2 + bit_value]); // Y[b]

				// K = Y[b]/(X[b]^r)
				Y[bit_value] /= X[bit_value]^r;
				m_ot_out.push_back(Y[bit_value].to_bytes().hash(Env::k()));
			m_timer_evl += MPI_Wtime() - start;
		}

		assert(m_ot_out.size() == m_ot_bit_cnt);
	EVL_END

	GEN_BEGIN // generator (OT sender)
		for (size_t bix = 0; bix < m_ot_bit_cnt; bix++)
		{
			// Step 1: gr=g[b]^r, hr=h[b]^r, where b is the receiver's bit
			start = MPI_Wtime();
				recv.clear();
				recv += GEN_RECV(); // receive gr, hr
			m_timer_com += MPI_Wtime() - start;

			m_comm_sz += recv.size();

			// Step 2: the evaluator computes X[0], Y[0], X[1], Y[1]
			start = MPI_Wtime();
				recv_chunks = recv.split(Env::elm_size_in_bytes());

				gr.from_bytes(recv_chunks[0]);
				hr.from_bytes(recv_chunks[1]);

				Y[0].random(); Y[1].random(); // K[0], K[1] sampled at random

				m_ot_out.push_back(Y[0].to_bytes().hash(Env::k()));
				m_ot_out.push_back(Y[1].to_bytes().hash(Env::k()));

				s[0].random(); s[1].random();
				t[0].random(); t[1].random();

				// X[b] = ( g[b]^s[b] ) * ( h[b]^t[b] ) for b = 0, 1
				X[0] = m_ot_g[0]^s[0]; X[0] *= m_ot_h[0]^t[0];
				X[1] = m_ot_g[1]^s[1]; X[1] *= m_ot_h[1]^t[1];

				// Y[b] = ( gr^s[b] ) * ( hr^t[b] ) * K[b] for b = 0, 1
				Y[0] *= gr^s[0]; Y[0] *= hr^t[0];
				Y[1] *= gr^s[1]; Y[1] *= hr^t[1];

				send.clear();
				send += X[0].to_bytes();
				send += X[1].to_bytes();
				send += Y[0].to_bytes();
				send += Y[1].to_bytes();
			m_timer_gen += MPI_Wtime() - start;

			start = MPI_Wtime();
				GEN_SEND(send);
			m_timer_com += MPI_Wtime() - start;

			m_comm_sz += send.size();
		}

		assert(m_ot_out.size() == 2*m_ot_bit_cnt);
	GEN_END
}
