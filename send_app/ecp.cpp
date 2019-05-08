/**
 *
 * Copyright(c) 2011-2016 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and treaty
 * provisions. No part of the Material may be used, copied, reproduced, modified,
 * published, uploaded, posted, transmitted, distributed, or disclosed in any
 * way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel(R) in writing.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/smcsgxra.h"
#include "../sample_libcrypto/sample_libcrypto.h"

#ifdef _MSC_VER
#include "stdafx.h"
#endif

#define MAC_KEY_SIZE       16

// This is the ECDSA NIST P-256 private key used to sign platform_info_blob.
// This private
// key and the public key in SDK untrusted KElibrary should be a temporary key
// pair. For production parts the IAS will sign the platform_info_blob with the
// production private key and the SDK untrusted KE library will have the public
// key for verifcation.
// @TODO:  This key will will not be available when the production backend
// is avaialbe.  The remote attestation sample will need to change to use the
// real backend. This will likely be an RSA2048 type of key.
static const sample_ec256_private_t g_rk_priv_key =
{{
    0x63,0x2c,0xd4,0x02,0x7a,0xdc,0x56,0xa5,
    0x59,0x6c,0x44,0x3e,0x43,0xca,0x4e,0x0b,
    0x58,0xcd,0x78,0xcb,0x3c,0x7e,0xd5,0xb9,
    0xf2,0x91,0x5b,0x39,0x0d,0xb3,0xb5,0xfb
}};


bool verify_cmac128(
		smcsgx_ec_key_128bit_t mac_key,
		const uint8_t *p_data_buf,
		uint32_t buf_size,
		const uint8_t *p_mac_buf)
{
	uint8_t data_mac[SMC_SGX_EC_MAC_SIZE];
	sample_status_t sample_ret;

	sample_ret = sample_rijndael128_cmac_msg((sample_cmac_128bit_key_t*)mac_key,
			p_data_buf,
			buf_size,
			(sample_cmac_128bit_tag_t *)data_mac);
	if(sample_ret != SAMPLE_SUCCESS)
		return false;
	// In real implementation, should use a time safe version of memcmp here,
	// in order to avoid side channel attack.
	if(!memcmp(p_mac_buf, data_mac, SMC_SGX_EC_MAC_SIZE))
		return true;
	return false;
}

#define EC_DERIVATION_BUFFER_SIZE(label_length) ((label_length) +4)

const char str_SMK[] = "SMK";
const char str_SK[] = "SK";
const char str_MK[] = "MK";
const char str_VK[] = "VK";

// Derive key from shared key and key id.
// key id should be sample_derive_key_type_t.
bool derive_key(
		const smcsgx_ec_dh_shared_t *p_shared_key,
		uint8_t key_id,
		smcsgx_ec_key_128bit_t* derived_key)
{
	sample_status_t sample_ret = SAMPLE_SUCCESS;
	uint8_t cmac_key[MAC_KEY_SIZE];
	smcsgx_ec_key_128bit_t key_derive_key;

	memset(&cmac_key, 0, MAC_KEY_SIZE);

	sample_ret = sample_rijndael128_cmac_msg(
			(sample_cmac_128bit_key_t *)&cmac_key,
			(uint8_t*)p_shared_key,
			sizeof(smcsgx_ec_dh_shared_t),
			(sample_cmac_128bit_tag_t *)&key_derive_key);
	if (sample_ret != SAMPLE_SUCCESS)
	{
		// memset here can be optimized away by compiler, so please use memset_s on
		// windows for production code and similar functions on other OSes.
		memset(&key_derive_key, 0, sizeof(key_derive_key));
		return false;
	}

	const char *label = NULL;
	uint32_t label_length = 0;
	switch (key_id)
	{
		case SMC_SGX_DERIVE_KEY_SMK:
			label = str_SMK;
			label_length = sizeof(str_SMK) -1;
			break;
		case SMC_SGX_DERIVE_KEY_SK:
			label = str_SK;
			label_length = sizeof(str_SK) -1;
			break;
		case SMC_SGX_DERIVE_KEY_MK:
			label = str_MK;
			label_length = sizeof(str_MK) -1;
			break;
		case SMC_SGX_DERIVE_KEY_VK:
			label = str_VK;
			label_length = sizeof(str_VK) -1;
			break;
		default:
			// memset here can be optimized away by compiler, so please use memset_s on
			// windows for production code and similar functions on other OSes.
			memset(&key_derive_key, 0, sizeof(key_derive_key));
			return false;
			break;
	}
	/* derivation_buffer = counter(0x01) || label || 0x00 || output_key_len(0x0080) */
	uint32_t derivation_buffer_length = EC_DERIVATION_BUFFER_SIZE(label_length);
	uint8_t *p_derivation_buffer = (uint8_t *)malloc(derivation_buffer_length);
	if (p_derivation_buffer == NULL)
	{
		// memset here can be optimized away by compiler, so please use memset_s on
		// windows for production code and similar functions on other OSes.
		memset(&key_derive_key, 0, sizeof(key_derive_key));
		return false;
	}
	memset(p_derivation_buffer, 0, derivation_buffer_length);

	/*counter = 0x01 */
	p_derivation_buffer[0] = 0x01;
	/*label*/
	memcpy(&p_derivation_buffer[1], label, label_length);
	/*output_key_len=0x0080*/
	uint16_t *key_len = (uint16_t *)(&(p_derivation_buffer[derivation_buffer_length - 2]));
	*key_len = 0x0080;


	sample_ret = sample_rijndael128_cmac_msg(
			(sample_cmac_128bit_key_t *)&key_derive_key,
			p_derivation_buffer,
			derivation_buffer_length,
			(sample_cmac_128bit_tag_t *)derived_key);
	free(p_derivation_buffer);
	// memset here can be optimized away by compiler, so please use memset_s on
	// windows for production code and similar functions on other OSes.
	memset(&key_derive_key, 0, sizeof(key_derive_key));
	if (sample_ret != SAMPLE_SUCCESS)
	{
		return false;
	}
	return true;
}


// Simulates the IAS function for verifying the quote produce by
// the ISV enclave. It doesn't decrypt or verify the quote in
// the simulation.  Just produces the attestaion verification
// report with the platform info blob.
//
// @param p_isv_quote Pointer to the quote generated by the ISV
//                    enclave.
// @param pse_manifest Pointer to the PSE manifest if used.
// @param p_attestation_verification_report Pointer the outputed
//                                          verification report.
//
// @return int
int ias_verify_attestation_evidence(
		smcsgx_quote_t *p_isv_quote,
		uint8_t* pse_manifest,
		ias_att_report_t* p_attestation_verification_report)
{
	int ret = 0;
	sample_ecc_state_handle_t ecc_state = NULL;

	if((NULL == p_isv_quote) ||
			(NULL == p_attestation_verification_report))
	{
		return -1;
	}
	//Decrypt the Quote signature and verify.

	p_attestation_verification_report->id = 0x12345678;
	p_attestation_verification_report->status = IAS_QUOTE_OK;
	p_attestation_verification_report->revocation_reason =
		IAS_REVOC_REASON_NONE;
	p_attestation_verification_report->info_blob.smcsgx_epid_group_status =
		0 << IAS_EPID_GROUP_STATUS_REVOKED_BIT_POS
		| 0 << IAS_EPID_GROUP_STATUS_REKEY_AVAILABLE_BIT_POS;
	p_attestation_verification_report->info_blob.smcsgx_tcb_evaluation_status =
		0 << IAS_TCB_EVAL_STATUS_CPUSVN_OUT_OF_DATE_BIT_POS
		| 0 << IAS_TCB_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS;
	p_attestation_verification_report->info_blob.pse_evaluation_status =
		0 << IAS_PSE_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS
		| 0 << IAS_PSE_EVAL_STATUS_EPID_GROUP_REVOKED_BIT_POS
		| 0 << IAS_PSE_EVAL_STATUS_PSDASVN_OUT_OF_DATE_BIT_POS
		| 0 << IAS_PSE_EVAL_STATUS_SIGRL_OUT_OF_DATE_BIT_POS
		| 0 << IAS_PSE_EVAL_STATUS_PRIVRL_OUT_OF_DATE_BIT_POS;
	memset(p_attestation_verification_report->
			info_blob.latest_equivalent_tcb_psvn, 0, PSVN_SIZE);
	memset(p_attestation_verification_report->info_blob.latest_pse_isvsvn,
			0, ISVSVN_SIZE);
	memset(p_attestation_verification_report->info_blob.latest_psda_svn,
			0, PSDA_SVN_SIZE);
	memset(p_attestation_verification_report->info_blob.performance_rekey_gid,
			0, GID_SIZE);

	// @TODO: Product signing algorithm still TBD.  May be RSA2048 signing.
	// Generate the Service providers ECCDH key pair.
	do {
		ret = sample_ecc256_open_context(&ecc_state);
		if (SAMPLE_SUCCESS != ret) {
			printf("ecp-rror: cannot get ECC cotext in [%s]\n", __func__);
			ret = -1;
			break;
		}
		// Sign
		ret = sample_ecdsa_sign(
				(uint8_t *)&p_attestation_verification_report->
				info_blob.smcsgx_epid_group_status,
				sizeof(ias_platform_info_blob_t) - sizeof(smcsgx_ec_sign256_t),
				(sample_ec256_private_t *)&g_rk_priv_key,
				(sample_ec256_signature_t *)&p_attestation_verification_report->
				info_blob.signature,
				ecc_state);
		if (SAMPLE_SUCCESS != ret) {
			printf("ecp-error: sign ga_gb fail in [%s]\n", __func__);
			ret = SP_INTERNAL_ERROR;
			break;
		}
		SWAP_ENDIAN_32B(p_attestation_verification_report->
				info_blob.signature.x);
		SWAP_ENDIAN_32B(p_attestation_verification_report->
				info_blob.signature.y);

	}while (0);
	if (ecc_state) {
		sample_ecc256_close_context(ecc_state);
	}
	p_attestation_verification_report->pse_status = IAS_PSE_OK;

	// For now, don't simulate the policy reports.
	p_attestation_verification_report->policy_report_size = 0;
	return(ret);
}

