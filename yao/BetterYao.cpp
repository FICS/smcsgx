#define RDTSC ({unsigned long long res;  unsigned hi, lo;   __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi)); res =  ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );res;})
unsigned long startTime, endTime;
unsigned long startTimeb, endTimeb;


#include "BetterYao.h"

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>



int MPI_Wtime()
{
	return 0;
}


BetterYao::BetterYao(EnvParams &params) : YaoBase(params)
{
	// Init variables
	m_coms.resize(Env::node_load());
	m_rnds.resize(Env::node_load());
	m_ccts.resize(Env::node_load());
	m_gen_inp_masks.resize(Env::node_load());
	m_gen_inp_com.resize(Env::node_load());

        	
}



void BetterYao::initSGXProgram()
{
}


void BetterYao::runSGXProgram( string filename, vector<Bytes> * input, vector<Bytes> * output, vector<char> * permutes)
{
	Env::circuit().load_binary(filename.c_str());

CircuitInput0CircuitBase.resize(Env::circuit().evl_inp_cnt()*2);

        if(Env::is_root())
        {
                for(int i=0;i<Env::circuit().evl_inp_cnt()*2;i++)
                {
                        CircuitInput0CircuitBase[i] = m_prng.rand(80);
                }
	}

	/*CircuitInput0CircuitBase.resize(Env::circuit().evl_inp_cnt()*2);

        if(Env::is_root())
        {
                for(int i=0;i<Env::circuit().evl_inp_cnt()*2;i++)
                {
                        CircuitInput0CircuitBase[i] = m_prng.rand(80);
                }
	}*/

	init_private("inp.txt");

	// Random Combination Technique from PSSW09
//	ot_ext();
	// Committing OT from SS11

	struct timeval startot, endot;

	long mtime, seconds, useconds;  
	gettimeofday(&startot, NULL);


//cout << "OTs\n"<<Env::circuit().evl_inp_cnt() <<"\n";

//cout << "g\n"<<Env::circuit().gen_inp_cnt() <<"\n";




//	oblivious_transfer();
	if(Env::circuit().evl_inp_cnt() > 0)
		BM_OT_ext_with_third(80,80);

//cout << "OTe\n";

	gettimeofday(&endot, NULL);

	seconds  = endot.tv_sec  - startot.tv_sec;
	useconds = endot.tv_usec - startot.tv_usec;

	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

	//if(Env::is_root()) printf("OT time: %ld, ", mtime);


	
	gettimeofday(&startot, NULL);
	circuit_commit();


	gettimeofday(&endot, NULL);

	seconds  = endot.tv_sec  - startot.tv_sec;
	useconds = endot.tv_usec - startot.tv_usec;

	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

//	if(Env::is_root()) printf("%ld, ", mtime);



	gettimeofday(&startot, NULL);
	cut_and_choose();



	consistency_check();

//cout << "evals\n";

	//m_rnds[0] = m_prng.rand(Env::k());

	m_ccts[0].m_sgx_keys = input;
	m_ccts[0].sgx_keys_out = output;

	m_ccts[0].sgx_permutes = permutes;
	
	circuit_evaluate();
//cout << "evale\n";
	final_report();
}


void BetterYao::start()
{
	// Random Combination Technique from PSSW09
//	ot_ext();
	// Committing OT from SS11

	struct timeval startot, endot;

	long mtime, seconds, useconds;  
	gettimeofday(&startot, NULL);

	//oblivious_transfer();

	//cout << "start1\n";

	if(Env::circuit().evl_inp_cnt() > 0)
	{
		BM_OT_ext_with_third(80,80);
	}

	gettimeofday(&endot, NULL);

	seconds  = endot.tv_sec  - startot.tv_sec;
	useconds = endot.tv_usec - startot.tv_usec;

	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

	if(Env::is_root()) printf("OT time: %ld, ", mtime);


	
	gettimeofday(&startot, NULL);
	circuit_commit();


	//cout << "start2\n";

	gettimeofday(&endot, NULL);

	seconds  = endot.tv_sec  - startot.tv_sec;
	useconds = endot.tv_usec - startot.tv_usec;

	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

	if(Env::is_root()) printf("%ld, ", mtime);



	gettimeofday(&startot, NULL);
	cut_and_choose();

	//cout << "start3\n";

	consistency_check();

	//cout << "start4\n";

	circuit_evaluate();

	//cout << "start5\n";

	gettimeofday(&endot, NULL);
	seconds  = endot.tv_sec  - startot.tv_sec;
	useconds = endot.tv_usec - startot.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	if(Env::is_root()) printf("%ld, ", mtime);

    final_report();
}


void BetterYao::oblivious_transfer()
{
	//step_init();

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
		//MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm);
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
			//MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm); // now every evaluator has r's
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
			//MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm); // now every Bob has bufr
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

	//step_report("ob-transfer");
}


void BetterYao::circuit_commit()
{
	//step_init();

	double start; // time marker

	Bytes bufr;

	// construct garbled circuits and generate their hashes on the fly
	GEN_BEGIN
		start = MPI_Wtime();
			for (size_t ix = 0; ix < m_ccts.size(); ix++)
			{
			
				m_rnds[ix] = m_prng.rand(Env::k());
				m_gen_inp_masks[ix] = m_prng.rand(Env::circuit().gen_inp_cnt());
				m_ccts[ix].com_init(m_ot_keys[ix], m_gen_inp_masks[ix], m_rnds[ix]);
				//m_ccts[ix].createinput();	
		}
		m_timer_gen += MPI_Wtime() - start;

		//step_report("pre-cir-gen");
		//step_init();

		start = MPI_Wtime();

		m_timer_gen += MPI_Wtime() - start;
	GEN_END
	
    //step_report("circuit-gen");
}


void BetterYao::cut_and_choose()
{
	//step_init();

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
					//LOG4CXX_FATAL(logger, "commitment to coins can't be properly opened");
					//MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
					std::cout << "commitment to coins can't be properly opened"<<"\n";
					exit(1);
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

			// Fisher\D0Yates shuffle
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
				//LOG4CXX_FATAL(logger, "there isn't enough circuits for cut-and-choose");
				//MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				//std::cout << "there isn't enough circuits for cut-and-choose"<<"\n";
				//exit(1);
				num_of_evls = 1;
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

		m_chks = m_all_chks;
		//MPI_Scatter(&m_all_chks[0], m_chks.size(), MPI_BYTE, &m_chks[0], m_chks.size(), MPI_BYTE, 0, m_mpi_comm);
	m_timer_mpi += MPI_Wtime() - start;

	//step_report("cut-&-check");
}


void BetterYao::consistency_check()
{
	//step_init();

	Bytes send, recv, bufr;
	std::vector<Bytes> recv_chunks, bufr_chunks;

	double start;
//	std::cout <<"prenew\n";
	GEN_BEGIN


	for (size_t jx = 0; jx < Env::circuit().gen_inp_cnt(); jx++)
	{
		//m_gen_inp_com[0][jx] =  m_ccts[0].new_m_M[jx^bit];
		GEN_SEND(m_ccts[0].new_m_M[2*jx+m_gen_inp.get_ith_bit(jx)]);
	}
	GEN_END


	EVL_BEGIN
	
	m_ccts[0].new_m_M.resize(Env::circuit().gen_inp_cnt());

	for (size_t jx = 0; jx < Env::circuit().gen_inp_cnt(); jx++)
	{
		
		m_ccts[0].new_m_M[jx] = EVL_RECV();
	
	}
	EVL_END
//	std::cout <<"postnew\n";
	
	//step_report("const-check");
}

void BetterYao::circuit_evaluate()
{
	//step_init();

	//Env::circuit().reload_binary();

	double start;
	Bytes bufr;

	for (size_t ix = 0; ix < m_ccts.size(); ix++)
	{
		if (m_chks[ix]) // check-circuits
		{
	
			m_comm_sz += m_gen_inp_masks[ix].size() + m_rnds[ix].size() + bufr.size();
		}
		else // evaluation-circuits
		{
			EVL_BEGIN
				start = MPI_Wtime();
					m_gen_inp_masks[ix] = EVL_RECV();
				m_timer_com += MPI_Wtime() - start;

				start = MPI_Wtime();
					m_ccts[ix].evl_init(m_ot_keys[ix], m_gen_inp_masks[ix], m_evl_inp);
				m_timer_evl += MPI_Wtime() - start;
			EVL_END

			GEN_BEGIN
				start = MPI_Wtime();
					GEN_SEND(m_gen_inp_masks[ix] ^ m_gen_inp); // send the masked gen_inp
				m_timer_com += MPI_Wtime() - start;

				start = MPI_Wtime();
					m_ccts[ix].gen_init(m_ot_keys[ix], m_gen_inp_masks[ix], m_rnds[ix]);
				m_timer_gen += MPI_Wtime() - start;
			GEN_END

			m_comm_sz += m_gen_inp_masks[ix].size();
		}
	}

	//step_report("pre-cir-evl");
	//step_init();

	int isxor =0;

	int gatesbeforesend = 4000;
	int howmanygates = 0;

	int heldgates = 0;
	long long count1 = 0;
	long long countnextgate = 0;
	long long countnetwork = 0;
	

	long nonxorgates=0;
	long gates=0;

	Bytes hBytes;
	hBytes.resize(200000);

	EVL_BEGIN // check (or evaluate) garbled circuits
		start = MPI_Wtime();
			startTimeb = RDTSC;
			while (Env::circuit().more_gate_binary())
			{
			
				startTime = RDTSC;	

				const Gate &g = Env::circuit().next_gate_binary();

				endTime = RDTSC;

				countnextgate += (endTime - startTime);

			
				startTime = RDTSC;
	
				if(heldgates == 0)
				{
					EVL_RECV2(&hBytes[0],&m_ccts[0].buffersize);
					m_ccts[0].recv(hBytes);
					//bufr = EVL_RECV();
					//m_ccts[0].recv(bufr);
					heldgates = gatesbeforesend;
				}
				

				heldgates--;

				endTime = RDTSC;
				
				countnetwork += (endTime - startTime);

				

				//for (size_t ix = 0; ix < m_ccts.size(); ix++)
				{
				
				
				startTime = RDTSC;
			
					
					m_ccts[0].evl_next_gate(g);
				
				endTime = RDTSC;
				
				count1 += (endTime - startTime);



				//	if(gates%1000==0)
				//		std::cout <<"evl: "<< (endTime - startTime)<<"\n";
	
						gates++;				
				}
			}
			endTimeb = RDTSC;
			/*std::cout <<"\nevl Cycles per gate evl total: " << (endTimeb - startTimeb)/gates<<"\n";
			std::cout <<"evl Cycles per evaluate: "<< count1/gates <<"\n";
			std::cout <<"evl Cycles per evaluate in gcc: "<< m_ccts[0].countrd/gates <<"\n";
			std::cout <<"NETT: "<<gatesbeforesend <<" " <<countnetwork<<"\n";
			std::cout <<"evl Cycles per network: "<< countnetwork/gates <<"\n";
			std::cout <<"evl Cycles per nextgate: "<< countnextgate/gates <<"\n";	
			*/	
		m_timer_evl += MPI_Wtime() - start;
	EVL_END

	vector<Bytes> sendvec;
	//long long count1 = 0;
	//long gates=0;

	long nonxortime=0;
	unsigned long st=0, et=0;
	GEN_BEGIN // re-generate the evaluation-circuits
		start = MPI_Wtime();
			startTimeb = RDTSC;
			while (Env::circuit().more_gate_binary())
			{
				st = RDTSC;
				startTime = RDTSC;
				const Gate &g = Env::circuit().next_gate_binary();
				endTime = RDTSC;

				countnextgate += (endTime - startTime);

				//for (size_t ix = 0; ix < m_ccts.size(); ix++)
				{
					//std::cout << ix <<"\n";
					/*if (m_chks[ix]) { continue; }*/

			
			
					startTime = RDTSC;
	
					m_ccts[0].gen_next_gate(g);
					//is_xor(g);				
	
					endTime = RDTSC;
					
					if(g.tbl_gate != 6) {count1 += (endTime - startTime); nonxorgates++;}

	
		
					//if(gates%1000==0)
					//	std::cout << "gen: "<< (endTime - startTime)<<"\n";
					
					
				}

				howmanygates++;
				gates++;

				startTime = RDTSC;
		
				if(howmanygates == gatesbeforesend)
				{
					        /*static Bytes tmp;
                                                tmp.merge(sendvec);
                                                sendvec.clear();*/
								
					//	std::cout <<m_ccts[0].buffersize<<" ";

						GEN_SEND2(&(m_ccts[0].buffer[0]),m_ccts[0].buffersize);
						bufr = m_ccts[0].send();
						//GEN_SEND(bufr);
						//std::cout << bufr.size()<<"\n";
						howmanygates = 0;
				}

				endTime = RDTSC;

				countnetwork += (endTime - startTime);
				

				et = RDTSC;

				if(g.tbl_gate != 6){nonxortime += et-st;}
			}

			if(howmanygates > 0)
			{
				//static Bytes tmp;
				//tmp.merge(sendvec);
				
				GEN_SEND2(&(m_ccts[0].buffer[0]),m_ccts[0].buffersize);
				bufr = m_ccts[0].send();


				//bufr = m_ccts[0].send();
				//std::cout << bufr.size()<<"\nend\n";
				//GEN_SEND(bufr);
			}	
			endTimeb = RDTSC;
		
			/*std::cout <<"\nGen Cycles per gate evl total: " << (endTimeb - startTimeb)/gates<<"\n";
			std::cout <<"total cycles: "<<(endTimeb - startTimeb)<<"\n";
			std::cout <<"Gen Cycles per evaluate nonxor: "<< count1/gates <<"\n";
			std::cout <<"Gen Cycles per evaluate in gcc: "<< m_ccts[0].countrd/gates <<"\n";
			std::cout <<"Non-xor Gates: "<<nonxorgates<<"\n"; 	
			std::cout <<"Non-xor total time: "<<nonxortime<<"\n";
			std::cout <<"Gen Cycles per network: "<< countnetwork/gates <<"\n";
			std::cout <<"NETT: "<<gatesbeforesend <<" "  <<countnetwork<<"\n";
			std::cout <<"Gen Cycles per nextgate: "<< countnextgate/gates <<"\n";	
			*/
	GEN_END
	
	//step_report("circuit-evl");

	if (Env::circuit().evl_out_cnt() != 0)
		proc_evl_out();

    if (Env::circuit().gen_out_cnt() != 0)
        proc_gen_out();
}


void BetterYao::proc_evl_out()
{
	EVL_BEGIN
		//step_init();

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
			//MPI_Gather(&send[0], send.size(), MPI_BYTE, &recv[0], send.size(), MPI_BYTE, 0, m_mpi_comm);
			recv = send;
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

		//step_report("chk-evl-out");
	EVL_END
}

void BetterYao::proc_gen_out()
{
	//step_init();


	EVL_BEGIN
		//start = MPI_Wtime();
			m_gen_out = m_ccts[0].m_gen_out;
		//m_timer_evl += MPI_Wtime() - start;

		//start = MPI_Wtime();
			EVL_SEND(m_gen_out);
		//m_timer_com += MPI_Wtime() - start;
	EVL_END

	GEN_BEGIN
		//start = MPI_Wtime();
			m_gen_out = GEN_RECV();
		//m_timer_com += MPI_Wtime() - start;
	GEN_END

	//step_report("chk-gen-out");

}

Bytes sendhold;

void BetterYao::oblivious_transfer_with_inputs_gen_third_SINGLE(int lengthOfInOuts)
{
	//step_init();
    
	double start;
	uint64_t comm_sz = 0;
    
	Bytes send, recv, bufr(Env::elm_size_in_bytes()*4);
	std::vector<Bytes> bufr_chunks, recv_chunks;
    
	G  gr, hr, X[2], Y[2];
    Z r, y, a, s[2], t[2];
    
    
	//std::cout <<" Node load: " <<  Env::node_load() << "\n";
    
	// step 1: generating the CRS: g[0], h[0], g[1], h[1]
	//struct timeval startot2, endot2;
    
	//gettimeofday(&startot2, NULL);
	
	//if(iteration==0)
	{
        
        if (Env::is_root())
        {
            GEN_BEGIN
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
            	GEN_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
            GEN_END
            
            EVL_BEGIN
			start = MPI_Wtime();
            bufr = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;
            EVL_END
            
            comm_sz += bufr.size();
        }
        //std::cout << "1\n";
        // send g[0], g[1], h[0], h[1] to slave processes
        start = MPI_Wtime();
		//MPI_Bcast(&bufr[0], bufr.size(), MPI_BYTE, 0, m_mpi_comm);
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
			m_ot_keys[ix].reserve(m_ot_bit_cnt*2);
		}
        m_timer_evl += MPI_Wtime() - start;
        m_timer_gen += MPI_Wtime() - start;
        //std::cout << "2\n";
        // Step 2: ZKPoK of (g[0], g[1], h[0], h[1])
        
        //MPI_Barrier(m_mpi_comm);
        //std::cout << m_ot_g[0].to_bytes().to_hex()<<"   " << m_ot_g[1].to_bytes().to_hex()<< "     "<<m_ot_h[0].to_bytes().to_hex() << "   " << m_ot_h[1].to_bytes().to_hex()<<"\n";
        
        // Step 3: gr=g[b]^r, hr=h[b]^r, where b is the evaluator's bit
        
        GEN_BEGIN
		start = MPI_Wtime();
        send.resize(Env::exp_size_in_bytes()*m_ot_bit_cnt);
        bufr.resize(Env::elm_size_in_bytes()*m_ot_bit_cnt*2);
        
        if (Env::is_root())
        {
            send.clear(); bufr.clear();
            for (size_t bix = 0; bix < m_ot_bit_cnt; bix++)
            {
                r.random();
                send += r.to_bytes();  // to be shared with slaves
                
                byte bit_value = m_evl_inp.get_ith_bit(bix);
                
                bufr += (m_ot_g[bit_value]^r).to_bytes(); // gr
                bufr += (m_ot_h[bit_value]^r).to_bytes(); // hr
            }
        }
		m_timer_evl += MPI_Wtime() - start;
        
		start = MPI_Wtime();
        //MPI_Bcast(&send[0], send.size(), MPI_BYTE, 0, m_mpi_comm); // now every evaluator has r's
        sendhold = send;
		m_timer_mpi += MPI_Wtime() - start;
        GEN_END
        //std::cout << "3\n";
        if (Env::is_root())
        {
            GEN_BEGIN
			// send (gr, hr)'s
			start = MPI_Wtime();
            	GEN_SEND(bufr);
			m_timer_com += MPI_Wtime() - start;
            GEN_END
            
            EVL_BEGIN
			// receive (gr, hr)'s
			start = MPI_Wtime();
            bufr = EVL_RECV();
			m_timer_com += MPI_Wtime() - start;
            EVL_END
            
            comm_sz += bufr.size();
        }
        
	}
	//std::cout << "4\n";
	// Step 4: the generator computes X[0], Y[0], X[1], Y[1]
    
	/*gettimeofday(&endot2, NULL);
     
     seconds  = endot2.tv_sec  - startot2.tv_sec;
     useconds = endot2.tv_usec - startot2.tv_usec;
     
     mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
     
     printf("time: %ld \n", mtime);	*/
	//gettimeofday(&startot, NULL);
	m_ot_keys.resize(1);
	for (size_t ix = 0; ix < m_ot_keys.size(); ix++)
	{
		m_ot_keys[ix].reserve(m_ot_bit_cnt*2);
	}
    
	EVL_BEGIN
    // forward (gr, hr)'s to slaves
    start = MPI_Wtime();
    bufr.resize(m_ot_bit_cnt*2*Env::elm_size_in_bytes());
    m_timer_gen += MPI_Wtime() - start;
    
       start = MPI_Wtime();
    bufr_chunks = bufr.split(Env::elm_size_in_bytes());
    m_timer_gen += MPI_Wtime() - start;
    
    //std::cout << bufr.to_hex() << "\n";
    
    for (size_t bix = 0; bix < m_ot_bit_cnt; bix++)
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
            
            
            m_ot_keys[cix].push_back(Y[0].to_bytes().hash(lengthOfInOuts));
            m_ot_keys[cix].push_back(Y[1].to_bytes().hash(lengthOfInOuts));
            
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
            EVL_SEND(send);
            m_timer_com += MPI_Wtime() - start;
            
            comm_sz += send.size();
        }
    }
    
    for (size_t ix = 0; ix < m_ot_keys.size(); ix++)
    {
        assert(m_ot_keys[ix].size() == m_ot_bit_cnt*2);
    }
	EVL_END
	//std::cout << "5\n";
	// Step 5: the evaluator computes K = Y[b]/X[b]^r
	GEN_BEGIN
    send = sendhold;
    
    start = MPI_Wtime(); // send has r's
    bufr_chunks = send.split(Env::exp_size_in_bytes());
    m_timer_evl += MPI_Wtime() - start;
    
    for (size_t bix = 0; bix < m_ot_bit_cnt; bix++)
    {
        start = MPI_Wtime();
        
        int bit_value = m_evl_inp.get_ith_bit(bix);
        
        
        r.from_bytes(bufr_chunks[bix]);
        m_timer_evl += MPI_Wtime() - start;
        
        for (size_t cix = 0; cix < 1; cix++)
        {
            start = MPI_Wtime();
            recv = GEN_RECV(); // receive X[0], X[1], Y[0], Y[1]
            m_timer_com += MPI_Wtime() - start;
            
            comm_sz += recv.size();
            
            start = MPI_Wtime();
            recv_chunks = recv.split(Env::elm_size_in_bytes());
            
            X[bit_value].from_bytes(recv_chunks[    bit_value]); // X[b]
            Y[bit_value].from_bytes(recv_chunks[2 + bit_value]); // Y[b]
            
            // K = Y[b]/(X[b]^r)
            //std::cout <<" "<<X[bit_value].size()<<" "<<r.size()<<"\n";
            
            Y[bit_value] /= X[bit_value]^r;
            m_ot_keys[cix].push_back(Y[bit_value].to_bytes().hash(lengthOfInOuts));
            m_timer_evl += MPI_Wtime() - start;
        }
    }
    
    for (size_t ix = 0; ix < 1; ix++)
    {
        assert(m_ot_keys[ix].size() == m_ot_bit_cnt);
    }
	GEN_END
}


void BetterYao::BM_OT_ext_with_third( const size_t k, const size_t l)
{
    
    
    
    /*string stt;

    
  GEN_BEGIN
	stt = "GEN";	 	 
    GEN_END

EVL_BEGIN
	stt = "EVL";
EVL_END
*/
    
   
	Bytes                   S, R, switchBits;
	vector<Bytes>	        T;
	vector<Bytes>         &Q = T;
	vector<vector<Bytes> > X, Y;
	
   
	Bytes send, recv, bufr;
	vector<Bytes> bufr_chunks;
	
   
//	cout << "1\n"; 
    
	int m = Env::circuit().evl_inp_cnt();
	//k=128;
    
    
	S = m_prng.rand(k); //should be k
   	R = m_evl_inp; 
	
	vector<Bytes> keys_for_node;
	keys_for_node.resize(k*256);
    
	vector<Bytes> outs;
	outs.resize(k*256);
    
//cout << "2\n";

	Bytes m_evl_inp_back = m_evl_inp;	



	GEN_BEGIN
    
	m_ot_keys.clear();
        
	if(Env::is_root())
	{
        
        
		Bytes in = m_evl_inp;
		m_evl_inp = m_prng.rand(k*Env::s());
		/*std::cout<<"\n";
         for(int i=0;i<k;i++)
         std::cout << (int)S.get_ith_bit(i);*/
        
		//std::cout<<"\n";
        
		for(int i=0;i<k*Env::s();i++)
		{
			m_evl_inp.set_ith_bit(i,S.get_ith_bit(i%k));
			//std::cout << (int)m_evl_inp.get_ith_bit(i);
		}
		//std::cout<<"\n";
        
		//int num_of_evls = m_all_chks.size()*2/5;
        
		m_ot_bit_cnt = k;
		

        
        
		oblivious_transfer_with_inputs_gen_third_SINGLE(Env::circuit().evl_inp_cnt());
        
	
		//std::cout <<"\n"<< m_ot_keys[0].size() <<"\n";
        
		for(int i=0;i<k;i++)
			outs[i] = m_ot_keys[0][i];		
        
		m_ot_keys.clear();
		m_evl_inp = in;
        
		int temp=0;
        
	
        
        
		
	}
	else
	{
		
	}	
    
	for(int j=0;j<k*Env::node_load();j++)
	{
		//std::cout <<Env::world_rank()<<" "<< outs[j+(Env::world_rank())*k*Env::node_load()].to_hex()<<"\n";
	}
	
	//MPI_Bcast(&switchBits[0], switchBits.size(), MPI_BYTE, 0, m_mpi_comm);
    
    
	GEN_END
    
   
//cout <<stt<< "3\n";



	EVL_BEGIN

		Bytes in = m_evl_inp;
       

                m_ot_bit_cnt = k;
                oblivious_transfer_with_inputs_gen_third_SINGLE(m);

                outs.resize(k*2);
                for(int i=0;i<k*2;i++)
                {
                        outs[i] = m_ot_keys[0][i];
                }


                m_ot_keys.clear();
                m_evl_inp = in;
	
	EVL_END    
    
    

//cout << stt<<"4\n";


    
	m_evl_inp_back = m_evl_inp;
    
	GEN_BEGIN
    //change input wires
    m_evl_inp = S;//m_prng.rand(k);
   
    S = m_evl_inp;
    
    switchBits = m_prng.rand(m);
  
    
	GEN_END
    
    
    
    //std::cout <<"node load: "<<Env::node_load()<<"\n";
    
    //for(iteration=0;iteration<Env::node_load();iteration++)
    //{
	//if(m_chks[iteration]) continue;
   

//cout << stt<<"5\n";

    if(Env::is_root())
    {
        
        GEN_BEGIN
		X.resize(m);
        
		for (size_t i = 0; i < m; i++)
		{
			X[i].resize(2);
			X[i][0] = CircuitInput0CircuitBase[i*2]; //KeySaves[iteration][i*2];// m_prng.rand(l);
			X[i][1] = CircuitInput0CircuitBase[i*2+1]; //KeySaves[iteration][i*2+1];  //m_prng.rand(l);
			/*if(Env::is_root())
             std::cout << X[i][0].to_hex() <<" " << X[i][1].to_hex() << "\n";*/
		}
        GEN_END
        
        
        //cout << Env::circuit().evl_inp_cnt() << "\n"
        R = m_evl_inp;//m_prng.rand(m);// instead of m
                
        
        //std::cout << S.to_hex() << "\n";
        //std::cout << R.to_hex() << "\n";
        
        EVL_BEGIN
		T.resize(k);
        
		for (size_t jx = 0; jx < k; jx++)
		{
			T[jx] = m_prng.rand(m); //instead of m - which it should be m
		}
        EVL_END
        
//cout << stt<<"6\n";

        m_ot_bit_cnt = k;
        
        EVL_BEGIN // evaluator (OT-EXT receiver/OT sender)
		//start = MPI_Wtime();
        m_ot_send_pairs.clear();
        m_ot_send_pairs.reserve(2*k);
        for (size_t ix = 0; ix < k; ix++)
        {
            /*for(int i=0;i<32;i++)
             {
             T[ix].set_ith_bit(i,1);
             R.set_ith_bit(i,0);
             }*/
            
            Bytes q = T[ix]^R;
            
            /*if(Env::group_rank()==1)
             std::cout << T[ix].to_hex() << " " << (q).to_hex() << "\n";*/
            
            m_ot_send_pairs.push_back(T[ix]);
            m_ot_send_pairs.push_back(q);
        }
		//m_timer_evl += MPI_Wtime() - start;
        EVL_END
        
        /*GEN_BEGIN // generator (OT-EXT sender/OT receiver)
         m_ot_recv_bits = S;
         GEN_END*/
        
        
        
      //  cout << stt<<"7\n";
        
        
        //backup eval's input
        
        
        
        //std::cout << "enter ot\n";
        
        
        // real OT
        //oblivious_transfer_with_inputs_gen_third(m); 
        
	GEN_BEGIN

        m_ot_keys.resize(1);
        m_ot_keys[0].resize(k);
        
        for(int i=0;i<k;i++)
        {
            m_ot_keys[0][i] = outs[i];
        }
        
	GEN_END

	EVL_BEGIN
		m_ot_keys.resize(1);
        	m_ot_keys[0].resize(k*2);
		for(int i=0;i<k*2;i++)
		{
			m_ot_keys[0][i] = outs[i];
		}
	EVL_END        

//cout << stt<<"8\n";

        
        EVL_BEGIN
		vector<Bytes> xorValues;
		xorValues.resize(2*k);
		send.clear();
		
		for(int i=0;i<k;i++)
		{
			Bytes x0 = T[i]^m_ot_keys[0][i*2];
			Bytes x1 = T[i]^m_ot_keys[0][i*2+1]^R;
            
			send += x0;
			send += x1;
            
			//std::cout << x0.to_hex() << " " << x1.to_hex() << "\n";
		}
		EVL_SEND(send);
        EVL_END
        
//cout << stt<<"9\n";

        int splitSize = (m+7)/8;
        GEN_BEGIN
        
        
		Bytes recieved = GEN_RECV();
		vector<Bytes> recv_vect = recieved.split(splitSize);
		m_ot_keys[0].resize(recv_vect.size());		
        
		for(int i=0;i<recv_vect.size()/2;i++)
		{
			if(m_evl_inp.get_ith_bit(i)==0)
			{
				m_ot_keys[0][i]^=recv_vect[i*2];
			}
			else
			{
				m_ot_keys[0][i]^=recv_vect[i*2+1];
			}
		}
        
		/*if(Env::is_root())
         for(int i=0;i<m_ot_keys[0].size();i++)
         std::cout << m_ot_keys[0][i].to_hex()<<"\n";*/
		
		
        GEN_END
        
//cout << stt<<"10\n";

        GEN_BEGIN // generator (OT-EXT sender/OT receiver)
		//start = MPI_Wtime();
        Q.resize(k);
        for (size_t ix = 0; ix < k; ix++)
        {
            
            Q[ix] = m_ot_keys[0][ix]; // also Q for the generator
        }
        
		
		//m_timer_gen += MPI_Wtime() - start;
        GEN_END
       
//cout << stt<<"11\n"; 
        
        //START MATRIX TRANSOFMR
        GEN_BEGIN
		vector<Bytes> src = T;
		vector<Bytes> &dst = T;
        
		dst.clear();
		dst.resize(m);
		for(int i=0;i<m;i++)
		{
			dst[i].resize((k+7)/8, 0);
			for(int j=0;j<k;j++)
			{
				dst[i].set_ith_bit(j,src[j].get_ith_bit(i));
			}
		}
        
        GEN_END
        EVL_BEGIN
		vector<Bytes> src = T;
		vector<Bytes> &dst = T;
        
		dst.clear();
		dst.resize(m);
		for(int i=0;i<m;i++)
		{
			dst[i].resize((k+7)/8, 0);
			for(int j=0;j<k;j++)
			{
				dst[i].set_ith_bit(j,src[j].get_ith_bit(i));
			}
		}
        
        EVL_END
        //END MATRIX TRANSFORM
        
        //cout << stt<< "12\n";
                
        GEN_BEGIN
		send.clear();
		send.reserve(2*m);
        
        
        
		int bit = 0;
        
		for (size_t jx = 0; jx < m; jx++)
		{
			bit = 0;
            
            
			//could also be Q
			Bytes a = X[jx][0] ^ (Q[jx]).hash(l);
			Bytes b = X[jx][1] ^ (Q[jx]^S).hash(l);
            
			/*if(switchBits.get_ith_bit(jx)==1)
			{
				Bytes t;
				t = a;
				a = b;
				b = t;
			}*/
            
			send += a;
			send += b;
            
			/*if(Env::is_root())
             {
             std::cout <<"bit: "<<bit<<" getbit: "<<switchBits.get_ith_bit(jx)<<
             "\norig:" << (a).to_hex() << " " <<(b).to_hex() <<
             "\nnow:"<< (b).to_hex() << " " <<(a).to_hex() <<"\n";
             }*/
		}
		//std::cout <<send.size()<<"\n";
		GEN_SEND(switchBits);
		GEN_SEND(send);

        GEN_END
        
        EVL_BEGIN
		switchBits = EVL_RECV();
		//std::cout << switchBits.to_hex()<<"\n";
        EVL_END
        
        //std::cout <<"here1\n";
        EVL_BEGIN
		bufr = EVL_RECV();
		//std::cout <<bufr.size()<<" "<<(l+7)/8<<"\n";
        
		bufr_chunks = bufr.split((l+7)/8);
		//std::cout <<bufr.size()<<" "<<(l+7)/8<<"\n";
		
		Y.resize(m);
		
		int idx = 0;
        
		//bufr should evtually be send
		for (size_t jx = 0; jx < m; jx++)
		{
			Y[jx].resize(2);
			
			Y[jx][0] = bufr_chunks[idx++];
			Y[jx][1] = bufr_chunks[idx++];
		}
        EVL_END	
        
        //send R
        EVL_BEGIN
		send.clear();
		send.resize(1);
		send = R;
		for(int i=0;i<m;i++)
		{
			int bit = R.get_ith_bit(i);
			/*if(switchBits.get_ith_bit(i)==1)
				bit = 1 - bit;*/
			send.set_ith_bit(i,bit);
            
		
		}
		 
      		R = send; 
        EVL_END
        
        //send T's
        EVL_BEGIN
		send.clear();
		send.reserve(m);
        
		for (size_t jx = 0; jx < m; jx++)
		{
			//could also be Q
			send += T[jx];
			//send += X[jx][1] ^ (Q[jx]^S).hash(l);
		}
		
		bufr = send; 
		
        
		bufr_chunks = bufr.split((k+7)/8);
		T.resize(m);
		//Y.resize(m);
		
		//int idx = 0;
        
		//bufr should evtually be send
		for (size_t jx = 0; jx < m; jx++)
		{
			T[jx] = bufr_chunks[jx];
			/*Y[jx].resize(2);
             
             Y[jx][0] = bufr_chunks[idx++];
             Y[jx][1] = bufr_chunks[idx++];*/
		}
        EVL_END
        
        
        //std::cout <<"here2\n";
        EVL_BEGIN
		vector<Bytes> outputs;
		outputs.resize(m);
		m_ot_keys[0].resize(m);
        
		for(size_t i=0;i<m;i++)
		{
			outputs[i]= Y[i][R.get_ith_bit(i)]^T[i].hash(l);
			if(Env::is_root())
			{
				// Y[i][R.get_ith_bit(i)]^(T[i]^R).hash(l);
				//std::cout << outputs[i].to_hex()<<" "<<R.get_ith_bit(i) << " \n";
			}
			m_ot_keys[0][i] = outputs[i];
			//KeySaves[iteration][i] = outputs[i];
		}
        
        
        
        
        EVL_END
        
        GEN_BEGIN
		m_ot_keys[0].resize(m*2);
		//std::cout << "mkey size: " << m_ot_keys.size() << "\n";
		for(int i=0;i<m;i++)
		{
			m_ot_keys[0][i*2] = X[i][0];
			m_ot_keys[0][i*2+1] = X[i][1];	
            
			//KeySaves[iteration][i*2] = X[i][0];
			//KeySaves[iteration][i*2+1] = X[i][1];
            
			
		}
        GEN_END
        
        
    }
    
   /* 
    EVL_BEGIN
	
	//std::cout <<"beginotmodpart\n";
    
	m_ot_keys.resize(KeySaves.size());
    
	Bytes buf;
	
	buf.resize(m*10);
    
	if(Env::is_root())
	{
		buf.merge(m_ot_keys[0]);
	}
    
	//std::cout <<"beginotmodpart1\n";
    
    
    
	MPI_Bcast(&buf[0], buf.size(), MPI_BYTE, 0, m_mpi_comm);	
	//std::cout <<"beginotmodpar2\n";
    
    
    
	m_ot_keys[0] = buf.split(10);
    
	//std::cout <<"beginotmodpart3\n";
    
    
	CircuitInputKeys.resize(Env::node_load());
    
	for(int i=0;i<Env::node_load();i++)
	{
		CircuitInputKeys[i] = TO_THIRD_GEN_RECV(); 
	}
    
	//std::cout <<"beginotmodpart4\n";
    
    
    
	for(int i=0;i<Env::node_load();i++)
	{
        
		for(int j=0;j<m_ot_keys[0].size();j++)
		{
			KeySaves[i][j] = hashBytes(m_ot_keys[0][j],CircuitInputKeys[i]); 
		}
	}
    
	//std::cout <<"endotmodpart\n";
    
    EVL_END
	
	
	//if(iteration==0)
	{
		m_ot_keys.clear();
	}
    
    
	m_ot_keys.resize(KeySaves.size());
	for(int i=0;i<KeySaves.size();i++)
	{
		m_ot_keys[i].resize(KeySaves[i].size());
		for(int j=0;j<KeySaves[i].size();j++)
			m_ot_keys[i][j] = KeySaves[i][j];
	}
    
	//std::cout <<"endfullot\n";
     */	
    
	//reset input to be correct wires
	m_evl_inp = m_evl_inp_back;
}






