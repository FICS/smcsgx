/*
 * send_eval.cpp
 * Main file for the sender 
 * Apr 10, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h> /* TCP_NODELAY */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "trans.h"
#include "mysgx.h"
#include "../include/smcsgx.h"
#include "../include/smcsgxsfeparam.h"
#include "../include/timing.h"

/* Global vars */
static char send_target_ip[SMC_SGX_IP_MAX_LEN];
static int send_target_port = SMC_SGX_DEFAULT_PORT;
static int send_debug;
static int send_stm_ut;
static int send_sgx;
static int send_sfe;
static int send_yao;
static int send_sfe_test;
static int send_yao_test;
static int bypass_sgx = 0;	/* daveti: for debugging purpose */
static smcsgx_msghdr stm_ut_input[SMC_SGX_UT_SEND_CASE_NUM] = {
	{SMC_SGX_MSG_TAG_ATT_S1, SMC_SGX_UT_LEN},
	{SMC_SGX_MSG_TAG_ATT_S3, SMC_SGX_UT_LEN},
	{SMC_SGX_MSG_TAG_RESULT, SMC_SGX_UT_LEN},
};

/* Usage */
static void usage(void)
{
	fprintf(stderr, "\tusage: sender [-a address] [-p num] [-d] [-y num] [-s num] [-i <input file>] [-c <config file> [-h]\n\n");
	fprintf(stderr, "\t-a|--address\tassign the socket IPv4 address\n");
	fprintf(stderr, "\t-p|--port\tassign the socket port\n");
	fprintf(stderr, "\t-d|--debug\tenable debug mode\n");
	fprintf(stderr, "\t-y|--Yao\tuse the Yao hybrid model\n");
	fprintf(stderr, "\t\t\t0: tst (absolete), 1: db, 2: dij, 3: qs\n");
	fprintf(stderr, "\t-s|--SFE\tuse the nth SFE testing case\n");
	fprintf(stderr, "\t\t\t0: mil (default), 1: uc, 2: dij, 3: oram\n");
	fprintf(stderr, "\t-i|--input\tuse certain SFE input file as input\n");
	fprintf(stderr, "\t-c|--config\tpath to configuration file (TBD)\n");
	fprintf(stderr, "\t-h|--help\tdisplay this help message\n");
	fprintf(stderr, "\n");
}

/* UT - internal API */
static void do_ut(void)
{
	int i;

	for (i = 0; i < SMC_SGX_UT_SEND_CASE_NUM; i++)
		trans_main(&stm_ut_input[i]);
}

static bool disable_nagle(int sock_fd)
{
#ifdef DISABLE_NAGLE
	int flag = 1;
        int result = setsockopt(sock_fd,            /* socket affected */
                                IPPROTO_TCP,     /* set option at TCP level */
                                TCP_NODELAY,     /* name of option */
                                (char *) &flag,  /* the cast is historical
                                                        cruft */
                                sizeof(int));    /* length of option value */	

	return result >= 0;
#endif
}

int main(int argc, char **argv)
{
	struct sockaddr_in serv_addr;
	smcsgx_msghdr *msg;
	int c, option_index = 0;
	int sock_fd = 0;
	int len;
	char *buf = NULL;
	char *input_file = NULL;
	struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"debug", 0, NULL, 'd'},
		{"yao", 1, NULL, 'y'},
		{"sfe", 1, NULL, 's'},
		{"address", 1, NULL, 'a'},
		{"port", 1, NULL, 'p'},
		{"input", 1, NULL, 'i'},
		{"config", 1, NULL, 'c'},
		{0, 0, 0, 0}
	};

	timing_start();

	printf("smcsgx: sender pid [%u]\n", getpid());

	/* Print out the build configuration */
	printf("sender: built on %s\n", __DATE__);

	printf("sender: config\n");
	printf("  mil - size %d (%d *32)\n", sfe_mil_size, sfe_mil_size*32);
	printf("   uc - G %d, P %d, I %d, O %d\n", sfe_uc_gatecount,
			sfe_uc_poolsize, sfe_uc_numinputs, sfe_uc_numoutputs);
	printf("   dj - nodes %d (edges %d)\n", sfe_dj_Nodes, sfe_dj_Edges_per_node);
	printf("   db - size %d, queries %d\n", sfe_or_sizeOfDatabase, sfe_or_queries);
	printf("   db-hyb - size %d (%d sec), queries %d, wire %s\n",
		yao_or_datasize, yao_or_secretsize, yao_or_queries, yao_or_wir_file);
	printf("   qs-hyb - size %d, queries %d (%d sec), wire %s\n",
		yao_or_split_datasize, yao_or_split_queries, yao_or_split_secretqueries,
		yao_or_split_wir_file);
        printf("   dj-hyb - node %d, supernode %d, wire %s\n",
		yao_dj_Nodes, yao_dj_SuperNodeSize, yao_dj_wir_file);
#define MAKE_STRING(NAME) "" #NAME ""
#ifdef DISABLE_NAGLE
	printf("   %s=y\n", MAKE_STRING(DISABLE_NAGLE));
#endif
#ifdef SGX_TIMING_ENABLED
	printf("   %s=y\n", MAKE_STRING(SGX_TIMING_ENABLED));
#endif

	/* Process the arguments */
	while ((c = getopt_long(argc, argv, "ha:p:dc:y:s:i:", long_options, &option_index)) != -1) {
		switch (c) {
			case 'a':
				strncpy(send_target_ip, optarg, SMC_SGX_IP_MAX_LEN-1);
				printf("sender: use IP address %s\n", send_target_ip);
				break;
			case 'p':
				send_target_port = strtol(optarg, NULL, 10);
				printf("sender: use port number %d\n", send_target_port);
				break;
			case 'd':
				printf("sender: enable debug\n");
				send_debug = 1;
				break;
			case 'c':
				printf("sender: Warning - may support in future\n");
				break;
			case 'y':
				printf("sender: use Yao hybrid model\n");
				send_yao = 1;
				send_yao_test = strtol(optarg, NULL, 10);
				printf("sender: use YAO testing case #%d\n", send_yao_test);
				break;
			case 's':
				printf("sender: use SGX enhanced model\n");
				send_sfe = 1;
				send_sfe_test = strtol(optarg, NULL, 10);
				printf("sender: use SFE testing case #%d\n", send_sfe_test);
				break;
			case 'i':
				input_file = optarg;
				printf("sender: using input file %s\n", input_file);
				break;
			case 'h':
				/* fall through */
			default:
				usage();
				return -1;
		}
	}

	/* Cannot have YAO and SFE the same time */
	if ((send_yao) && (send_sfe)) {
		printf("sender: cannot have YAO and SFE testings the same time\n");
		return -1;
	}
	if (send_sfe_test >= SMC_SGX_SFE_TEST_NUM_MAX) {
		printf("sender: invalid SFE testing case index\n");
		return -1;
	}
	if (send_yao_test >= SMC_SGX_YAO_TEST_NUM_MAX) {
		printf("sender: invalid YAO testing case index\n");
		return -1;
	}

	timing_tick("getopt", "");

	/* Use local IPv4 address */
	if (send_target_ip[0] == '\0') {
		printf("sender: use local IPv4 address: %s\n",
			SMC_SGX_LOCAL_IP_ADDR);
		snprintf(send_target_ip, SMC_SGX_IP_MAX_LEN, "%s",
			SMC_SGX_LOCAL_IP_ADDR);
	}

	/* STM UT - should be disabled for real socket comm */
	if (send_stm_ut) {
		printf("sender: UT enabled\n");
		goto INIT_TRANS;
	}

	/* Create the TCP socket */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		printf("sender: socket creation failed - %s\n", strerror(errno));
		goto DONE;
	}

	// disable TCP buffering by the kernel. Affects fast runs
	if(!disable_nagle(sock_fd)) {
		printf("sender: failed to disable nagle\n");
		goto DONE;
	}

	/* Build the target address */
	memset(&serv_addr, 0x0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_pton(AF_INET, send_target_ip, &(serv_addr.sin_addr));
	serv_addr.sin_port = htons(send_target_port); 

	timing_tick("socket", "");

	/* Get socket ready */
	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("sender: socket connection failed - %s\n", strerror(errno));
		goto DONE;
	}

	timing_tick("connected", "");

	printf("sender: connected with evaluator\n");

	/* Create recv buf */
	buf = (char *)malloc(SMC_SGX_BUF_LEN);
	if (!buf) {
		printf("sender: mem allocation failed - abort\n");
		goto DONE;
	}

	memset(buf, 0x0, SMC_SGX_BUF_LEN);

INIT_TRANS:
	/* Init the SGX */
	if (!send_stm_ut && !bypass_sgx) {
		if (mysgx_init()) {
			printf("sender: mysgx_init failed - abort\n");
			goto DONE;
		} else {
			send_sgx = 1;
		}
		printf("sender: sgx initialized\n");
	}

	timing_tick("mysgx_init", "");

	/* Init the STM */
	trans_init(sock_fd, send_debug, send_stm_ut,
		send_yao, send_yao_test, send_sfe_test);

	timing_tick("trans_init", "");

	/* Read the input */
	if (input_file || send_yao)
		trans_read_input(input_file);

	timing_tick("trans_read_input", "");

	/* Start the Att */
	printf("sender: start challenge...\n");
	trans_start_att();

	timing_tick("trans_start_att", "");

	/* Start UT */
	if (send_stm_ut) {
		do_ut();
		goto DONE;
	}

	/* Recv */
	while (1) {

		size_t sizeLeft = sizeof(smcsgx_msghdr);
		size_t iter = 0;

		while(1) {
			if(send_debug) {
				printf("send-debug: receiving header %zu bytes (got %zu/%zu already)\n", sizeLeft, iter, iter+sizeLeft);
			}

			len = read(sock_fd, buf+iter, sizeLeft);

			if (len <= 0) {
				printf("sender: connection died during header recv - abort (got %zu bytes, expected %zu)\n", iter, sizeLeft + iter);
				goto DONE;
			}

			sizeLeft -= len;
			iter += len;

			if(sizeLeft == 0) {
				break;
			}
		}

		msg = (smcsgx_msghdr *)buf;

		if(send_debug)
			printf("send-debug: got message tag [%d], len [%d]\n",
				msg->tag, msg->len);

		if(msg->len < 0 || msg->len >= (int)(SMC_SGX_BUF_LEN-sizeof(smcsgx_msghdr)))
		{
			printf("sender: got INSANE message len. Bailing...\n");
			goto DONE;
		}

		iter = 0;
		sizeLeft = msg->len;

		while(1) {
			if(send_debug) {
				printf("send-debug: receiving msg %zu bytes (got %zu/%zu already)\n", sizeLeft, iter, iter+sizeLeft);
			}

			len = read(sock_fd, buf+iter+sizeof(smcsgx_msghdr),
					sizeLeft);

			if (len <= 0) {
				printf("sender: connection died during msg recv - abort (got %zu bytes, expected %zu)\n", iter, sizeLeft + iter);
				goto DONE;
			}

			sizeLeft -= len;
			iter += len;

			if(sizeLeft == 0) {
				break;
			}
		}

		if(send_debug)
			printf("eval-debug: got completed message\n");

		timing_tick("recv", "");

		/* Launch the STM trans */
		trans_main(msg);

		timing_tick("trans_main", "tag [%d], len [%d]",
				msg->tag, msg->len);

		/* Check for quit */
		if (trans_fini())
			break;
	}

DONE:
	printf("sender: cleaning up\n");

	if (buf)
		free(buf);
	if (sock_fd > 0)
		close(sock_fd);
	if (send_sgx)
		mysgx_exit();

	timing_stop();

	return 0;
}
