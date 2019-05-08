/*
 * Common YAO wrappers for both the sender and the evaluator
 * Ref: SFE_Frigate15/exec/sgxship/main.cpp
 * based on Dr. Mood's code
 * added a translation layer between ECalls and C++ calls with vectors
 * Apr 29, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include <iostream>
#include "../include/smcsgxyao.h"
#include "../include/smcsgxyaoparam.h"


/* Helpers */
Bytes transform_to_bytes(vector<char> inarray)
{
	int i;
	Bytes b;
	b.resize(SMC_SGX_YAO_INPUT_BIN_LEN);

	for (i = 0; i < SMC_SGX_YAO_MAGIC_J; i++)
		b[i] = inarray[i];

	b[10] = 0;
	b[11] = 0;
	b[12] = 0;
	b[13] = 0;
	b[14] = 0;
	b[15] = 0;

	return b;
}

void transform_all_to_bytes_vector(vector<vector<char>> &input,  vector<Bytes> &out)
{
	int i;

	out.resize(input.size());
	for (i = 0; i < input.size(); i++)
		out[i] = transform_to_bytes(input[i]);
}


/* APIs */
Input rebuildInput(char *gen, char *evl, char *pem)
{
	int i, j;
	Input toReturn;
	vector<vector<char>> evalgc;
	vector<vector<char>> gengc;
	vector<char> permutes;

	/* Rebuild gengc */
	gengc.resize(SMC_SGX_YAO_INPUT_GC_SEND_LEN);
	for (i = 0; i < SMC_SGX_YAO_INPUT_GC_SEND_LEN; i++) {
		gengc[i].resize(SMC_SGX_YAO_MAGIC_J);
		for (j = 0; j < SMC_SGX_YAO_MAGIC_J; j++)
			gengc[i][j] = gen[i*SMC_SGX_YAO_MAGIC_J+j];
	}

	/* Rebuild evalgc */
	evalgc.resize(SMC_SGX_YAO_INPUT_GC_EVAL_LEN);
	for (i = 0; i < SMC_SGX_YAO_INPUT_GC_EVAL_LEN; i++) {
		evalgc[i].resize(SMC_SGX_YAO_MAGIC_J);
		for (j = 0; j < SMC_SGX_YAO_MAGIC_J; j++)
			evalgc[i][j] = evl[i*SMC_SGX_YAO_MAGIC_J+j];
	}

	/* Rebuild the permutes */
	permutes.resize(SMC_SGX_YAO_PERMUTES_LEN);
	for (i = 0; i < SMC_SGX_YAO_PERMUTES_LEN; i++)
		permutes[i] = pem[i];

	toReturn.evalInput.evalgc = evalgc;
	toReturn.genInput.gengc = gengc;
	toReturn.genInput.permubits = permutes;

	return toReturn;
}

Input rebuildInput2(char *gen, int gen_len, char *evl, int evl_len, char *pem, int pem_len)
{
	int i, j;
	Input toReturn;
	vector<vector<char>> evalgc;
	vector<vector<char>> gengc;
	vector<char> permutes;

	/* Rebuild gengc */
	gengc.resize(gen_len);
	for (i = 0; i < gen_len; i++) {
		gengc[i].resize(SMC_SGX_YAO_MAGIC_J);
		for (j = 0; j < SMC_SGX_YAO_MAGIC_J; j++)
			gengc[i][j] = gen[i*SMC_SGX_YAO_MAGIC_J+j];
	}

	/* Rebuild evalgc */
	evalgc.resize(evl_len);
	for (i = 0; i < evl_len; i++) {
		evalgc[i].resize(SMC_SGX_YAO_MAGIC_J);
		for (j = 0; j < SMC_SGX_YAO_MAGIC_J; j++)
			evalgc[i][j] = evl[i*SMC_SGX_YAO_MAGIC_J+j];
	}

	/* Rebuild the permutes */
	permutes.resize(pem_len);
	for (i = 0; i < pem_len; i++)
		permutes[i] = pem[i];

	toReturn.evalInput.evalgc = evalgc;
	toReturn.genInput.gengc = gengc;
	toReturn.genInput.permubits = permutes;

	return toReturn;
}


Output runProgram(string programname,EvalInput evalInput, GenInput genInput, BetterYao *sysC)
{
	int i, j;
	vector<Bytes> input;
	vector<Bytes> output;
	vector<char> permutes;

	vector<Bytes> evalin;
	transform_all_to_bytes_vector(evalInput.evalgc,evalin);

	vector<Bytes> genin;
	transform_all_to_bytes_vector(genInput.gengc,genin);

	permutes = genInput.permubits;

GEN_BEGIN
	sysC->runSGXProgram(programname,&genin,&output,&permutes);
GEN_END

EVL_BEGIN
	sysC->runSGXProgram(programname,&evalin,&output,&permutes);
EVL_END

	Output out;
	out.output.resize(output.size());

	for (i = 0; i < output.size(); i++) {
		out.output[i].resize(SMC_SGX_YAO_MAGIC_J);
		for (j = 0; j < SMC_SGX_YAO_MAGIC_J; j++)
			out.output[i][j] = output[i][j];
	}

	return out;
}


BetterYao *startProgram(void)
{
	int setting;
	BetterYao *sysC;
	EnvParams *paramsGlobal = new EnvParams();

	/* Ref: parsing from the argv
	paramsGlobal->secu_param   = atoi(argv[1]);
	paramsGlobal->stat_param   = atoi(argv[2]);
	paramsGlobal->circuit_file = argv[3];
	paramsGlobal->private_file = argv[4];
	paramsGlobal->ipserve_addr = argv[5];
	paramsGlobal->port_base    = atoi(argv[6]);
	int setting         = atoi(argv[7]);
	*/

	/* daveti:
	NOTE: for SGX sender, we are YAO evaluator
	So we should only care about EVL_CODE settings!
	currently gen and evl share the same parameters */
GEN_BEGIN
	paramsGlobal->secu_param = SMC_SGX_YAO_PARAM_SECURITY;
	paramsGlobal->stat_param = SMC_SGX_YAO_PARAM_STATS;
	paramsGlobal->circuit_file = SMC_SGX_YAO_PARAM_CIRCUIT_FILE;
	paramsGlobal->private_file = SMC_SGX_YAO_PARAM_PRIVATE_FILE;
	paramsGlobal->ipserve_addr = SMC_SGX_YAO_PARAM_SERVER_IP;
	paramsGlobal->port_base = SMC_SGX_YAO_PARAM_PORT_BASE;
GEN_END

EVL_BEGIN
	paramsGlobal->secu_param = SMC_SGX_YAO_PARAM_SECURITY;
	paramsGlobal->stat_param = SMC_SGX_YAO_PARAM_STATS;
	paramsGlobal->circuit_file = SMC_SGX_YAO_PARAM_CIRCUIT_FILE;
	paramsGlobal->private_file = SMC_SGX_YAO_PARAM_PRIVATE_FILE;
	paramsGlobal->ipserve_addr = HYBRID_YAO_SERVER_IP; // it is .45 now!
	paramsGlobal->port_base = SMC_SGX_YAO_PARAM_PORT_BASE;
EVL_END

	setting = SMC_SGX_YAO_PARAM_SETTING;
	switch (setting)
	{
		case 0:
			//sys = new Yao(params);
			break;

		case 1:
			sysC = new BetterYao(*paramsGlobal);
			break;

		case 2:
			//sys = new BetterYao2(params);
			break;
	}
	sysC->initSGXProgram();

	return sysC;
}
