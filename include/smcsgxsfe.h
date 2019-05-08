/*
 * Header file for SFE related stuffs
 * May 3, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#ifndef _SMC_SGX_SFE_H
#define _SMC_SGX_SFE_H

// common build parameters
#include "../include/smcsgxsfeparam.h"

#define SMC_SGX_SFE_INPUT_LEN		sfe_mil_size
#define SMC_SGX_SFE_OUTPUT_LEN		4
#define SMC_SGX_SFE_OUTPUT_ENC_LEN	16	// padded

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


/* from eval_enclave/mysfe.h */
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


#endif
