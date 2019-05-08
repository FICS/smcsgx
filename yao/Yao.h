#ifndef YAO_H_
#define YAO_H_

#include "YaoBase.h"
#include "GarbledCct.h"

class Yao : public YaoBase
{

public:
	Yao(EnvParams &params);
	virtual ~Yao() {}

	virtual void start();

private:
	void oblivious_transfer();
	void circuit_evaluate();
	void proc_gen_out();
	void proc_evl_out();

	// variables for SS11 committing OT implementation
	G                      m_ot_g[2];
	G                      m_ot_h[2];
	vector<vector<Bytes> > m_ot_keys; // ot output

	// variables for Yao protocol
	vector<Bytes>          m_gen_inp_masks;
	vector<Bytes>          m_rnds;
	vector<GarbledCct>     m_ccts;
};

#endif
