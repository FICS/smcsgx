#include <cstdlib>
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

#include "Bytes.h"
#include "Circuit.h"

#include "interpreter.h"

/*
inline void tokenize
(
	std::vector<std::string>& tokens,
	const std::string& str,
	const std::string& delimiters = " "
)
{
	using namespace std;

    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}
*/

const Gate & Circuit::next_gate()
{
	/*assert(!m_circuit_fs.eof());

	using std::string;
	using std::vector;

	string line;
	std::getline(m_circuit_fs, line);

	m_current_gate.m_input_idx.clear();
	m_current_gate.m_table.clear();

	if (line.find("output.bob") != string::npos)
	{
		m_current_gate.m_tag = GEN_OUT;
	}
	else if (line.find("output.alice") != string::npos)
	{
		m_current_gate.m_tag = EVL_OUT;
	}
	else if (line.find("input.bob") != string::npos)
	{
		m_current_gate.m_tag = GEN_INP;
	}
	else if (line.find("input.alice") != string::npos)
	{
		m_current_gate.m_tag = EVL_INP;
	}
	else
	{
		m_current_gate.m_tag = ETC;
	}

	if (line.find("table") != string::npos)
	{
		vector<string> tokens;
		tokenize(tokens, line);

		vector<string>::iterator it = tokens.begin();

		// parsing gate arity
		while (it->compare("arity")) it++;
		int arity = atoi((++it)->c_str());

		// parsing table entries
		while (it->compare("table")) it++;
		it += 2; // skip 'table' and '['
		for (int i = 0; i < (1<<arity); i++, it++)
			m_current_gate.m_table.push_back(it->at(0) - '0');

		while (it->compare("inputs")) it++;
		it += 2; // skip 'inputs' and '['
		for (int i = 0; i < arity; i++, it++)
		{
			const char *str = it->c_str();
			char *endptr = const_cast<char*>(str + it->size());
			long long idx = strtoll(str, &endptr, 10);
			m_current_gate.m_input_idx.push_back(idx);
		}
	}
	*/
	return m_current_gate;
}


const Gate & Circuit::next_gate_binary_old()
{
	/*assert(m_ptr < m_ptr_end);

	uint8_t hdr = *m_ptr; m_ptr += sizeof(uint8_t);

	m_current_gate.m_tag = (hdr>>5)&0x07;

	if (m_current_gate.m_tag == GEN_INP || m_current_gate.m_tag == EVL_INP)
	{
		m_current_gate.m_input_idx.clear();
		m_current_gate.m_table.clear();
	}
	else
	{
		uint8_t arity = ((hdr>>4)&0x01) + 1;

		m_current_gate.m_input_idx.resize(arity);
		m_current_gate.m_table.resize(1<<arity);

		for (size_t ix = 0; ix < m_current_gate.m_table.size(); ix++)
			m_current_gate.m_table[ix] = (hdr>>ix)&0x01;

		if (m_gate_idx <= UINT16_MAX)
		{
			for (size_t ix = 0; ix < m_current_gate.m_input_idx.size(); ix++)
			{
				m_current_gate.m_input_idx[ix] = *reinterpret_cast<uint16_t *>(m_ptr);
				m_ptr += sizeof(uint16_t);
			}
		}
		else if (m_gate_idx <= UINT32_MAX)
		{
			for (size_t ix = 0; ix < m_current_gate.m_input_idx.size(); ix++)
			{
				m_current_gate.m_input_idx[ix] = *reinterpret_cast<uint32_t *>(m_ptr);
				m_ptr += sizeof(uint32_t);
			}
		}
		else
		{
			for (size_t ix = 0; ix < m_current_gate.m_input_idx.size(); ix++)
			{
				m_current_gate.m_input_idx[ix] = *reinterpret_cast<uint64_t *>(m_ptr);
				m_ptr += sizeof(uint64_t);
			}
		}
	}

	m_current_gate.m_idx = 0;
	memcpy(&m_current_gate.m_idx, m_ptr, m_cnt_size);
	m_ptr += m_cnt_size;

	m_current_gate.m_idx2 = m_gate_idx;

	m_gate_idx++;


	
	*/
	return m_current_gate;
}


 

const Gate & Circuit::next_gate_binary()
{
	//assert(m_ptr < m_ptr_end);

	/*uint8_t hdr = *m_ptr; m_ptr += sizeof(uint8_t);

	m_current_gate.m_tag = (hdr>>5)&0x07;
	m_current_gate.tbl_gate = 0;
	if (m_current_gate.m_tag == GEN_INP || m_current_gate.m_tag == EVL_INP)
	{
	}
	else
	{
		uint8_t arity = ((hdr>>4)&0x01) + 1;

		m_current_gate.arty = arity;	

		uint8_t tablesize = 1 << arity;
		m_current_gate.tbl_gate = hdr&0x0F;	

		uint64_t * tableptr = (uint64_t *) &(m_current_gate.m_input_idx[0]);

		if(arity == 2)
		{
			m_current_gate.m_table[0] = (hdr)&0x01;
			m_current_gate.m_table[1] = (hdr>>1)&0x01;
			m_current_gate.m_table[2] = (hdr>>2)&0x01;
			m_current_gate.m_table[3] = (hdr>>3)&0x01;
			
		
			memcpy(&(tableptr[0]), m_ptr, m_cnt_size);
			m_ptr += m_cnt_size;
	
			memcpy(&(tableptr[1]), m_ptr, m_cnt_size);
			m_ptr += m_cnt_size;
		}
		else if(arity == 1)
		{
			m_current_gate.m_table[0] = (hdr)&0x01;
			m_current_gate.m_table[1] = (hdr>>1)&0x01;
			
			memcpy(&(tableptr[0]), m_ptr, m_cnt_size);
			m_ptr += m_cnt_size;
		}	
	}

	m_current_gate.m_idx = 0;
	memcpy(&m_current_gate.m_idx, m_ptr, m_cnt_size);
	m_ptr += m_cnt_size;

	m_gate_idx++;

	return m_current_gate;
	*/


againlabel:
	    switch(t_gate->options)
            {
                    //gate
                case 0:
                 	m_current_gate.m_tag = 0;   
 
			m_current_gate.m_table[0]  = (t_gate->table)&0x1;
			m_current_gate.m_table[1]  = (t_gate->table >> 1)&0x1;
			m_current_gate.m_table[2]  = (t_gate->table >> 2)&0x1;
			m_current_gate.m_table[3]  = (t_gate->table >> 3)&0x1;				

			m_current_gate.arty = 2;	
			m_current_gate.tbl_gate	= t_gate->table;
			m_current_gate.m_idx = t_gate->dest;	

			m_current_gate.m_input_idx[0] = t_gate->y;
			m_current_gate.m_input_idx[1] = t_gate->x;

                    //                if(outputgatelist)
                    //{
                        if(t_gate->table == 15 && !foundone)
                        {
                            oneplace = t_gate->dest;
                            foundone = true;
                        }
                        else if(t_gate->table == 0 && !foundzero)
                        {
                            zeroplace = t_gate->dest;
                            foundzero = true;
                        }
                        
                    //    fgatelistfile <<  (int)(g->table)<<" "<<g->dest <<" "<< g->x  <<" "<<g->y<<"\n";
                    //} 
                	break;
                    
                    //input
                case 1:
        	        if(t_gate->x == 1)
			{
				m_current_gate.m_tag = GEN_INP;
			}
			else if(t_gate->x == 2)
			{
				m_current_gate.m_tag = EVL_INP;
			}
			else if(t_gate->x == 3)
			{
				m_current_gate.m_tag = SGX_INP;
			}      		
			m_current_gate.m_idx = t_gate->dest;

			break;
	
		//output
		case 2:
			if(t_gate->x == 1)
			{
				m_current_gate.m_tag = GEN_OUT;
			}
			else if(t_gate->x == 2)
			{
				m_current_gate.m_tag = EVL_OUT;
			}
			else if(t_gate->x == 3)
			{
				m_current_gate.m_tag = SGX_OUT;
			}


			//idea - make the gate a pass through gate where both input wires (into the gate) are the output wire value
			m_current_gate.m_table[0]  = 0;
			m_current_gate.m_table[1]  = 1;
			m_current_gate.m_table[2]  = 1;
			m_current_gate.m_table[3]  = 0;				

			m_current_gate.arty = 2;	
			m_current_gate.tbl_gate	= 0x06;
			m_current_gate.m_idx = t_gate->dest;	

			m_current_gate.m_input_idx[0] = t_gate->dest;
			m_current_gate.m_input_idx[1] = zeroplace;

			break;

		//copy
		case 3:

                 	m_current_gate.m_tag = 0;   
 
			m_current_gate.m_table[0]  = 0;
			m_current_gate.m_table[1]  = 1;
			m_current_gate.m_table[2]  = 1;
			m_current_gate.m_table[3]  = 0;				

			m_current_gate.arty = 2;	
			m_current_gate.tbl_gate	= 0x6;
			m_current_gate.m_idx = t_gate->x;	

			m_current_gate.m_input_idx[0] = t_gate->y;
			m_current_gate.m_input_idx[1] = zeroplace;



			break;

		//functioncall just jump as we ignore in this system
		case 4:

			t_gate = interp->getNextGate();		
			goto againlabel;
			break;
	
		default:
			std::cout <<"undefined op\n";	
	}
	 
	t_gate = interp->getNextGate();
	return m_current_gate;	
}

bool Circuit::load(const char *circuit_file)
{
	/*m_circuit_fs.open(circuit_file, std::ifstream::in);

	if (m_circuit_fs.fail())
	{
		perror("can't open file for reading");
		return false;
	}
	*/
	return true;
}


bool Circuit::load_binary_old(const char *circuit_file)
{
	/*struct stat statbuf;

	if ((m_circuit_fd = open(circuit_file, O_RDONLY)) < 0)
	{
		perror("can't open file for reading");
		return false;
	}

	if (fstat(m_circuit_fd, &statbuf) < 0)
	{
		perror("fstat in load_binary failed");
		return false;
	}

    if ((m_ptr_begin = (uint8_t *)mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, m_circuit_fd, 0)) == MAP_FAILED)
	{
		perror("mmap in load_binary failed");
		return false;
	}
    m_ptr = m_ptr_begin;
	m_ptr_end = m_ptr_begin + statbuf.st_size; // mark the end of the file

	m_gate_cnt    = *reinterpret_cast<uint64_t *>(m_ptr); m_ptr += sizeof(uint64_t);
	m_gen_inp_cnt = *reinterpret_cast<uint32_t *>(m_ptr); m_ptr += sizeof(uint32_t);
	m_evl_inp_cnt = *reinterpret_cast<uint32_t *>(m_ptr); m_ptr += sizeof(uint32_t);
	m_gen_out_cnt = *reinterpret_cast<uint32_t *>(m_ptr); m_ptr += sizeof(uint32_t);
	m_evl_out_cnt = *reinterpret_cast<uint32_t *>(m_ptr); m_ptr += sizeof(uint32_t);
	m_cnt_size    = *reinterpret_cast<uint8_t  *>(m_ptr); m_ptr += sizeof(uint8_t );

	m_gate_idx = 0;

	return true;*/
}


bool Circuit::load_binary(const char *circuit_file)
{
	//std::cout <<"startload\n";

	if(interp != 0)
	{
		delete interp;
		interp=0;
	}	

	//std::cout << "1\n";	

	interp = new Interpreter(false,false,false,true,true,false,"");

	interp->readyProgram(circuit_file);

	m_cnt = interp->maxwire+1;

	m_gen_inp_cnt = 0;
	m_evl_inp_cnt = 0;
	m_gen_out_cnt = 0;
	m_evl_out_cnt = 0;

	//std::cout << "2\n"; 

	for(int i=0;i<interp->inputlist.size();i+=3)
	{
		if(interp->inputlist[i] == 1)
		{
			m_gen_inp_cnt = interp->inputlist[i+2];
		}
                if(interp->inputlist[i] == 2)
                {
                        m_evl_inp_cnt = interp->inputlist[i+2];
                }
	}

	//std::cout << "3\n"; 

        for(int i=0;i<interp->outputlist.size();i+=3)
        {
                if(interp->outputlist[i] == 1)
                {
                        m_gen_out_cnt = interp->outputlist[i+2];
                }
                if(interp->outputlist[i] == 2)
                {
                        m_evl_out_cnt = interp->outputlist[i+2];
                }
        }

	//std::cout << "4\n"; 

	t_gate = interp->getNextGate();

	//std::cout <<"endload\n";

	return true;
}

std::ostream &operator<<(std::ostream &os, const Gate &g)
{
	/*switch(g.m_tag)
	{
	case Circuit::GEN_INP:
		os << "GEN_INP: ";
		break;

	case Circuit::EVL_INP:
		os << "EVL_INP: ";
		break;

	case Circuit::GEN_OUT:
		os << "GEN_OUT: ";
		break;

	case Circuit::EVL_OUT:
		os << "EVL_OUT: ";
		break;
	}

	if (g.m_input_idx.size() == 0)
		return os;

	os << "[";
	for (vector<bool>::const_iterator it = g.m_table.begin(); it != g.m_table.end(); it++)
		os << " " << (int)*it;
	os << " ]";

	os << " [";
	for (vector<uint64_t>::const_iterator it = g.m_input_idx.begin(); it != g.m_input_idx.end(); it++)
		os << " " << *it;
	os << " ]";

	//os << " " << g.m_idx;
	*/
	return os;
}


void Circuit::evaluate(const Bytes &gen_inp, const Bytes &evl_inp, Bytes &gen_out, Bytes &evl_out)
{
/*	uint32_t gen_inp_ix = 0, evl_inp_ix = 0, gen_out_ix = 0, evl_out_ix = 0;
	uint64_t wire_ix = 0;

	Bytes wire(1);
	gen_out.resize(1);
	evl_out.resize(1);

	while (more_gate())
	{
		const Gate &g = next_gate();

		if (wire_ix/8+1 > wire.size())
			wire.resize(wire.size()*2);

		if (g.m_tag == GEN_INP)
		{
			wire.set_ith_bit(wire_ix, gen_inp.get_ith_bit(gen_inp_ix++));
		}
		else if (g.m_tag == EVL_INP)
		{
			wire.set_ith_bit(wire_ix, evl_inp.get_ith_bit(evl_inp_ix++));
		}
		else
		{
			size_t inp_ix = 0;

			for (size_t ix = 0; ix < g.m_input_idx.size(); ix++)
			{
				inp_ix |= ((wire.get_ith_bit(g.m_input_idx[ix]) & 0x01) << ix);
			}
			wire.set_ith_bit(wire_ix, g.m_table[inp_ix]);

			if (g.m_tag == GEN_OUT)
			{
				if (gen_out_ix/8+1 > gen_out.size())
					gen_out.resize(gen_out.size()*2);
				gen_out.set_ith_bit(gen_out_ix++, wire.get_ith_bit(wire_ix));
			}
			else if (g.m_tag == EVL_OUT)
			{
				if (evl_out_ix/8+1 > evl_out.size())
					evl_out.resize(evl_out.size()*2);
				evl_out.set_ith_bit(evl_out_ix++, wire.get_ith_bit(wire_ix));
			}
		}

		wire_ix++;
	}
	gen_out.resize(gen_out_ix/8+1);
	evl_out.resize(evl_out_ix/8+1);*/
}


void Circuit::evaluate_binary_old(const Bytes &gen_inp, const Bytes &evl_inp, Bytes &gen_out, Bytes &evl_out)
{
	/*uint32_t gen_inp_ix = 0, evl_inp_ix = 0, gen_out_ix = 0, evl_out_ix = 0;
	uint64_t wire_ix = 0;

	Bytes wire((m_gate_cnt+7)/8);

	gen_out.resize((m_gen_out_cnt+7)/8);
	evl_out.resize((m_evl_out_cnt+7)/8);

	while (more_gate_binary())
	{
		const Gate &g = next_gate_binary_old();

		if (g.m_tag == GEN_INP)
		{
			wire.set_ith_bit(wire_ix, gen_inp.get_ith_bit(gen_inp_ix++));
		}
		else if (g.m_tag == EVL_INP)
		{
			wire.set_ith_bit(wire_ix, evl_inp.get_ith_bit(evl_inp_ix++));
		}
		else
		{
			size_t tbl_ix = 0;

			for (size_t ix = 0; ix < g.m_input_idx.size(); ix++)
			{
				tbl_ix |= ((wire.get_ith_bit(g.m_input_idx[ix]) & 0x01) << ix);
			}
			wire.set_ith_bit(wire_ix, g.m_table[tbl_ix]);

			if (g.m_tag == GEN_OUT)
			{
				gen_out.set_ith_bit(gen_out_ix++, wire.get_ith_bit(wire_ix));
			}
			else if (g.m_tag == EVL_OUT)
			{
				evl_out.set_ith_bit(evl_out_ix++, wire.get_ith_bit(wire_ix));
			}
		}

		wire_ix++;
	}*/
}


void Circuit::evaluate_binary(const Bytes &gen_inp, const Bytes &evl_inp, Bytes &gen_out, Bytes &evl_out)
{
	/*uint32_t gen_inp_ix = 0, evl_inp_ix = 0, gen_out_ix = 0, evl_out_ix = 0;
	uint64_t wire_ix = 0;

	Bytes wire((m_cnt+7)/8);

	std::cout << "wire.size(): " << wire.size() << std::endl;

	gen_out.resize((m_gen_out_cnt+7)/8);
	evl_out.resize((m_evl_out_cnt+7)/8);

	while (more_gate_binary())
	{
		const Gate &g = next_gate_binary();

		if (g.m_tag == GEN_INP)
		{
			wire.set_ith_bit(g.m_idx, gen_inp.get_ith_bit(gen_inp_ix++));
		}
		else if (g.m_tag == EVL_INP)
		{
			wire.set_ith_bit(g.m_idx, evl_inp.get_ith_bit(evl_inp_ix++));
		}
		else
		{
			size_t tbl_ix = 0;

			for (size_t ix = 0; ix < g.m_input_idx.size(); ix++)
			{
				tbl_ix |= wire.get_ith_bit(g.m_input_idx[ix]) << ix;
			}
			wire.set_ith_bit(g.m_idx, g.m_table[tbl_ix]);

			if (g.m_tag == GEN_OUT)
			{
				gen_out.set_ith_bit(gen_out_ix++, wire.get_ith_bit(g.m_idx));
			}
			else if (g.m_tag == EVL_OUT)
			{
				evl_out.set_ith_bit(evl_out_ix++, wire.get_ith_bit(g.m_idx));
			}
		}

		wire_ix++;
	}*/
}

Circuit::~Circuit()
{
 /*   m_circuit_fs.close();

	struct stat statbuf;

	if (fstat(m_circuit_fd, &statbuf) < 0)
	{
		perror("fstat in load_binary failed");
	}

    if (m_ptr_begin != 0 && munmap(m_ptr_begin, statbuf.st_size) == -1)
    {
		perror("munmap in ~Circuit() failed");
    }

	if (close(m_circuit_fd) != 0)
	{
		perror("can't close file");
	}
	*/

	if(interp != 0)
	{
		delete interp;
	}

//	std::cout <<"circuit close\n";
}
