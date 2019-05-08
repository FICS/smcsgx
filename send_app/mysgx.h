/*
 * mysgx.h
 * SGX related APIs
 * Apr 15, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include "sgx_eid.h"

int mysgx_init(void);
void mysgx_exit(void);
int mysgx_status(void);
sgx_enclave_id_t mysgx_geteid(void);
