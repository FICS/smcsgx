/*
 * trans.h
 * STM Transition header
 * Apr 5, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include "../include/smcsgx.h"

void trans_init(int fd, int debug, int ut, int yao, int yao_idx, int sfe);
void trans_main(smcsgx_msghdr *msg);
void trans_start_att(void);
void trans_read_input(char *path);
int trans_fini(void);
