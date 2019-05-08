


#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <map>
#include <queue>

#include <unistd.h>
#include <ext/hash_map>

#include "GarbledCct.h"

struct my_hash_fun
{
  size_t
  operator()(uint64_t __x) const
  { return static_cast<size_t>(__x); }
};

inline void print_uint64(uint64_t l)
{
	char buf[8];
	std::string str;

	while (l >= 1000)
	{
		sprintf(buf, ",%03d", (int)(l%1000));
		str = buf + str;

		l = l / 1000LL;
	}

	sprintf(buf, "%d", (int)l);
	str = buf + str;

	std::cout << std::setw(15) << str;
}

//
// 1) Binary Parsing Test
//
void parse_test(const char *file_name)
{
	Circuit circuit;
	circuit.load_binary_old(file_name);

	uint64_t wire_ix = 0;
	uint32_t gen_inp_cnt = 0, evl_inp_cnt = 0, gen_out_cnt = 0, evl_out_cnt = 0;

	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary_old();

		if (g.m_table.size() != 0 && g.m_table.size() != (0x01<<g.m_input_idx.size()))
		{
			std::cerr << "table-input number mismatch: " << wire_ix << std::endl;
			exit(EXIT_FAILURE);
		}

		for (size_t jx = 0; jx < g.m_input_idx.size(); jx++)
		{
			if (g.m_input_idx[jx] >= wire_ix)
			{
				std::cerr << "input wire error: " << wire_ix << " (" << std::ios::hex << g.m_input_idx[jx] << ")" << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		switch(g.m_tag)
		{
		case Circuit::GEN_INP:
			gen_inp_cnt++; break;

		case Circuit::EVL_INP:
			evl_inp_cnt++; break;

		case Circuit::GEN_OUT:
			gen_out_cnt++; break;

		case Circuit::EVL_OUT:
			evl_out_cnt++; break;
		}

		wire_ix++;
	}

	std::cout << "Read data:" << std::endl
			  << "gate_cnt: "; print_uint64(circuit.gate_cnt()); std::cout << std::endl
			  << "gen_inp_cnt: "; print_uint64(circuit.gen_inp_cnt()); std::cout << std::endl
			  << "evl_inp_cnt: "; print_uint64(circuit.evl_inp_cnt()); std::cout << std::endl
			  << "gen_out_cnt: "; print_uint64(circuit.gen_out_cnt()); std::cout << std::endl
			  << "evl_out_cnt: "; print_uint64(circuit.evl_out_cnt()); std::cout << std::endl;

	std::cout << "Parsed data:" << std::endl
			  << "gate_cnt: "; print_uint64(wire_ix); std::cout << std::endl
			  << "gen_inp_cnt: "; print_uint64(gen_inp_cnt); std::cout << std::endl
			  << "evl_inp_cnt: "; print_uint64(evl_inp_cnt); std::cout << std::endl
			  << "gen_out_cnt: "; print_uint64(gen_out_cnt); std::cout << std::endl
			  << "evl_out_cnt: "; print_uint64(evl_out_cnt); std::cout << std::endl;
}

//
// 2) Evaluating Test
//
void evaluate_test(const char *file_name, const char *input_name)
{
	Circuit circuit;
	circuit.load_binary_old(file_name);

	std::ifstream private_file(input_name);
	std::string input;
	Bytes gen_inp, evl_inp, gen_out, evl_out;

	if (!private_file.is_open())
	{
		std::cerr << "input file open failure: " << input_name << std::endl;
		exit(EXIT_FAILURE);
	}

	private_file >> input; // 1st line is evaluator's input
	evl_inp.from_hex(input);

	private_file >> input; // 2nd line is generator's input
	gen_inp.from_hex(input);

	evl_inp.resize((circuit.evl_inp_cnt()+7)/8);
	gen_inp.resize((circuit.gen_inp_cnt()+7)/8);

	std::cout << "GEN_INP: " << gen_inp.to_hex() << std::endl;
	std::cout << "EVL_INP: " << evl_inp.to_hex() << std::endl;

	circuit.evaluate_binary_old(gen_inp, evl_inp, gen_out, evl_out);

	std::cout << "GEN_OUT: " << gen_out.to_hex() << std::endl;
	std::cout << "EVL_OUT: " << evl_out.to_hex() << std::endl;

	private_file.close();
}

//
// 3) Max Distance Test
//
void max_distance_test(const char *file_name)
{
	Circuit circuit;
	circuit.load_binary_old(file_name);

	uint64_t max_dist = 0, gate_ix = 0, max_dist_gate_ix;

	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary_old();

		for (size_t ix = 0; ix < g.m_input_idx.size(); ix++)
		{
			uint64_t dist = gate_ix - g.m_input_idx[ix];

			if (dist > max_dist)
			{
				max_dist = dist;
				max_dist_gate_ix = gate_ix;
			}
		}

		gate_ix++;
	}

	std::cout << "Maximum distance: " << max_dist
			  << " in gate " << max_dist_gate_ix <<std::endl;
}

//
// 4) Component Statistics Test
//
void statistics_test(const char *file_name)
{
	Circuit circuit;
	circuit.load_binary_old(file_name);

	std::vector<uint64_t> arity, xor_gates;
	uint64_t cnt_00 = 0, cnt_11 = 0, cnt_01 = 0, cnt_10 = 0;

	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary_old();

		if (g.m_input_idx.size() == 0)
			 continue;

		if (g.m_input_idx.size() > arity.size())
		{
			arity.resize(g.m_input_idx.size());
			xor_gates.resize(g.m_input_idx.size());
		}

		arity[g.m_input_idx.size()-1]++;

		if (is_xor(g))
			xor_gates[g.m_input_idx.size()-1]++;

		if (g.m_input_idx.size() == 1)
		{
			if (!g.m_table[0] && !g.m_table[1])
				cnt_00++;

			if (g.m_table[0] && g.m_table[1])
				cnt_11++;

			if (!g.m_table[0] && g.m_table[1])
				cnt_01++;

			if (g.m_table[0] && !g.m_table[1])
				cnt_10++;
		}

	}

	std::cout << "arity: ";
	for (size_t ix = 0; ix < arity.size(); ix++)
	{
		std::cout << (ix+1) << ")"; print_uint64(arity[ix]); std::cout << "; ";
	}
	std::cout << std::endl;

	std::cout << "  xor: ";
	for (size_t ix = 0; ix < xor_gates.size(); ix++)
	{
		std::cout << (ix+1) << ")"; print_uint64(xor_gates[ix]); std::cout << "; ";
	}
	std::cout << std::endl;

	std::cout << "# of [ 0 0 ]: "; print_uint64(cnt_00); std::cout << std::endl;
	std::cout << "# of [ 0 1 ]: "; print_uint64(cnt_01); std::cout << std::endl;
	std::cout << "# of [ 1 0 ]: "; print_uint64(cnt_10); std::cout << std::endl;
	std::cout << "# of [ 1 1 ]: "; print_uint64(cnt_11); std::cout << std::endl;
}

//
// 5) Counter Correctness Test
//
void use_count_test(const char *file_name)
{
	Circuit circuit;
	circuit.load_binary_old(file_name);
	uint64_t current_gate_ix = 0;
	Prng prng;

	const int COUNT = 1000;

	std::vector<uint64_t> ixs;
	std::vector<uint64_t>::const_iterator iit;

	std::vector<Gate>     gates;
	std::vector<Gate>::iterator git;

	// sample random gates to check
	for (size_t ix = 0; ix < COUNT; ix++)
		ixs.push_back(prng.rand_range(circuit.gate_cnt()));
	std::sort(ixs.begin(), ixs.end());
	ixs.erase(std::unique(ixs.begin(), ixs.end()), ixs.end()); // remove duplicates

	// check the usage counter for chosen gates
	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary_old();

		// check if current gate uses the chosen gates
		for (iit = g.m_input_idx.begin(); iit != g.m_input_idx.end(); iit++)
			for (size_t ix = 0; ix < gates.size(); ix++)
		{
			if (*iit == ixs[ix])
			{
				gates[ix].m_idx--;
			}
		}

		if (std::binary_search(ixs.begin(), ixs.end(), current_gate_ix))
			gates.push_back(g);

		current_gate_ix++;
	}

	bool ret = true;
	for (int i = 0; i < gates.size(); i++)
	{
		// skip output wires
		if (gates[i].m_tag == Circuit::EVL_OUT || gates[i].m_tag == Circuit::GEN_OUT)
		{
			continue;
		}

		if (gates[i].m_idx != 0)
		{
			std::cout << ixs[i] << " : " << gates[i].m_idx << " (wrong)" << std::endl;
			ret = false;
		}
	}

	if (ret)
		std::cout << "use count check succeeded!" << std::endl;
}

//
// 6) Semi-honest Yao Protocol Test
//
void yao_test(const char *file_name, const char *input_name)
{
	// init Env
	const int k = 80;
	EnvParams params;
	params.secu_param = k;
	params.claw_free.init();
	params.circuit.load_binary_old(file_name);

	Env::init(params);

	// init inputs
	std::ifstream private_file(input_name);
	std::string input;
	Bytes gen_inp, evl_inp, gen_out, evl_out;

	if (!private_file.is_open())
	{
		std::cerr << "file open failed: " << input_name << std::endl;
		exit(EXIT_FAILURE);
	}

	private_file >> input; // 1st line is evaluator's input
	evl_inp.from_hex(input);

	private_file >> input; // 2nd line is generator's input
	gen_inp.from_hex(input);

	private_file.close();

	evl_inp.resize((Env::circuit().evl_inp_cnt()+7)/8);
	gen_inp.resize((Env::circuit().gen_inp_cnt()+7)/8);

	std::cout << "GEN_INP: " << gen_inp.to_hex() << std::endl;
	std::cout << "EVL_INP: " << evl_inp.to_hex() << std::endl;

	Prng prng;

	// ot
	std::vector<Bytes> ot_gen_keys, ot_evl_keys;
	for (size_t ix = 0; ix < Env::circuit().evl_inp_cnt(); ix++)
	{
		ot_evl_keys.push_back(prng.rand(k));
		if ((evl_inp[ix/8]>>(ix%8)) & 0x01)
		{
			ot_gen_keys.push_back(prng.rand(k));
			ot_gen_keys.push_back(ot_evl_keys.back());
		}
		else
		{
			ot_gen_keys.push_back(ot_evl_keys.back());
			ot_gen_keys.push_back(prng.rand(k));
		}
	}

	// Garbled circuit generation/evaluation
	std::cout << "circuit generation/evaluation" << std::endl;

	clock_t start, end;

	Bytes gen_inp_mask = prng.rand(Env::circuit().gen_inp_cnt());
	gen_inp.resize((Env::circuit().gen_inp_cnt()+7)/8);
	Bytes rnd = prng.rand(k);

	GarbledCct gen_cct, evl_cct, com_cct;
	com_cct.gen_init(ot_gen_keys, gen_inp_mask, rnd);
//	gen_cct.gen_init(ot_gen_keys, gen_inp_mask, rnd);
//	evl_cct.evl_init(ot_evl_keys, gen_inp_mask^gen_inp, evl_inp);

	clock_t timer[4] = {0}, timer_start;
	int gen_inp_ix = 0, cnt = 0, gate_ix = 0;

//	std::cout << "com_cct.m_gen_w.bucket_count(): " << com_cct.m_gen_w.bucket_count() << std::endl
//			  << "com_cct.m_evl_w.bucket_count(): " << com_cct.m_evl_w.bucket_count() << std::endl;

	// circuit committing test
	start = clock();
	while (Env::circuit().more_gate_binary())
	{
		com_cct.com_next_gate(Env::circuit().next_gate_binary_old());
	}
	std::cout << "Time to commit/construct circuit: "
			  << (clock() - start)/(double)CLOCKS_PER_SEC << std::endl;


//	com_cct.gen_init(ot_gen_keys, gen_inp_mask, rnd);
//	Env::circuit().reload_binary_old();
//
//	// hash_map load_factor test
//	double load_factor_1 = 0.0, load_factor_2 = 0.0;
//	gate_ix = 0;
//
//	std::cout << "com_cct.m_gen_w.bucket_count(): " << com_cct.m_gen_w.bucket_count() << std::endl
//			  << "com_cct.m_evl_w.bucket_count(): " << com_cct.m_evl_w.bucket_count() << std::endl;
//
//	start = clock();
//	while (Env::circuit().more_gate_binary())
//	{
//		com_cct.com_next_gate(Env::circuit().next_gate_binary_old());
//		load_factor_1 = (load_factor_1*gate_ix + com_cct.m_gen_w.size()/(double)com_cct.m_gen_w.bucket_count())/(gate_ix+1);
//		gate_ix++;
//		load_factor_2 += com_cct.m_gen_w.size()/(double)com_cct.m_gen_w.bucket_count();
//	}
//	std::cout << "Time to commit/construct circuit: "
//			  << (clock() - start)/(double)CLOCKS_PER_SEC << std::endl;
//	std::cout << "load_factor_1: " << load_factor_1 << std::endl
//			  << "load_factor_2: " << load_factor_2/gate_ix << std::endl;
//
//	Env::circuit().reload_binary_old();
//
//	// circuit generation/evaluation test
//	start = clock();
//	while (Env::circuit().more_gate_binary())
//	{
//		const Gate &g = Env::circuit().next_gate_binary_old();
//		gen_cct.gen_next_gate(g);
//		evl_cct.recv(gen_cct.send());
//
//		if (g.m_tag == Circuit::GEN_INP)
//		{
//			// send the generator's input keys to the evaluator
//			int bit_value = (gen_inp[gen_inp_ix/8]>>(gen_inp_ix%8)) & 0x01;
//			evl_cct.m_M.push_back(gen_cct.m_M[2*gen_inp_ix + bit_value]);
//			gen_inp_ix++;
//		}
//
//		evl_cct.evl_next_gate(g);
//		gate_ix++;
//	}
//	std::cout << "Time to run semi-honest Yao: "
//			  << (clock() - start)/(double)CLOCKS_PER_SEC << std::endl;
//
//	std::cout << "com_cct.m_gen_w.bucket_count(): " << com_cct.m_gen_w.bucket_count() << std::endl
//   			  << "com_evl.m_evl_w.bucket_count(): " << com_cct.m_evl_w.bucket_count() << std::endl;
//
//	std::cout << "max_use_size: " << gen_cct.m_max_map_size << std::endl;
//	std::cout << "max_map_size: " << gen_cct.m_gen_w.max_size() << std::endl;
//	std::cout << "m_gen_w.size: " << gen_cct.m_gen_w.size() << std::endl
//              << "m_evl_w.size: " << evl_cct.m_evl_w.size() << std::endl;
//	std::cout << "GEN_OUT: " << evl_cct.m_gen_out.to_hex() << std::endl;
//	std::cout << "EVL_OUT: " << evl_cct.m_evl_out.to_hex() << std::endl;
}

//
// 7) Circuit Dump Test
//
void dump_test(const char *file_name)
{
	Circuit circuit;
	circuit.load_binary_old(file_name);

	uint64_t idx = 0;
	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary_old();
		std::cout << idx++ << " " << g << std::endl;
	}
}


//
// 8) Remove hash table Test
//
void remove_hash(const char *circuit_file_name, const char *input_name)
{
	Circuit circuit;
	circuit.load_binary_old(circuit_file_name);

	// computing hash table size
    uint64_t tbl_max = 0;
	uint64_t gate_ix = 0;

	__gnu_cxx::hash_map<uint64_t, std::pair<uint32_t, uint64_t>, my_hash_fun> hash_tbl;
	std::priority_queue<uint32_t, std::deque<uint32_t>, std::greater<uint32_t> > avail_idx;

	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary_old();

		for (size_t jx = 0; jx < g.m_input_idx.size(); jx++)
		{
			assert(hash_tbl.count(g.m_input_idx[jx]) == 1);

			__gnu_cxx::hash_map<uint64_t, std::pair<uint32_t, uint64_t>, my_hash_fun>::data_type
				&E = hash_tbl[g.m_input_idx[jx]];

			if (--E.second == 0)
			{
				avail_idx.push(E.first);
				hash_tbl.erase(g.m_input_idx[jx]);
			}
		}

		if (avail_idx.empty())
		{
			avail_idx.push(tbl_max++);
		}
		uint32_t idx = avail_idx.top();
		avail_idx.pop();

		hash_tbl[gate_ix] = std::make_pair(idx, g.m_idx);

		gate_ix++;
	}

	std::cout << "avail_tbl max: " << tbl_max << std::endl;

	std::string of_name(circuit_file_name);
	of_name += ".hash-free";
	std::ofstream of(of_name.c_str(), std::ios::out | std::ios::binary);

	circuit.reload_binary_old();

	uint64_t gate_cnt = circuit.gate_cnt();
	uint32_t gen_inp_cnt = circuit.gen_inp_cnt();
	uint32_t evl_inp_cnt = circuit.evl_inp_cnt();
	uint32_t gen_out_cnt = circuit.gen_out_cnt();
	uint32_t evl_out_cnt = circuit.evl_out_cnt();

	of.write(reinterpret_cast<char*>(&gate_cnt), sizeof(uint64_t));
	of.write(reinterpret_cast<char*>(&gen_inp_cnt), sizeof(uint32_t));
	of.write(reinterpret_cast<char*>(&evl_inp_cnt), sizeof(uint32_t));
	of.write(reinterpret_cast<char*>(&gen_out_cnt), sizeof(uint32_t));
	of.write(reinterpret_cast<char*>(&evl_out_cnt), sizeof(uint32_t));

	uint8_t idx_sz = 0;
	if (tbl_max <= UINT8_MAX)
	{
		idx_sz = 1;
		of.write(reinterpret_cast<char*>(&idx_sz), sizeof(uint8_t));
		of.write(reinterpret_cast<char*>(&tbl_max), sizeof(uint8_t));
		std::cout << "idx_sz: " << (int)idx_sz << ", max: " << tbl_max << std::endl;
	}
	else if (tbl_max <= UINT16_MAX)
	{
		idx_sz = 2;
		of.write(reinterpret_cast<char*>(&idx_sz), sizeof(uint8_t));
		of.write(reinterpret_cast<char*>(&tbl_max), sizeof(uint16_t));
		std::cout << "idx_sz: " << (int)idx_sz << ", max: " << tbl_max << std::endl;
	}
	else if (tbl_max <= UINT32_MAX)
	{
		idx_sz = 4;
		of.write(reinterpret_cast<char*>(&idx_sz), sizeof(uint8_t));
		of.write(reinterpret_cast<char*>(&tbl_max), sizeof(uint32_t));
		std::cout << "idx_sz: " << (int)idx_sz << ", max: " << tbl_max << std::endl;
	}
	else if (tbl_max <= UINT64_MAX)
	{
		idx_sz = 8;
		of.write(reinterpret_cast<char*>(&idx_sz), sizeof(uint8_t));
		of.write(reinterpret_cast<char*>(&tbl_max), sizeof(uint64_t));
		std::cout << "idx_sz: " << (int)idx_sz << ", max: " << tbl_max << std::endl;
	}

	tbl_max = 0;
	gate_ix = 0;

	hash_tbl.clear();
	while(!avail_idx.empty()) avail_idx.pop();

	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary_old();

		uint8_t tbl = 0;
		for (size_t jx = 0; jx < g.m_table.size(); jx++)
			tbl |= (g.m_table[jx]<<jx);
		tbl &= 0x0f;

		uint8_t tag = (g.m_tag<<5) | ((g.m_input_idx.size()==2)<<4) | tbl;

		of.write(reinterpret_cast<char*>(&tag), sizeof(tag));

		for (size_t jx = 0; jx < g.m_input_idx.size(); jx++)
		{
			assert(hash_tbl.count(g.m_input_idx[jx]) == 1);

			__gnu_cxx::hash_map<uint64_t, std::pair<uint32_t, uint64_t>, my_hash_fun>::data_type
				&E = hash_tbl[g.m_input_idx[jx]];

			of.write(reinterpret_cast<const char*>(&(E.first)), idx_sz);

			if (--E.second == 0)
			{
				avail_idx.push(E.first);
				hash_tbl.erase(g.m_input_idx[jx]);
			}
		}

		if (avail_idx.empty())
		{
			avail_idx.push(tbl_max++);
		}
		uint32_t idx = avail_idx.top();
		avail_idx.pop();

		// where current gate goes
		of.write(reinterpret_cast<const char*>(&idx), idx_sz);

		hash_tbl[gate_ix] = std::make_pair(idx, g.m_idx);

		gate_ix++;
	}

	std::cout << "hash tbl size: " << hash_tbl.size() << ", avail size: " << avail_idx.size() << std::endl;
	of.close();
}

//
// 9) Binary Parsing Test (hash free version)
//
void parse_test_hash_free(const char *file_name)
{
	Circuit circuit;
	circuit.load_binary(file_name);

	uint64_t wire_ix = 0;
	uint32_t gen_inp_cnt = 0, evl_inp_cnt = 0, gen_out_cnt = 0, evl_out_cnt = 0;


	long totalgates=0;
	long xorgates=0;
	long inputgates=0;

	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary();

		if (g.m_input_idx.size() == 0)
		{
			inputgates++;
		}

		{
			totalgates++;

			if(is_xor(g))
				xorgates++;
		}



		


		

		if (g.m_table.size() != 0 && g.m_table.size() != (0x01<<g.m_input_idx.size()))
		{
			std::cerr << "table-input number mismatch: " << wire_ix << std::endl;
			exit(EXIT_FAILURE);
		}

		switch(g.m_tag)
		{
		case Circuit::GEN_INP:
			gen_inp_cnt++; break;

		case Circuit::EVL_INP:
			evl_inp_cnt++; break;

		case Circuit::GEN_OUT:
			gen_out_cnt++; break;

		case Circuit::EVL_OUT:
			evl_out_cnt++; break;
		}

		wire_ix++;
	}

/*	std::cout << "Read data:" << std::endl
			  << "gate_cnt: "; print_uint64(circuit.gate_cnt()); std::cout << std::endl
			  << "gen_inp_cnt: "; print_uint64(circuit.gen_inp_cnt()); std::cout << std::endl
			  << "evl_inp_cnt: "; print_uint64(circuit.evl_inp_cnt()); std::cout << std::endl
			  << "gen_out_cnt: "; print_uint64(circuit.gen_out_cnt()); std::cout << std::endl
			  << "evl_out_cnt: "; print_uint64(circuit.evl_out_cnt()); std::cout << std::endl;

	std::cout << "Parsed data:" << std::endl
			  << "gate_cnt: "; print_uint64(wire_ix); std::cout << std::endl
			  << "gen_inp_cnt: "; print_uint64(gen_inp_cnt); std::cout << std::endl
			  << "evl_inp_cnt: "; print_uint64(evl_inp_cnt); std::cout << std::endl
			  << "gen_out_cnt: "; print_uint64(gen_out_cnt); std::cout << std::endl
			  << "evl_out_cnt: "; print_uint64(evl_out_cnt); std::cout << std::endl;

    std::cout << "m_cnt_size: " << (int)circuit.m_cnt_size << ", m_cnt: " << (int)circuit.m_cnt << std::endl;
*/	std::cout << "Gates: "<< totalgates<<"   xorgates: "<<xorgates<<"    inputgates: "<<inputgates<<std::endl;
}

//
// 10) Evaluating Test (hash free version)
//
void evaluate_test_hash_free(const char *file_name, const char *input_name)
{
	Circuit circuit;
	circuit.load_binary(file_name);

	std::ifstream private_file(input_name);
	std::string input;
	Bytes gen_inp, evl_inp, gen_out, evl_out;

	if (!private_file.is_open())
	{
		std::cerr << "input file open failure: " << input_name << std::endl;
		exit(EXIT_FAILURE);
	}

	private_file >> input; // 1st line is evaluator's input
	evl_inp.from_hex(input);

	private_file >> input; // 2nd line is generator's input
	gen_inp.from_hex(input);

	std::cout << "GEN_INP: " << gen_inp.to_hex() << std::endl;
	std::cout << "EVL_INP: " << evl_inp.to_hex() << std::endl;

	circuit.evaluate_binary(gen_inp, evl_inp, gen_out, evl_out);

	std::cout << "GEN_OUT: " << gen_out.to_hex() << std::endl;
	std::cout << "EVL_OUT: " << evl_out.to_hex() << std::endl;

	private_file.close();
}

//
// 11) Circuit Dump Test (hash free version)
//
void dump_test_hash_free(const char *file_name)
{
	Circuit circuit;
	circuit.load_binary(file_name);

	uint64_t idx = 0;
	while (circuit.more_gate_binary())
	{
		const Gate &g = circuit.next_gate_binary();
		std::cout << idx++ << " " << g <<  std::endl;
	}
}

//
// 12) Semi-honest Yao Protocol Test (hash free)
//
void yao_test_hash_free(const char *file_name, const char *input_name)
{
	// init Env
	const int k = 80;
	EnvParams params;
	params.secu_param = k;
	params.claw_free.init();
	params.circuit.load_binary(file_name);

	Env::init(params);

	// init inputs
	std::ifstream private_file(input_name);
	std::string input;
	Bytes gen_inp, evl_inp, gen_out, evl_out;

	if (!private_file.is_open())
	{
		std::cerr << "file open failed: " << input_name << std::endl;
		exit(EXIT_FAILURE);
	}

	private_file >> input; // 1st line is evaluator's input
	evl_inp.from_hex(input);

	private_file >> input; // 2nd line is generator's input
	gen_inp.from_hex(input);

	private_file.close();

	evl_inp.resize((Env::circuit().evl_inp_cnt()+7)/8);
	gen_inp.resize((Env::circuit().gen_inp_cnt()+7)/8);

	std::cout << "GEN_INP: " << gen_inp.to_hex() << std::endl;
	std::cout << "EVL_INP: " << evl_inp.to_hex() << std::endl;

	Prng prng;

	// ot
	std::vector<Bytes> ot_gen_keys, ot_evl_keys;
	for (size_t ix = 0; ix < Env::circuit().evl_inp_cnt(); ix++)
	{
		ot_evl_keys.push_back(prng.rand(k));
		if ((evl_inp[ix/8]>>(ix%8)) & 0x01)
		{
			ot_gen_keys.push_back(prng.rand(k));
			ot_gen_keys.push_back(ot_evl_keys.back());
		}
		else
		{
			ot_gen_keys.push_back(ot_evl_keys.back());
			ot_gen_keys.push_back(prng.rand(k));
		}
	}

	// Garbled circuit generation/evaluation
	std::cout << "circuit generation/evaluation" << std::endl;

	clock_t start, end;

	Bytes gen_inp_mask = prng.rand(Env::circuit().gen_inp_cnt());
	gen_inp.resize((Env::circuit().gen_inp_cnt()+7)/8);
	Bytes rnd = prng.rand(k);

	GarbledCct gen_cct, evl_cct, com_cct;
	com_cct.gen_init(ot_gen_keys, gen_inp_mask, rnd);
	gen_cct.gen_init(ot_gen_keys, gen_inp_mask, rnd);
	evl_cct.evl_init(ot_evl_keys, gen_inp_mask^gen_inp, evl_inp);

	clock_t timer[4] = {0}, timer_start;
	int gen_inp_ix = 0, cnt = 0, gate_ix = 0;

	// circuit committing test
	start = clock();
	while (Env::circuit().more_gate_binary())
	{
		const Gate &g = Env::circuit().next_gate_binary();
		com_cct.com_next_gate(g);
	}
	std::cout << "Time to commit/construct circuit: "
			  << (clock() - start)/(double)CLOCKS_PER_SEC << std::endl;

	Env::circuit().reload_binary();

	for (size_t ix = 0; ix < Env::circuit().gen_inp_cnt(); ix++)
	{
		// send the generator's input keys to the evaluator
		int bit_value = (gen_inp[ix/8]>>(ix%8)) & 0x01;
		evl_cct.m_M.push_back(gen_cct.m_M[2*ix + bit_value]);
	}

	// circuit generation/evaluation test
	start = clock();
	while (Env::circuit().more_gate_binary())
	{
		const Gate &g = Env::circuit().next_gate_binary();
		gen_cct.gen_next_gate(g);
		evl_cct.recv(gen_cct.send());

//		if (g.m_tag == Circuit::GEN_INP)
//		{
//			// send the generator's input keys to the evaluator
//			int bit_value = (gen_inp[gen_inp_ix/8]>>(gen_inp_ix%8)) & 0x01;
//			evl_cct.m_M.push_back(gen_cct.m_M[2*gen_inp_ix + bit_value]);
//			gen_inp_ix++;
//		}

		evl_cct.evl_next_gate(g);
		gate_ix++;
	}
	std::cout << "Time to run semi-honest Yao: "
			  << (clock() - start)/(double)CLOCKS_PER_SEC << std::endl;

	std::cout << "GEN_OUT: " << evl_cct.m_gen_out.to_hex() << std::endl;
	std::cout << "EVL_OUT: " << evl_cct.m_evl_out.to_hex() << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		std::cerr << "Usage: [OPTION] [CIRCUIT_FILE] [INPUT_VEC]" << std::endl
				  << " 1: binary parse test" << std::endl
				  << " 2: evaluate test" << std::endl
				  << " 3: maximum distance test" << std::endl
				  << " 4: statistic test" << std::endl
				  << " 5: use count test" << std::endl
				  << " 6: garbled test" << std::endl
				  << " 7: circuit dump test" << std::endl
				  << " 8: remove hash table test" << std::endl
				  << " 9: binary parse test (hash free)" << std::endl
				  << "10: evaluate test (hash free)" << std::endl
				  << "11: circuit dump test (hash free)" << std::endl
				  << "12: garbled test (hash free)" << std::endl;
		exit(EXIT_FAILURE);
	}

	char *endptr;
	int choice = strtol(argv[1], &endptr, 10);

	switch(choice)
	{
	case 1:
		parse_test(argv[2]);
		break;

	case 2:
		evaluate_test(argv[2], argv[3]);
		break;

	case 3:
		max_distance_test(argv[2]);
		break;

	case 4:
		statistics_test(argv[2]);
		break;

	case 5:
		use_count_test(argv[2]);
		break;

	case 6:
		yao_test(argv[2], argv[3]);
		break;

	case 7:
		dump_test(argv[2]);
		break;

	case 8:
		remove_hash(argv[2], argv[3]);
		break;

	case 9:
		parse_test_hash_free(argv[2]);
		break;

	case 10:
		evaluate_test_hash_free(argv[2], argv[3]);
		break;

	case 11:
		dump_test_hash_free(argv[2]);
		break;

	case 12:
		yao_test_hash_free(argv[2], argv[3]);
		break;
	}

	return 0;
}
