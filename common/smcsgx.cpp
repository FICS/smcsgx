#include "../include/smcsgx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int debug = 0;

const char * smcsgx_stm_name(int state)
{
	if(state < 0 || state >= SMC_SGX_STM_NUM_MAX)
		return NULL;

	return smc_sgx_stm_name[state];
}

int smcsgx_read_input(char *path, char *buf, int buf_len)
{
	FILE *fp;
	char *line;
	char *ptr;
	size_t len;
	ssize_t read;
	int total;

	if (debug)
		printf("smcsgx-debug: into %s with file %s, buf %p, buf_len %d\n",
			__func__, path, buf, buf_len);

	fp = fopen(path, "r");
	if (fp == NULL) {
		printf("smcsgx-error: fopen failed in %s with error %s\n",
			__func__, strerror(errno));
		return -1;
	}

	ptr = buf;
	total = 0;
	line = NULL;
	len = 0;
	while ((read = getline(&line, &len, fp)) != -1) {
		if (debug)
			printf("smcsgx-debug: retrieved line of length %zu -\n"
				"%s", read, line);
		/* Ignore comments */
		if (line[0] == '#')
			continue;

		/* Defensive checking */
		if (total + read >= buf_len) {
			printf("smcsgx-error: input buffer is not large enough\n");
			break;
		}

		/* Copy out */
		read++;	/* including the null in the line */
		memcpy(ptr, line, read);
		ptr += read;
		total += read;
	}

	fclose(fp);
	if (line)
		free(line);

	return total;
}
