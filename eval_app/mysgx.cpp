/*
 * mysgx.cpp
 * SGX implementations for the evaluator
 * Apr 17, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "sgx_eid.h"
#include "sgx_status.h"
#include "sgx_ukey_exchange.h"
#include "sgx_urts.h"
#include "eval_enclave_u.h"
#include "../include/smcsgx.h"
#include "../include/smcsgxra.h"
#include "../include/mysgx_common.h"

/* Global vars */
sgx_enclave_id_t global_eid;
static sgx_ra_context_t global_context = INT_MAX;
static bool pse_flag = false;
static int enclave_created;
static int enclave_ra_created;
static int debug_fix_ga = 0;
static const char my_fix_ga[] = {
0xd2, 0x7b, 0x24, 0xab, 0x37, 0xdc, 0x94, 0xc3, 
0x42, 0xc8, 0xa, 0x6f, 0x10, 0x15, 0x32, 0xe7, 
0x0, 0xaa, 0x1c, 0xcf, 0x91, 0xfc, 0xdc, 0x57, 
0x30, 0x42, 0xfe, 0x88, 0x70, 0xac, 0xad, 0x7a, 
0x41, 0xd, 0x56, 0x42, 0xa7, 0xbc, 0xd6, 0x29, 
0xd1, 0x47, 0x7e, 0xc4, 0x8e, 0xd1, 0x37, 0x8, 
0xa6, 0x81, 0x5a, 0x13, 0x82, 0xd2, 0x4, 0x4e, 
0xf2, 0xc7, 0x94, 0xe0, 0xc8, 0x67, 0xed, 0x55};
static const char my_fix_ga2[] = {
0xf, 0x7b, 0x5f, 0xaa, 0x23, 0x81, 0x6e, 0xcd, 
0x43, 0xb9, 0x8f, 0x76, 0x28, 0xa4, 0x84, 0x11, 
0x30, 0xd0, 0x2c, 0xc9, 0xed, 0x21, 0x86, 0x9, 
0x3d, 0xad, 0x12, 0x53, 0xfe, 0x2, 0x5e, 0xbe, 
0x27, 0xe3, 0xdb, 0x21, 0xe2, 0xe, 0x8b, 0xd7, 
0xc1, 0x8e, 0x54, 0xa1, 0xfa, 0xdf, 0x40, 0x3f, 
0x45, 0x7, 0x85, 0x82, 0x59, 0x8a, 0xde, 0x10, 
0x2f, 0xb4, 0x9, 0xd2, 0x58, 0x46, 0xc, 0xf6};

/* Helpers */


/* APIs */
sgx_status_t mysgx_gen_S1(int *len, char *buf)
{
	sgx_status_t ret;

	ret = sgx_ra_get_msg1(global_context,
			global_eid,
			sgx_ra_get_ga,
			(sgx_ra_msg1_t *)buf);
	if (ret != SGX_SUCCESS) {
		mysgx_perror(ret, "sgx_ra_get_msg1 failed");
		return ret;
	}
	*len = sizeof(sgx_ra_msg1_t);

	if (debug_fix_ga)
		memcpy(buf, my_fix_ga2, sizeof(my_fix_ga));

	return SGX_SUCCESS;
}

/* Process S2 and generate S3 */
sgx_status_t mysgx_gen_S3(int s2_len, char *s2, int *s3_len, char **s3)
{
	sgx_status_t ret;
	int busy_retry_time = 2;

	/* Defensive checking */
	if (sizeof(sgx_ra_msg2_t) != s2_len)
		printf("daveti-error: smcsgx_msg2_len %d, sgx_msg2_len %d\n",
			sizeof(sgx_ra_msg2_t), s2_len);

	do {
		ret = sgx_ra_proc_msg2(global_context,
			global_eid,
			sgx_ra_proc_msg2_trusted,
			sgx_ra_get_msg3_trusted,
			(sgx_ra_msg2_t *)s2,
			sizeof(sgx_ra_msg2_t),
			(sgx_ra_msg3_t **)s3,
			(uint32_t *)s3_len);
	} while (SGX_ERROR_BUSY == ret && busy_retry_time--);

	if (ret != SGX_SUCCESS) {
		mysgx_perror(ret, "sgx_ra_proc_msg2 failed");
		return ret;
	}

	return SGX_SUCCESS;
}

/* Verify the mac in the att result msg */
sgx_status_t mysgx_verify_att_res_mac(smcsgx_ra_att_result_msg_t *res)
{
	sgx_status_t ret;
	sgx_status_t status;

	ret = verify_att_result_mac(global_eid,
				&status,
				global_context,
				(uint8_t*)&res->platform_info_blob,
				sizeof(ias_platform_info_blob_t),
				(uint8_t*)&res->mac,
				sizeof(sgx_mac_t));
        if((SGX_SUCCESS != ret) || (SGX_SUCCESS != status)) {
		mysgx_perror(ret, "verify_att_result_mac failed");
		return ret;
	}

	return SGX_SUCCESS;
}

/* Generate the shared secret in the enclave */
sgx_status_t mysgx_gen_secret(smcsgx_ra_att_result_msg_t *res)
{
	sgx_status_t ret;
	sgx_status_t status;

	ret = put_secret_data(global_eid,
			&status,
			global_context,
			res->secret.payload,
			res->secret.payload_size,
			res->secret.payload_tag);
	if((SGX_SUCCESS != ret)  || (SGX_SUCCESS != status)) {
		mysgx_perror(ret, "put_secret_data failed");
		return ret;
	}

	return SGX_SUCCESS;
}


sgx_enclave_id_t mysgx_geteid(void)
{
	return global_eid;
}

int mysgx_init(void)
{
	int ret;
	int launch_token_update;
	sgx_status_t rtn;
	sgx_launch_token_t launch_token;

	memset(&launch_token, 0, sizeof(sgx_launch_token_t));

	/* Create an enclave */
	ret = sgx_create_enclave(SMC_SGX_EVAL_ENCLAVE_FILENAME,
			SGX_DEBUG_FLAG,
			&launch_token,
			&launch_token_update,
			&global_eid, NULL);
	if (SGX_SUCCESS != ret) {
		printf("mysgx-error: call sgx_create_enclave fail [%s]\n",
			__func__);
		return SMC_SGX_RTN_FAILURE;
	}
	printf("mysgx-info: enclave is created\n");
	enclave_created = 1;

	/* Init RA */
	ret = enclave_init_ra(global_eid, &rtn, pse_flag, &global_context);
	if (ret != SGX_SUCCESS || rtn) {
		printf("mysgx-error: enclave_init_ra failed\n");
		return SMC_SGX_RTN_FAILURE;
	}
	printf("mysgx-info: enclave-ra is created\n");
	enclave_ra_created = 1;

	return SMC_SGX_RTN_SUCCESS;
}

void mysgx_exit(void)
{
	sgx_status_t rtn;

	if (enclave_ra_created)
		enclave_ra_close(global_eid, &rtn, global_context);
	if (enclave_created)
		sgx_destroy_enclave(global_eid);
}
