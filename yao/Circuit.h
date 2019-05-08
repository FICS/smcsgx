#ifndef CIRCUIT_H_
#define CIRCUIT_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Bytes.h"

#include "interpreter.h"

using std::vector;

struct Gate
{
	uint8_t          m_tag;
	uint64_t         m_idx;
	uint64_t         m_idx2;
	vector<uint64_t> m_input_idx; // input gates
	vector<bool>     m_table;     // truth table

	uint8_t tbl_gate;
	char arty;
};


inline bool is_xor(const Gate &g)
{
	//uint8_t tbl = 0;
	//for (size_t ix = 0; ix < g.m_table.size(); ix++) { tbl |= g.m_table[ix]<<ix; }
	//return (g.m_input_idx.size()==1 && tbl==0x02) || (g.m_input_idx.size()==2 && tbl==0x06); 
	return (g.arty == 1 && g.tbl_gate == 0x02) || (g.arty == 2 && g.tbl_gate == 0x06);
}


class Circuit
{
private:
	Gate          m_current_gate;

	Interpreter * interp;

	T_Gate * t_gate;
        long oneplace,zeroplace;
        bool foundzero, foundone;
	// for text parser
	std::ifstream m_circuit_fs;

	// for binary parser
	uint64_t      m_gate_idx;
	int           m_circuit_fd;
	uint8_t      *m_ptr, *m_ptr_begin, *m_ptr_end;

	// hearder information
	uint64_t      m_gate_cnt;

	uint32_t      m_gen_inp_cnt;
	uint32_t      m_gen_out_cnt;
	uint32_t      m_evl_inp_cnt;
	uint32_t      m_evl_out_cnt;

public:
	uint8_t       m_cnt_size;
	uint64_t      m_cnt;

	enum { ETC = 0, GEN_INP = 1, EVL_INP = 2, GEN_OUT = 3, EVL_OUT = 4, SGX_INP = 5, SGX_OUT = 6};

	Circuit() :
		interp(0), m_gate_idx(0),
		m_ptr(0), m_ptr_begin(0), m_ptr_end(0),
		m_gate_cnt(0), m_gen_inp_cnt(0), m_gen_out_cnt(0), m_evl_inp_cnt(0), m_evl_out_cnt(0),
		m_cnt_size(0), m_cnt(0), m_circuit_fd(0) { m_current_gate.m_table.resize(4); m_current_gate.m_input_idx.resize(4); foundzero = false;foundone=false; }

	~Circuit();

	bool load(const char *circuit_file);
	bool load_binary(const char *circuit_file);
	bool load_binary_old(const char *circuit_file);

//	bool more_gate() { return !m_circuit_fs.eof(); }
	bool more_gate_binary() const {  return interp->currentFilemm != 0; }

	const Gate &next_gate();
	const Gate &next_gate_binary();
	const Gate &next_gate_binary_old();

	void evaluate(const Bytes &gen_inp, const Bytes &evl_inp, Bytes &gen_out, Bytes &evl_out);
	void evaluate_binary(const Bytes &gen_inp, const Bytes &evl_inp, Bytes &gen_out, Bytes &evl_out);
	void evaluate_binary_old(const Bytes &gen_inp, const Bytes &evl_inp, Bytes &gen_out, Bytes &evl_out);

/*	void reload() { m_circuit_fs.clear();  m_circuit_fs.seekg(0, std::ios_base::beg); }
	void reload_binary_old()
	{ m_ptr = m_ptr_begin+25; m_gate_idx = 0; }
	void reload_binary()
	{ m_ptr = m_ptr_begin+25+m_cnt_size; m_gate_idx = 0; }
*/
	//uint64_t gate_cnt()    const { return m_gate_cnt;    }
	uint32_t gen_inp_cnt() const { return m_gen_inp_cnt; }
	uint32_t gen_out_cnt() const { return m_gen_out_cnt; }
	uint32_t evl_inp_cnt() const { return m_evl_inp_cnt; }
	uint32_t evl_out_cnt() const { return m_evl_out_cnt; }
};

std::ostream &operator <<(std::ostream &os, const Gate &gate);

#endif /* CIRCUIT_H_ */
