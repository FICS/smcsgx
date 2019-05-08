#ifndef MYSGX_COMMON_H_
#define MYSGX_COMMON_H_

#include <vector>
#include "sgx_status.h"

typedef struct _sgx_errlist_t {
	sgx_status_t err;
	const char *msg;
	const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Check error conditions for loading enclave */
void mysgx_print_error(sgx_status_t ret);
const char * sgx_strerror(sgx_status_t ret);
int mysgx_perror(sgx_status_t err, const char * fmt, ...);

/* Hybrid helper */
void fileToVector(char *filename, std::vector<unsigned int> & inputvec, int size);

#endif /* MYSGX_COMMON_H_ */
