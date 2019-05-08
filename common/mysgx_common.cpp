#include "../include/mysgx_common.h"

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <vector>


/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
	{	SGX_SUCCESS,
		"Damn, we are good.",
		NULL
	},
	{
		SGX_ERROR_UNEXPECTED,
		"Unexpected error occurred.",
		NULL
	},
	{
		SGX_ERROR_INVALID_PARAMETER,
		"Invalid parameter.",
		NULL
	},
	{
		SGX_ERROR_OUT_OF_MEMORY,
		"Out of memory.",
		NULL
	},
	{
		SGX_ERROR_ENCLAVE_LOST,
		"Power transition occurred.",
		"Please refer to the sample \"PowerTransition\" for details."
	},
	{
		SGX_ERROR_INVALID_ENCLAVE,
		"Invalid enclave image.",
		NULL
	},
	{
		SGX_ERROR_INVALID_ENCLAVE_ID,
		"Invalid enclave identification.",
		NULL
	},
	{
		SGX_ERROR_INVALID_SIGNATURE,
		"Invalid enclave signature.",
		NULL
	},
	{
		SGX_ERROR_OUT_OF_EPC,
		"Out of EPC memory.",
		NULL
	},
	{
		SGX_ERROR_NO_DEVICE,
		"Invalid SGX device.",
		"Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
	},
	{
		SGX_ERROR_MEMORY_MAP_CONFLICT,
		"Memory map conflicted.",
		NULL
	},
	{
		SGX_ERROR_INVALID_METADATA,
		"Invalid enclave metadata.",
		NULL
	},
	{
		SGX_ERROR_DEVICE_BUSY,
		"SGX device was busy.",
		NULL
	},
	{
		SGX_ERROR_INVALID_VERSION,
		"Enclave version was invalid.",
		NULL
	},
	{
		SGX_ERROR_INVALID_ATTRIBUTE,
		"Enclave was not authorized.",
		NULL
	},
	{
		SGX_ERROR_ENCLAVE_FILE_ACCESS,
		"Can't open enclave file.",
		NULL
	},
	{
		SGX_ERROR_MAC_MISMATCH,
		"The input MAC does not match the MAC calculated.",
		NULL
	},
};

/* Check error conditions for loading enclave */
void mysgx_print_error(sgx_status_t ret)
{
	size_t idx;
	size_t ttl = sizeof(sgx_errlist)/sizeof(sgx_errlist[0]);

	for (idx = 0; idx < ttl; idx++) {
		if(ret == sgx_errlist[idx].err) {
			if(NULL != sgx_errlist[idx].sug)
				printf("mysgx-Info: %s\n", sgx_errlist[idx].sug);
			printf("mysgx-Error: %s\n", sgx_errlist[idx].msg);
			break;
		}
	}

	if (idx == ttl)
		printf("mysgx-Error: Unexpected error occurred.\n");
}

const char * sgx_strerror(sgx_status_t ret)
{
	size_t idx;
	size_t ttl = sizeof(sgx_errlist)/sizeof(sgx_errlist[0]);

	for (idx = 0; idx < ttl; idx++) {
		if(ret == sgx_errlist[idx].err) {
                        return sgx_errlist[idx].msg;
		}
	}

        return "Unknown SGX error code";
}

int mysgx_perror(sgx_status_t err, const char * fmt, ...)
{
  va_list list;
  va_start(list, fmt);

  int amt = printf("mysgx-error: ");

  amt += vprintf(fmt, list);
  amt += printf(" - %s\n", sgx_strerror(err));

  return amt;
}


void fileToVector(char *filename, std::vector<unsigned int> & inputvec, int size)
{
	std::ifstream fs;
	fs.open(filename,std::fstream::in);
	for(int i=0;i<size;i++)
	{
		if(!fs.eof())
		{
			fs >> inputvec.at(i);		
		}
		else
		{
			inputvec.at(i) = 0;
		}
	}
}
