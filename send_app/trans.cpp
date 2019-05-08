/*
 * trans.cpp
 * STM Transition Implementations for the sender
 * NOTE: I stole a lot of code from the service_provider (SP) implementation,
 * make a cast when reading the code sender=SP!
 * Ref: /opt/intel/sgxsdk/SampleCode/RemoteAttestation/service_provider/service_provider.cpp
 * Apr 12, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>
#include "sgx_status.h"
#include "../include/smcsgx.h"
#include "../include/smcsgxra.h"
#include "../include/smcsgxyao.h"
#include "../include/smcsgxsfe.h"
#include "../include/mysgx_common.h"
#include "../include/smcsgxyaoparam.h"
#include "mysgx.h"
#include "send_enclave_u.h"
#include "ecp.h"
#include "utils.h"
#include "../sample_libcrypto/sample_libcrypto.h"

//daveti: I do not want the header-including-dependency-mess!
#define SMC_SGX_YAO_INPUT_GEN_ENC_LEN	(SMC_SGX_YAO_INPUT_GEN_LEN+SMC_SGX_SP_TAG_SIZE)

/* Global vars - NOT thread-safe */
static int remote_fd;
static int current_state;
static int trans_debug;
static int trans_ut;
static int trans_bypass_sgx = 0;
static int trans_yao;
static int trans_yao_idx;
static int trans_sfe;
static int trans_done;
static int trans_hardcode_input = 1;
static char trans_buff[SMC_SGX_BUF_LEN];
static smcsgx_msghdr trans_msghdr;
static smcsgx_ra_msg1_t trans_S1;
static smcsgx_ra_msg2_t trans_S2;
static smcsgx_ra_msg3_t *trans_S3;
static int trans_S3_len;
static smcsgx_stm_trans_entry stm_tbl[SMC_SGX_STM_NUM_MAX][SMC_SGX_MSG_TAG_NUM_MAX];
static smcsgx_spid_t g_sim_spid = {"daveti"};
static ias_att_report_t attestation_report;
static char smcsgx_yao_gengc[SMC_SGX_YAO_INPUT_GC_GEN_LEN];
static char smcsgx_yao_evlgc[SMC_SGX_YAO_INPUT_GC_EVL_LEN];
static char smcsgx_yao_permutes[SMC_SGX_YAO_PERMUTES_LEN];
static char smcsgx_yao_input[SMC_SGX_YAO_INPUT_GEN_LEN];
static char smcsgx_input[SMC_SGX_INPUT_LEN_MAX];
BetterYao *smcsgx_yao;
static const uint8_t aes_gcm_iv[SMC_SGX_SP_IV_SIZE] = {0};
static int smcsgx_sfe_output;
static bool smcsgx_sfe_output_uc[sfe_uc_numoutputs];
static int smcsgx_sfe_output_dj[sfe_dj_Nodes];
static unsigned long smcsgx_sfe_output_or[sfe_or_queries];
static char smcsgx_eval_enclave_hash[SMC_SGX_SHA256_LEN];
static uint32_t smcsgx_sfe_gen_input[SMC_SGX_SFE_INPUT_LEN];
static bool smcsgx_sfe_uc_gen_input[sfe_uc_numinputs];
static int smcsgx_sfe_dj_gen_input[SMC_SGX_SFE_DJ_INPUT_NUM];
static int sfe_dj_edges[SMC_SGX_SFE_DJ_INPUT_EDGES_NUM];
static int sfe_dj_conns[SMC_SGX_SFE_DJ_INPUT_CONNS_NUM];
static unsigned int smcsgx_sfe_or_gen_input[sfe_or_queries];
static int trans_perf;
static struct timeval start_tv, end_tv;
static unsigned int smcsgx_yao_or_gen_input[yao_or_queries];
static char smcsgx_yao_or_gengc[HYBRID_OR_GENGC_LEN];
static char smcsgx_yao_or_evlgc[HYBRID_OR_EVLGC_LEN];
static char smcsgx_yao_or_permutes[HYBRID_OR_PERMUTES_LEN];
static unsigned long smcsgx_yao_or_output_sgx[yao_or_queries];
static char smcsgx_yao_or_input[HYBRID_OR_GEN_OUTPUT_LEN];
static unsigned int smcsgx_yao_dj_gen_input[HYBRID_DJ_GEN_INPUT_NUM];
static unsigned int smcsgx_yao_dj_evl_input[HYBRID_DJ_EVL_INPUT_NUM];
static char smcsgx_yao_dj_gengc[HYBRID_DJ_GENGC_LEN];
static char smcsgx_yao_dj_evlgc[HYBRID_DJ_EVLGC_LEN];
static char smcsgx_yao_dj_permutes[HYBRID_DJ_PERMUTES_LEN];
static char smcsgx_yao_dj_input[HYBRID_DJ_GEN_OUTPUT_LEN];
static unsigned int smcsgx_yao_or_split_gen_input[yao_or_split_queries];
static char smcsgx_yao_or_split_gengc[HYBRID_OR_SPLIT_GENGC_LEN];
static char smcsgx_yao_or_split_evlgc[HYBRID_OR_SPLIT_EVLGC_LEN];
static char smcsgx_yao_or_split_permutes[HYBRID_OR_SPLIT_PERMUTES_LEN];
static unsigned long smcsgx_yao_or_split_output_sgx[yao_or_split_queries];
static char smcsgx_yao_or_split_input[HYBRID_OR_SPLIT_GEN_OUTPUT_LEN];


// This is the private EC key of SP, the corresponding public EC key is
// hard coded in isv_enclave. It is based on NIST P-256 curve.
static const smcsgx_ec256_private_t g_sp_priv_key = {
	{
		0x90, 0xe7, 0x6c, 0xbb, 0x2d, 0x52, 0xa1, 0xce,
		0x3b, 0x66, 0xde, 0x11, 0x43, 0x9c, 0x87, 0xec,
		0x1f, 0x86, 0x6a, 0x3b, 0x65, 0xb6, 0xae, 0xea,
		0xad, 0x57, 0x34, 0x53, 0xd1, 0x03, 0x8c, 0x01
	}
};

// This is the public EC key of SP, this key is hard coded in isv_enclave.
// It is based on NIST P-256 curve. Not used in the SP code.
static const smcsgx_ec_pub_t g_sp_pub_key = {
	{
		0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
		0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
		0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
		0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38
	},
	{
		0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
		0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
		0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
		0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06
	}
};

// This is a context data structure used on SP side
typedef struct _sp_db_item_t
{
	smcsgx_ec_pub_t             g_a;
	smcsgx_ec_pub_t             g_b;
	smcsgx_ec_key_128bit_t      vk_key;// Shared secret key for the REPORT_DATA
	smcsgx_ec_key_128bit_t      mk_key;// Shared secret key for generating MAC's
	smcsgx_ec_key_128bit_t      sk_key;// Shared secret key for encryption
	smcsgx_ec_key_128bit_t      smk_key;// Used only for SIGMA protocol
	smcsgx_ec_priv_t            b;
	smcsgx_ps_sec_prop_desc_t   ps_sec_prop;
} sp_db_item_t;
static sp_db_item_t g_sp_db;
static bool g_is_sp_registered = false;
static int g_sp_credentials = 0;
static int g_authentication_token = 0;
//static uint8_t g_secret[8] = {0,1,2,3,4,5,6,7};
//Expand the shared secret to be 128 bits for future usage
static uint8_t g_secret[SMC_SGX_SHARED_SECRET_LEN] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
static smcsgx_spid_t g_spid;



/* Helpers */
static inline void build_smcsgx_msg(int tag, int len, char *payload, char *subtag)
{
	trans_msghdr.tag = tag;
	if (!subtag) {
		trans_msghdr.len = len;
		memcpy(trans_buff, &trans_msghdr, sizeof(trans_msghdr));
		/* We assume the buffer is large enough */
		memcpy(trans_buff+sizeof(trans_msghdr), payload, len);
	} else {
		trans_msghdr.len = len + strlen(subtag) + 1;
		memcpy(trans_buff, &trans_msghdr, sizeof(trans_msghdr));
		memcpy(trans_buff+sizeof(trans_msghdr), subtag, strlen(subtag)+1);
		memcpy(trans_buff+sizeof(trans_msghdr)+strlen(subtag)+1, payload, len);
	}
}

static int build_send_smcsgx_msg(int tag, int len, char *payload, char *subtag)
{
	int ret;
	int total;

	build_smcsgx_msg(tag, len, payload, subtag);
	total = sizeof(trans_msghdr)+trans_msghdr.len;
	ret = write(remote_fd, trans_buff, total);
	if (ret != total) {
		printf("trans-error: %s failed, requested %d, sent %d\n",
				__func__, total, ret);
		return SMC_SGX_RTN_FAILURE;
	}

	return SMC_SGX_RTN_SUCCESS;
}

static void dump_att_report(void)
{
	printf("\n\n\tAtestation Report:"
			"\n\tid: 0x%0x."
			"\n\tstatus: %d."
			"\n\trevocation_reason: %u."
			"\n\tpse_status: %d.\n",
			attestation_report.id,
			attestation_report.status,
			attestation_report.revocation_reason,
			attestation_report.pse_status);
}

static void dump_enclave_report(void)
{
	int i;
	smcsgx_quote_t *p_quote;

	p_quote = (smcsgx_quote_t *)trans_S3->quote;

	printf("\n\n\tEnclave Report:"
			"\n\tSignature Type: 0x%x",
			p_quote->sign_type);
	printf("\n\tSignature Basename: ");
	for (i = 0; i < sizeof(p_quote->basename.name) && p_quote->basename.name[i]; i++) 
		printf("%c", p_quote->basename.name[i]);
#ifdef __x86_64__
	printf("\n\tattributes.flags: 0x%0lx"
			"\n\tattributes.xfrm: 0x%0lx",
			p_quote->report_body.attributes.flags,
			p_quote->report_body.attributes.xfrm);
#else
	printf("\n\tattributes.flags: 0x%0llx"
			"\n\tattributes.xfrm: 0x%0llx",
			p_quote->report_body.attributes.flags,
			p_quote->report_body.attributes.xfrm);
#endif
	printf("\n\tmr_enclave: ");
	for (i = 0; i < sizeof(smcsgx_measurement_t); i++)
		printf("%02x", p_quote->report_body.mr_enclave[i]);

	printf("\n\tmr_signer: ");
	for (i = 0; i < sizeof(smcsgx_measurement_t); i++)
		printf("%02x", p_quote->report_body.mr_signer[i]);

	printf("\n\tisv_prod_id: 0x%0x"
			"\n\tisv_svn: 0x%0x\n",
			p_quote->report_body.isv_prod_id,
			p_quote->report_body.isv_svn);
}

/* daveti: helpers from trans.cpp in eval */
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
	smcsgx_ra_msg2_t* p_msg2_body = (smcsgx_ra_msg2_t*)(&trans_S2);

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

static void dump_att_res(smcsgx_ra_att_result_msg_t *p_att_result)
{
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

static void dump_shared_secret(sp_aes_gcm_data_t *sec)
{
	printf("SHARED SECRET (encrypted) - ");
	dump_byte_array((void *)sec->payload, sec->payload_size);
}

static void dump_yao_gc(void)
{
	printf("gen input (decrypted) - ");
	dump_byte_array((void *)smcsgx_yao_input, SMC_SGX_YAO_INPUT_GEN_LEN);

	printf("gengc - ");
	dump_byte_array((void *)smcsgx_yao_gengc, SMC_SGX_YAO_INPUT_GC_GEN_LEN);

	printf("evlgc - ");
	dump_byte_array((void *)smcsgx_yao_evlgc, SMC_SGX_YAO_INPUT_GC_EVL_LEN);

	printf("permutes - ");
	dump_byte_array((void *)smcsgx_yao_permutes, SMC_SGX_YAO_PERMUTES_LEN);
}

static void dump_yao_or_all(void)
{
        printf("gen input (decrypted) - ");
        dump_byte_array((void *)smcsgx_yao_or_input, HYBRID_OR_GEN_OUTPUT_LEN);

        printf("gengc - ");
        dump_byte_array((void *)smcsgx_yao_or_gengc, HYBRID_OR_GENGC_LEN);

        printf("evlgc - ");
        dump_byte_array((void *)smcsgx_yao_or_evlgc, HYBRID_OR_EVLGC_LEN);

        printf("permutes - ");
        dump_byte_array((void *)smcsgx_yao_or_permutes, HYBRID_OR_PERMUTES_LEN);
}

static void dump_yao_or_split_all(void)
{
        printf("gen input (decrypted) - ");
        dump_byte_array((void *)smcsgx_yao_or_split_input, HYBRID_OR_SPLIT_GEN_OUTPUT_LEN);

        printf("gengc - ");
        dump_byte_array((void *)smcsgx_yao_or_split_gengc, HYBRID_OR_SPLIT_GENGC_LEN);

        printf("evlgc - ");
        dump_byte_array((void *)smcsgx_yao_or_split_evlgc, HYBRID_OR_SPLIT_EVLGC_LEN);

        printf("permutes - ");
        dump_byte_array((void *)smcsgx_yao_or_split_permutes, HYBRID_OR_SPLIT_PERMUTES_LEN);
}

static void dump_yao_dj_all(void)
{
        printf("gen input (decrypted) - ");
        dump_byte_array((void *)smcsgx_yao_dj_input, HYBRID_DJ_GEN_OUTPUT_LEN);

        printf("gengc - ");
        dump_byte_array((void *)smcsgx_yao_dj_gengc, HYBRID_DJ_GENGC_LEN);

        printf("evlgc - ");
        dump_byte_array((void *)smcsgx_yao_dj_evlgc, HYBRID_DJ_EVLGC_LEN);

        printf("permutes - ");
        dump_byte_array((void *)smcsgx_yao_dj_permutes, HYBRID_DJ_PERMUTES_LEN);
}

static void dump_yao_or_output_sgx(void)
{
	int i;

	cout << "gen sgx otuput -\n";
	for (i = 0; i < yao_or_queries; i++)
		cout << smcsgx_yao_or_output_sgx[i] << " ";
	cout << endl;
}

static void dump_yao_or_split_output_sgx(void)
{
	int i;

	cout << "gen sgx otuput -\n";
	for (i = 0; i < yao_or_split_queries; i++)
		cout << smcsgx_yao_or_split_output_sgx[i] << " ";
	cout << endl;
}

static void dump_sfe_output_uc(void)
{
	int i;

	cout << "sfe uc output -\n";
	for (i = 0; i < sfe_uc_numoutputs; i++)
		cout << smcsgx_sfe_output_uc[i] << " ";
	cout << "\n";
}

static void dump_sfe_output_dj(void)
{
	int i;

	cout << "sfe dijk output -\n";
	for (i = 0; i < sfe_dj_Nodes; i++)
		cout << smcsgx_sfe_output_dj[i] << " ";
	cout << endl;
}

static void dump_sfe_output_or(void)
{
	int i;

	cout << "sfe oram output -\n";
	for (i = 0; i < sfe_or_queries; i++)
		cout << smcsgx_sfe_output_or[i] << " ";
	cout << endl;
}

static void dump_sfe_user_input(void)
{
	int i;

	cout << "SFE mil gen input -\n";
	for (i = 0; i < SMC_SGX_SFE_INPUT_LEN; i++)
		cout << smcsgx_sfe_gen_input[i] << " ";
	cout << endl;

	cout << "SFE uc gen input -\n";
	for (i = 0; i < sfe_uc_numinputs; i++)
		cout << smcsgx_sfe_uc_gen_input[i] << " ";
	cout << endl;

	cout << "SFE dijk gen input (edges) -\n";
	for (i = 0; i < SMC_SGX_SFE_DJ_INPUT_EDGES_NUM; i++)
		cout << smcsgx_sfe_dj_gen_input[i] << " ";
	cout << "\nSFE dijk gen input (connections) -\n";
	for ( ; i < SMC_SGX_SFE_DJ_INPUT_NUM; i++)
		cout << smcsgx_sfe_dj_gen_input[i] << " ";
	cout << endl;

	cout << "SFE oram gen input -\n";
	for (i = 0; i < sfe_or_queries; i++)
		cout << smcsgx_sfe_or_gen_input[i] << " ";
	cout << endl;
}

static void dump_yao_user_input(void)
{
	int i;

	cout << "YAO or gen input -\n";
	for (i = 0; i < yao_or_queries; i++)
		cout << smcsgx_yao_or_gen_input[i] << " ";
	cout << endl;

        cout << "YAO dij gen input -\n";
        for (i = 0; i < HYBRID_DJ_GEN_INPUT_NUM; i++)
                cout << smcsgx_yao_dj_gen_input[i] << " ";
        cout << endl;

	cout << "YAO dij evl input -\n";
	for (i = 0; i < HYBRID_DJ_EVL_INPUT_NUM; i++)
		cout << smcsgx_yao_dj_evl_input[i] << " ";
	cout << endl;

	cout << "YAO or split gen input -\n";
	for (i = 0; i < yao_or_split_queries; i++)
		cout << smcsgx_yao_or_split_gen_input[i] << " ";
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
			smcsgx_sfe_gen_input[i] = 0;
		smcsgx_sfe_gen_input[0] = 4;
		smcsgx_sfe_gen_input[1] = 2;

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
			smcsgx_sfe_gen_input[i] = strtol(head, NULL, 10);
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
		for (i = 0; i < sfe_uc_numinputs; i++)
			smcsgx_sfe_uc_gen_input[i] = 0;
		smcsgx_sfe_uc_gen_input[1] = 1;
		smcsgx_sfe_uc_gen_input[2] = 1;
		return 0;
	}

	return 0;
}

static int parse_input_sfe_dj(char *buf, int len)
{
	char *ptr;
	char *head;
	int i;

	if (trans_hardcode_input) {
		/*
		   for(i = 0; i < SMC_SGX_SFE_DJ_INPUT_EDGES_NUM; i++) 
		   sfe_dj_edges[i] = 0;
		   for (i = 0; i < SMC_SGX_SFE_DJ_INPUT_CONNS_NUM; i++)
		   sfe_dj_conns[i] = 0xFFFF;
		 */
		if (trans_debug)
			printf("trans-debug: edges/conns init done in %s\n",
					__func__);

		/* 
		   sfe_dj_edges[0] = 10;
		   sfe_dj_conns[0] = 1;
		   sfe_dj_edges[1] = 1;
		   sfe_dj_conns[1] = 2;
		   sfe_dj_edges[4] = 10;
		   sfe_dj_conns[4] = 0;
		   sfe_dj_edges[5] = 1;
		   sfe_dj_conns[5] = 2;
		   sfe_dj_edges[6] = 1;
		   sfe_dj_conns[6] = 3;
		   sfe_dj_edges[8] = 1;
		   sfe_dj_conns[8] = 0;
		   sfe_dj_edges[9] = 1;
		   sfe_dj_conns[9] = 1;
		   sfe_dj_edges[10] = 1;
		   sfe_dj_conns[10] = 4;
		   sfe_dj_edges[12] = 1;
		   sfe_dj_conns[12] = 1;
		   sfe_dj_edges[13] = 1;
		   sfe_dj_conns[13] = 4;
		   sfe_dj_edges[14] = 1;
		   sfe_dj_conns[14] = 5;
		   sfe_dj_edges[16] = 1;
		   sfe_dj_conns[16] = 2;
		   sfe_dj_edges[17] = 1;
		   sfe_dj_conns[17] = 3;
		   sfe_dj_edges[18] = 10;
		   sfe_dj_conns[18] = 5;
		   sfe_dj_edges[20] = 1;
		   sfe_dj_conns[20] = 3;
		   sfe_dj_edges[21] = 10;
		   sfe_dj_conns[21] = 4;
		 */

		//daveti: missing part?
		for(int i=0;i<sfe_dj_Nodes*sfe_dj_Edges_per_node;i++)
		{
			sfe_dj_edges[i] = 1;
			sfe_dj_conns[i] = 0xFFFF;
		}

		for(int i=0;i<sfe_dj_Nodes*sfe_dj_Edges_per_node;i+=4)
		{
			sfe_dj_conns[i] = (i+1)/4+1;
		}

		for(int i=4;i<sfe_dj_Nodes*sfe_dj_Edges_per_node-4;i+=4)
		{
			sfe_dj_conns[i+1] = (i+1)/4+1;
			sfe_dj_conns[i] = (i+1)/4-1;
		}

		//connections.at(0) = 1;
		sfe_dj_conns[sfe_dj_Nodes*sfe_dj_Edges_per_node-4] = sfe_dj_Nodes-2;

		memcpy(smcsgx_sfe_dj_gen_input, sfe_dj_edges,
				SMC_SGX_SFE_DJ_INPUT_EDGES_LEN);
		if (trans_debug)
			printf("trans-debug: first memcpy seems good in %s\n"
					"gen input len %d, edges len %d, conns len %d\n"
					"gen input %p, gen input next %p\n",
					__func__, sizeof(smcsgx_sfe_dj_gen_input),
					SMC_SGX_SFE_DJ_INPUT_EDGES_LEN,
					SMC_SGX_SFE_DJ_INPUT_CONNS_LEN,
					smcsgx_sfe_dj_gen_input,
					smcsgx_sfe_dj_gen_input+SMC_SGX_SFE_DJ_INPUT_EDGES_LEN);
		/* daveti: lines of the tricky bug above */
		memcpy((void *)smcsgx_sfe_dj_gen_input+SMC_SGX_SFE_DJ_INPUT_EDGES_LEN,
				sfe_dj_conns, SMC_SGX_SFE_DJ_INPUT_CONNS_LEN);
		if (trans_debug)
			printf("trans-debug: second memcpy seems good in %s\n",
					__func__);

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
	while ((ptr <= (buf+len)) && (i < SMC_SGX_SFE_DJ_INPUT_EDGES_NUM)) {
		if ((*ptr == ',') || (*ptr == '\n') || (*ptr == '\0')) {
			/* Get the value */
			*ptr = '\0';
			smcsgx_sfe_dj_gen_input[i] = strtol(head, NULL, 10);
			i++;
			ptr++;
			head = ptr;
		} else {
			ptr++;
		}
	}
	ptr++;	/* bypass the null terminator of the current line */
	if (trans_debug)
		printf("trans-debug: %s - first line is done [%s]\n",
				__func__, ptr);

	head = ptr;
	while ((ptr <= (buf+len)) && (i < SMC_SGX_SFE_DJ_INPUT_NUM)) {
		if ((*ptr == ',') || (*ptr == '\n') || (*ptr == '\0')) {
			/* Get the value */
			*ptr = '\0';
			smcsgx_sfe_dj_gen_input[i] = strtol(head, NULL, 10);
			i++;
			ptr++;
			head = ptr;
		} else {
			ptr++;
		}
	}

	return 0;
}

static int parse_input_sfe_or(char *buf, int len)
{
	int i;

	/*
	 * daveti: we probably should hardcode the input
	 * rather than playing with CSV file....
	 * FIXME
	 */

	/* Hardcode for UT */
	if (trans_hardcode_input) {
		for (i = 0; i < sfe_or_queries; i++)
			smcsgx_sfe_or_gen_input[i] = i;
		return 0;
	}

	return 0;
}




/*
 * Transition functions
 * Note: all trans handlers should update the current_state
 * excluding error state settings during the processing!
 * all trans handlers should return
 * SMC_SGX_RTN_SUCCESS or
 * SMC_SGX_RTN_FAILURE
 */

/*
 * Start a remote attestation
 * send the nonce to the evaluator
 * NOTE: the nonce may need to be generated from enclave
 * len: the length of the nonce
 * buf: NOT used!
 */
static int start_att_nonce(int len, char *buf)
{
	sgx_status_t rtn;
	char *nonce = NULL;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		return SMC_SGX_RTN_SUCCESS;
	}

	/* Create a buffer for the nonce */
	nonce = (char *)malloc(len);
	if (!nonce) {
		printf("trans-error: malloc failed in %s\n", __func__);
		return SMC_SGX_RTN_FAILURE;
	}

	/* Generate the nonce */
	if (trans_bypass_sgx) {
		printf("trans-warn: bypassing sgx - nonce fixed\n");
		memset(nonce, 0x7, len);
	} else {
		if (gen_nonce(mysgx_geteid(), &rtn, len, nonce) != SGX_SUCCESS) {
			printf("trans-error: ECall gen_nonce failed\n");
			free(nonce);
			return SMC_SGX_RTN_FAILURE;
		}
	}
	if (trans_debug)
		dump_byte_array((void *)nonce, len);

	/* Send out the nonce */
	if (build_send_smcsgx_msg(SMC_SGX_MSG_TAG_ATT_CHALLENGE, len, nonce, NULL)
			!= SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: build_send_smcsgx_msg failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	printf("trans-info: waiting S1 from evaluator...\n");

	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Handle the S1 from the evaluator 
 * Generates S2 and send it to the evaluator
 * Note: Ecall involved (maybe?)
 * Ref: sp_ra_proc_msg1_req()
 */
static int handle_att_S1(int len, char *buf)
{
	sample_ecc_state_handle_t ecc_state;
	sample_status_t sample_ret;
	uint8_t *sig_rl;
	uint32_t sig_rl_size;
	bool derive_ret;
	sample_ec256_public_t pub_key;
	sample_ec256_private_t priv_key;
	smcsgx_ec_dh_shared_t dh_key;
	smcsgx_ec_pub_t gb_ga[2];
	uint8_t mac[SMC_SGX_EC_MAC_SIZE];
	uint32_t cmac_size;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		current_state = SMC_SGX_STM_ATT_S2_SENT;
		return SMC_SGX_RTN_SUCCESS;
	}

	/* Defensive checking */
	if (len != sizeof(trans_S1)) {
		printf("trans-error: invalid S1 msg len %d\n", len);
		return SMC_SGX_RTN_FAILURE;
	}
	memcpy(&trans_S1, buf, len);
	if (trans_debug)
		dump_S1();

	/*
	 * SP needs to talk with the so called IAS to do a lot of things
	 * However, as a sender, we live by our own.
	 * Get rid of all these IAS parts and pretend we own FaceBook~
	 * Skip: ias_enroll, ias_get_sigrl
	 */
	memcpy(&g_spid, &g_sim_spid, sizeof(g_spid));
	g_is_sp_registered = true;
	sig_rl_size = 0;
	sig_rl = NULL;

	/* Save the client's public ECCDH key to local storage */
	memcpy(&g_sp_db.g_a, &trans_S1.g_a, sizeof(g_sp_db.g_a));

	// Generate the Service providers ECCDH key pair.
	sample_ret = sample_ecc256_open_context(&ecc_state);
	if (SAMPLE_SUCCESS != sample_ret) {
		printf("trans-error: cannot get ECC cotext in [%s]\n", __func__);
		return SMC_SGX_RTN_FAILURE;
	}
	sample_ret = sample_ecc256_create_key_pair(&priv_key, &pub_key, ecc_state);
	if (SAMPLE_SUCCESS != sample_ret) {
		printf("trans-error: cannot generate key pair in [%s]\n", __func__);
		goto ERROR_S1;
	}

	/* Save the SP ECCDH key pair to local storage */
	memcpy(&g_sp_db.b, &priv_key, sizeof(g_sp_db.b));
	memcpy(&g_sp_db.g_b, &pub_key, sizeof(g_sp_db.g_b));
	if (trans_debug) {
		printf("trans-debug: private key (b) -\n");
		dump_byte_array(&g_sp_db.b, sizeof(g_sp_db.b));
		printf("trans-debug: public key (g_b) -\n");
		dump_byte_array(&g_sp_db.g_b, sizeof(g_sp_db.g_b));
	}

	// Generate the client/SP shared secret
	sample_ret = sample_ecc256_compute_shared_dhkey(&priv_key,
			(sample_ec256_public_t *)&trans_S1.g_a,
			(sample_ec256_dh_shared_t *)&dh_key, ecc_state);
	if (SAMPLE_SUCCESS != sample_ret) {
		printf("trans-error: compute shared key failed in [%s]\n", __func__);
		goto ERROR_S1;
	}

	// smk is only needed for msg2 generation.
	derive_ret = derive_key(&dh_key, SMC_SGX_DERIVE_KEY_SMK,
			&g_sp_db.smk_key);
	if (derive_ret != true) {
		printf("trans-error: derive key for smk failed in [%s]\n", __func__);
		goto ERROR_S1;
	}

	// The rest of the keys are the shared secrets for future communication.
	derive_ret = derive_key(&dh_key, SMC_SGX_DERIVE_KEY_MK,
			&g_sp_db.mk_key);
	if (derive_ret != true) {
		printf("trans-error: derive key for mk failed in [%s]\n", __func__);
		goto ERROR_S1;
	}
	derive_ret = derive_key(&dh_key, SMC_SGX_DERIVE_KEY_SK,
			&g_sp_db.sk_key);
	if (derive_ret != true) {
		printf("trans-error: derive key for sk failed in [%s]\n", __func__);
		goto ERROR_S1;
	}
	derive_ret = derive_key(&dh_key, SMC_SGX_DERIVE_KEY_VK,
			&g_sp_db.vk_key);
	if (derive_ret != true) {
		printf("trans-error: derive key for vk failed in [%s]\n", __func__);
		goto ERROR_S1;
	}

	// Assemble S2 
	memcpy(&trans_S2.g_b, &g_sp_db.g_b, sizeof(g_sp_db.g_b));
	memcpy(&trans_S2.spid, &g_spid, sizeof(g_spid));

	// The service provider is responsible for selecting the proper EPID
	// signature type and to understand the implications of the choice!
	trans_S2.quote_type = SMC_SGX_QUOTE_LINKABLE_SIGNATURE;
	trans_S2.kdf_id = SMC_SGX_AES_CMAC_KDF_ID;

	// Create gb_ga
	memcpy(&gb_ga[0], &g_sp_db.g_b, sizeof(g_sp_db.g_b));
	memcpy(&gb_ga[1], &g_sp_db.g_a, sizeof(g_sp_db.g_a));

	// Sign gb_ga
	sample_ret = sample_ecdsa_sign((uint8_t *)&gb_ga, sizeof(gb_ga),
			(sample_ec256_private_t *)&g_sp_priv_key,
			(sample_ec256_signature_t *)&trans_S2.sign_gb_ga,
			ecc_state);
	if (SAMPLE_SUCCESS != sample_ret) {
		printf("trans-error: sign ga_gb failed in [%s]\n", __func__);
		goto ERROR_S1;
	}
	if (trans_debug) {
		printf("trans-debug: gb_ga -\n");
		dump_byte_array(&gb_ga, sizeof(gb_ga));
		printf("trans-debug: sign_gb_ga -\n");
		dump_byte_array(&trans_S2.sign_gb_ga, sizeof(trans_S2.sign_gb_ga));
	}

	// Generate the CMACsmk for gb||SPID||TYPE||KDF_ID||Sigsp(gb,ga)
	cmac_size = offsetof(smcsgx_ra_msg2_t, mac);
	sample_ret = sample_rijndael128_cmac_msg(&g_sp_db.smk_key,
			(uint8_t *)&trans_S2.g_b, cmac_size, &mac);
	if (SAMPLE_SUCCESS != sample_ret) {
		printf("trans-error: cmac failed in [%s]\n", __func__);
		goto ERROR_S1;
	}
	memcpy(&trans_S2.mac, mac, sizeof(mac));
	// daveti: we probably should get rid of the 2 lines below...
	// memcpy(&trans_S2.sig_rl[0], sig_rl_size);
	trans_S2.sig_rl_size = sig_rl_size;
	if (trans_debug)
		dump_S2();

	if (ecc_state)
		sample_ecc256_close_context(ecc_state);

	/* Send S2 */
	if (build_send_smcsgx_msg(SMC_SGX_MSG_TAG_ATT_S2, sizeof(trans_S2),
				(char *)&trans_S2, NULL) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: sending S2 failed\n");
		goto ERROR_S1;
	}
	printf("trans-info: sent S2 to evaluator\n"
			"trans-info: waiting S3 from evaluator...\n");

	current_state = SMC_SGX_STM_ATT_S2_SENT;
	return SMC_SGX_RTN_SUCCESS;

ERROR_S1:
	if (ecc_state)
		sample_ecc256_close_context(ecc_state);
	return SMC_SGX_RTN_FAILURE;
}

/*
 * NOTE: Intel's remote att code does not verify the quote at all!
 * Ideally, Intel's EPID would be used to verify the signature of the Quote,
 * which makes sure that the enclave is running on the SGX-enabled CPU.
 * Since EPID is not working on Linux, we skip the verification for this step.
 * (daveti: maybe we should get some measurement using Windows?)
 * For smcsgx, we need to verify:
 * 1. enclave measurement
 *	this should be reproducable from the sender since we assume
 *	the sender has the same copy of the evaluator's enclave (program).
 * 2. enclave signature
 *	we should know the public key of the evaluator, who signs the
 *	its enclave using the private key
 */
static int verify_quote_enclave(smcsgx_quote_t *quote)
{
	/* Verify the measurement */
	if (memcmp(quote->report_body.mr_enclave,
				smcsgx_eval_enclave_hash, SMC_SGX_SHA256_LEN)) {
		printf("trans-error: bad enclave measurement in %s\n"
				"received measurement from the quote: - ",
				__func__);
		dump_byte_array((void *)quote->report_body.mr_enclave,
				SMC_SGX_SHA256_LEN);
		printf("expected measurement from the sender: - ");
		dump_byte_array((void *)smcsgx_eval_enclave_hash,
				SMC_SGX_SHA256_LEN);
		return SMC_SGX_RTN_FAILURE;
	}
	printf("trans-info: enclave measurement verified\n");


	/* Verify the signature */
	/* daveti: TODO */

	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Process S3 from the evaluator
 * Ref: sp_ra_proc_msg3_req()
 */
static int verify_att_res(int len, char *buf, int *res)
{
	smcsgx_ra_msg3_t *p_msg3;
	sample_status_t sample_ret;
	smcsgx_quote_t *p_quote;
	sample_sha_state_handle_t sha_handle;
	smcsgx_report_data_t report_data;
	uint8_t *p_msg3_cmaced;
	uint32_t mac_size;
	sample_cmac_128bit_tag_t mac;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);

	if (len < sizeof(smcsgx_ra_msg3_t)) {
		printf("trans-error: invalid S3 len %d\n", len);
		return SMC_SGX_RTN_FAILURE;
	}

	// Save the msg
	if (trans_S3)
		free(trans_S3);
	trans_S3 = (smcsgx_ra_msg3_t *)malloc(len);
	if (!trans_S3) {
		printf("trans-error: malloc failed for trans_S3\n");
		return SMC_SGX_RTN_FAILURE;
	}
	trans_S3_len = len;
	memcpy((void *)trans_S3, (void *)buf, len);
	if (trans_debug)
		dump_S3();
	p_msg3 = trans_S3;

	// Compare g_a in message 3 with local g_a.
	if (memcmp(&g_sp_db.g_a, &p_msg3->g_a, sizeof(smcsgx_ec_pub_t))) {
		printf("trans-error: g_a is not the same\n");
		return SMC_SGX_RTN_FAILURE;
	}

	//Make sure that msg3_size is bigger than sample_mac_t.
	mac_size = len - sizeof(smcsgx_mac_t);
	p_msg3_cmaced = (uint8_t *)p_msg3;
	p_msg3_cmaced += sizeof(smcsgx_mac_t);

	// Verify the message mac using SMK
	sample_ret = sample_rijndael128_cmac_msg(&g_sp_db.smk_key,
			p_msg3_cmaced,
			mac_size,
			&mac);
	if (SAMPLE_SUCCESS != sample_ret) {
		printf("trans-error: cmac failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	if (memcmp(&p_msg3->mac, mac, sizeof(mac))) {
		printf("trans-error: verify cmac failed\n");
		return SMC_SGX_RTN_FAILURE;
	}

	// Save the sec prop
	memcpy(&g_sp_db.ps_sec_prop, &p_msg3->ps_sec_prop, sizeof(g_sp_db.ps_sec_prop));
	p_quote = (smcsgx_quote_t *)p_msg3->quote;

	// Verify the the report_data in the Quote matches the expected value.
	// The first 32 bytes of report_data are SHA256 HASH of {ga|gb|vk}.
	// The second 32 bytes of report_data are set to zero.
	memset(&report_data, 0x0, sizeof(report_data));
	sample_ret = sample_sha256_init(&sha_handle);
	if (sample_ret != SAMPLE_SUCCESS) {
		printf("trans-error: init hash failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	sample_ret = sample_sha256_update((uint8_t *)&(g_sp_db.g_a),
			sizeof(g_sp_db.g_a), sha_handle);
	if (sample_ret != SAMPLE_SUCCESS) {
		printf("trans-error: update hash 1 failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	sample_ret = sample_sha256_update((uint8_t *)&(g_sp_db.g_b),
			sizeof(g_sp_db.g_b), sha_handle);
	if (sample_ret != SAMPLE_SUCCESS) {
		printf("trans-error: update hash 2 failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	sample_ret = sample_sha256_update((uint8_t *)&(g_sp_db.vk_key),
			sizeof(g_sp_db.vk_key), sha_handle);
	if (sample_ret != SAMPLE_SUCCESS) {
		printf("trans-error: update hash 3 failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	sample_ret = sample_sha256_get_hash(sha_handle,
			(sample_sha256_hash_t *)&report_data);
	if (sample_ret != SAMPLE_SUCCESS) {
		printf("trans-error:  get hash failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	if (memcmp((uint8_t *)&report_data,
				(uint8_t *)&(p_quote->report_body.report_data),
				sizeof(report_data))) {
		printf("trans-error: verify hash failed\n");
		if (trans_debug) {
			printf("trans-debug: computed hash -\n");
			dump_byte_array(&report_data, sizeof(report_data));
			printf("trans-debug: sent hash -\n");
			dump_byte_array(&(p_quote->report_body.report_data),
					sizeof(report_data));
		}
		return SMC_SGX_RTN_FAILURE;
	}

	// Verify Enclave policy (IAS may provide an API for this if we
	// registered an Enclave policy)
	// Verify quote with IAS.
	// @IAS_Q: What is the proper JSON format for attestation evidence?
	// @TODO: Convert this call to a 'network' send/receive
	// once the IAS server is a vaialable.
	if (ias_verify_attestation_evidence(p_quote, NULL, &attestation_report)) {
		printf("trans-error: ias_verify_attestation_evidence failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	if (trans_debug) {
		dump_att_report();
		dump_enclave_report();
	}

	/* Verify the measurement of the enclave and the signature */
	if (verify_quote_enclave(p_quote) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: verify_quote_enclave failed\n");
		attestation_report.status =
			IAS_QUOTE_MEASUREMENT_OR_SIGNATURE_INVLAID;
	}

	/* Return the verification result */
	if ((attestation_report.status == IAS_QUOTE_OK) &&
			(attestation_report.pse_status == IAS_PSE_OK))
		*res = SMC_SGX_ATT_RES_GOOD;
	else
		*res = SMC_SGX_ATT_RES_BAD;

	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Construct the Att result and send it back
 * Ref: sp_ra_proc_msg3_req()
 */
static int send_att_res(int res)
{
	int ret;
	sample_status_t sample_ret;
	uint32_t att_result_msg_size;
	smcsgx_ra_att_result_msg_t *att_msg;

	if (trans_debug)
		printf("trans-debug: into %s, res %d\n", __func__, res);

	// Respond the client with the results of the attestation
	// daveti: ignore the policy report, whose size is set to 0 already
	att_result_msg_size = sizeof(smcsgx_ra_att_result_msg_t) + sizeof(g_secret);
	att_msg = (smcsgx_ra_att_result_msg_t *)malloc(att_result_msg_size);
	if (!att_msg) {
		printf("trans-error: malloc failed in %s\n", __func__);
		return SMC_SGX_RTN_FAILURE;
	}

	/* Init the att msg */
	att_msg->result = res;
	memcpy((void *)&att_msg->platform_info_blob,
			(void *)&attestation_report.info_blob,
			sizeof(ias_platform_info_blob_t));

	// Generate mac based on the mk key.
	sample_ret = sample_rijndael128_cmac_msg(&g_sp_db.mk_key,
			(const uint8_t*)&att_msg->platform_info_blob,
			sizeof(ias_platform_info_blob_t),
			&att_msg->mac);
	if (SAMPLE_SUCCESS != sample_ret) {
		printf("trans-error: cmac failed in [%s]\n", __func__);
		ret = SMC_SGX_RTN_FAILURE;
		goto END_ATT_RES;
	}

	// Generate shared secret and encrypt it with SK, if attestation passed.
	// daveti: the generation of shared secret probably should happen within
	// the enclave, where the shared secret will be saved as well...
	//att_msg->secret.payload_size = SMC_SGX_SECRET_LEN_NOW;
	att_msg->secret.payload_size = SMC_SGX_SHARED_SECRET_LEN;
	if (res == SMC_SGX_ATT_RES_GOOD) {
		sample_ret = sample_rijndael128GCM_encrypt(&g_sp_db.sk_key,
				&g_secret[0],
				att_msg->secret.payload_size,
				att_msg->secret.payload,
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				&att_msg->secret.payload_tag);
		if (SAMPLE_SUCCESS != sample_ret) {
			printf("trans-error: rijndae1128GCM encryption failed in [%s]\n", __func__);
			ret = SMC_SGX_RTN_FAILURE;
			goto END_ATT_RES;
		}
	}

	if (trans_debug) {
		dump_att_res(att_msg);
		dump_shared_secret(&att_msg->secret);
	}

	/* Build and send the att response */
	if (build_send_smcsgx_msg(SMC_SGX_MSG_TAG_ATT_RESULT, 
				att_result_msg_size, (char *)att_msg, NULL) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: build_send_smcsgx_msg failed in %s\n", __func__);
		ret = SMC_SGX_RTN_FAILURE;
		goto END_ATT_RES;
	}
	ret = SMC_SGX_RTN_SUCCESS;

END_ATT_RES:
	free(att_msg);
	return ret;
}

/*
 * Retrieve the input from the enclave and send it
 * NOTE: ECall involved
 */
static int send_input(void)
{
	sgx_status_t status;
	sgx_status_t ret;
	char *subtag;
	int len;
	int cnt;

	if (trans_debug)
		printf("trans-debug: into %s\n", __func__);

	/* Get the input from the enclave */
	if (trans_yao) {
		printf("trans-info: using YAO hybrid model\n");
		if (trans_yao_idx == 0) {
#ifdef SEND_GEN_INPUT
			if (gen_input_yao(mysgx_geteid(), &status, 0, NULL,
						SMC_SGX_YAO_INPUT_GC_GEN_LEN, smcsgx_yao_gengc,
						SMC_SGX_YAO_INPUT_GC_EVL_LEN, smcsgx_yao_evlgc,
						SMC_SGX_YAO_PERMUTES_LEN, smcsgx_yao_permutes) != SGX_SUCCESS) {
				printf("trans-error: gen_input_yao failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			if (trans_debug)
				dump_yao_gc();
#endif
			printf("trans-info: using YAO default testing case\n");
			/* Generate the Yao input */
			len = 4;
			strcpy(smcsgx_input, SMC_SGX_INPUT_MSG_YAO);
			subtag = SMC_SGX_INPUT_MSG_YAO;
		}
		else if (trans_yao_idx == 1) {
			printf("trans-info: using YAO DB/ORAM testing case\n");
			len = HYBRID_OR_QUERY_LEN+SMC_SGX_SP_TAG_SIZE;
			ret = gen_input_yao_or(mysgx_geteid(), &status, len, smcsgx_input);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value:");
				mysgx_perror(status, "status value:");
				printf("trans-error: gen_input_yao_or failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			subtag = SMC_SGX_INPUT_MSG_YAO_OR;
		}
		else if (trans_yao_idx == 2) {
			printf("trans-info: using YAO DJ testing case\n");
#ifdef BEN_IS_NICE_TO_DAVE
                        len = HYBRID_DJ_GEN_INPUT_LEN+SMC_SGX_SP_TAG_SIZE;
#else
			len = HYBRID_DJ_EVL_INPUT_LEN+SMC_SGX_SP_TAG_SIZE;
#endif
			ret = gen_input_yao_dj(mysgx_geteid(), &status, len, smcsgx_input);
                        if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
                                mysgx_perror(ret, "ret value:");
                                mysgx_perror(status, "status value:");
                                printf("trans-error: gen_input_yao_dj failed\n");
                                return SMC_SGX_RTN_FAILURE;
                        }
			subtag = SMC_SGX_INPUT_MSG_YAO_DJ;
		}
		else if (trans_yao_idx == 3) {
			printf("trans-info: using YAO DB/ORAM split testing case\n");
			len = HYBRID_OR_SPLIT_QUERY_LEN+SMC_SGX_SP_TAG_SIZE;
			ret = gen_input_yao_or_split(mysgx_geteid(), &status, len, smcsgx_input);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value:");
				mysgx_perror(status, "status value:");
				printf("trans-error: gen_input_yao_or_split failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			subtag = SMC_SGX_INPUT_MSG_YAO_OR_SPLIT;
		}
	} else {
		printf("trans-info: using SFE model\n");
		if (trans_sfe == 0) {
			printf("trans-info: using SFE MIL testing case\n");
			len = SMC_SGX_SFE_INPUT_LEN*sizeof(uint32_t)+SMC_SGX_SP_TAG_SIZE;
			ret = gen_input_sfe(mysgx_geteid(), &status, len, smcsgx_input);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value:");
				mysgx_perror(status, "status value:");
				printf("trans-error: gen_input_sfe failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			subtag = SMC_SGX_INPUT_MSG_SFE;
		}
		else if (trans_sfe == 1) {
			printf("trans-info: using SFE UC testing case\n");
			len = SMC_SGX_SFE_UC_INPUT_LEN+SMC_SGX_SP_TAG_SIZE;
			ret = gen_input_sfe_uc(mysgx_geteid(), &status, len, smcsgx_input);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value:");
				mysgx_perror(status, "status value:");
				printf("trans-error: gen_input_sfe_uc failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			subtag = SMC_SGX_INPUT_MSG_SFE_UC;
		}
		else if (trans_sfe == 2) {
			printf("trans-info: using SFE DJ testing case\n");
			len = SMC_SGX_SFE_DJ_INPUT_LEN+SMC_SGX_SP_TAG_SIZE;
			ret = gen_input_sfe_dj(mysgx_geteid(), &status, len, smcsgx_input);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value:");
				mysgx_perror(status, "status value:");
				printf("trans-error: gen_input_sfe_dj failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			subtag = SMC_SGX_INPUT_MSG_SFE_DJ;
		}
		else if (trans_sfe == 3) {
			printf("trans-info: using SFE ORAM testing case\n");
			len = SMC_SGX_SFE_OR_INPUT_GEN_LEN+SMC_SGX_SP_TAG_SIZE;
			ret = gen_input_sfe_or(mysgx_geteid(), &status, len, smcsgx_input);
			if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
				mysgx_perror(ret, "ret value:");
				mysgx_perror(status, "status value:");
				printf("trans-error: gen_input_sfe_or failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			subtag = SMC_SGX_INPUT_MSG_SFE_OR;
		}
		if (trans_debug)
			dump_byte_array((void *)smcsgx_input, len);
	}

	/* Build and send the input */
	if (build_send_smcsgx_msg(SMC_SGX_MSG_TAG_INPUT, len, smcsgx_input, subtag) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: sending input to evaluator failed\n");
		return SMC_SGX_RTN_FAILURE;
	}

	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Handle the S3 from the evaluator 
 * 1. check S3 to verify the att result
 * 2. send the att result
 * 3. send the input if att succeeded
 * NOTE: this function needs to take care of the state setting
 * to make sure a correct transition if everything is good!
 */
static int handle_att_S3(int len, char *buf)
{
	int res;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		current_state = SMC_SGX_STM_INPUT_SENT;
		return SMC_SGX_RTN_SUCCESS;
	}

	/* 1 */
	if (verify_att_res(len, buf, &res) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-error: verify_att_res failed\n");
		return SMC_SGX_RTN_FAILURE;
	}

	if (res == SMC_SGX_ATT_RES_GOOD)
		printf("trans-info: att verification succeeded\n");
	else
		printf("trans-warn: att verification failed\n");

	/* 2 */
	if (send_att_res(res) != SMC_SGX_RTN_SUCCESS) {
		printf("trans-err: sending att result failed\n");
		/* We can return early now */
		return SMC_SGX_RTN_FAILURE;
	}
	printf("trans-info: att result sent\n");
	current_state = SMC_SGX_STM_ATT_RES_SENT;

	/* 3 */
	if (send_input() != SMC_SGX_RTN_SUCCESS) {
		printf("trans-err: sending input failed\n");
		/* Return early */
		return SMC_SGX_RTN_FAILURE;
	}
	if (trans_yao)
		printf("trans-info: Yao to be started...\n"
				"trans-info: waiting Yao input from evaluator...\n");
	else
		printf("trans-info: input sent to sender...\n"
				"trans-info: waiting output from evaluator...\n");

	current_state = SMC_SGX_STM_INPUT_SENT;
	return SMC_SGX_RTN_SUCCESS;
}

/*
 * Handle the final result from the evaluator
 */
static int handle_result(int len, char *buf)
{
	sgx_status_t status;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);
	if (trans_ut) {
		current_state = SMC_SGX_STM_RES_RECVD;
		return SMC_SGX_RTN_SUCCESS;
	}

	if (!trans_yao) {
		/* Decode the SFE result */
		if (trans_sfe == 0) {
			if (len != SMC_SGX_SFE_OUTPUT_ENC_LEN+SMC_SGX_SP_TAG_SIZE) {
				printf("trans-error: invalid length for SFE output\n");
				return SMC_SGX_RTN_FAILURE;
			}
			if (decrypt_output_sfe(mysgx_geteid(), &status, len, buf,
						SMC_SGX_SFE_OUTPUT_LEN,
						(char *)&smcsgx_sfe_output) != SGX_SUCCESS) {
				printf("trans-error: decrypt_output_sfe failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			printf("trans-info: SFE Mil output [%d]\n", smcsgx_sfe_output);
		}
		else if (trans_sfe == 1) {
			if (len != SMC_SGX_SFE_UC_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) {
				printf("trans-error: invalid length for SFE output\n");
				return SMC_SGX_RTN_FAILURE;
			}
			if (decrypt_output_sfe_uc(mysgx_geteid(), &status, len, buf,
						SMC_SGX_SFE_UC_OUTPUT_LEN,
						(char *)&smcsgx_sfe_output_uc) != SGX_SUCCESS) {
				printf("trans-error: decrypt_output_sfe_uc failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			dump_sfe_output_uc();
		}
		else if (trans_sfe == 2) {
			if (len != SMC_SGX_SFE_DJ_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) {
				printf("trans-error: invalid length for SFE output\n");
				return SMC_SGX_RTN_FAILURE;
			}
			if (decrypt_output_sfe_dj(mysgx_geteid(), &status, len, buf,
						SMC_SGX_SFE_DJ_OUTPUT_LEN,
						(char *)&smcsgx_sfe_output_dj) != SGX_SUCCESS) {
				printf("trans-error: decrypt_output_sfe_dj failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			dump_sfe_output_dj();
		}
		else if (trans_sfe == 3) {
			if (len != SMC_SGX_SFE_OR_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) {
				printf("trans-error: invalid length for SFE output\n");
				return SMC_SGX_RTN_FAILURE;
			}
			if (decrypt_output_sfe_or(mysgx_geteid(), &status, len, buf,
						SMC_SGX_SFE_OR_OUTPUT_LEN,
						(char *)smcsgx_sfe_output_or) != SGX_SUCCESS) {
				printf("trans-error: decrypt_output_sfe_or failed\n");
				return SMC_SGX_RTN_FAILURE;
			}
			/* Defensive checking */
			if (status != SGX_SUCCESS)
				printf("trans-error: decrypt_output_sfe_or failed internally\n");
			dump_sfe_output_or();
		}
	}

	if (trans_perf) {
		gettimeofday(&end_tv, NULL);
		printf("trans-perf: smcsgx took [%lu] us\n",
				SMC_SGX_MBM_SUB_TV(start_tv, end_tv));
	}

	printf("trans-info: evaluation done\n");

	current_state = SMC_SGX_STM_RES_RECVD;
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

	printf("trans-error: STM needs to be reset\n");
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

/*
 * Handler for sender to receive the Yao input from the evaluator
 * and start Yao accordinly
 * NOTE: ECall involved to decrypt the input from the evaluator
 */
static int start_yao(int len, char *buf)
{
	int i, j;
	sgx_status_t ret;
	sgx_status_t status;
	Input programInput;
	Output out;

	if (trans_debug)
		printf("trans-debug: into %s, len %d, buf 0x%p\n",
				__func__, len, buf);

	if (trans_yao_idx == 0) {
		/* Default YAO testing case */
#ifdef SEND_RAW_GEN_INPUT
		/* Defensive checking */
		if (len != SMC_SGX_YAO_INPUT_GC_GEN_LEN+SMC_SGX_YAO_PERMUTES_LEN) {
			printf("trans-error: invalid length of Yao input\n");
			return SMC_SGX_RTN_FAILURE;
		}

		/* Save the gengc and premutes */
		memcpy(smcsgx_yao_gengc, buf, SMC_SGX_YAO_INPUT_GC_GEN_LEN);
		memcpy(smcsgx_yao_permutes, buf+SMC_SGX_YAO_INPUT_GC_GEN_LEN,
				SMC_SGX_YAO_PERMUTES_LEN);
#endif
		/* Defensive checking */
		if (len != SMC_SGX_YAO_INPUT_GEN_ENC_LEN) {
			printf("trans-error: invalid lenght of Yao input\n");
			return SMC_SGX_RTN_FAILURE;
		}
		if (trans_debug)
			dump_byte_array((void *)buf, len);

		/* Decrypt the gengc and premutes
		 * NOTE: Intel does not provide the decryption
		 * in the sample libcrypto. I guess I could not complain too much.
		 * There are 2 solutions:
		 * 1. write your own AES-128-GCM decryption in C/C++
		 * 2. do the decryption in the enclave, where the sgx_rijndael128GCM_decrypt
		 *	can be called to decrypt.
		 * And the winner is ... 2, of course!
		 * May 2, 2016
		 * daveti
		 */
#ifdef DAVETI_IS_INSANE
		ret = sgx_rijndael128GCM_decrypt(&g_secret,		// shared secret as the key
				(uint8_t *)buf,			// encrypted input
				SMC_SGX_YAO_INPUT_GEN_LEN,	// input length (excluding the mac)
				&smcsgx_yao_input[0],		// decrypted output
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(buf+SMC_SGX_YAO_INPUT_GEN_LEN));
		if (ret != SGX_SUCCESS) {
			printf("trans-error: Yao input decryption failed\n");
			return SMC_SGX_RTN_FAILURE;
		}
#endif
		ret = decrypt_input_yao(mysgx_geteid(), &status,
				SMC_SGX_YAO_INPUT_GEN_ENC_LEN, buf,
				SMC_SGX_YAO_INPUT_GEN_LEN, smcsgx_yao_input);
		if (ret != SGX_SUCCESS) {
			printf("trans-error: Yao input decryption failed\n");
			return SMC_SGX_RTN_FAILURE;
		}

		memcpy(smcsgx_yao_gengc, smcsgx_yao_input,
				SMC_SGX_YAO_INPUT_GC_GEN_LEN);
		memcpy(smcsgx_yao_permutes,
				smcsgx_yao_input+SMC_SGX_YAO_INPUT_GC_GEN_LEN,
				SMC_SGX_YAO_PERMUTES_LEN);
		if (trans_debug)
			dump_yao_gc();

		/* Start Yao */
		printf("trans-info: staring Yao default...\n");
		smcsgx_yao = startProgram();

		/* We do not really care about evlgc */
		programInput = rebuildInput(smcsgx_yao_gengc,
				smcsgx_yao_evlgc,
				smcsgx_yao_permutes);

		/* Ignore the evalInput */
		out = runProgram(SMC_SGX_YAO_PARAM_CIRCUIT_FILE,
				programInput.evalInput,
				programInput.genInput,
				smcsgx_yao);
	}
	else if (trans_yao_idx == 1) {
		/* Hybrid DB/ORAM case */
                /* Defensive checking */
                if (len != HYBRID_OR_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) {
                        printf("trans-error: invalid lenght of Yao input\n");
                        return SMC_SGX_RTN_FAILURE;
                }
                if (trans_debug)
                        dump_byte_array((void *)buf, len);

                ret = decrypt_output_yao_or(mysgx_geteid(), &status, len, buf,
                                HYBRID_OR_GEN_OUTPUT_LEN, smcsgx_yao_or_input);
                if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
			mysgx_perror(ret, "return");
			mysgx_perror(status, "status");
                        printf("trans-error: decrypt_output_yao_or failed\n");
                        return SMC_SGX_RTN_FAILURE;
                }

		memcpy(smcsgx_yao_or_output_sgx, smcsgx_yao_or_input,
			HYBRID_OR_SGX_OUTPUT_LEN);
                memcpy(smcsgx_yao_or_evlgc,
			smcsgx_yao_or_input+HYBRID_OR_SGX_OUTPUT_LEN,
			HYBRID_OR_EVLGC_LEN);
                memcpy(smcsgx_yao_or_permutes,
			smcsgx_yao_or_input+HYBRID_OR_SGX_OUTPUT_LEN+HYBRID_OR_EVLGC_LEN,
			HYBRID_OR_PERMUTES_LEN);

		if (trans_debug)
			dump_yao_or_all();
		dump_yao_or_output_sgx();

		/* Start Yao */
		printf("trans-info: starting Yao or ...\n");
		smcsgx_yao = startProgram();

		if (trans_debug)
			printf("trans-debug: startProgram done\n");

		/* We do not really care about gengc */
		programInput = rebuildInput2(smcsgx_yao_or_gengc,
				yao_or_secretsize*2,
				smcsgx_yao_or_evlgc,
				yao_or_secretsize,
				smcsgx_yao_or_permutes,
				yao_or_secretsize);

		if (trans_debug)
			printf("trans-debug: rebuildInput2 done\n");

		/* Ignore the evalInput */
		out = runProgram(HYBRID_OR_CIRCUIT_FILE,
				programInput.evalInput,
				programInput.genInput,
				smcsgx_yao);

		if (trans_debug)
			printf("trans-debug: runProgram done\n");
	}
	else if (trans_yao_idx == 2) {
		/* Hybrid DJ case */
                if (len != HYBRID_DJ_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) {
                        printf("trans-error: invalid lenght of Yao input\n");
                        return SMC_SGX_RTN_FAILURE;
                }
                if (trans_debug)
                        dump_byte_array((void *)buf, len);

                ret = decrypt_output_yao_dj(mysgx_geteid(), &status, len, buf,
                                HYBRID_DJ_GEN_OUTPUT_LEN, smcsgx_yao_dj_input);
                if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
			mysgx_perror(ret, "return");
			mysgx_perror(status, "status");
                        printf("trans-error: decrypt_output_yao_or failed\n");
                        return SMC_SGX_RTN_FAILURE;
                }

                memcpy(smcsgx_yao_dj_evlgc, smcsgx_yao_dj_input,
			HYBRID_DJ_EVLGC_LEN);
                memcpy(smcsgx_yao_dj_permutes,
			smcsgx_yao_dj_input+HYBRID_DJ_EVLGC_LEN,
			HYBRID_DJ_PERMUTES_LEN);

		if (trans_debug)
			dump_yao_dj_all();

		/* Start Yao */
		printf("trans-info: starting Yao dj...\n");
		smcsgx_yao = startProgram();

		if (trans_debug)
			printf("trans-debug: startProgram done\n");

		/* We do not really care about gengc */
		programInput = rebuildInput2(smcsgx_yao_dj_gengc,
				yao_dj_dijsgxinput*2,
				smcsgx_yao_dj_evlgc,
				yao_dj_dijsgxinput,
				smcsgx_yao_dj_permutes,
				yao_dj_dijsgxinput);

		if (trans_debug)
			printf("trans-debug: rebuildInput2 done\n");

		/* Ignore the evalInput */
		out = runProgram(HYBRID_DJ_CIRCUIT_FILE,
				programInput.evalInput,
				programInput.genInput,
				smcsgx_yao);

		if (trans_debug)
			printf("trans-debug: runProgram done\n");
	}
	else if (trans_yao_idx == 3) {
		/* Hybrid DB/ORAM split case */
                /* Defensive checking */
                if (len != HYBRID_OR_SPLIT_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) {
                        printf("trans-error: invalid lenght of Yao input\n");
                        return SMC_SGX_RTN_FAILURE;
                }
                if (trans_debug)
                        dump_byte_array((void *)buf, len);

                ret = decrypt_output_yao_or_split(mysgx_geteid(), &status, len, buf,
                                HYBRID_OR_SPLIT_GEN_OUTPUT_LEN, smcsgx_yao_or_split_input);
                if (ret != SGX_SUCCESS || status != SGX_SUCCESS) {
			mysgx_perror(ret, "return");
			mysgx_perror(status, "status");
                        printf("trans-error: decrypt_output_yao_or_split failed\n");
                        return SMC_SGX_RTN_FAILURE;
                }

		memcpy(smcsgx_yao_or_split_output_sgx, smcsgx_yao_or_split_input,
			HYBRID_OR_SPLIT_SGX_OUTPUT_LEN);
                memcpy(smcsgx_yao_or_split_evlgc,
			smcsgx_yao_or_split_input+HYBRID_OR_SPLIT_SGX_OUTPUT_LEN,
			HYBRID_OR_SPLIT_EVLGC_LEN);
                memcpy(smcsgx_yao_or_split_permutes,
			smcsgx_yao_or_split_input+HYBRID_OR_SPLIT_SGX_OUTPUT_LEN+HYBRID_OR_SPLIT_EVLGC_LEN,
			HYBRID_OR_SPLIT_PERMUTES_LEN);

		if (trans_debug)
			dump_yao_or_split_all();
		dump_yao_or_split_output_sgx();

		/* Start Yao */
		printf("trans-info: starting Yao or split...\n");
		smcsgx_yao = startProgram();

		if (trans_debug)
			printf("trans-debug: startProgram done\n");

		/* We do not really care about gengc */
		programInput = rebuildInput2(smcsgx_yao_or_split_gengc,
				yao_or_split_datasize*64*2,
				smcsgx_yao_or_split_evlgc,
				yao_or_split_datasize*64,
				smcsgx_yao_or_split_permutes,
				yao_or_split_datasize*64);

		if (trans_debug)
			printf("trans-debug: rebuildInput2 done\n");

		/* Ignore the evalInput */
		out = runProgram(HYBRID_OR_SPLIT_CIRCUIT_FILE,
				programInput.evalInput,
				programInput.genInput,
				smcsgx_yao);

		if (trans_debug)
			printf("trans-debug: runProgram done\n");
	}

	cout << "\nYAO output:\n";
	for (i = 0; i < out.output.size(); i++) {
		for (j = 0; j < SMC_SGX_YAO_MAGIC_J; j++)
			printf("%hhu ",out.output[i][j]);
		cout << "\n";
	}
	printf("trans-info: evaluation done\n");

	/* NOTE: we do not change the state here! */
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
		SMC_SGX_STM_ATT_STARTED, start_att_nonce},
	{SMC_SGX_STM_ATT_STARTED, SMC_SGX_MSG_TAG_ATT_S1,
		SMC_SGX_STM_ATT_S1_RECVD, handle_att_S1},
	{SMC_SGX_STM_ATT_S2_SENT, SMC_SGX_MSG_TAG_ATT_S3,
		SMC_SGX_STM_ATT_S3_RECVD, handle_att_S3},
	/* handle_att_S3 should set the correct state for transition */
	{SMC_SGX_STM_INPUT_SENT, SMC_SGX_MSG_TAG_RESULT,
		SMC_SGX_STM_RES_RECVD, handle_result},
	/* start Yao locally once got the input from the evaluator */
	{SMC_SGX_STM_INPUT_SENT, SMC_SGX_MSG_TAG_YAO_INPUT,
		SMC_SGX_STM_INPUT_SENT, start_yao},
	/* Unless trans_start_att is called again - we are done */
	{SMC_SGX_STM_RES_RECVD, SMC_SGX_MSG_TAG_STUB,
		SMC_SGX_STM_INIT, handle_stub},
	/* errors */
	/* Note: we just keep down when error happens */
	{SMC_SGX_STM_ERROR, SMC_SGX_MSG_TAG_ANY,
		SMC_SGX_STM_ERROR, handle_err},
	/* Padding */
	trans_stub,
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
	vector<unsigned int> alicein;
	unsigned int *input_array;

	if (trans_debug)
		printf("trans-debug: into %s with file %s\n",
				__func__, path);

	/* Handle the hybrid model */
	if (trans_yao) {
		switch (trans_yao_idx) {
			case 0:
				break;
			case 1:
				alicein.resize(yao_or_queries);
				fileToVector(HYBRID_OR_GEN_INPUT_FILE, alicein, alicein.size());
				input_array = &alicein[0];
				memcpy(smcsgx_yao_or_gen_input, input_array, HYBRID_OR_QUERY_LEN);
				if (put_input_yao_or(mysgx_geteid(), &status,
							HYBRID_OR_QUERY_LEN,
							(char *)smcsgx_yao_or_gen_input) != SGX_SUCCESS)
					printf("trans-error: put_input_yao_or failed in %s\n", __func__);
				break;
			case 3:
				alicein.resize(yao_or_split_queries);
				fileToVector(HYBRID_OR_SPLIT_GEN_INPUT_FILE, alicein, alicein.size());
				input_array = &alicein[0];
				memcpy(smcsgx_yao_or_split_gen_input, input_array, HYBRID_OR_SPLIT_QUERY_LEN);
				if (put_input_yao_or_split(mysgx_geteid(), &status,
							HYBRID_OR_SPLIT_QUERY_LEN,
							(char *)smcsgx_yao_or_split_gen_input) != SGX_SUCCESS)
					printf("trans-error: put_input_yao_or_split failed in %s\n", __func__);
				break;
			case 2:
#ifdef BEN_IS_NICE_TO_DAVE
                                alicein.resize(HYBRID_DJ_GEN_INPUT_NUM);
                                fileToVector(HYBRID_DJ_GEN_INPUT_FILE, alicein, alicein.size());
                                input_array = &alicein[0];
                                memcpy(smcsgx_yao_dj_gen_input, input_array,
					HYBRID_DJ_GEN_INPUT_LEN);
                                if (put_input_yao_dj(mysgx_geteid(), &status,
                                                        HYBRID_DJ_GEN_INPUT_LEN,
                                                        (char *)smcsgx_yao_dj_gen_input) != SGX_SUCCESS)
                                        printf("trans-error: put_input_yao_dj failed in %s\n", __func__);
				break;
#else
				/* Reuse alicein for bobin */
				printf("trans-info: new hybrid model for dijkstra\n");
                                alicein.resize(HYBRID_DJ_EVL_INPUT_NUM);
                                fileToVector(HYBRID_DJ_EVL_INPUT_FILE, alicein, alicein.size());
                                input_array = &alicein[0];
                                memcpy(smcsgx_yao_dj_evl_input, input_array, HYBRID_DJ_EVL_INPUT_LEN);
                                if (put_input_yao_dj2(mysgx_geteid(), &status,
                                                        HYBRID_DJ_EVL_INPUT_LEN,
                                                        (char *)smcsgx_yao_dj_evl_input) != SGX_SUCCESS)
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
	if (trans_debug)
		printf("trans-debug: smcsgx_read_input done\n");

	/* Parse different inputs accordingly */
	if (!strncasecmp(trans_buff, SMC_SGX_INPUT_MSG_SFE,
				strlen(SMC_SGX_INPUT_MSG_SFE))) {
		if (trans_debug)
			printf("trans-debug: processing input for SFE mil\n");
		if (parse_input_sfe(trans_buff, len)) {
			printf("trans-error: parse_input_sfe failed in %s\n", __func__);
			return;
		}
		if (put_input_sfe(mysgx_geteid(), &status,
					SMC_SGX_SFE_INPUT_LEN*sizeof(uint32_t),
					(char *)smcsgx_sfe_gen_input) != SGX_SUCCESS) {
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
		if (put_input_sfe_uc(mysgx_geteid(), &status,
					SMC_SGX_SFE_UC_INPUT_LEN,
					(char *)smcsgx_sfe_uc_gen_input) != SGX_SUCCESS) {
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
					SMC_SGX_SFE_DJ_INPUT_LEN,
					(char *)smcsgx_sfe_dj_gen_input) != SGX_SUCCESS) {
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
					SMC_SGX_SFE_OR_INPUT_GEN_LEN,
					(char *)smcsgx_sfe_or_gen_input) != SGX_SUCCESS) {
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

void trans_init(int fd, int debug, int ut, int yao, int yao_idx, int sfe)
{
	int rtn;

	/* Init global vars */
	remote_fd = fd;
	trans_debug = debug;
	trans_ut = ut;
	trans_yao = yao;
	trans_yao_idx = yao_idx;
	trans_sfe = sfe;
	current_state = SMC_SGX_STM_INIT;

	/* Init the STM entry table */
	init_stm_tbl();

	/* Get the trusted measurement of the eval enclave */
	rtn = get_enclave_hash(SMC_SGX_EVAL_ENCLAVE_HASH_FILE,
			smcsgx_eval_enclave_hash);
	if (rtn)
		printf("trans-error: sha256_enclave failed with rtn %d\n", rtn);
	if (trans_debug)
		dump_byte_array((void *)smcsgx_eval_enclave_hash,
				SMC_SGX_SHA256_LEN);
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
	if (current_state == SMC_SGX_STM_RES_RECVD) {
		current_state = stm_tbl[current_state][SMC_SGX_MSG_TAG_STUB].new_state;
		trans_done = 1;
	}

	if (trans_debug)
		printf("trans-debug: transition done with old|next|current states: %s|%s|%s\n",
				smcsgx_stm_name(current), smcsgx_stm_name(entry->new_state), smcsgx_stm_name(current_state));
}

/* A wrapper of trans_main */
void trans_start_att(void)
{
	smcsgx_msghdr msg;

	if (trans_ut) {
		current_state = SMC_SGX_STM_ATT_STARTED;
		return;
	}

	if (trans_perf)
		gettimeofday(&start_tv, NULL);

	msg.tag = SMC_SGX_MSG_TAG_ATT_CHALLENGE;
	msg.len = SMC_SGX_NONCE_LEN;

	trans_main(&msg);
}
