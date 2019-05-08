/*
 * Header for SFE program
 * May 3, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#ifndef _MYSFE_H
#define _MYSFE_H

// common build parameters
#include "../include/smcsgxsfeparam.h"

#define SMC_SGX_SP_IV_SIZE              12
#define SMC_SGX_SP_TAG_SIZE             16
#define SEND_SFE_INPUT_LEN              sfe_mil_size
#define SEND_SFE_OUTPUT_PAD_LEN         16
#define SEND_SFE_OUTPUT_ENC_LEN         (SEND_SFE_OUTPUT_PAD_LEN+SMC_SGX_SP_TAG_SIZE)
#define SEND_SFE_OUTPUT_LEN             4

#define SMC_SGX_SFE_UC_INPUT_LEN	(sfe_uc_numinputs*sfe_bool_size)
#define SMC_SGX_SFE_UC_OUTPUT_LEN	(sfe_uc_numoutputs*sfe_bool_size)

#define SMC_SGX_SFE_DJ_INPUT_EDGES_NUM	(sfe_dj_Nodes*sfe_dj_Edges_per_node)
#define SMC_SGX_SFE_DJ_INPUT_CONNS_NUM	(sfe_dj_Nodes*sfe_dj_Edges_per_node)
#define SMC_SGX_SFE_DJ_INPUT_NUM	(SMC_SGX_SFE_DJ_INPUT_EDGES_NUM+SMC_SGX_SFE_DJ_INPUT_CONNS_NUM)
#define SMC_SGX_SFE_DJ_INPUT_EDGES_LEN	(SMC_SGX_SFE_DJ_INPUT_EDGES_NUM*sizeof(int))
#define SMC_SGX_SFE_DJ_INPUT_CONNS_LEN	(SMC_SGX_SFE_DJ_INPUT_CONNS_NUM*sizeof(int))
#define SMC_SGX_SFE_DJ_INPUT_LEN	(SMC_SGX_SFE_DJ_INPUT_NUM*sizeof(int))
#define SMC_SGX_SFE_DJ_OUTPUT_LEN	(sfe_dj_Nodes*sizeof(int))

#define SMC_SGX_SFE_OR_INPUT_GEN_LEN	(sfe_or_queries*sizeof(unsigned int))
#define SMC_SGX_SFE_OR_INPUT_EVL_LEN	(sfe_or_sizeOfDatabase*sizeof(unsigned long))
#define SMC_SGX_SFE_OR_OUTPUT_LEN	(sfe_or_queries*sizeof(unsigned long))

typedef struct _sfe_uc_input_program {
	short x[sfe_uc_gatecount];
	short y[sfe_uc_gatecount];
     	char tt[sfe_uc_gatecount];
	short d[sfe_uc_gatecount];
	short output[sfe_uc_numoutputs];
} sfe_uc_input_program;

typedef struct _sfe_dj_input_eval {
	int startnode;
	int endnode;
} sfe_dj_input_eval;

typedef struct _sfe_dj_input_send {
	int edges[sfe_dj_Nodes*sfe_dj_Edges_per_node];
	int connections[sfe_dj_Nodes*sfe_dj_Edges_per_node];
} sfe_dj_input_send;


void sfe_entry(unsigned int alice[sfe_mil_size],
		unsigned int bob[sfe_mil_size],
		unsigned int *alice_out, unsigned int *bob_out);

void sfe_uc_entry(sfe_uc_input_program *prog,
		bool input[sfe_uc_numinputs],
		bool output[sfe_uc_numoutputs]);

void sfe_dj_entry(sfe_dj_input_eval *evl_input,
		sfe_dj_input_send *gen_input,
		int output[sfe_dj_Nodes]);

void sfe_or_entry(unsigned long evl_input[sfe_or_sizeOfDatabase],
		unsigned int gen_input[sfe_or_queries],
		unsigned long output[sfe_or_queries],
		int oram);

#ifdef ORAM_TESTING
bool oram_run_tests();
#endif

#endif
