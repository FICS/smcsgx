/*
 * mysgx.h
 * SGX header file for the evaluator
 * Apr 17, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include "sgx_eid.h"
#include "sgx_status.h"
#include "../include/smcsgxra.h"

int mysgx_init(void);
void mysgx_exit(void);
sgx_status_t mysgx_gen_S1(int *len, char *buf);
sgx_status_t mysgx_gen_S3(int s2_len, char *s2, int *s3_len, char **s3);
sgx_enclave_id_t mysgx_geteid(void);
sgx_status_t mysgx_verify_att_res_mac(smcsgx_ra_att_result_msg_t *res);
sgx_status_t mysgx_gen_secret(smcsgx_ra_att_result_msg_t *res);
