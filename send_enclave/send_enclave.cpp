/*
 * send_enclave.cpp
 * Enclave implementations for the sender
 * Ref: /opt/intel/sgxsdk/SampleCode/RemoteAttestation/service_provider/service_provider.cpp
 * Apr 16, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include <stdlib.h>
#include "string.h"
#include "sgx_status.h"
#include "sgx_error.h"
#include "sgx_trts.h"
#include "sgx_tcrypto.h"
#include "send_enclave_t.h"
#include "mysfe.h"
#include "../include/smcsgxyaoparam.h"


/* Global vars */
static int nonce_len;
static char *nonce;
static int verification_res;
static int user_input;
static const char input[SEND_ENCLAVE_INPUT_LEN] = {0};
static const char input_yao[SEND_YAO_INPUT_BIN_LEN] = {1,1,0,0,0,0,0,0  ,1,0,1,0,0,0,0,0};
static char gengc[SEND_YAO_INPUT_GC_SEND_LEN][SEND_YAO_MAGIC_J];
static char evlgc[SEND_YAO_INPUT_GC_EVAL_LEN][SEND_YAO_MAGIC_J];
static char permutes[SEND_YAO_PERMUTES_LEN];
static uint8_t sendin[SEND_SFE_INPUT_LEN];
static uint8_t sendin_encrypted[SEND_SFE_INPUT_LEN];
static uint8_t sendin_mac[SMC_SGX_SP_TAG_SIZE];
static uint32_t sendin2[SEND_SFE_INPUT_LEN];
static uint32_t sendin2_encrypted[SEND_SFE_INPUT_LEN];
static char evalin[SEND_SFE_INPUT_LEN];
static uint8_t sfe_output_pad[SEND_SFE_OUTPUT_ENC_LEN];
static uint8_t g_secret[SMC_SGX_SHARED_SECRET_LEN] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
static const uint8_t aes_gcm_iv[SMC_SGX_SP_IV_SIZE] = {0};
static uint8_t yao_input[SEND_YAO_INPUT_GEN_LEN];
static bool sfe_uc_input[sfe_uc_numinputs];
static uint8_t sfe_uc_input_encrypted[SEND_SFE_UC_INPUT_LEN];
static uint8_t sfe_uc_output[SEND_SFE_UC_OUTPUT_LEN];
static int sfe_dj_edges[SEND_SFE_DJ_INPUT_EDGES_NUM];
static int sfe_dj_conns[SEND_SFE_DJ_INPUT_CONNS_NUM];
static uint8_t sfe_dj_input[SEND_SFE_DJ_INPUT_LEN];
static uint8_t sfe_dj_input_encrypted[SEND_SFE_DJ_INPUT_LEN];
static uint8_t sfe_dj_output[SEND_SFE_DJ_OUTPUT_LEN];
static uint8_t sfe_or_input[SEND_SFE_OR_INPUT_LEN];
static uint8_t sfe_or_input_encrypted[SEND_SFE_OR_INPUT_LEN];
static uint8_t sfe_or_output[SEND_SFE_OR_OUTPUT_LEN];
static uint8_t yao_or_input[HYBRID_OR_QUERY_LEN];
static uint8_t yao_or_input_encrypted[HYBRID_OR_QUERY_LEN+SMC_SGX_SP_TAG_SIZE];
static uint8_t yao_or_output[HYBRID_OR_GEN_OUTPUT_LEN];
static uint8_t yao_dj_input[HYBRID_DJ_GEN_INPUT_LEN];
static uint8_t yao_dj_input_evl[HYBRID_DJ_EVL_INPUT_LEN];
static uint8_t yao_dj_input_encrypted[HYBRID_DJ_GEN_INPUT_LEN+SMC_SGX_SP_TAG_SIZE];
static uint8_t yao_dj_input_evl_encrypted[HYBRID_DJ_EVL_INPUT_LEN+SMC_SGX_SP_TAG_SIZE];
static uint8_t yao_dj_output[HYBRID_DJ_GEN_OUTPUT_LEN];
static uint8_t yao_or_split_input[HYBRID_OR_SPLIT_QUERY_LEN];
static uint8_t yao_or_split_input_encrypted[HYBRID_OR_SPLIT_QUERY_LEN+SMC_SGX_SP_TAG_SIZE];
static uint8_t yao_or_split_output[HYBRID_OR_SPLIT_GEN_OUTPUT_LEN];


/* ECall */
sgx_status_t gen_nonce(int len, char *buf)
{
	/* Alloc mem */
	nonce = (char *)malloc(len);
	if (!nonce) {
		nonce_len = -1;
		return SGX_ERROR_OUT_OF_MEMORY;
	}

	/* Call the HW TRNG */
	if (sgx_read_rand((unsigned char *)nonce, len) != SGX_SUCCESS) {
		nonce_len = -1;
		return SGX_ERROR_UNEXPECTED;
	}
	nonce_len = len;

	/* Copy out the rand */
	memcpy(buf, nonce, len);

	return SGX_SUCCESS;
}

/* Process S1 to generate S2 */
sgx_status_t gen_S2(int s1_len, char *s1, int s2_len, char *s2)
{
	return SGX_SUCCESS;
}

/* Process S3 to generate the att result */
sgx_status_t verify_att(int s3_len, char *s3, int *res)
{
	return SGX_SUCCESS;
}

/* Get the SFE input for the other party */
/* NOTE: this function is replaced by gen_input_sfe */
sgx_status_t gen_input(int len, char *buf)
{
	int i;
	sgx_status_t ret;

	/* daveti: we have to know the length */
	if (len != SEND_SFE_INPUT_LEN+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;

	if (!user_input) {
		/* Generate the input */
		for (i = 0; i < SEND_SFE_INPUT_LEN; i++)
			sendin[i] = 0;
		sendin[0] = 4;
		sendin[1] = 2;
	}

	/* We are Alice and we hate Bob!
	 * because Love hurts without smcsgx~
	 * May 3, 2016
	 * daveti
	 */

	/* Do the encryption before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			&sendin[0],			// src to be encrypted
			SEND_SFE_INPUT_LEN,		// src len
			sendin_encrypted,		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(sendin_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	memcpy(buf, sendin_encrypted, SEND_SFE_INPUT_LEN);
	memcpy(buf+SEND_SFE_INPUT_LEN, sendin_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

/* Get the SFE output */
sgx_status_t gen_output(int len, char *input, int out_len, char *output)
{
	return SGX_SUCCESS;
}

sgx_status_t decode_res(int res_len, char *res, int fin_len, char *fin)
{
	return SGX_SUCCESS;
}

/* Get the YAO input for the other party */
sgx_status_t gen_input_yao(int len, char *input,
			int gen_len, char *gen,
			int evl_len, char *evl,
			int pem_len, char *pem)
{
	int i, j;
	sgx_status_t ret;

	/* Ignore the len and input for now
	 * since the input is saved in the enclave
	 */

	/* Defensive checking */
	if ((gen_len != SEND_YAO_INPUT_GC_SEND_LEN*SEND_YAO_MAGIC_J) ||
		(evl_len != SEND_YAO_INPUT_GC_EVAL_LEN*SEND_YAO_MAGIC_J) ||
		(pem_len != SEND_YAO_PERMUTES_LEN))
		return SGX_ERROR_INVALID_PARAMETER;

	/* transformToGarbled */		
	for (i = 0; i < SEND_YAO_INPUT_GC_SEND_LEN; i += 2) {
		for (j = 0; j < SEND_YAO_MAGIC_J; j++) {
			ret = sgx_read_rand((unsigned char *)&gengc[i][j], 1);
			if (ret != SGX_SUCCESS)
				return ret;
			ret = sgx_read_rand((unsigned char *)&gengc[i+1][j], 1);
			if (ret != SGX_SUCCESS)
				return ret;
		}

		if (((gengc[i][0])&0x01) == ((gengc[i+1][0])&0x01))
			(gengc[i][0]) ^= 0x01;

		if (((gengc[i][0])&0x01) == 0)
			permutes[i/2] = 0;
		else
			permutes[i/2] = 1;

		for (j = 0; j < SEND_YAO_MAGIC_J; j++)
			evlgc[i/2][j] = input_yao[i/2] == 0 ? gengc[i][j] : gengc[i+1][j]; 
	}

	/* Copy outside */
	memcpy(gen, gengc, gen_len);
	memcpy(evl, evlgc, evl_len);
	memcpy(pem, permutes, pem_len);

	return SGX_SUCCESS;
}

/* Generate the SFE input for the evaluator */
sgx_status_t gen_input_sfe(int len, char *buf)
{
	int i;
	sgx_status_t ret;

	/* daveti: we have to know the length */
	if (len != SEND_SFE_INPUT_LEN*sizeof(uint32_t)+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;

	/* Generate the input */
	if (!user_input) {
		for (i = 0; i < SEND_SFE_INPUT_LEN; i++)
			sendin2[i] = 0;
		sendin2[0] = 4;
		sendin2[1] = 2;
	}

	/* We are Alice and we hate Bob!
	 * because Love hurts without smcsgx~
	 * May 3, 2016
	 * daveti
	 */

	/* Do the encryption before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			(uint8_t *)sendin2,		// src to be encrypted
			(SEND_SFE_INPUT_LEN*sizeof(uint32_t)),// src len
			(uint8_t *)sendin2_encrypted,	// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(sendin_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	memcpy(buf, sendin2_encrypted,
		SEND_SFE_INPUT_LEN*sizeof(uint32_t));
	memcpy(buf+SEND_SFE_INPUT_LEN*sizeof(uint32_t),
		sendin_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

static void init_input_sfe_uc(void)
{
	/* Change the input here */
	sfe_uc_input[0] = 0;
	sfe_uc_input[1] = 1;
	sfe_uc_input[2] = 1;
	sfe_uc_input[3] = 0;
}

/* Generate the SFE UC input for the evaluator */
sgx_status_t gen_input_sfe_uc(int len, char *buf)
{
	int i;
	sgx_status_t ret;

	/* daveti: we have to know the length */
	if (len != SEND_SFE_UC_INPUT_LEN+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;

	/* Generate the input */
	if (!user_input)
		init_input_sfe_uc();

	/* Do the encryption before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			(uint8_t *)sfe_uc_input,		// src to be encrypted
			(SEND_SFE_UC_INPUT_LEN),// src len
			(uint8_t *)sfe_uc_input_encrypted,	// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(sendin_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	memcpy(buf, sfe_uc_input_encrypted, SEND_SFE_UC_INPUT_LEN);
	memcpy(buf+SEND_SFE_UC_INPUT_LEN,
		sendin_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

static void init_input_sfe_dj(void)
{
	int i;

	for(i = 0; i < sfe_dj_Nodes*sfe_dj_Edges_per_node; i++) {
		sfe_dj_edges[i] = 0;
		sfe_dj_conns[i] = 0xFFFF;
	}
    
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
}

/* Generate the SFE Dijk input for the evaluator */
sgx_status_t gen_input_sfe_dj(int len, char *buf)
{
	int i;
	sgx_status_t ret;

	/* daveti: we have to know the length */
	if (len != SEND_SFE_DJ_INPUT_LEN+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;

	/* Generate the input */
	if (!user_input)
		init_input_sfe_dj();
	memcpy(sfe_dj_input, sfe_dj_edges, SEND_SFE_DJ_INPUT_EDGES_LEN);
	memcpy(sfe_dj_input+SEND_SFE_DJ_INPUT_EDGES_LEN, sfe_dj_conns,
		SEND_SFE_DJ_INPUT_CONNS_LEN);

	/* Do the encryption before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			sfe_dj_input,		// src to be encrypted
			(SEND_SFE_DJ_INPUT_LEN),// src len
			sfe_dj_input_encrypted,	// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(sendin_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	memcpy(buf, sfe_dj_input_encrypted, SEND_SFE_DJ_INPUT_LEN);
	memcpy(buf+SEND_SFE_DJ_INPUT_LEN,
		sendin_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

static void init_input_sfe_or(void)
{
	/* FIXME: do nothing now */
}

/* Generate the SFE ORAM input for the evaluator */
sgx_status_t gen_input_sfe_or(int len, char *buf)
{
	int i;
	sgx_status_t ret;

	/* daveti: we have to know the length */
	if (len != SEND_SFE_OR_INPUT_LEN+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;

	/* Generate the input */
	if (!user_input)
		init_input_sfe_or();

	/* Do the encryption before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			sfe_or_input,		// src to be encrypted
			(SEND_SFE_OR_INPUT_LEN),// src len
			sfe_or_input_encrypted,	// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(sendin_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	memcpy(buf, sfe_or_input_encrypted, SEND_SFE_OR_INPUT_LEN);
	memcpy(buf+SEND_SFE_OR_INPUT_LEN,
		sendin_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

static void init_input_yao_or(void)
{
	/* FIXME (if needed) */
}

/* Generate the YAO ORAM input for the evaluator */
sgx_status_t gen_input_yao_or(int len, char *buf)
{
	int i;
	sgx_status_t ret;

	/* daveti: we have to know the length */
	if (len != HYBRID_OR_QUERY_LEN+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;

	/* Generate the input */
	if (!user_input)
		init_input_yao_or();

	/* Do the encryption before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			yao_or_input,		// src to be encrypted
			(HYBRID_OR_QUERY_LEN),// src len
			yao_or_input_encrypted,	// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(sendin_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	memcpy(buf, yao_or_input_encrypted, HYBRID_OR_QUERY_LEN);
	memcpy(buf+HYBRID_OR_QUERY_LEN,
		sendin_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

/* Generate the YAO ORAM query split input for the evaluator */
sgx_status_t gen_input_yao_or_split(int len, char *buf)
{
	int i;
	sgx_status_t ret;

	/* daveti: we have to know the length */
	if (len != HYBRID_OR_SPLIT_QUERY_LEN+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;

	/* Generate the input */
	if (!user_input)
		init_input_yao_or();

	/* Do the encryption before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			yao_or_split_input,		// src to be encrypted
			(HYBRID_OR_SPLIT_QUERY_LEN),// src len
			yao_or_split_input_encrypted,	// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(sendin_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	memcpy(buf, yao_or_split_input_encrypted, HYBRID_OR_SPLIT_QUERY_LEN);
	memcpy(buf+HYBRID_OR_SPLIT_QUERY_LEN,
		sendin_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

static void init_input_yao_dj(void)
{
	/* FIXME (if needed) */
}

/* Generate the YAO dijkstra input for the evaluator */
sgx_status_t gen_input_yao_dj(int len, char *buf)
{
	int i;
	sgx_status_t ret;

#ifdef BEN_IS_NICE_TO_DAVE
	/* daveti: we have to know the length */
	if (len != HYBRID_DJ_GEN_INPUT_LEN+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;
#else
	if (len != HYBRID_DJ_EVL_INPUT_LEN+SMC_SGX_SP_TAG_SIZE)
		return SGX_ERROR_INVALID_PARAMETER;
#endif

	/* Generate the input */
	if (!user_input)
		init_input_yao_dj();

#ifdef BEN_IS_NICE_TO_DAVE
	/* Do the encryption before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			yao_dj_input,		// src to be encrypted
			(HYBRID_DJ_GEN_INPUT_LEN),// src len
			yao_dj_input_encrypted,	// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(sendin_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	memcpy(buf, yao_dj_input_encrypted, HYBRID_DJ_GEN_INPUT_LEN);
	memcpy(buf+HYBRID_DJ_GEN_INPUT_LEN,
		sendin_mac, SMC_SGX_SP_TAG_SIZE);
#else
        ret = sgx_rijndael128GCM_encrypt(
                        (const sgx_aes_gcm_128bit_key_t *)
                        (&g_secret[0]),                 // use shared secret as the key
                        yao_dj_input_evl,           // src to be encrypted
                        (HYBRID_DJ_EVL_INPUT_LEN),// src len
                        yao_dj_input_evl_encrypted, // dst buf
                        &aes_gcm_iv[0],                 // IV
                        SMC_SGX_SP_IV_SIZE,             // IV size (12)
                        NULL,
                        0,
                        (sgx_aes_gcm_128bit_tag_t *)
                        (sendin_mac));          // output mac
        if (ret != SGX_SUCCESS)
                return ret;

        memcpy(buf, yao_dj_input_evl_encrypted, HYBRID_DJ_EVL_INPUT_LEN);
        memcpy(buf+HYBRID_DJ_EVL_INPUT_LEN,
                sendin_mac, SMC_SGX_SP_TAG_SIZE);
#endif

	return SGX_SUCCESS;
}





/* Decrypt the Yao input from the evaluator */
sgx_status_t decrypt_input_yao(int len, char *input, int out_len, char *output)
{
	sgx_status_t ret;

	/* Defensive checking */
	if ((len != SEND_YAO_INPUT_GEN_ENC_LEN) ||
		(out_len != SEND_YAO_INPUT_GEN_LEN))
		return SGX_ERROR_INVALID_PARAMETER;

	/* Decrypt */
	ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
				(&g_secret[0]),			// shared secret as the key
				(uint8_t *)input,		// encrypted input
				SEND_YAO_INPUT_GEN_LEN,		// input length (excluding the mac)
				&yao_input[0],			// decrypted output
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(input+SEND_YAO_INPUT_GEN_LEN));
        if (ret != SGX_SUCCESS)
                return ret;

	/* Copy outside */
	memcpy(output, yao_input, out_len);

	return SGX_SUCCESS;
}

/* Decrypt the SFE output from the evaluator */
sgx_status_t decrypt_output_sfe(int len, char *input, int out_len, char *output)
{
	sgx_status_t ret;

	/* Defensive checking */
	if ((len != SEND_SFE_OUTPUT_ENC_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != SEND_SFE_OUTPUT_LEN))
		return SGX_ERROR_INVALID_PARAMETER;

        /* Decrypt */
	ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
				(&g_secret[0]),			// shared secret as the key
				(uint8_t *)input,		// encrypted input
				SEND_SFE_OUTPUT_ENC_LEN,	// input length (excluding the mac)
				&sfe_output_pad[0],		// decrypted output
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(input+SEND_SFE_OUTPUT_ENC_LEN));
	if (ret != SGX_SUCCESS)
		return ret;

        /* Copy outside */
        memcpy(output, sfe_output_pad, out_len);

	return SGX_SUCCESS;
}

/* Decrypt the SFE UC output from the evaluator */
sgx_status_t decrypt_output_sfe_uc(int len, char *input, int out_len, char *output)
{
	sgx_status_t ret;

	/* Defensive checking */
	if ((len != SEND_SFE_UC_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != SEND_SFE_UC_OUTPUT_LEN))
		return SGX_ERROR_INVALID_PARAMETER;

        /* Decrypt */
	ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
				(&g_secret[0]),			// shared secret as the key
				(uint8_t *)input,		// encrypted input
				SEND_SFE_UC_OUTPUT_LEN,		// input length (excluding the mac)
				&sfe_uc_output[0],		// decrypted output
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(input+SEND_SFE_UC_OUTPUT_LEN));
	if (ret != SGX_SUCCESS)
		return ret;

        /* Copy outside */
        memcpy(output, sfe_uc_output, out_len);

	return SGX_SUCCESS;
}

/* Decrypt the SFE Dijk output from the evaluator */
sgx_status_t decrypt_output_sfe_dj(int len, char *input, int out_len, char *output)
{
	sgx_status_t ret;

	/* Defensive checking */
	if ((len != SEND_SFE_DJ_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != SEND_SFE_DJ_OUTPUT_LEN))
		return SGX_ERROR_INVALID_PARAMETER;

        /* Decrypt */
	ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
				(&g_secret[0]),			// shared secret as the key
				(uint8_t *)input,		// encrypted input
				SEND_SFE_DJ_OUTPUT_LEN,		// input length (excluding the mac)
				&sfe_dj_output[0],		// decrypted output
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(input+SEND_SFE_DJ_OUTPUT_LEN));
	if (ret != SGX_SUCCESS)
		return ret;

        /* Copy outside */
        memcpy(output, sfe_dj_output, out_len);

	return SGX_SUCCESS;
}

/* Decrypt the SFE ORAM output from the evaluator */
sgx_status_t decrypt_output_sfe_or(int len, char *input, int out_len, char *output)
{
        sgx_status_t ret;

        /* Defensive checking */
        if ((len != SEND_SFE_OR_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
                (out_len != SEND_SFE_OR_OUTPUT_LEN))
                return SGX_ERROR_INVALID_PARAMETER;

        /* Decrypt */
        ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
                                (&g_secret[0]),                 // shared secret as the key
                                (uint8_t *)input,               // encrypted input
                                SEND_SFE_OR_OUTPUT_LEN,         // input length (excluding the mac)
                                &sfe_or_output[0],              // decrypted output
                                &aes_gcm_iv[0],
                                SMC_SGX_SP_IV_SIZE,
                                NULL,
                                0,
                                (const sgx_aes_gcm_128bit_tag_t *)
                                (input+SEND_SFE_OR_OUTPUT_LEN));
        if (ret != SGX_SUCCESS)
                return ret;

        /* Copy outside */
        memcpy(output, sfe_or_output, out_len);

        return SGX_SUCCESS;
}

/* Decrypt the YAO ORAM output from the evaluator */
sgx_status_t decrypt_output_yao_or(int len, char *input, int out_len, char *output)
{
        sgx_status_t ret;

        /* Defensive checking */
        if ((len != HYBRID_OR_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != HYBRID_OR_GEN_OUTPUT_LEN))
                return SGX_ERROR_INVALID_PARAMETER;

        /* Decrypt */
        ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
                                (&g_secret[0]),                 // shared secret as the key
                                (uint8_t *)input,               // encrypted input
                                out_len,         // input length (excluding the mac)
                                &yao_or_output[0],              // decrypted output
                                &aes_gcm_iv[0],
                                SMC_SGX_SP_IV_SIZE,
                                NULL,
                                0,
                                (const sgx_aes_gcm_128bit_tag_t *)
                                (input+out_len));
        if (ret != SGX_SUCCESS)
                return ret;

        /* Copy outside */
        memcpy(output, yao_or_output, out_len);

        return SGX_SUCCESS;
}

/* Decrypt the YAO ORAM query split output from the evaluator */
sgx_status_t decrypt_output_yao_or_split(int len, char *input, int out_len, char *output)
{
        sgx_status_t ret;

        /* Defensive checking */
        if ((len != HYBRID_OR_SPLIT_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != HYBRID_OR_SPLIT_GEN_OUTPUT_LEN))
                return SGX_ERROR_INVALID_PARAMETER;

        /* Decrypt */
        ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
                                (&g_secret[0]),                 // shared secret as the key
                                (uint8_t *)input,               // encrypted input
                                out_len,         // input length (excluding the mac)
                                &yao_or_split_output[0],              // decrypted output
                                &aes_gcm_iv[0],
                                SMC_SGX_SP_IV_SIZE,
                                NULL,
                                0,
                                (const sgx_aes_gcm_128bit_tag_t *)
                                (input+out_len));
        if (ret != SGX_SUCCESS)
                return ret;

        /* Copy outside */
        memcpy(output, yao_or_split_output, out_len);

        return SGX_SUCCESS;
}

/* Decrypt the YAO dijkstra output from the evaluator */
sgx_status_t decrypt_output_yao_dj(int len, char *input, int out_len, char *output)
{
        sgx_status_t ret;

        /* Defensive checking */
        if ((len != HYBRID_DJ_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
                (out_len != HYBRID_DJ_GEN_OUTPUT_LEN))
                return SGX_ERROR_INVALID_PARAMETER;

        /* Decrypt */
        ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
                                (&g_secret[0]),                 // shared secret as the key
                                (uint8_t *)input,               // encrypted input
                                out_len,         // input length (excluding the mac)
                                &yao_dj_output[0],              // decrypted output
                                &aes_gcm_iv[0],
                                SMC_SGX_SP_IV_SIZE,
                                NULL,
                                0,
                                (const sgx_aes_gcm_128bit_tag_t *)
                                (input+out_len));
        if (ret != SGX_SUCCESS)
                return ret;

        /* Copy outside */
        memcpy(output, yao_dj_output, out_len);

        return SGX_SUCCESS;
}





/* Receive the SFE mil input from the application */
sgx_status_t put_input_sfe(int len, char *buf)
{
	if (len != SEND_SFE_INPUT_LEN*sizeof(uint32_t))
		return SGX_ERROR_INVALID_PARAMETER;

	memcpy(sendin2, buf, SEND_SFE_INPUT_LEN*sizeof(uint32_t));
	user_input = 1;

	return SGX_SUCCESS;
}

/* Receive the SFE uc input from the application */
sgx_status_t put_input_sfe_uc(int len, char *buf)
{
	if (len != SEND_SFE_UC_INPUT_LEN)
		return SGX_ERROR_INVALID_PARAMETER;

	memcpy(sfe_uc_input, buf, SEND_SFE_UC_INPUT_LEN);
	user_input = 1;

	return SGX_SUCCESS;
}

/* Receive the SFE dj input from the application */
sgx_status_t put_input_sfe_dj(int len, char *buf)
{
	if (len != SEND_SFE_DJ_INPUT_LEN)
		return SGX_ERROR_INVALID_PARAMETER;

	memcpy(sfe_dj_edges, buf, SEND_SFE_DJ_INPUT_EDGES_LEN);
	memcpy(sfe_dj_conns, buf+SEND_SFE_DJ_INPUT_EDGES_LEN,
		SEND_SFE_DJ_INPUT_CONNS_LEN);
	user_input = 1;

	return SGX_SUCCESS;
}

/* Receive the SFE or input from the application */
sgx_status_t put_input_sfe_or(int len, char *buf)
{
        if (len != SEND_SFE_OR_INPUT_LEN)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(sfe_or_input, buf, SEND_SFE_OR_INPUT_LEN);
        user_input = 1;

        return SGX_SUCCESS;
}

/* Receive the YAO or input from the application */
sgx_status_t put_input_yao_or(int len, char *buf)
{
        if (len != HYBRID_OR_QUERY_LEN)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(yao_or_input, buf, HYBRID_OR_QUERY_LEN);
        user_input = 1;

        return SGX_SUCCESS;
}

/* Receive the YAO dj input from the application */
sgx_status_t put_input_yao_dj(int len, char *buf)
{
        if (len != HYBRID_DJ_GEN_INPUT_LEN)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(yao_dj_input, buf, HYBRID_DJ_GEN_INPUT_LEN);
        user_input = 1;

        return SGX_SUCCESS;
}

/* Receive the YAO dj input from the application */
sgx_status_t put_input_yao_dj2(int len, char *buf)
{
        if (len != HYBRID_DJ_EVL_INPUT_LEN)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(yao_dj_input_evl, buf, HYBRID_DJ_EVL_INPUT_LEN);
        user_input = 1;

        return SGX_SUCCESS;
}

/* Receive the YAO or split input from the application */
sgx_status_t put_input_yao_or_split(int len, char *buf)
{
        if (len != HYBRID_OR_SPLIT_QUERY_LEN)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(yao_or_split_input, buf, HYBRID_OR_SPLIT_QUERY_LEN);
        user_input = 1;

        return SGX_SUCCESS;
}
