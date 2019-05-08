/*
 * Implementations for utils
 * May 9, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define UTILS_BUF_LEN			1024
#define UTILS_ENCLAVE_HASH_FILE_SIZE	256
#define UTILS_ENCLAVE_HASH_INDEX	188	//0xbc
#define UTILS_ENCLAVE_HASH_LEN		32	// sha256

static char utils_buf[UTILS_BUF_LEN];

int get_enclave_hash(char *path, char *output)
{
	int fd;
	int len;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("utils-error: open failed in %s with error %s\n",
			__func__, strerror(errno));
		return -1;
	}

	len = read(fd, utils_buf, UTILS_ENCLAVE_HASH_FILE_SIZE);
	if (len != UTILS_ENCLAVE_HASH_FILE_SIZE) {
		printf("utils-error: read failed in %s - returned %d bytes,"
			" requested %d bytes\n",
			__func__, len, UTILS_ENCLAVE_HASH_FILE_SIZE);
		return -1;
	}

	memcpy(output, utils_buf+UTILS_ENCLAVE_HASH_INDEX,
			UTILS_ENCLAVE_HASH_LEN);

	return 0;
}

