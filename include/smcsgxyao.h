/*
 * smcsgxyao.h
 * A wrapper header to include other YAO headers
 * Apr 27, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#ifndef _SMC_SGX_YAO_H
#define _SMC_SGC_YAO_H

#include "../yao/BetterYao.h"

#define SMC_SGX_YAO_MAGIC_J		10
#define SMC_SGX_YAO_PERMUTES_LEN	16
#define SMC_SGX_YAO_INPUT_BIN_LEN	16
#define SMC_SGX_YAO_OUTPUT_EVAL_LEN	8
#define SMC_SGX_YAO_OUTPUT_SEND_LEN	16
#define SMC_SGX_YAO_INPUT_GC_EVAL_LEN	16
#define SMC_SGX_YAO_INPUT_GC_SEND_LEN	32
#define SMC_SGX_YAO_INPUT_GC_EVL_LEN	(SMC_SGX_YAO_INPUT_GC_EVAL_LEN*SMC_SGX_YAO_MAGIC_J)
#define SMC_SGX_YAO_INPUT_GC_GEN_LEN	(SMC_SGX_YAO_INPUT_GC_SEND_LEN*SMC_SGX_YAO_MAGIC_J)
#define SMC_SGX_YAO_INPUT_GEN_LEN	(SMC_SGX_YAO_INPUT_GC_GEN_LEN+SMC_SGX_YAO_PERMUTES_LEN)
#define SMC_SGX_YAO_PARAM_CIRCUIT_FILE	"./yao/TestPrograms/arrayinit.wir"
#define SMC_SGX_YAO_PARAM_PRIVATE_FILE	"./yao/original/inp.txt"
#define SMC_SGX_YAO_PARAM_SECURITY	80
#define SMC_SGX_YAO_PARAM_STATS		1
#define SMC_SGX_YAO_PARAM_SERVER_IP	"127.0.0.1"
#define SMC_SGX_YAO_PARAM_PORT_BASE	5000
#define SMC_SGX_YAO_PARAM_SETTING	1

class EvalInput
{
public:
//for eval
	vector<vector<char>> evalgc;
 
};

class GenInput
{
public:
//for gen/seder
	vector<vector<char>> gengc;
	vector<char> permubits;

};

class Input
{
public:
	EvalInput evalInput;
	GenInput genInput;
};


class Output
{
public:
	vector<vector<char>> output;
};

/* APIs */
BetterYao *startProgram(void);
Output runProgram(string programname,EvalInput evalInput, GenInput genInput, BetterYao *sysC);
Input rebuildInput(char *gen, char *evl, char *pem);
Input rebuildInput2(char *gen, int gen_len, char *evl, int evl_gen, char *pem, int pem_len);

#endif
