/*
 * eval_enclave.cpp
 * Enclave implementation for the evaluator
 * Ref: /opt/intel/sgxsdk/SampleCode/RemoteAttestation/isv_enclave/isv_enclave.cpp
 * Apr 18, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */

#include <stdlib.h>
#include "string.h"
#include "sgx_status.h"
#include "sgx_error.h"
#include "sgx_trts.h"
#include "sgx_tkey_exchange.h"
#include "sgx_tcrypto.h"
#include "eval_enclave_t.h"
#include "mysfe.h"
#include "mysfe_naive.h"
#include "util.h"
#include "hybrid.h"
#include "../include/smcsgxyaoparam.h"


#define ENCLAVE_NONCE_LEN_MAX		20
#define ENCLAVE_SECRET_LEN		16
// daveti: these defs are from the sender's enclave directly
#define SEND_YAO_MAGIC_J		10
#define SEND_YAO_PERMUTES_LEN		16
#define SEND_YAO_INPUT_BIN_LEN		16
#define SEND_YAO_OUTPUT_EVAL_LEN	8
#define SEND_YAO_OUTPUT_SEND_LEN	16
#define SEND_YAO_INPUT_GC_EVAL_LEN	16
#define SEND_YAO_INPUT_GC_SEND_LEN	32
#define SEND_YAO_INPUT_GEN_GC_LEN	(SEND_YAO_INPUT_GC_SEND_LEN*SEND_YAO_MAGIC_J)
#define SEND_YAO_INPUT_GEN_LEN		(SEND_YAO_INPUT_GEN_GC_LEN+SEND_YAO_PERMUTES_LEN)


// This is the public EC key of the SP. The corresponding private EC key is
// used by the SP to sign data used in the remote attestation SIGMA protocol
// to sign channel binding data in MSG2. A successful verification of the
// signature confirms the identity of the SP to the ISV app in remote
// attestation secure channel binding. The public EC key should be hardcoded in
// the enclave or delivered in a trustworthy manner. The use of a spoofed public
// EC key in the remote attestation with secure channel binding session may lead
// to a security compromise. Every different SP the enlcave communicates to
// must have a unique SP public key. Delivery of the SP public key is
// determined by the ISV. The TKE SIGMA protocl expects an Elliptical Curve key
// based on NIST P-256
static const sgx_ec256_public_t g_sp_pub_key_wrecked = {
	{
		0x46, 0x09, 0x1f, 0x1e, 0x25, 0x00, 0x29, 0x59,
		0x70, 0xac, 0xce, 0xbb, 0xd7, 0xa6, 0x68, 0xc2,
		0x30, 0x42, 0x4a, 0xae, 0xaa, 0x59, 0x64, 0x3a,
		0x33, 0xba, 0x7f, 0xca, 0x89, 0x52, 0xf7, 0x64
	},
	{
		0x60, 0x28, 0xc3, 0xcf, 0x5a, 0x3b, 0x66, 0x4a,
		0x46, 0xd1, 0x63, 0x54, 0xa2, 0x9f, 0xd0, 0xad,
		0xa0, 0xa6, 0x08, 0x0b, 0xec, 0x85, 0x9d, 0x76,
		0xf1, 0x0b, 0x53, 0xa1, 0xeb, 0x11, 0xb3, 0x2d
	}
};
// daveti: The key above is wrecked since Intel changed it the latest SDK...
// and I spent 10 hours debugging this. I guess I was wrecked as well...

// This is the public EC key of the SP. The corresponding private EC key is
// used by the SP to sign data used in the remote attestation SIGMA protocol
// to sign channel binding data in MSG2. A successful verification of the
// signature confirms the identity of the SP to the ISV app in remote
// attestation secure channel binding. The public EC key should be hardcoded in
// the enclave or delivered in a trustworthy manner. The use of a spoofed public
// EC key in the remote attestation with secure channel binding session may lead
// to a security compromise. Every different SP the enlcave communicates to
// must have a unique SP public key. Delivery of the SP public key is
// determined by the ISV. The TKE SIGMA protocl expects an Elliptical Curve key
// based on NIST P-256
static const sgx_ec256_public_t g_sp_pub_key = {
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


// Used to store the secret passed by the SP in the sample code. The
// size is forced to be 16 bytes. Expected value is
// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0f
static uint8_t g_secret[ENCLAVE_SECRET_LEN] = {0};

// Save the nonce in the enclave
static int nonce_len;
static char nonce[ENCLAVE_NONCE_LEN_MAX];

// In the name of YAO
static const char input_yao[SEND_YAO_INPUT_BIN_LEN] = {1,1,0,0,0,0,0,0  ,1,0,1,0,0,0,0,0};
static char gengc[SEND_YAO_INPUT_GC_SEND_LEN][SEND_YAO_MAGIC_J];
static char evlgc[SEND_YAO_INPUT_GC_EVAL_LEN][SEND_YAO_MAGIC_J];
static char permutes[SEND_YAO_PERMUTES_LEN];
static uint8_t gen_input[SEND_YAO_INPUT_GEN_LEN];
static uint8_t gen_input_encrypted[SEND_YAO_INPUT_GEN_LEN];
static uint8_t gen_input_mac[SMC_SGX_SP_TAG_SIZE];
static const uint8_t aes_gcm_iv[SMC_SGX_SP_IV_SIZE] = {0};
static uint32_t gen_input_sfe[SEND_SFE_INPUT_LEN];
static uint32_t evl_input_sfe[SEND_SFE_INPUT_LEN];
static uint8_t gen_output_sfe_pad[SEND_SFE_OUTPUT_PAD_LEN];
static uint8_t gen_output_sfe_enc[SEND_SFE_OUTPUT_PAD_LEN];
static uint8_t gen_output_sfe_mac[SMC_SGX_SP_TAG_SIZE];
static unsigned int gen_output_sfe;
static unsigned int evl_output_sfe;
static sfe_uc_input_program evl_input_sfe_uc;
static bool gen_input_sfe_uc[sfe_uc_numinputs];
static bool gen_output_sfe_uc[sfe_uc_numoutputs];
static uint8_t gen_output_sfe_uc_enc[SMC_SGX_SFE_UC_OUTPUT_LEN];
static sfe_dj_input_eval evl_input_sfe_dj;
static sfe_dj_input_send gen_input_sfe_dj;
static uint8_t gen_input_sfe_dj_array[SMC_SGX_SFE_DJ_INPUT_LEN];
static int gen_output_sfe_dj[sfe_dj_Nodes];
static uint8_t gen_output_sfe_dj_enc[SMC_SGX_SFE_DJ_OUTPUT_LEN];
static unsigned int gen_input_sfe_or[sfe_or_queries];
static unsigned long evl_input_sfe_or[sfe_or_sizeOfDatabase];
static unsigned long gen_output_sfe_or[sfe_or_queries];
static uint8_t gen_output_sfe_or_enc[SMC_SGX_SFE_OR_OUTPUT_LEN];
static int user_input;
static unsigned int evl_input_yao_or[yao_or_datasize*2];
static unsigned int gen_input_yao_or[yao_or_queries];
static unsigned long gen_output_yao_or_sgx[yao_or_queries];
static char gen_output_yao_or_gengc[HYBRID_OR_GENGC_LEN];
static char gen_output_yao_or_permutes[HYBRID_OR_PERMUTES_LEN];
static char evl_output_yao_or_evlgc[HYBRID_OR_EVLGC_LEN];
static uint8_t gen_output_yao_or[HYBRID_OR_GEN_OUTPUT_LEN];
static uint8_t gen_output_yao_or_enc[HYBRID_OR_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE];
static unsigned int evl_input_yao_dj[HYBRID_DJ_EVL_INPUT_NUM];
static unsigned int gen_input_yao_dj[HYBRID_DJ_GEN_INPUT_NUM];
static int gen_output_yao_dj_sgx[yao_dj_Nodes];
static char gen_output_yao_dj_gengc[HYBRID_DJ_GENGC_LEN];
static char gen_output_yao_dj_permutes[HYBRID_DJ_PERMUTES_LEN];
static char evl_output_yao_dj_evlgc[HYBRID_DJ_EVLGC_LEN];
static uint8_t gen_output_yao_dj[HYBRID_DJ_GEN_OUTPUT_LEN];
static uint8_t gen_output_yao_dj_enc[HYBRID_DJ_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE];
static unsigned int evl_input_yao_or_split[yao_or_split_datasize*2];
static unsigned int gen_input_yao_or_split[yao_or_split_queries];
static unsigned long gen_output_yao_or_split_sgx[yao_or_split_queries];
static char gen_output_yao_or_split_gengc[HYBRID_OR_SPLIT_GENGC_LEN];
static char gen_output_yao_or_split_permutes[HYBRID_OR_SPLIT_PERMUTES_LEN];
static char evl_output_yao_or_split_evlgc[HYBRID_OR_SPLIT_EVLGC_LEN];
static uint8_t gen_output_yao_or_split[HYBRID_OR_SPLIT_GEN_OUTPUT_LEN];
static uint8_t gen_output_yao_or_split_enc[HYBRID_OR_SPLIT_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE];


/* ECalls */

// NOTE: this ECall is only for debugging purpose!
sgx_status_t get_secret(int len, char *buf)
{
	if (len != ENCLAVE_SECRET_LEN)
		return SGX_ERROR_INVALID_PARAMETER;

	memcpy(buf, g_secret, ENCLAVE_SECRET_LEN);
	return SGX_SUCCESS;
}

// Save the nonce from the sender in then enclave
// FIXME: unclear what we should do with this nonce...
sgx_status_t put_nonce(int len, char *buf)
{
	if (len > ENCLAVE_NONCE_LEN_MAX)
		return SGX_ERROR_INVALID_PARAMETER;

	nonce_len = len;
	memcpy(nonce, buf, len);

	return SGX_SUCCESS;
}

// Given the input to compute the output
// Could be a SFE compuation or YAO
// daveti: depneding on the encryption,
// we need to decrypt input and then compute the output
// and then encrypt the output using the shared secret
// before returning the result back to the application
// Ideally, the crypto chosen from the sender should be
// the one supported by Intel trusted crypto lib!
// TODO
sgx_status_t gen_res(int input_len, char *input, int res_len, char *res)
{
	return  SGX_SUCCESS;
}


// This ecall is a wrapper of sgx_ra_init to create the trusted
// KE exchange key context needed for the remote attestation
// SIGMA API's. Input pointers aren't checked since the trusted stubs
// copy them into EPC memory.
//
// @param b_pse Indicates whether the ISV app is using the
//              platform services.
// @param p_context Pointer to the location where the returned
//                  key context is to be copied.
//
// @return Any error return from the create PSE session if b_pse
//         is true.
// @return Any error returned from the trusted key exchange API
//         for creating a key context.
sgx_status_t enclave_init_ra(
		int b_pse,
		sgx_ra_context_t *p_context)
{
	// isv enclave call to trusted key exchange library.
	// daveti: NOTE: PSE session is only available to the simuluaiton mode
	// Make sure PSE flag is false in HW mode...
	sgx_status_t ret;

	if (b_pse) {
		int busy_retry_times = 2;
		do {
			ret = sgx_create_pse_session();
		} while (ret == SGX_ERROR_BUSY && busy_retry_times--);
		if (ret != SGX_SUCCESS)
			return ret;
	}

	ret = sgx_ra_init(&g_sp_pub_key, b_pse, p_context);

	if (b_pse) {
		sgx_close_pse_session();
		return ret;
	}

	return ret;
}


// Closes the tKE key context used during the SIGMA key
// exchange.
//
// @param context The trusted KE library key context.
//
// @return Return value from the key context close API
sgx_status_t SGXAPI enclave_ra_close(
		sgx_ra_context_t context)
{
	sgx_status_t ret;
	ret = sgx_ra_close(context);
	return ret;
}


// Verify the mac sent in att_result_msg from the SP using the
// MK key. Input pointers aren't checked since the trusted stubs
// copy them into EPC memory.
//
//
// @param context The trusted KE library key context.
// @param p_message Pointer to the message used to produce MAC
// @param message_size Size in bytes of the message.
// @param p_mac Pointer to the MAC to compare to.
// @param mac_size Size in bytes of the MAC
//
// @return SGX_ERROR_INVALID_PARAMETER - MAC size is incorrect.
// @return Any error produced by tKE  API to get SK key.
// @return Any error produced by the AESCMAC function.
// @return SGX_ERROR_MAC_MISMATCH - MAC compare fails.
sgx_status_t verify_att_result_mac(sgx_ra_context_t context,
		uint8_t* p_message,
		size_t message_size,
		uint8_t* p_mac,
		size_t mac_size)
{
	sgx_status_t ret;
	sgx_ec_key_128bit_t mk_key;

	if (mac_size != sizeof(sgx_mac_t)) {
		ret = SGX_ERROR_INVALID_PARAMETER;
		return ret;
	}
	if (message_size > UINT32_MAX) {
		ret = SGX_ERROR_INVALID_PARAMETER;
		return ret;
	}

	do {
		uint8_t mac[SGX_CMAC_MAC_SIZE] = {0};

		ret = sgx_ra_get_keys(context, SGX_RA_KEY_MK, &mk_key);
		if(SGX_SUCCESS != ret)
			break;

		ret = sgx_rijndael128_cmac_msg(&mk_key,
				p_message,
				(uint32_t)message_size,
				&mac);
		if(SGX_SUCCESS != ret)
			break;
		if (0 == consttime_memequal(p_mac, mac, sizeof(mac))) {
			ret = SGX_ERROR_MAC_MISMATCH;
			break;
		}
	} while(0);

	return ret;
}


// Generate a secret information for the SP encrypted with SK.
// Input pointers aren't checked since the trusted stubs copy
// them into EPC memory.
//
// @param context The trusted KE library key context.
// @param p_secret Message containing the secret.
// @param secret_size Size in bytes of the secret message.
// @param p_gcm_mac The pointer the the AESGCM MAC for the
//                 message.
//
// @return SGX_ERROR_INVALID_PARAMETER - secret size if
//         incorrect.
// @return Any error produced by tKE  API to get SK key.
// @return Any error produced by the AESGCM function.
// @return SGX_ERROR_UNEXPECTED - the secret doesn't match the
//         expected value.
sgx_status_t put_secret_data(
		sgx_ra_context_t context,
		uint8_t *p_secret,
		uint32_t secret_size,
		uint8_t *p_gcm_mac)
{
	sgx_status_t ret = SGX_SUCCESS;
	sgx_ec_key_128bit_t sk_key;

	do {
		if (secret_size != ENCLAVE_SECRET_LEN) {
			ret = SGX_ERROR_INVALID_PARAMETER;
			break;
		}

		ret = sgx_ra_get_keys(context, SGX_RA_KEY_SK, &sk_key);
		if(SGX_SUCCESS != ret)
			break;

		ret = sgx_rijndael128GCM_decrypt(&sk_key,
				p_secret,
				secret_size,
				&g_secret[0],
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(p_gcm_mac));
		if (ret != SGX_SUCCESS)
			return ret;

		// daveti: yeah, Intel cheated here...
		uint32_t i;
		bool secret_match = true;
		for (i = 0; i < secret_size; i++) {
			if (g_secret[i] != i)
				secret_match = false;
		}

		if (!secret_match)
			ret = SGX_ERROR_UNEXPECTED;

		// Once the server has the shared secret, it should be sealed to
		// persistent storage for future use. This will prevents having to
		// perform remote attestation until the secret goes stale. Once the
		// enclave is created again, the secret can be unsealed.
	} while(0);

	return ret;
}


/* The similar (but not the same) copy as the sender's enclave impl
 * NOTE: we should not expose the gengc to the application, and
 * only the encrypted version should be exposed (gen_in). However,
 * for debugging purpose, we do it here. Be aware!
 * For production enclave, remove the gen_len and gen parameters
 * from the ECall
 * May 2, 2016
 * daveti
 */
sgx_status_t gen_input_yao(int len, char *input,
			int gen_len, char *gen,
			int evl_len, char *evl,
			int pem_len, char *pem,
			int gen_in_len, char *gen_in)
{
	int i, j;
	sgx_status_t ret;

	/* Ignore the len and input for now
	 * since the input is saved in the enclave
	 */

	/* Defensive checking */
	if ((gen_in_len != SEND_YAO_INPUT_GEN_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(gen_len != SEND_YAO_INPUT_GEN_GC_LEN) ||
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

	/* Compose the original gen_input = gengc + permutes */
	memcpy(gen_input, gengc, SEND_YAO_INPUT_GEN_GC_LEN);
	memcpy(gen_input+SEND_YAO_INPUT_GEN_GC_LEN,
		permutes, SEND_YAO_PERMUTES_LEN);

	/* Encrypt the gen_input before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),	// use shared secret as the key
			&gen_input[0],			// src to be encrypted
			SEND_YAO_INPUT_GEN_LEN,		// src len
			gen_input_encrypted,		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(gen_input_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	/* Copy outside */
	memcpy(gen_in, gen_input_encrypted, SEND_YAO_INPUT_GEN_LEN);
	memcpy(gen_in+SEND_YAO_INPUT_GEN_LEN,
		gen_input_mac, SMC_SGX_SP_TAG_SIZE);
	memcpy(gen, gengc, gen_len);			// daveti: should remove this in production enclave
	memcpy(evl, evlgc, evl_len);
	memcpy(pem, permutes, pem_len);

	return SGX_SUCCESS;
}

/* SFE mil computation in the enclave */
sgx_status_t compute_sfe(int len, char *input,
			int evl_len, char *evl_out,
			int gen_len, char *gen_out,
			int gen_in_len, char *gen_in,
			int gen_enc_len, char *gen_enc_out,
			int sfe_naive)
{
	int i;
	sgx_status_t ret;

	/* NOTE: both gen_out and gen_in should be removed
	 * in the production code to avoid exposing plain text
	 * outside the enclave!
	 */

	/* Defensive checking */
	if ((len != SEND_SFE_INPUT_LEN*sizeof(uint32_t)+SMC_SGX_SP_TAG_SIZE) ||
		(evl_len != SEND_SFE_OUTPUT_LEN) ||
		(gen_len != SEND_SFE_OUTPUT_LEN) ||
		(gen_in_len != SEND_SFE_INPUT_LEN*sizeof(uint32_t)) ||
		(gen_enc_len != SEND_SFE_OUTPUT_ENC_LEN))
		return SGX_ERROR_INVALID_PARAMETER;

	/* Decrypt the input */
	ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
				(&g_secret[0]),
				(uint8_t *)input,
				SEND_SFE_INPUT_LEN*sizeof(uint32_t),
				(uint8_t *)&gen_input_sfe[0],
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(input+SEND_SFE_INPUT_LEN*sizeof(uint32_t)));
	if (ret != SGX_SUCCESS)
		return ret;

	if (!user_input) {
		/* Init the input for ourselves/Bob */
		for (i = 0; i < SEND_SFE_INPUT_LEN; i++)
			evl_input_sfe[i] = 0;
		evl_input_sfe[0] = 5;
		evl_input_sfe[1] = 1;
	}

	/* Run the SFE program */
	if (sfe_naive)
		sfe_naive_entry(gen_input_sfe, evl_input_sfe,
			&gen_output_sfe, &evl_output_sfe);
	else
		sfe_entry(gen_input_sfe, evl_input_sfe,
			&gen_output_sfe, &evl_output_sfe);

	/* Pad the SFE output for sender/Alice */
	memset(gen_output_sfe_pad, 0x1, SEND_SFE_OUTPUT_PAD_LEN);
	memcpy(gen_output_sfe_pad, &gen_output_sfe, SEND_SFE_OUTPUT_LEN);

	/* Encrypt the output for gen */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			&gen_output_sfe_pad[0],		// src to be encrypted
			SEND_SFE_OUTPUT_PAD_LEN,	// src len
			&gen_output_sfe_enc[0],		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(gen_output_sfe_mac));	// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	/* Copy outside */
	memcpy(evl_out, &evl_output_sfe, evl_len);
	memcpy(gen_out, &gen_output_sfe, gen_len);
	memcpy(gen_in, gen_input_sfe, gen_in_len);
	memcpy(gen_enc_out, gen_output_sfe_enc, SEND_SFE_OUTPUT_PAD_LEN);
	memcpy(gen_enc_out+SEND_SFE_OUTPUT_PAD_LEN,
		gen_output_sfe_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

/* SFE uc computation in the enclave */
sgx_status_t compute_sfe_uc(int len, char *input,
			int out_len, char *output,
			int gen_in_len, char *gen_in,
			int gen_enc_len, char *gen_enc_out,
			int sfe_naive)
{
	int i;
	sgx_status_t ret;

	/* NOTE: both output and gen_in should be removed
	 * in the production code to avoid exposing plain text
	 * outside the enclave!
	 */

	/* Defensive checking */
	if ((len != SMC_SGX_SFE_UC_INPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != SMC_SGX_SFE_UC_OUTPUT_LEN) ||
		(gen_in_len != SMC_SGX_SFE_UC_INPUT_LEN) ||
		(gen_enc_len != SMC_SGX_SFE_UC_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE))
		return SGX_ERROR_INVALID_PARAMETER;

	/* Decrypt the input */
	ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
				(&g_secret[0]),
				(uint8_t *)input,
				SMC_SGX_SFE_UC_INPUT_LEN,
				(uint8_t *)&gen_input_sfe_uc[0],
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(input+SMC_SGX_SFE_UC_INPUT_LEN));
	if (ret != SGX_SUCCESS)
		return ret;

	/* Run the SFE program */
	if (user_input) {
		if (sfe_naive)
			sfe_naive_uc_entry(&evl_input_sfe_uc, gen_input_sfe_uc, gen_output_sfe_uc);
		else
			sfe_uc_entry(&evl_input_sfe_uc, gen_input_sfe_uc, gen_output_sfe_uc);
	} else {
		if (sfe_naive)
			sfe_naive_uc_entry(NULL, gen_input_sfe_uc, gen_output_sfe_uc);
		else
			sfe_uc_entry(NULL, gen_input_sfe_uc, gen_output_sfe_uc);
	}

	/* Encrypt the output for gen */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			(uint8_t *)&gen_output_sfe_uc[0],		// src to be encrypted
			SMC_SGX_SFE_UC_OUTPUT_LEN,
			&gen_output_sfe_uc_enc[0],		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(gen_output_sfe_mac));	// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	/* Copy outside */
	memcpy(output, &gen_output_sfe_uc, out_len);
	memcpy(gen_in, gen_input_sfe_uc, gen_in_len);
	memcpy(gen_enc_out, gen_output_sfe_uc_enc, SMC_SGX_SFE_UC_OUTPUT_LEN);
	memcpy(gen_enc_out+SMC_SGX_SFE_UC_OUTPUT_LEN,
		gen_output_sfe_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

/* SFE dijk computation in the enclave */
sgx_status_t compute_sfe_dj(int len, char *input,
			int out_len, char *output,
			int gen_in_len, char *gen_in,
			int gen_enc_len, char *gen_enc_out,
			int sfe_naive)
{
	int i;
	sgx_status_t ret;

	/* NOTE: both output and gen_in should be removed
	 * in the production code to avoid exposing plain text
	 * outside the enclave!
	 */

	/* Defensive checking */
	if ((len != SMC_SGX_SFE_DJ_INPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != SMC_SGX_SFE_DJ_OUTPUT_LEN) ||
		(gen_in_len != sizeof(sfe_dj_input_send)) ||
		(gen_enc_len != SMC_SGX_SFE_DJ_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE))
		return SGX_ERROR_INVALID_PARAMETER;

	/* Decrypt the input */
	ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
				(&g_secret[0]),
				(uint8_t *)input,
				SMC_SGX_SFE_DJ_INPUT_LEN,
				&gen_input_sfe_dj_array[0],
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(input+SMC_SGX_SFE_DJ_INPUT_LEN));
	if (ret != SGX_SUCCESS)
		return ret;

	/* Convert the array to the struct */
	memcpy(gen_input_sfe_dj.edges, gen_input_sfe_dj_array,
			SMC_SGX_SFE_DJ_INPUT_EDGES_LEN);
	memcpy(gen_input_sfe_dj.connections,
			gen_input_sfe_dj_array+SMC_SGX_SFE_DJ_INPUT_EDGES_LEN,
			SMC_SGX_SFE_DJ_INPUT_CONNS_LEN);

	/* Run the SFE program */
	if (user_input) {
		if (sfe_naive)
			sfe_naive_dj_entry(&evl_input_sfe_dj, &gen_input_sfe_dj, gen_output_sfe_dj);
		else
			sfe_dj_entry(&evl_input_sfe_dj, &gen_input_sfe_dj, gen_output_sfe_dj);
	} else {
		if (sfe_naive)
			sfe_naive_dj_entry(NULL, &gen_input_sfe_dj, gen_output_sfe_dj);
		else
			sfe_dj_entry(NULL, &gen_input_sfe_dj, gen_output_sfe_dj);
	}

	/* Encrypt the output for gen */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			(uint8_t *)&gen_output_sfe_dj[0],		// src to be encrypted
			SMC_SGX_SFE_DJ_OUTPUT_LEN,
			&gen_output_sfe_dj_enc[0],		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(gen_output_sfe_mac));	// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	/* Copy outside */
	memcpy(output, gen_output_sfe_dj, out_len);
	memcpy(gen_in, &gen_input_sfe_dj, gen_in_len);
	memcpy(gen_enc_out, gen_output_sfe_dj_enc, SMC_SGX_SFE_DJ_OUTPUT_LEN);
	memcpy(gen_enc_out+SMC_SGX_SFE_DJ_OUTPUT_LEN,
		gen_output_sfe_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

/* SFE ORAM computation in the enclave */
sgx_status_t compute_sfe_or(int len, char *input,
			int out_len, char *output,
			int gen_in_len, char *gen_in,
			int gen_enc_len, char *gen_enc_out,
			int oram)
{
	int i;
	sgx_status_t ret;

	/* NOTE: both output and gen_in should be removed
	 * in the production code to avoid exposing plain text
	 * outside the enclave!
	 */

	/* Defensive checking */
	if ((len != SMC_SGX_SFE_OR_INPUT_GEN_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != SMC_SGX_SFE_OR_OUTPUT_LEN) ||
		(gen_in_len != SMC_SGX_SFE_OR_INPUT_GEN_LEN) ||
		(gen_enc_len != SMC_SGX_SFE_OR_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE))
		return SGX_ERROR_INVALID_PARAMETER;

	/* Decrypt the input */
	ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
				(&g_secret[0]),
				(uint8_t *)input,
				SMC_SGX_SFE_OR_INPUT_GEN_LEN,
				(uint8_t *)&gen_input_sfe_or[0],
				&aes_gcm_iv[0],
				SMC_SGX_SP_IV_SIZE,
				NULL,
				0,
				(const sgx_aes_gcm_128bit_tag_t *)
				(input+SMC_SGX_SFE_OR_INPUT_GEN_LEN));
	if (ret != SGX_SUCCESS)
		return ret;

#ifdef ORAM_TESTING
	/* Perform ORAM sanity check */
	if (!disable_oram) {
	    if (!oram_run_tests())
		return SGX_ERROR_UNEXPECTED;
	}
#endif

	/* Run the SFE program */
	if (user_input)
		sfe_or_entry(evl_input_sfe_or, gen_input_sfe_or,
			gen_output_sfe_or, oram);

	/* Encrypt the output for gen */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),			// use shared secret as the key
			(uint8_t *)&gen_output_sfe_or[0],		// src to be encrypted
			SMC_SGX_SFE_OR_OUTPUT_LEN,
			&gen_output_sfe_or_enc[0],		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(gen_output_sfe_mac));	// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	/* Copy outside */
	memcpy(output, gen_output_sfe_or, out_len);
	memcpy(gen_in, gen_input_sfe_or, gen_in_len);
	memcpy(gen_enc_out, gen_output_sfe_or_enc, SMC_SGX_SFE_OR_OUTPUT_LEN);
	memcpy(gen_enc_out+SMC_SGX_SFE_OR_OUTPUT_LEN,
		gen_output_sfe_mac, SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

/* Hybrid model for ORAM DB */
sgx_status_t compute_yao_or(int len, char *input,
			int out_len, char *output,
			int gen_len, char *gen,
			int evl_len, char *evl,
			int pem_len, char *pem,
			int gen_in_len, char *gen_in,
			int gen_enc_len, char *gen_enc_out)
{
	int i, j;
	sgx_status_t ret;

	/* Defensive checking */
	if ((len != HYBRID_OR_QUERY_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != HYBRID_OR_SGX_OUTPUT_LEN) ||
		(gen_len != HYBRID_OR_GENGC_LEN) ||
		(evl_len != HYBRID_OR_EVLGC_LEN) ||
		(pem_len != HYBRID_OR_PERMUTES_LEN) ||
		(gen_in_len != HYBRID_OR_QUERY_LEN) ||
		(gen_enc_len != HYBRID_OR_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE))
		return SGX_ERROR_INVALID_PARAMETER;

	/* Decrypt the gen input */
        ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
                                (&g_secret[0]),
                                (uint8_t *)input,
                                HYBRID_OR_QUERY_LEN,
                                (uint8_t *)&gen_input_yao_or[0],
                                &aes_gcm_iv[0],
                                SMC_SGX_SP_IV_SIZE,
                                NULL,
                                0,
                                (const sgx_aes_gcm_128bit_tag_t *)
                                (input+HYBRID_OR_QUERY_LEN));
        if (ret != SGX_SUCCESS)
                return ret;

	/* Do not support hardcode input now */
	if (!user_input)
		return SGX_ERROR_UNEXPECTED;

	/* Init the ORAM DB */
	yao_or_init_db(evl_input_yao_or);

	/* Generate SGX output for gen */
	yao_or_get_sgx_output(gen_output_yao_or_sgx, gen_input_yao_or);

	/* Generate program input for YAO */
	if (yao_or_gen_program_input(gen_output_yao_or_gengc,
		evl_output_yao_or_evlgc,
		gen_output_yao_or_permutes))
		return SGX_ERROR_MAC_MISMATCH; //daveti: use this unusual rtn value for debugging purpose

	/* Compose the original gen_output = sgxoutput + evlgc + permutes */
	memcpy(gen_output_yao_or, gen_output_yao_or_sgx,
		HYBRID_OR_SGX_OUTPUT_LEN);
	memcpy(gen_output_yao_or+HYBRID_OR_SGX_OUTPUT_LEN,
		evl_output_yao_or_evlgc, HYBRID_OR_EVLGC_LEN);
	memcpy(gen_output_yao_or+HYBRID_OR_SGX_OUTPUT_LEN+HYBRID_OR_EVLGC_LEN,
		gen_output_yao_or_permutes, HYBRID_OR_PERMUTES_LEN);

	/* Encrypt the gen_output before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),	// use shared secret as the key
			&gen_output_yao_or[0],			// src to be encrypted
			HYBRID_OR_GEN_OUTPUT_LEN,		// src len
			gen_output_yao_or_enc,		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(gen_input_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	/* Copy outside */
	memcpy(gen, gen_output_yao_or_gengc, gen_len);
	memcpy(evl, evl_output_yao_or_evlgc, evl_len);
	memcpy(pem, gen_output_yao_or_permutes, pem_len);
	memcpy(output, gen_output_yao_or_sgx, out_len);
	memcpy(gen_in, gen_input_yao_or, gen_in_len);
	memcpy(gen_enc_out, gen_output_yao_or_enc, HYBRID_OR_GEN_OUTPUT_LEN);
	memcpy(gen_enc_out+HYBRID_OR_GEN_OUTPUT_LEN, gen_input_mac,
		SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

/* Hybrid model for ORAM DB SPLIT (QUERY) */
sgx_status_t compute_yao_or_split(int len, char *input,
			int out_len, char *output,
			int gen_len, char *gen,
			int evl_len, char *evl,
			int pem_len, char *pem,
			int gen_in_len, char *gen_in,
			int gen_enc_len, char *gen_enc_out)
{
	int i, j;
	sgx_status_t ret;

	/* Defensive checking */
	if ((len != HYBRID_OR_SPLIT_QUERY_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != HYBRID_OR_SPLIT_SGX_OUTPUT_LEN) ||
		(gen_len != HYBRID_OR_SPLIT_GENGC_LEN) ||
		(evl_len != HYBRID_OR_SPLIT_EVLGC_LEN) ||
		(pem_len != HYBRID_OR_SPLIT_PERMUTES_LEN) ||
		(gen_in_len != HYBRID_OR_SPLIT_QUERY_LEN) ||
		(gen_enc_len != HYBRID_OR_SPLIT_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE))
		return SGX_ERROR_INVALID_PARAMETER;

	/* Decrypt the gen input */
        ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
                                (&g_secret[0]),
                                (uint8_t *)input,
                                HYBRID_OR_SPLIT_QUERY_LEN,
                                (uint8_t *)&gen_input_yao_or_split[0],
                                &aes_gcm_iv[0],
                                SMC_SGX_SP_IV_SIZE,
                                NULL,
                                0,
                                (const sgx_aes_gcm_128bit_tag_t *)
                                (input+HYBRID_OR_SPLIT_QUERY_LEN));
        if (ret != SGX_SUCCESS)
                return ret;

	/* Do not support hardcode input now */
	if (!user_input)
		return SGX_ERROR_UNEXPECTED;

	/* Call the hybrid db split */
	yao_or_split_entry(evl_input_yao_or_split,
			gen_output_yao_or_split_sgx,
			gen_input_yao_or_split,
			gen_output_yao_or_split_gengc,
			evl_output_yao_or_split_evlgc,
			gen_output_yao_or_split_permutes);

	/* Compose the original gen_output = sgxoutput + evlgc + permutes */
	memcpy(gen_output_yao_or_split, gen_output_yao_or_split_sgx,
		HYBRID_OR_SPLIT_SGX_OUTPUT_LEN);
	memcpy(gen_output_yao_or_split+HYBRID_OR_SPLIT_SGX_OUTPUT_LEN,
		evl_output_yao_or_split_evlgc, HYBRID_OR_SPLIT_EVLGC_LEN);
	memcpy(gen_output_yao_or_split+HYBRID_OR_SPLIT_SGX_OUTPUT_LEN+HYBRID_OR_SPLIT_EVLGC_LEN,
		gen_output_yao_or_split_permutes, HYBRID_OR_SPLIT_PERMUTES_LEN);

	/* Encrypt the gen_output before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),	// use shared secret as the key
			&gen_output_yao_or_split[0],			// src to be encrypted
			HYBRID_OR_SPLIT_GEN_OUTPUT_LEN,		// src len
			gen_output_yao_or_split_enc,		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(gen_input_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	/* Copy outside */
	memcpy(gen, gen_output_yao_or_split_gengc, gen_len);
	memcpy(evl, evl_output_yao_or_split_evlgc, evl_len);
	memcpy(pem, gen_output_yao_or_split_permutes, pem_len);
	memcpy(output, gen_output_yao_or_split_sgx, out_len);
	memcpy(gen_in, gen_input_yao_or_split, gen_in_len);
	memcpy(gen_enc_out, gen_output_yao_or_split_enc, HYBRID_OR_SPLIT_GEN_OUTPUT_LEN);
	memcpy(gen_enc_out+HYBRID_OR_SPLIT_GEN_OUTPUT_LEN, gen_input_mac,
		SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}

/* Hybrid model for Dijkstra */
sgx_status_t compute_yao_dj(int len, char *input,
			int out_len, char *output,
			int gen_len, char *gen,
			int evl_len, char *evl,
			int pem_len, char *pem,
			int gen_in_len, char *gen_in,
			int gen_enc_len, char *gen_enc_out)
{
	int i, j;
	sgx_status_t ret;

#ifdef BEN_IS_NICE_TO_DAVE
	/* Defensive checking */
	if ((len != HYBRID_DJ_GEN_INPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
		(out_len != HYBRID_DJ_SGX_OUTPUT_LEN) ||
		(gen_len != HYBRID_DJ_GENGC_LEN) ||
		(evl_len != HYBRID_DJ_EVLGC_LEN) ||
		(pem_len != HYBRID_DJ_PERMUTES_LEN) ||
		(gen_in_len != HYBRID_DJ_GEN_INPUT_LEN) ||
		(gen_enc_len != HYBRID_DJ_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE))
		return SGX_ERROR_INVALID_PARAMETER;

	/* Decrypt the gen input */
        ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
                                (&g_secret[0]),
                                (uint8_t *)input,
                                HYBRID_DJ_GEN_INPUT_LEN,
                                (uint8_t *)&gen_input_yao_dj[0],
                                &aes_gcm_iv[0],
                                SMC_SGX_SP_IV_SIZE,
                                NULL,
                                0,
                                (const sgx_aes_gcm_128bit_tag_t *)
                                (input+HYBRID_DJ_GEN_INPUT_LEN));
        if (ret != SGX_SUCCESS)
                return ret;
#else
        /* Defensive checking */
        if ((len != HYBRID_DJ_EVL_INPUT_LEN+SMC_SGX_SP_TAG_SIZE) ||
                (out_len != HYBRID_DJ_SGX_OUTPUT_LEN) ||
                (gen_len != HYBRID_DJ_GENGC_LEN) ||
                (evl_len != HYBRID_DJ_EVLGC_LEN) ||
                (pem_len != HYBRID_DJ_PERMUTES_LEN) ||
                (gen_in_len != HYBRID_DJ_EVL_INPUT_LEN) ||
                (gen_enc_len != HYBRID_DJ_GEN_OUTPUT_LEN+SMC_SGX_SP_TAG_SIZE))
                return SGX_ERROR_INVALID_PARAMETER;

        /* Decrypt the gen input */
        ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)
                                (&g_secret[0]),
                                (uint8_t *)input,
                                HYBRID_DJ_EVL_INPUT_LEN,
                                (uint8_t *)&evl_input_yao_dj[0],
                                &aes_gcm_iv[0],
                                SMC_SGX_SP_IV_SIZE,
                                NULL,
                                0,
                                (const sgx_aes_gcm_128bit_tag_t *)
                                (input+HYBRID_DJ_EVL_INPUT_LEN));
        if (ret != SGX_SUCCESS)
                return ret;
#endif

	/* Do not support hardcode input now */
	if (!user_input)
		return SGX_ERROR_UNEXPECTED;

	/* Call DJ */
	yao_dj_entry(gen_input_yao_dj,
		evl_input_yao_dj,
		gen_output_yao_dj_sgx,
		gen_output_yao_dj_gengc,
		evl_output_yao_dj_evlgc,
		gen_output_yao_dj_permutes);

	/* Compose the original gen_output = evlgc + permutes */
	memcpy(gen_output_yao_dj, evl_output_yao_dj_evlgc, HYBRID_DJ_EVLGC_LEN);
	/* Aug 1, 2016
	 * no need for permutes for YAO evaluator
	memcpy(gen_output_yao_dj+HYBRID_DJ_EVLGC_LEN,
		gen_output_yao_dj_permutes, HYBRID_DJ_PERMUTES_LEN);
	 */
	memcpy(gen_output_yao_dj+HYBRID_DJ_EVLGC_LEN,
		0x0, HYBRID_DJ_PERMUTES_LEN);

	/* Encrypt the gen_output before exposing */
	ret = sgx_rijndael128GCM_encrypt(
			(const sgx_aes_gcm_128bit_key_t *)
			(&g_secret[0]),	// use shared secret as the key
			&gen_output_yao_dj[0],			// src to be encrypted
			HYBRID_DJ_GEN_OUTPUT_LEN,		// src len
			gen_output_yao_dj_enc,		// dst buf
			&aes_gcm_iv[0],			// IV
			SMC_SGX_SP_IV_SIZE,		// IV size (12)
			NULL,
			0,
			(sgx_aes_gcm_128bit_tag_t *)
			(gen_input_mac));		// output mac
	if (ret != SGX_SUCCESS)
		return ret;

	/* Copy outside */
	memcpy(gen, gen_output_yao_dj_gengc, gen_len);
	memcpy(evl, evl_output_yao_dj_evlgc, evl_len);
	memcpy(pem, gen_output_yao_dj_permutes, pem_len);
	memcpy(output, gen_output_yao_dj_sgx, out_len);
#ifdef BEN_IS_NICE_TO_DAVE
	memcpy(gen_in, gen_input_yao_dj, gen_in_len);
#else
	memcpy(gen_in, evl_input_yao_dj, gen_in_len);
#endif
	memcpy(gen_enc_out, gen_output_yao_dj_enc, HYBRID_DJ_GEN_OUTPUT_LEN);
	memcpy(gen_enc_out+HYBRID_DJ_GEN_OUTPUT_LEN, gen_input_mac,
		SMC_SGX_SP_TAG_SIZE);

	return SGX_SUCCESS;
}




/* Receive the SFE mil input from the application */
sgx_status_t put_input_sfe(int len, char *buf)
{
	if (len != SEND_SFE_INPUT_LEN*sizeof(uint32_t))
		return SGX_ERROR_INVALID_PARAMETER;

	memcpy(evl_input_sfe, buf, len);
	user_input = 1;

	return SGX_SUCCESS;
}

/* Receive the SFE uc input from the application */
sgx_status_t put_input_sfe_uc(int len, char *buf)
{
	if (len != sizeof(sfe_uc_input_program))
		return SGX_ERROR_INVALID_PARAMETER;

	memcpy(&evl_input_sfe_uc, buf, len);
	user_input = 1;

	return SGX_SUCCESS;
}

/* Receive the SFE dj input from the application */
sgx_status_t put_input_sfe_dj(int len, char *buf)
{
	if (len != sizeof(sfe_dj_input_eval))
		return SGX_ERROR_INVALID_PARAMETER;

	memcpy(&evl_input_sfe_dj, buf, len);
	user_input = 1;

	return SGX_SUCCESS;
}

/* Receive the SFE or input from the application */
sgx_status_t put_input_sfe_or(int len, char *buf)
{
        if (len != SMC_SGX_SFE_OR_INPUT_EVL_LEN)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(evl_input_sfe_or, buf, len);
        user_input = 1;

        return SGX_SUCCESS;
}

/* Receive the YAO or input from the application */
sgx_status_t put_input_yao_or(int len, char *buf)
{
        if (len != HYBRID_OR_DATABASE_LEN*2)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(evl_input_yao_or, buf, len);
        user_input = 1;

        return SGX_SUCCESS;
}

/* Receive the YAO dj input from the application */
sgx_status_t put_input_yao_dj(int len, char *buf)
{
        if (len != HYBRID_DJ_EVL_INPUT_LEN)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(evl_input_yao_dj, buf, len);
        user_input = 1;

        return SGX_SUCCESS;
}

/* Receive the YAO dj input from the application */
sgx_status_t put_input_yao_dj2(int len, char *buf)
{
        if (len != HYBRID_DJ_GEN_INPUT_LEN)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(gen_input_yao_dj, buf, len);
        user_input = 1;

        return SGX_SUCCESS;
}

/* Receive the YAO or split input from the application */
sgx_status_t put_input_yao_or_split(int len, char *buf)
{
        if (len != HYBRID_OR_SPLIT_DATABASE_LEN*2)
                return SGX_ERROR_INVALID_PARAMETER;

        memcpy(evl_input_yao_or_split, buf, len);
        user_input = 1;

        return SGX_SUCCESS;
}
