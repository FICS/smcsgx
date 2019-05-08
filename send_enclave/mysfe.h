/*
 * Headers file for mysfe for the sender
 * May 6, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#ifndef _MY_SFE_H
#define _MY_SFE_H

// common build parameters
#include "../include/smcsgxsfeparam.h"

#define SMC_SGX_SP_TAG_SIZE             16      // MAC size
#define SMC_SGX_SP_IV_SIZE              12      // IV size
#define SMC_SGX_SHARED_SECRET_LEN       16      // shared secret len
#define SEND_ENCLAVE_INPUT_LEN          8
#define SEND_YAO_MAGIC_J                10
#define SEND_YAO_PERMUTES_LEN           16
#define SEND_YAO_INPUT_BIN_LEN          16
#define SEND_YAO_OUTPUT_EVAL_LEN        8
#define SEND_YAO_OUTPUT_SEND_LEN        16
#define SEND_YAO_INPUT_GC_EVAL_LEN      16
#define SEND_YAO_INPUT_GC_SEND_LEN      32
#define SEND_YAO_INPUT_GEN_LEN          (SEND_YAO_INPUT_GC_SEND_LEN*SEND_YAO_MAGIC_J+SEND_YAO_PERMUTES_LEN)
#define SEND_YAO_INPUT_GEN_ENC_LEN      (SEND_YAO_INPUT_GEN_LEN+SMC_SGX_SP_TAG_SIZE)

#define SEND_SFE_INPUT_LEN              sfe_mil_size
#define SEND_SFE_OUTPUT_ENC_LEN         16
#define SEND_SFE_OUTPUT_LEN             4

#define SEND_SFE_UC_INPUT_LEN	(sfe_uc_numinputs*sfe_bool_size)
#define SEND_SFE_UC_OUTPUT_LEN	(sfe_uc_numoutputs*sfe_bool_size)

#define SEND_SFE_DJ_INPUT_EDGES_NUM	(sfe_dj_Nodes*sfe_dj_Edges_per_node)
#define SEND_SFE_DJ_INPUT_CONNS_NUM	(sfe_dj_Nodes*sfe_dj_Edges_per_node)
#define SEND_SFE_DJ_INPUT_NUM		(SEND_SFE_DJ_INPUT_EDGES_NUM+SEND_SFE_DJ_INPUT_CONNS_NUM)
#define SEND_SFE_DJ_INPUT_EDGES_LEN	(SEND_SFE_DJ_INPUT_EDGES_NUM*sizeof(int))
#define SEND_SFE_DJ_INPUT_CONNS_LEN	(SEND_SFE_DJ_INPUT_CONNS_NUM*sizeof(int))
#define SEND_SFE_DJ_INPUT_LEN		(SEND_SFE_DJ_INPUT_NUM*sizeof(int))
#define SEND_SFE_DJ_OUTPUT_LEN		(sfe_dj_Nodes*sizeof(int))

#define SEND_SFE_OR_INPUT_LEN		(sfe_or_queries*sizeof(unsigned int))
#define SEND_SFE_OR_OUTPUT_LEN		(sfe_or_queries*sizeof(unsigned long))

#endif
