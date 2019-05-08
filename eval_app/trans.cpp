/*
 * trans.cpp
 * STM Transition Implementations
 * Apr 5, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "../include/smcsgx.h"
#include "../include/smcsgxra.h"
#include "../include/smcsgxyao.h"
#include "../include/smcsgxsfe.h"
#include "../include/smcsgxyaoparam.h"
#include "mysgx.h"
#include "mysgx_common.h"
#include "eval_enclave_u.h"

#define SMC_SGX_YAO_INPUT_GEN_ENC_LEN	(SMC_SGX_YAO_INPUT_GEN_LEN+SMC_SGX_SP_TAG_SIZE)

/* Global vars - NOT thread-safe */
static int remote_fd;
static int current_state;
static int trans_debug;
static int trans_ut;
static int trans_oram;
static int trans_done;
static int trans_naive;
static int trans_hardcode_input = 1;
static smcsgx_ra_msg1_t trans_S1;
static smcsgx_ra_msg2_t trans_S2;
static smcsgx_ra_msg3_t *trans_S3;
static int trans_S3_len;
static smcsgx_ra_att_result_msg_t *trans_att_res;
static char trans_buff[SMC_SGX_BUF_LEN];	// general sending buffer
static char trans_output[SMC_SGX_BUF_LEN];	// save the SFE output
static char trans_hybrid[SMC_SGX_BUF_LEN];	// used by the hybrid model
static char trans_secret[SMC_SGX_SECRET_LEN_MAX];
static int trans_secret_len = SMC_SGX_SHARED_SECRET_LEN;// daveti: we need to know the length per sgx!
static int trans_output_len;			// daveti: we need to know the length per sgx!
static int trans_hybrid_len;
static int trans_yao;				// daveti: this flag should be set by the sender
static int trans_yao_idx;
static int trans_sfe;				// daveti: set by the sender
static smcsgx_msghdr trans_msghdr;
static smcsgx_stm_trans_entry stm_tbl[SMC_SGX_STM_NUM_MAX][SMC_SGX_MSG_TAG_NUM_MAX];
static char smcsgx_yao_gengc[SMC_SGX_YAO_INPUT_GC_GEN_LEN];
static char smcsgx_yao_evlgc[SMC_SGX_YAO_INPUT_GC_EVL_LEN];
static char smcsgx_yao_permutes[SMC_SGX_YAO_PERMUTES_LEN];
static char smcsgx_yao_gen_input[SMC_SGX_YAO_INPUT_GEN_ENC_LEN];
BetterYao *smcsgx_yao;
static uint32_t smcsgx_sfe_gen_input[SMC_SGX_SFE_INPUT_LEN];
static uint32_t smcsgx_sfe_evl_input[SMC_SGX_SFE_INPUT_LEN];
static unsigned int smcsgx_sfe_gen_output;
static unsigned int smcsgx_sfe_evl_output;
static sfe_uc_input_program smcsgx_sfe_uc_evl_input;
static bool smcsgx_sfe_uc_gen_input[sfe_uc_numinputs];
static bool smcsgx_sfe_uc_output[sfe_uc_numoutputs];
static sfe_dj_input_eval smcsgx_sfe_dj_evl_input;
static sfe_dj_input_send smcsgx_sfe_dj_gen_input;
static int smcsgx_sfe_dj_output[sfe_dj_Nodes];
static unsigned int smcsgx_sfe_or_gen_input[sfe_or_queries];
static unsigned long smcsgx_sfe_or_evl_input[sfe_or_sizeOfDatabase];
static unsigned long smcsgx_sfe_or_output[sfe_or_queries];
static unsigned int smcsgx_yao_or_evl_input[yao_or_datasize*2];
static unsigned int smcsgx_yao_or_gen_input[yao_or_queries];
static char smcsgx_yao_or_gengc[HYBRID_OR_GENGC_LEN];
static char smcsgx_yao_or_evlgc[HYBRID_OR_EVLGC_LEN];
static char smcsgx_yao_or_permutes[HYBRID_OR_PERMUTES_LEN];
static unsigned long smcsgx_yao_or_output_sgx[yao_or_queries];
static unsigned int smcsgx_yao_dj_evl_input[HYBRID_DJ_EVL_INPUT_NUM];
static unsigned int smcsgx_yao_dj_gen_input[HYBRID_DJ_GEN_INPUT_NUM];
static char smcsgx_yao_dj_gengc[HYBRID_DJ_GENGC_LEN];
static char smcsgx_yao_dj_evlgc[HYBRID_DJ_EVLGC_LEN];
static char smcsgx_yao_dj_permutes[HYBRID_DJ_PERMUTES_LEN];
static int smcsgx_yao_dj_output_sgx[yao_dj_Nodes];
static unsigned int smcsgx_yao_or_split_evl_input[yao_or_split_datasize*2];
static unsigned int smcsgx_yao_or_split_gen_input[yao_or_split_queries];
static char smcsgx_yao_or_split_gengc[HYBRID_OR_SPLIT_GENGC_LEN];
static char smcsgx_yao_or_split_evlgc[HYBRID_OR_SPLIT_EVLGC_LEN];
static char smcsgx_yao_or_split_permutes[HYBRID_OR_SPLIT_PERMUTES_LEN];
static unsigned long smcsgx_yao_or_split_output_sgx[yao_or_split_queries];

/* Helpers */
/* FIXME: these dups are the same as the ones in sender
 * May consider a common utils file to hold these
 */
static inline void build_smcsgx_msg(int tag, int len, char *payload)
{
	trans_msghdr.tag = tag;
	trans_msghdr.len = len;
	memcpy(trans_buff, &trans_msghdr, sizeof(trans_msghdr));
	/* We assume the buffer is large enough */
	memcpy(trans_buff+sizeof(trans_msghdr), payload, len);
}

static int build_send_smcsgx_msg(int tag, int len, char *payload)
{
	int ret;
	int total;

	build_smcsgx_msg(tag, len, payload);
	total = sizeof(trans_msghdr)+trans_msghdr.len;
	ret = write(remote_fd, trans_buff, total);
	if (ret != total) {
		printf("trans-error: %s failed, requested %d, sent %d\n",
				__func__, total, ret);
		return SMC_SGX_RTN_FAILURE;
	}

	return SMC_SGX_RTN_SUCCESS;
}

// Some utility functions to output some of the data structures passed between
// the ISV app and the remote attestation service provider.
static void dump_byte_array(void *mem, uint32_t len)
{
	if(!mem || !len) {
		printf("Error: null input\n");
		return;
	}

	uint8_t *array = (uint8_t *)mem;
	printf("%u bytes:\n{\n", len);
	uint32_t i = 0;
	for (i = 0; i < len - 1; i++) {
		printf("0x%x, ", array[i]);
		if(i % 8 == 7)
			printf("\n");
	}
	printf("0x%x ", array[i]);
	printf("\n}\n");
}

static void dump_S1(void)
{
	printf("MSG1 ga - ");
	dump_byte_array(&trans_S1.g_a, sizeof(trans_S1.g_a));

	printf("MSG1 gid - ");
	dump_byte_array(&trans_S1.gid, sizeof(trans_S1.gid));
}

static void dump_S2(void)
{
	// daveti: is sgx_ra_msg2_t and smcsgx_ra_msg2_t the same?
	// Aha, they are the same:)
	sgx_ra_msg2_t* p_msg2_body = (sgx_ra_msg2_t*)(&trans_S2);

	if (!p_msg2_body) {
		printf("Error: null S2\n");
		return;
	}

	printf("MSG2 gb - ");
	dump_byte_array(&(p_msg2_body->g_b), sizeof(p_msg2_body->g_b));

	printf("MSG2 spid - ");
	dump_byte_array(&(p_msg2_body->spid), sizeof(p_msg2_body->spid));

	printf("MSG2 sign_gb_ga - ");
	dump_byte_array(&(p_msg2_body->sign_gb_ga),
			sizeof(p_msg2_body->sign_gb_ga));

	printf("MSG2 mac - ");
	dump_byte_array(&(p_msg2_body->mac), sizeof(p_msg2_body->mac));

	printf("MSG2 sig_rl - ");
	dump_byte_array(&(p_msg2_body->sig_rl),
			p_msg2_body->sig_rl_size);
}

static void dump_S3(void)
{
	printf("MSG3 mac - ");
	dump_byte_array(&trans_S3->mac, sizeof(trans_S3->mac));

	printf("MSG3 ga - ");
	dump_byte_array(&trans_S3->g_a, sizeof(trans_S3->g_a));

	printf("MSG3 ps_sec_prop - ");
	dump_byte_array(&trans_S3->ps_sec_prop, sizeof(trans_S3->ps_sec_prop));

	printf("MSG3 quote - ");
	dump_byte_array(&trans_S3->quote, trans_S3_len-sizeof(smcsgx_ra_msg3_t));
}

static void dump_att_res(void)
{
	smcsgx_ra_att_result_msg_t *p_att_result = trans_att_res;

	printf("ATTESTATION RESULT result - %d\n",
			p_att_result->result);

	printf("ATTESTATION RESULT MSG platform_info_blob - ");
	dump_byte_array(&(p_att_result->platform_info_blob),
			sizeof(p_att_result->platform_info_blob));

	printf("ATTESTATION RESULT MSG mac - ");
	dump_byte_array(&(p_att_result->mac), sizeof(p_att_result->mac));

	printf("ATTESTATION RESULT MSG secret.payload_tag - %u bytes\n",
			p_att_result->secret.payload_size);

	printf("ATTESTATION RESULT MSG secret.payload - ");
	dump_byte_array(p_att_result->secret.payload,
			p_att_result->secret.payload_size);
}

static void dump_shared_secret(void)
{
	printf("SHARED SECRET - ");
	dump_byte_array((void *)trans_secret, trans_secret_len);
}

/* daveti: save copy from sender's app */
static void dump_yao_gc(void)
{
	printf("gengc - ");
	dump_byte_array((void *)smcsgx_yao_gengc, SMC_SGX_YAO_INPUT_GC_GEN_LEN);

	printf("evlgc - ");
	dump_byte_array((void *)smcsgx_yao_evlgc, SMC_SGX_YAO_INPUT_GC_EVL_LEN);

	printf("permutes - ");
	dump_byte_array((void *)smcsgx_yao_permutes, SMC_SGX_YAO_PERMUTES_LEN);

	printf("gen input (encrypted) - ");
	dump_byte_array((void *)smcsgx_yao_gen_input, SMC_SGX_YAO_INPUT_GEN_ENC_LEN);
}

static void dump_yao_or_all(void)
{
	int i;

        printf("gengc - ");
        dump_byte_array((void *)smcsgx_yao_or_gengc, HYBRID_OR_GENGC_LEN);

        printf("evlgc - ");
        dump_byte_array((void *)smcsgx_yao_or_evlgc, HYBRID_OR_EVLGC_LEN);

        printf("permutes - ");
        dump_byte_array((void *)smcsgx_yao_or_permutes, HYBRID_OR_PERMUTES_LEN);

        printf("gen input -\n");
	for (i = 0; i < yao_or_queries; i++)
		cout << smcsgx_yao_or_gen_input[i] << " ";
	cout << endl;

	cout << "gen sgx output -\n";
	for (i = 0; i < yao_or_queries; i++)
		cout << smcsgx_yao_or_output_sgx[i] << " ";
	cout << endl;
}

static void dump_yao_or_split_all(void)
{
	int i;

        printf("gengc - ");
        dump_byte_array((void *)smcsgx_yao_or_split_gengc, HYBRID_OR_SPLIT_GENGC_LEN);

        printf("evlgc - ");
        dump_byte_array((void *)smcsgx_yao_or_split_evlgc, HYBRID_OR_SPLIT_EVLGC_LEN);

        printf("permutes - ");
        dump_byte_array((void *)smcsgx_yao_or_split_permutes, HYBRID_OR_SPLIT_PERMUTES_LEN);

        printf("gen input -\n");
	for (i = 0; i < yao_or_split_queries; i++)
		cout << smcsgx_yao_or_split_gen_input[i] << " ";
	cout << endl;

	cout << "gen sgx output -\n";
	for (i = 0; i < yao_or_split_queries; i++)
		cout << smcsgx_yao_or_split_output_sgx[i] << " ";
	cout << endl;
}

static void dump_yao_dj_all(void)
{
        int i;

        printf("gengc - ");
        dump_byte_array((void *)smcsgx_yao_dj_gengc, HYBRID_DJ_GENGC_LEN);

        printf("evlgc - ");
        dump_byte_array((void *)smcsgx_yao_dj_evlgc, HYBRID_DJ_EVLGC_LEN);

        printf("permutes - ");
        dump_byte_array((void *)smcsgx_yao_dj_permutes, HYBRID_DJ_PERMUTES_LEN);

        printf("gen input -\n");
        for (i = 0; i < HYBRID_DJ_GEN_INPUT_NUM; i++)
                cout << smcsgx_yao_dj_gen_input[i] << " ";
        cout << endl;

	printf("evl input -\n");
	for (i = 0; i < HYBRID_DJ_EVL_INPUT_NUM; i++)
		cout << smcsgx_yao_dj_evl_input[i] << " ";
	cout << endl;
}

static void dump_yao_dj_sgx_output(void)
{
	int i;

        cout << "gen sgx output -\n";
        for (i = 0; i < yao_dj_Nodes; i++)
                cout << smcsgx_yao_dj_output_sgx[i] << " ";
        cout << endl;
}

static void dump_sfe_things(void)
{
	int i;

	printf("evl output - %u\n"
			"gen output - %u\n",
			smcsgx_sfe_evl_output,
			smcsgx_sfe_gen_output);

	printf("gen input - \n");
	for (i = 0; i < SMC_SGX_SFE_INPUT_LEN; i++)
		printf("%u ", smcsgx_sfe_gen_input[i]);
	printf("\n");

	printf("gen output (encrypted) - ");
	dump_byte_array((void *)trans_output, trans_output_len);
}

static void dump_sfe_uc_things(void)
{
	int i;

	cout << "gen input - \n";
	for (i = 0; i < sfe_uc_numinputs; i++)
		cout << smcsgx_sfe_uc_gen_input[i] << " ";
	cout << "\n";

	cout << "output - \n";
	for (i = 0; i < sfe_uc_numoutputs; i++)
		cout << smcsgx_sfe_uc_output[i] << " ";
	cout << "\n";

	printf("gen output (encrypted) - ");
	dump_byte_array((void *)trans_output, trans_output_len);
}

static void dump_sfe_dj_things(void)
{
	int i;

	cout << "gen input (edges) - \n";
	for (i = 0; i < sfe_dj_Nodes*sfe_dj_Edges_per_node; i++)
		cout << smcsgx_sfe_dj_gen_input.edges[i] << " ";
	cout << endl;

	cout << "gen input (connections) - \n";
	for (i = 0; i < sfe_dj_Nodes*sfe_dj_Edges_per_node; i++)
		cout << smcsgx_sfe_dj_gen_input.connections[i] << " ";
	cout << endl;

	cout << "output - \n";
	for (i = 0; i < sfe_dj_Nodes; i++)
		cout << smcsgx_sfe_dj_output[i] << " ";
	cout << endl;

	cout << "gen output (encrypted) - ";
	dump_byte_array((void *)trans_output, trans_output_len);
}

static void dump_sfe_or_things(void)
{
	int i;

	cout << "gen input (queris) -\n";
	for (i = 0; i < sfe_or_queries; i++)
		cout << smcsgx_sfe_or_gen_input[i] << " ";
	cout << endl;

	cout << "output -\n";
	for (i = 0; i < sfe_or_queries; i++)
		cout << smcsgx_sfe_or_output[i] << " ";
	cout << endl;

	cout << "gen output (encrypted) - ";
	dump_byte_array((void *)trans_output, trans_output_len);
}

static void dump_sfe_user_input(void)
{
	int i;

	cout << "SFE mil evl input -\n";
	for (i = 0; i < SMC_SGX_SFE_INPUT_LEN; i++)
		cout << smcsgx_sfe_evl_input[i] << " ";
	cout << endl;

	cout << "SFE uc evl input -\n";
	cout << "x -\n";
	for (i = 0; i < sfe_uc_gatecount; i++)
		cout << smcsgx_sfe_uc_evl_input.x[i] << " ";
	cout << "\ny -\n";
	for (i = 0; i < sfe_uc_gatecount; i++)
		cout << smcsgx_sfe_uc_evl_input.y[i] << " ";
	cout << "\ntt -\n";
	for (i = 0; i < sfe_uc_gatecount; i++)
		cout << smcsgx_sfe_uc_evl_input.tt[i] << " ";
	cout << "\nd -\n";
	for (i = 0; i < sfe_uc_gatecount; i++)
		cout << smcsgx_sfe_uc_evl_input.d[i] << " ";
	cout << "\noutput -\n";
	for (i = 0; i < sfe_uc_numoutputs; i++)
		cout << smcsgx_sfe_uc_evl_input.output[i] << " ";
	cout << endl;

	cout << "SFE dijk evl input -\n";
	printf("startnode: %d, endnode: %d\n",
			smcsgx_sfe_dj_evl_input.startnode,
			smcsgx_sfe_dj_evl_input.endnode);
	cout << endl;

	cout << "SFE oram evl input -\n";
	for (i = 0; i < sfe_or_sizeOfDatabase; i++)
		cout << smcsgx_sfe_or_evl_input[i] << " ";
	cout << endl;
}

static void dump_yao_user_input(void)
{
	int i;

	cout << "YAO oram/db evl input -\n";
	for (i = 0; i < yao_or_datasize*2; i++)
		cout << smcsgx_yao_or_evl_input[i] << " ";
	cout << endl;

	cout << "YAO dijkstra evl input -\n";
	for (i = 0; i < HYBRID_DJ_EVL_INPUT_NUM; i++)
		cout << smcsgx_yao_dj_evl_input[i] << " ";
	cout << endl;

        cout << "YAO dijkstra gen input -\n";
        for (i = 0; i < HYBRID_DJ_GEN_INPUT_NUM; i++)
                cout << smcsgx_yao_dj_gen_input[i] << " ";
        cout << endl;

	cout << "YAO oram/db query split evl input -\n";
	for (i = 0; i < yao_or_split_datasize*2; i++)
		cout << smcsgx_yao_or_split_evl_input[i] << " ";
	cout << endl;
}


/* Parser APIs for reading in input files */
static int parse_input_sfe(char *buf, int len)
{
	char *ptr;
	char *head;
	int i;

	if (trans_hardcode_input) {
		for (i = 0; i < SMC_SGX_SFE_INPUT_LEN; i++)
			smcsgx_sfe_evl_input[i] = 0;
		smcsgx_sfe_evl_input[0] = 5;
		smcsgx_sfe_evl_input[1] = 1;

		return 0;
	}

	/* Bypass the tag line */
	ptr = buf;
	while ((ptr <= (buf+len)) && (*ptr != '\0'))
		ptr++;
	ptr++;

	/* Parse the input line */
	i = 0;
	head = ptr;
	while ((ptr <= (buf+len)) && (i < SMC_SGX_SFE_INPUT_LEN)) {
		if ((*ptr == ',') || (*ptr == '\n') || (*ptr == '\0')) {
			/* Get the value */
			*ptr = '\0';
			smcsgx_sfe_evl_input[i] = strtol(head, NULL, 10);
			i++;
			ptr++;
			head = ptr;
		} else {
			ptr++;
		}
	}

	return 0;
}

static int parse_input_sfe_uc(char *buf, int len)
{
	int i;

	/* FIXME: instead of parsing the huge CSV file
	 * would be better to hardcode the input here
	 * May 11, 2016
	 * daveti
	 */
	if (trans_hardcode_input) {
		for (i = 0; i < sfe_uc_gatecount; i++) {
			smcsgx_sfe_uc_evl_input.x[i] = i;
			smcsgx_sfe_uc_evl_input.y[i] = i;
			smcsgx_sfe_uc_evl_input.tt[i] = 5;
			smcsgx_sfe_uc_evl_input.d[i] = i;
			if (i < sfe_uc_numoutputs)
				smcsgx_sfe_uc_evl_input.output[i] = i;
		}

		return 0;
	}

	return 0;
}

static int parse_input_sfe_dj(char *buf, int len)
{
	char *ptr;

	if (trans_hardcode_input) {
		smcsgx_sfe_dj_evl_input.startnode = 0;
		smcsgx_sfe_dj_evl_input.endnode = sfe_dj_Nodes - 1;	
		return 0;
	}

	/* Bypass the tag line */
	ptr = buf;
	while ((ptr <= (buf+len)) && (*ptr != '\0'))
		ptr++;
	ptr++;

	/* Parse the input line */
	if (trans_debug)
		printf("%s: line 1 [%s]\n", __func__, ptr);
	smcsgx_sfe_dj_evl_input.startnode = strtol(ptr, NULL, 10);
	while ((ptr <= (buf+len)) && (*ptr != '\0'))
		ptr++;
	ptr++;
	if (trans_debug)
		printf("%s: line 2 [%s]\n", __func__, ptr);
	smcsgx_sfe_dj_evl_input.endnode = strtol(ptr, NULL, 10);

	return 0;
}

static int parse_input_sfe_or(char *buf, int len)
{
	int i;

	/* Hardcode for UT */
	if (trans_hardcode_input) { 
		for (i = 0; i < sfe_or_sizeOfDatabase; i++)
			smcsgx_sfe_or_evl_input[i] = i;
		return 0;
	}

	return 0;
}


static void startYao(void)
{
	int i, j;
	sgx_status_t ret, status;
	Output out;
	Input programInput;

	/* Start Yao now */
	smcsgx_yao = startProgram();

	if (trans_yao_idx == 0) {
		/* NOTE: for evaluator, we do not care about evlgc */
		programInput = rebuildInput(smcsgx_yao_gengc,
				smcsgx_yao_evlgc,
				smcsgx_yao_permutes);

		out = runProgram(SMC_SGX_YAO_PARAM_CIRCUIT_FILE,
				programInput.evalInput,
				programInput.genInput,
				smcsgx_yao);
	}
	else if (trans_yao_idx == 1) {
                programInput = rebuildInput2(smcsgx_yao_or_gengc,
				yao_or_secretsize*2,
                                smcsgx_yao_or_evlgc,
				yao_or_secretsize,
                                smcsgx_yao_or_permutes,
				yao_or_secretsize);

                out = runProgram(HYBRID_OR_CIRCUIT_FILE,
                                programInput.evalInput,
                                programInput.genInput,
                                smcsgx_yao);
	}
	else if (trans_yao_idx == 2) {
                programInput = rebuildInput2(smcsgx_yao_dj_gengc,
                                yao_dj_dijsgxinput*2,
                                smcsgx_yao_dj_evlgc,
                                yao_dj_dijsgxinput,
                                smcsgx_yao_dj_permutes,
                                yao_dj_dijsgxinput);

                out = runProgram(HYBRID_DJ_CIRCUIT_FILE,
                                programInput.evalInput,
                                programInput.genInput,
                                smcsgx_yao);
	}
	else if (trans_yao_idx == 3) {
                programInput = rebuildInput2(smcsgx_yao_or_split_gengc,
				yao_or_split_datasize*64*2,
                                smcsgx_yao_or_split_evlgc,
				yao_or_split_datasize*64,
                                smcsgx_yao_or_split_permutes,
				yao_or_split_datasize*64);

                out = runProgram(HYBRID_OR_SPLIT_CIRCUIT_FILE,
                                programInput.evalInput,
                                programInput.genInput,
                                smcsgx_yao);
	}

	cout << "\nYAO output:\n";
	for (i = 0; i < out.output.size(); i++) {
		for (j = 0; j < SMC_SGX_YAO_MAGIC_J; j++)
			printf("%hhu ",out.output[i][j]);
		cout << "\n";
	}	
}



/*
 * Transition functions
 * Note: all trans handlers should update the current_state
 * excluding the error state setting during the processing!
 * all trans handlers should return
 * SMC_SGX_RTN_SUCCESS or
 * SMC_SGX_RTN_FAILURE
 */

/*
 * Handle the start of a remote attestation
 * Creates the enclave and generates S1 and send it
 * Note: Ecall/SGX involved
 */
static int handle_att_nonce(int len, char *buf)
{
	sgx_status_t	retval;
	int		s1_len;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		current_state = SMC_SGX_STM_ATT_S1_SENT;
		return SMC_SGX_RTN_SUCCESS;
	}

	/* Save the nonce into enclave */
	if (put_nonce(mysgx_geteid(), &retval, len, buf) != SGX_SUCCESS) {
		printf("trans-error: ECall put_nonce failed\n");
		return SMC_SGX_RTN_FAILURE;
	}

	/* Generate S1 */
	if (mysgx_gen_S1(&s1_len, (char *)&trans_S1) != SGX_SUCCESS) {
		printf("trans-error: mysgx_gen_S1 failed\n");
		return SMC_SGX_RTN_FAILURE;
	}

	/* daveti: just wanna make sure... */
	if (s1_len != sizeof(trans_S1)) {
		printf("daveti-error: X_ra_msg1_t mismatch\n");
		return SMC_SGX_RTN_FAILURE;
	}
	if (trans_debug) {
		dump_S1();
		dump_byte_array((void *)&trans_S1, s1_len);
	}

	/* Build and send S1 */
	if (build_send_smcsgx_msg(SMC_SGX_MSG_TAG_ATT_S1,
				s1_len, (char *)&trans_S1) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: sending S1 failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	printf("trans-info: waiting for S2 from sender...\n");

	current_state = SMC_SGX_STM_ATT_S1_SENT;
	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Handle the S2 from the sender
 * Generates S3 and send it to the sender
 * Note: Ecall/SGX involved
 */
static int handle_att_S2(int len, char *buf)
{
	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		current_state = SMC_SGX_STM_ATT_S3_SENT;
		return SMC_SGX_RTN_SUCCESS;
	}

	/* Defensive checking */
	if (len != sizeof(sgx_ra_msg2_t)) {
		printf("trans-error: X_ra_msg2_t mismatch\n");
		return SMC_SGX_RTN_FAILURE;
	}

	memcpy(&trans_S2, buf, len);
	if (trans_debug) {
		dump_byte_array(&trans_S2, len);
		dump_S2();
	}

	/* Generate S3 */
	if (trans_S3)
		free(trans_S3);
	if (mysgx_gen_S3(sizeof(trans_S2), (char *)&trans_S2,
				&trans_S3_len, (char **)&trans_S3) != SGX_SUCCESS) {
		printf("trans-error: mysgx_gen_S3 failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	if (trans_debug) {
		dump_byte_array((void *)trans_S3, trans_S3_len);
		dump_S3();
	}

	/* Build and send S3 */
	if (build_send_smcsgx_msg(SMC_SGX_MSG_TAG_ATT_S3,
				trans_S3_len, (char *)trans_S3) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: sending S3 failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	printf("trans-info: sent S3 to sender...\n"
			"trans-info: waiting for att result from sender...\n");

	current_state = SMC_SGX_STM_ATT_S3_SENT;
	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Handle the attestation result from the sender
 * Redirect the STM to error if the att failed
 * otherwise move on
 * NOTE: ECall involved
 */
static int handle_att_res(int len, char *buf)
{
	sgx_status_t status;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		current_state = SMC_SGX_STM_INPUT_WAIT;
		return SMC_SGX_RTN_SUCCESS;
	}

	/* Defensive checking */
	if (len < sizeof(smcsgx_ra_att_result_msg_t)) {
		printf("trans-error: invalid length of the att result\n");
		return SMC_SGX_RTN_FAILURE;
	}

	/* Save the att result */
	if (trans_att_res)
		free(trans_att_res);
	trans_att_res = (smcsgx_ra_att_result_msg_t *)malloc(len);
	if (!trans_att_res) {
		printf("trans-error: malloc for trans_att_res failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	memcpy((void *)trans_att_res, (void *)buf, len);
	if (trans_debug)
		dump_att_res();

	/* Verify the att result */
	if (trans_att_res->result == SMC_SGX_ATT_RES_BAD) {
		printf("trans-info: attestation result bad\n");
		return SMC_SGX_RTN_FAILURE;
	}
	printf("trans-info: attestation result good\n");
	if (mysgx_verify_att_res_mac(trans_att_res) != SGX_SUCCESS) {
		printf("trans-error: attestation mac verification failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	printf("trans-info: attestation mac verification succeeded\n");

	/* Decrypt and save the shared secret in the enclave */
	if (mysgx_gen_secret(trans_att_res) != SGX_SUCCESS) {
		printf("trans-error: shared secret generation failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	if (trans_debug) {
		/* FIXME: this is a security risk! */
		if (get_secret(mysgx_geteid(), &status,
					trans_secret_len, trans_secret) != SGX_SUCCESS) {
			printf("trans-error: getting shared secret failed\n");
			return SMC_SGX_RTN_FAILURE;
		}
		dump_shared_secret();
	}

	printf("trans-info: waiting for input from sender...\n");

	current_state = SMC_SGX_STM_INPUT_WAIT;
	return SMC_SGX_RTN_SUCCESS;
}


/*
 * Handle the input from the sender
 * run sgx/yao to generate the final result
 * Note: Ecall involved.
 */
static int handle_input(int len, char *buf)
{
	int idx;
	sgx_status_t ret;
	sgx_status_t status;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		current_state = SMC_SGX_STM_RES_SENT;
		return SMC_SGX_RTN_SUCCESS;
	}

	if (trans_debug)
		dump_byte_array((void *)buf, len);

	/* Figure out the computation model */
	if (trans_yao) {
		printf("trans-info: using YAO hybrid model\n");
		/* Look into the subtag */
		if (!strcasecmp(buf, SMC_SGX_INPUT_MSG_YAO)) {
			printf("trans-info: YAO default testing case requested\n");
			idx = 0;
		} else if (!strcasecmp(buf, SMC_SGX_INPUT_MSG_YAO_OR)) {
			printf("trans-info: YAO oram/db testing case requested\n");
			idx = 1;
		} else if (!strcasecmp(buf, SMC_SGX_INPUT_MSG_YAO_DJ)) {
			printf("trans-info: YAO dijkstra testing case requested\n");
			idx = 2;
		} else if (!strcasecmp(buf, SMC_SGX_INPUT_MSG_YAO_OR_SPLIT)) {
			printf("trans-info: YAO query/split testing case requested\n");
			idx = 3;
		}
		if (trans_yao_idx != idx) {
			printf("trans-error: YAO index mismatching between 2 parties\n");
			return SMC_SGX_RTN_FAILURE;
		}
	} else {
		printf("trans-info: using SFE model\n");
		/* Look into the subtag */
		if (!strcasecmp(buf, SMC_SGX_INPUT_MSG_SFE)) {
			printf("trans-info: using SFE mil testing case\n");
			trans_sfe = 0;
		} else if (!strcasecmp(buf, SMC_SGX_INPUT_MSG_SFE_UC)) {
			printf("trans-info: using SFE uc testing case\n");
			trans_sfe = 1;
		} else if (!strcasecmp(buf, SMC_SGX_INPUT_MSG_SFE_DJ)) {
			printf("trans-info: using SFE dijk testing case\n");
			trans_sfe = 2;
		} else if (!strcasecmp(buf, SMC_SGX_INPUT_MSG_SFE_OR)) {
			printf("trans-info: using SFE oram testing case\n");
			trans_sfe = 3;
		}
	}

	/*
	 * Pass the input to the enclave
	 * and retrieve the output.
	 * NOTE: for hybrid model, YAO would be involved
	 * - a loop starting SGX, then YAO, then SGX...
	 */
	if (!trans_yao) {
		/* Normal SFE */
		if (trans_sfe == 0) {
			trans_output_len = SMC_SGX_SFE_OUTPUT_ENC_LEN + SMC_SGX_SP_TAG_SIZE;
			if (compute_sfe(mysgx_geteid(), &status,
						len-sizeof(SMC_SGX_INPUT_MSG_SFE),
						buf+sizeof(SMC_SGX_INPUT_MSG_SFE),
						SMC_SGX_SFE_OUTPUT_LEN, (char *)&smcsgx_sfe_evl_output,
						SMC_SGX_SFE_OUTPUT_LEN, (char *)&smcsgx_sfe_gen_output,
						SMC_SGX_SFE_INPUT_LEN*sizeof(uint32_t),
						(char *)smcsgx_sfe_gen_input,
						trans_output_len, trans_output,
						trans_naive) != SGX_SUCCESS) {
				printf("trans-error: SFE mil computation failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			if (trans_debug)
				dump_sfe_things();
		}
		else if (trans_sfe == 1) {
			trans_output_len = SMC_SGX_SFE_UC_OUTPUT_LEN + SMC_SGX_SP_TAG_SIZE;
			if (compute_sfe_uc(mysgx_geteid(), &status,
						len-sizeof(SMC_SGX_INPUT_MSG_SFE_UC),
						buf+sizeof(SMC_SGX_INPUT_MSG_SFE_UC),
						SMC_SGX_SFE_UC_OUTPUT_LEN, (char *)smcsgx_sfe_uc_output,
						SMC_SGX_SFE_UC_INPUT_LEN, (char *)smcsgx_sfe_uc_gen_input,
						trans_output_len, trans_output,
						trans_naive) != SGX_SUCCESS) {
				printf("trans-error: SFE uc computation failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			if (trans_debug)
				dump_sfe_uc_things();
		}
		else if (trans_sfe == 2) { 
			trans_output_len = SMC_SGX_SFE_DJ_OUTPUT_LEN + SMC_SGX_SP_TAG_SIZE;
			if (compute_sfe_dj(mysgx_geteid(), &status,
						len-sizeof(SMC_SGX_INPUT_MSG_SFE_DJ),
						buf+sizeof(SMC_SGX_INPUT_MSG_SFE_DJ),
						SMC_SGX_SFE_DJ_OUTPUT_LEN, (char *)smcsgx_sfe_dj_output,
						sizeof(sfe_dj_input_send), (char *)&smcsgx_sfe_dj_gen_input,
						trans_output_len, trans_output,
						trans_naive) != SGX_SUCCESS) {
				printf("trans-error: SFE dijk computation failed\n");
				mysgx_perror(status, "status value");
				return SMC_SGX_RTN_FAILURE;
			}
			if (trans_debug)
				dump_sfe_dj_things();
		}
		else if (trans_sfe == 3) {
			trans_output_len = SMC_SGX_SFE_OR_OUTPUT_LEN + SMC_SGX_SP_TAG_SIZE;
			if (compute_sfe_or(mysgx_geteid(), &status,
						len-sizeof(SMC_SGX_INPUT_MSG_SFE_OR),
						buf+sizeof(SMC_SGX_INPUT_MSG_SFE_OR),
						SMC_SGX_SFE_OR_OUTPUT_LEN, (char *)smcsgx_sfe_or_output,
						SMC_SGX_SFE_OR_INPUT_GEN_LEN, (char *)smcsgx_sfe_or_gen_input,
						trans_output_len, trans_output,
						trans_oram) != SGX_SUCCESS) {
				printf("trans-error: SFE oram computation failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			/* Defensive checking */
			if (status != SGX_SUCCESS)
				printf("trans-error: compute_sfe_or failed internally\n");
			if (trans_debug)
				dump_sfe_or_things();
		}
		printf("trans-info: SFE computation done\n");
	} else {
		/* Hybrid model - YAO */
		if (trans_yao_idx == 0) {
			/* NOTE: we discard the input from the sender in this case */
			/* FIXME: exposing gengc here only for debugging purpose! */	
			ret = gen_input_yao(mysgx_geteid(), &status, 0, NULL,
					SMC_SGX_YAO_INPUT_GC_GEN_LEN, smcsgx_yao_gengc,
					SMC_SGX_YAO_INPUT_GC_EVL_LEN, smcsgx_yao_evlgc,
					SMC_SGX_YAO_PERMUTES_LEN, smcsgx_yao_permutes,
					SMC_SGX_YAO_INPUT_GEN_ENC_LEN, smcsgx_yao_gen_input);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value");
				mysgx_perror(status, "status value");
				printf("trans-error: gen_input_yao failed, ret 0x%x, status 0x%x\n",
						ret, status);
				return SMC_SGX_RTN_FAILURE;
			}
			if (trans_debug) {
				printf("trans-debug: gen_input_yao succeedded\n");
				dump_yao_gc();
			}

			memcpy(trans_hybrid, smcsgx_yao_gen_input, SMC_SGX_YAO_INPUT_GEN_ENC_LEN);
			trans_hybrid_len = SMC_SGX_YAO_INPUT_GEN_ENC_LEN;
		}
		else if (trans_yao_idx == 1) {
			/* Compute DB query in the enclave */
			trans_hybrid_len = HYBRID_OR_GEN_OUTPUT_LEN + SMC_SGX_SP_TAG_SIZE;
			ret = compute_yao_or(mysgx_geteid(), &status,
					len-sizeof(SMC_SGX_INPUT_MSG_YAO_OR),
					buf+sizeof(SMC_SGX_INPUT_MSG_YAO_OR),
					HYBRID_OR_SGX_OUTPUT_LEN, (char *)smcsgx_yao_or_output_sgx,
					HYBRID_OR_GENGC_LEN, smcsgx_yao_or_gengc,
					HYBRID_OR_EVLGC_LEN, smcsgx_yao_or_evlgc,
					HYBRID_OR_PERMUTES_LEN, smcsgx_yao_or_permutes,
					HYBRID_OR_QUERY_LEN, (char *)smcsgx_yao_or_gen_input,
					trans_hybrid_len, trans_hybrid);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value");
				mysgx_perror(status, "status value");
				printf("trans-error: compute_yao_or failed\n");
				return SMC_SGX_RTN_FAILURE;
			}

			if (trans_debug) {
				printf("trans-debug: compute_yao_or succeedded\n");
				dump_yao_or_all();
			}
		}
		else if (trans_yao_idx == 2) {
			/* Compute dijkstra in the enclave */
                        trans_hybrid_len = HYBRID_DJ_GEN_OUTPUT_LEN + SMC_SGX_SP_TAG_SIZE;
#ifdef BEN_IS_NICE_TO_DAVE
                        ret = compute_yao_dj(mysgx_geteid(), &status,
                                        len-sizeof(SMC_SGX_INPUT_MSG_YAO_DJ),
                                        buf+sizeof(SMC_SGX_INPUT_MSG_YAO_DJ),
                                        HYBRID_DJ_SGX_OUTPUT_LEN, (char *)smcsgx_yao_dj_output_sgx,
                                        HYBRID_DJ_GENGC_LEN, smcsgx_yao_dj_gengc,
                                        HYBRID_DJ_EVLGC_LEN, smcsgx_yao_dj_evlgc,
                                        HYBRID_DJ_PERMUTES_LEN, smcsgx_yao_dj_permutes,
                                        HYBRID_DJ_GEN_INPUT_LEN, (char *)smcsgx_yao_dj_gen_input,
                                        trans_hybrid_len, trans_hybrid);
#else
                        ret = compute_yao_dj(mysgx_geteid(), &status,
                                        len-sizeof(SMC_SGX_INPUT_MSG_YAO_DJ),
                                        buf+sizeof(SMC_SGX_INPUT_MSG_YAO_DJ),
                                        HYBRID_DJ_SGX_OUTPUT_LEN, (char *)smcsgx_yao_dj_output_sgx,
                                        HYBRID_DJ_GENGC_LEN, smcsgx_yao_dj_gengc,
                                        HYBRID_DJ_EVLGC_LEN, smcsgx_yao_dj_evlgc,
                                        HYBRID_DJ_PERMUTES_LEN, smcsgx_yao_dj_permutes,
                                        HYBRID_DJ_EVL_INPUT_LEN, (char *)smcsgx_yao_dj_evl_input,
                                        trans_hybrid_len, trans_hybrid);
#endif
                        if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
                                mysgx_perror(ret, "ret value");
                                mysgx_perror(status, "status value");
                                printf("trans-error: compute_yao_dj failed\n");
                                return SMC_SGX_RTN_FAILURE;
                        }

                        if (trans_debug) {
                                printf("trans-debug: compute_yao_dj succeedded\n");
                                dump_yao_dj_all();
                        }
			dump_yao_dj_sgx_output();
		}
		else if (trans_yao_idx == 3) {
			/* Compute DB query split in the enclave */
			trans_hybrid_len = HYBRID_OR_SPLIT_GEN_OUTPUT_LEN + SMC_SGX_SP_TAG_SIZE;
			ret = compute_yao_or_split(mysgx_geteid(), &status,
					len-sizeof(SMC_SGX_INPUT_MSG_YAO_OR_SPLIT),
					buf+sizeof(SMC_SGX_INPUT_MSG_YAO_OR_SPLIT),
					HYBRID_OR_SPLIT_SGX_OUTPUT_LEN, (char *)smcsgx_yao_or_split_output_sgx,
					HYBRID_OR_SPLIT_GENGC_LEN, smcsgx_yao_or_split_gengc,
					HYBRID_OR_SPLIT_EVLGC_LEN, smcsgx_yao_or_split_evlgc,
					HYBRID_OR_SPLIT_PERMUTES_LEN, smcsgx_yao_or_split_permutes,
					HYBRID_OR_SPLIT_QUERY_LEN, (char *)smcsgx_yao_or_split_gen_input,
					trans_hybrid_len, trans_hybrid);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value");
				mysgx_perror(status, "status value");
				printf("trans-error: compute_yao_or_split failed\n");
				return SMC_SGX_RTN_FAILURE;
			}

			if (trans_debug) {
				printf("trans-debug: compute_yao_or_split succeedded\n");
				dump_yao_or_split_all();
			}
		}

		/* Send the encrypted gen input to sender */
		if (build_send_smcsgx_msg(SMC_SGX_MSG_TAG_YAO_INPUT,
					trans_hybrid_len, trans_hybrid) != SMC_SGX_RTN_SUCCESS) {
			printf("trans-error: sending Yao input failed\n");
			return SMC_SGX_RTN_FAILURE;
		}
		printf("trans-info: sent Yao input to sender (YAO Evaluator)...\n"
				"trans-info: starting Yao...\n");

		startYao();
		printf("trans-info: YAO computation done\n");
		trans_output_len = 8;	// daveti: just fake a number here
	}

	if (trans_debug)
		dump_byte_array((void *)trans_output, trans_output_len);

	/* Build and send the output */
	if (build_send_smcsgx_msg(SMC_SGX_MSG_TAG_RESULT,
				trans_output_len, trans_output) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: sending result failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	printf("trans-info: evaluation done\n");

	current_state = SMC_SGX_STM_RES_SENT;
	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Handle all other exceptions in the STM
 * Reset the STM to be Err
 */
static int handle_err(int len, char *buf)
{
	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		current_state = SMC_SGX_STM_ERROR;
		return SMC_SGX_RTN_SUCCESS;
	}

	printf("trans-error: oops\n");
	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Stub for STM table init
 */
static int handle_stub(int len, char *buf)
{
	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);

	return SMC_SGX_RTN_SUCCESS;
}


/* STM transitions */
static smcsgx_stm_trans trans_stub = {
	SMC_SGX_STM_FINI,
	SMC_SGX_MSG_TAG_FINI,
	SMC_SGX_STM_FINI,
	handle_stub};
static smcsgx_stm_trans_entry entry_stub = {
	SMC_SGX_STM_FINI,
	handle_stub};
static smcsgx_stm_trans stm_trans[SMC_SGX_STM_NUM_MAX] = {
	/* expected */
	{SMC_SGX_STM_INIT, SMC_SGX_MSG_TAG_ATT_CHALLENGE,
		SMC_SGX_STM_ATT_RECVD, handle_att_nonce},
	{SMC_SGX_STM_ATT_S1_SENT, SMC_SGX_MSG_TAG_ATT_S2,
		SMC_SGX_STM_ATT_S2_RECVD, handle_att_S2},
	{SMC_SGX_STM_ATT_S3_SENT, SMC_SGX_MSG_TAG_ATT_RESULT,
		SMC_SGX_STM_ATT_RES_RECVD, handle_att_res},
	{SMC_SGX_STM_INPUT_WAIT, SMC_SGX_MSG_TAG_INPUT,
		SMC_SGX_STM_INPUT_RECVD, handle_input},
	{SMC_SGX_STM_RES_SENT, SMC_SGX_MSG_TAG_STUB,
		SMC_SGX_STM_INIT, handle_stub},
	/* errors */
	/* Note: always allow new att */
	{SMC_SGX_STM_ANY, SMC_SGX_MSG_TAG_ATT_CHALLENGE,
		SMC_SGX_STM_ATT_RECVD, handle_att_nonce},
	/* Note: we just keep down when error happens */
	{SMC_SGX_STM_ERROR, SMC_SGX_MSG_TAG_ANY,
		SMC_SGX_STM_ERROR, handle_err},
	{SMC_SGX_STM_ANY, SMC_SGX_MSG_TAG_INPUT,
		SMC_SGX_STM_ERROR, handle_err},
	/* Padding */
	trans_stub,
	trans_stub,
	/* 10 */
	trans_stub,
	trans_stub,
	trans_stub,
	trans_stub,
	trans_stub,
	/* 15 */
	trans_stub,
	trans_stub,
	trans_stub,
	trans_stub,
	trans_stub
		/* 20 */
};

/* Internal APIs */
static void init_stm_tbl(void)
{
	int i, j;

	/* Init the stm table */
	for (i = 0; i < SMC_SGX_STM_NUM_MAX; i++) 
		for (j = 0; j < SMC_SGX_MSG_TAG_NUM_MAX; j++)
			stm_tbl[i][j] = entry_stub;

	/* Construct the stm transition */
	for (i = 0; i < SMC_SGX_STM_NUM_MAX; i++) {
		if (stm_trans[i].current_state == SMC_SGX_STM_FINI)
			break;
		stm_tbl[stm_trans[i].current_state][stm_trans[i].event] = {
			stm_trans[i].new_state,
			stm_trans[i].transition};
	}
}


/* APIs */
int trans_fini(void)
{
	return trans_done;
}

void trans_read_input(char *path)
{
	int len;
	sgx_status_t status;
	vector<unsigned int> bobin;
	unsigned int *input_array;

	if (trans_yao) {
		switch (trans_yao_idx) {
			case 0:
				break;
			case 1:
				bobin.resize(yao_or_datasize*2);
				fileToVector(HYBRID_OR_EVL_INPUT_FILE, bobin, bobin.size());
				input_array = &bobin[0];
				memcpy(smcsgx_yao_or_evl_input, input_array, HYBRID_OR_DATABASE_LEN*2);
				if (put_input_yao_or(mysgx_geteid(), &status,
							HYBRID_OR_DATABASE_LEN*2,
							(char *)smcsgx_yao_or_evl_input) != SGX_SUCCESS)
					printf("trans-error: put_input_yao_or failed in %s\n", __func__);
				break;
			case 3:
				bobin.resize(yao_or_split_datasize*2);
				fileToVector(HYBRID_OR_SPLIT_EVL_INPUT_FILE, bobin, bobin.size());
				input_array = &bobin[0];
				memcpy(smcsgx_yao_or_split_evl_input, input_array, HYBRID_OR_SPLIT_DATABASE_LEN*2);
				if (put_input_yao_or_split(mysgx_geteid(), &status,
							HYBRID_OR_SPLIT_DATABASE_LEN*2,
							(char *)smcsgx_yao_or_split_evl_input) != SGX_SUCCESS)
					printf("trans-error: put_input_yao_or_split failed in %s\n", __func__);
				break;
			case 2:
#ifdef BEN_IS_NICE_TO_DAVE
				bobin.resize(HYBRID_DJ_EVL_INPUT_NUM);
				fileToVector(HYBRID_DJ_EVL_INPUT_FILE, bobin, bobin.size());
                                input_array = &bobin[0];
                                memcpy(smcsgx_yao_dj_evl_input, input_array, HYBRID_DJ_EVL_INPUT_LEN);
                                if (put_input_yao_dj(mysgx_geteid(), &status,
                                                        HYBRID_DJ_EVL_INPUT_LEN,
                                                        (char *)smcsgx_yao_dj_evl_input) != SGX_SUCCESS)
                                        printf("trans-error: put_input_yao_dj failed in %s\n", __func__);
				break;
#else
				/* Reuse bobin for alicein */
				printf("trans-info: new hybrid model for dijkstra\n");
                                bobin.resize(HYBRID_DJ_GEN_INPUT_NUM);
                                fileToVector(HYBRID_DJ_GEN_INPUT_FILE, bobin, bobin.size());
                                input_array = &bobin[0];
                                memcpy(smcsgx_yao_dj_gen_input, input_array,
                                        HYBRID_DJ_GEN_INPUT_LEN);
                                if (put_input_yao_dj2(mysgx_geteid(), &status,
                                                        HYBRID_DJ_GEN_INPUT_LEN,
                                                        (char *)smcsgx_yao_dj_gen_input) != SGX_SUCCESS)
                                        printf("trans-error: put_input_yao_dj2 failed in %s\n", __func__);
                                break;
#endif

			default:
				printf("trans-error: unsupported YAO index %d\n",
						trans_yao_idx);
				break;
		}

		if (trans_debug)
			dump_yao_user_input();
		return;
	}

	/* Reuse trans_buff to read file in */
	len = smcsgx_read_input(path, trans_buff, SMC_SGX_BUF_LEN);
	if (len <= 0) {
		printf("trans-error: smcsgx_read_input failed in %s for file %s\n",
				__func__, path);
		return;
	}

	/* Parse different inputs accordingly */
	if (!strncasecmp(trans_buff, SMC_SGX_INPUT_MSG_SFE,
				strlen(SMC_SGX_INPUT_MSG_SFE))) {
		if (parse_input_sfe(trans_buff, len)) {
			printf("trans-error: parse_input_sfe failed in %s\n", __func__);
			return;
		}
		if (put_input_sfe(mysgx_geteid(), &status,
					SMC_SGX_SFE_INPUT_LEN*sizeof(uint32_t),
					(char *)smcsgx_sfe_evl_input) != SGX_SUCCESS) {
			printf("trans-error: put_input_sfe failed in %s\n", __func__);
			return;
		}
	}
	else if (!strncasecmp(trans_buff, SMC_SGX_INPUT_MSG_SFE_UC,
				strlen(SMC_SGX_INPUT_MSG_SFE_UC))) {
		if (parse_input_sfe_uc(trans_buff, len)) {
			printf("trans-error: parse_input_sfe_uc failed in %s\n", __func__);
			return;
		}

		if ((status = put_input_sfe_uc(mysgx_geteid(), &status,
						sizeof(sfe_uc_input_program),
						(char *)&smcsgx_sfe_uc_evl_input)) != SGX_SUCCESS) {
			mysgx_perror(status, "trans-error: sgx error - ");
			printf("trans-error: put_input_sfe_uc failed in %s\n", __func__);
			return;
		}
	}
	else if (!strncasecmp(trans_buff, SMC_SGX_INPUT_MSG_SFE_DJ,
				strlen(SMC_SGX_INPUT_MSG_SFE_DJ))) {
		if (parse_input_sfe_dj(trans_buff, len)) {
			printf("trans-error: parse_input_sfe_dj failed in %s\n", __func__);
			return;
		}
		if (put_input_sfe_dj(mysgx_geteid(), &status,
					sizeof(sfe_dj_input_eval),
					(char *)&smcsgx_sfe_dj_evl_input) != SGX_SUCCESS) {
			printf("trans-error: put_input_sfe_dj failed in %s\n", __func__);
			return;
		}
	}
	else if (!strncasecmp(trans_buff, SMC_SGX_INPUT_MSG_SFE_OR,
				strlen(SMC_SGX_INPUT_MSG_SFE_OR))) {
		if (parse_input_sfe_or(trans_buff, len)) {
			printf("trans-error: parse_input_sfe_or failed in %s\n", __func__);
			return;
		}
		if (put_input_sfe_or(mysgx_geteid(), &status,
					SMC_SGX_SFE_OR_INPUT_EVL_LEN,
					(char *)smcsgx_sfe_or_evl_input) != SGX_SUCCESS) {
			printf("trans-error: put_input_sfe_dj failed in %s\n", __func__);
			return;
		}
	}
	else {
		printf("trans-error: unknown input file %s\n", path);
		return;
	}

	if (trans_debug)
		dump_sfe_user_input();
}

void trans_init(int fd, int debug, int ut, int oram, int naive, int hybrid, int yao_idx)
{
	int rtn;

	/* Init global vars */
	remote_fd = fd;
	trans_debug = debug;
	trans_ut = ut;
	trans_oram = oram;
	trans_naive = naive;
	trans_yao = hybrid;	/* Reuse the flag set by the sender */
	trans_yao_idx = yao_idx;
	current_state = SMC_SGX_STM_INIT;

	/* Init the STM entry table */
	init_stm_tbl();
}

void trans_main(smcsgx_msghdr *msg)
{
	int rtn;
	int current;
	char *buf;
	smcsgx_stm_trans_entry *entry;

	/* Look up the table */
	current = current_state;
	entry = &(stm_tbl[current][msg->tag]);

	/* UT */
	if (trans_ut)
		buf = NULL;
	else
		buf = msg->val;

	/* Transition */
	current_state = entry->new_state;
	rtn = entry->transition(msg->len, buf);
	if (rtn != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: transition failed with old|next|current states: %s|%s|%s\n",
				smcsgx_stm_name(current), smcsgx_stm_name(entry->new_state), smcsgx_stm_name(current_state));
		current_state = SMC_SGX_STM_ERROR;
	}

	/* Force an extra transition to reinit the STM */
	if (current_state == SMC_SGX_STM_RES_SENT) {
		current_state = stm_tbl[current_state][SMC_SGX_MSG_TAG_STUB].new_state;
		trans_done = 1;
	}

	if (trans_debug)
		printf("trans-debug: transition done with old|next|current states: %s|%s|%s\n",
				smcsgx_stm_name(current), smcsgx_stm_name(entry->new_state), smcsgx_stm_name(current_state));
}
