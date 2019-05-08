/*
 * eval_app.cpp
 * Main file for the evaluator
 * Apr 4, 2016
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
#include "eval_enclave_u.h"

/* Global vars */
static int eval_debug;
static int eval_stm_ut;
static int eval_enclave;
static smcsgx_msghdr stm_ut_input[SMC_SGX_UT_EVAL_CASE_NUM] = {
	{SMC_SGX_MSG_TAG_ATT_CHALLENGE, SMC_SGX_UT_LEN},
	{SMC_SGX_MSG_TAG_ATT_S2, SMC_SGX_UT_LEN},
	{SMC_SGX_MSG_TAG_ATT_RESULT, SMC_SGX_UT_LEN},
	{SMC_SGX_MSG_TAG_INPUT, SMC_SGX_UT_LEN},
};

/* Usage */
static void usage(void)
{
	fprintf(stderr, "\tusage: evaluator [-p num] [-d] [-i <input file>] [-y num] [-o version] [-c <config file>] [-n] [-h]\n\n");
	fprintf(stderr, "\t-p|--port\tassign the socket port\n");
	fprintf(stderr, "\t-d|--debug\tenable debug mode\n");
	fprintf(stderr, "\t-i|--input\tuser defined input file\n");
	fprintf(stderr, "\t-y|--YAO\tuse YAO hybrid model\n");
	fprintf(stderr, "\t\t\t0: default, 1: oram/db, 2: dijkstra, 3: query/split\n");
	fprintf(stderr, "\t-o|--oram\tuse oram version for testing\n");
	fprintf(stderr, "\t\t\t0: non-oram, 1: tree-oram, 2: linear-oram\n");
	fprintf(stderr, "\t-n|--naive\tuse the naive version of the program\n");
	fprintf(stderr, "\t-c|--config\tpath to configuration file (TBD)\n");
	fprintf(stderr, "\t-h|--help\tdisplay this help message\n");
	fprintf(stderr, "\n");
}

/* UT - internal API */
static void do_ut(void)
{
	int i;

	for (i = 0; i < SMC_SGX_UT_EVAL_CASE_NUM; i++)
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
	struct sockaddr_in client_addr;
	struct hostent *host;
	smcsgx_msghdr *msg;
	int c, option_index = 0;
	int sock_fd = 0;
	int conn_fd = 0;
	int port = SMC_SGX_DEFAULT_PORT;
	int len;
	char *buf = NULL;
	char *host_addr;
	char *input_file = NULL;
	int oram_version = 0;
	int sfe_naive = 0;
	int hybrid_model = 0;
	int yao_idx = 0;
	struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"debug", 0, NULL, 'd'},
		{"port", 1, NULL, 'p'},
		{"input", 1, NULL, 'i'},
		{"yao", 1, NULL, 'y'},
		{"oram", 1, NULL, 'o'},
		{"naive", 0, NULL, 'n'},
		{"config", 1, NULL, 'c'},
		{0, 0, 0, 0}
	};

	printf("smcsgx: evaluator pid [%u]\n", getpid());

	/* Print out the build configuration */
	printf("evaluator: built on %s\n", __DATE__);

	printf("evaluator: config\n");
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
	while ((c = getopt_long(argc, argv, "hp:dc:i:o:ny:", long_options, &option_index)) != -1) {
		switch (c) {
		case 'p':
			port = strtol(optarg, NULL, 10);
			printf("evaluator: use port number %d\n", port);
			break;
		case 'd':
			printf("evaluator: enable debug\n");
			eval_debug = 1;
			break;
		case 'c':
			printf("evaluator: Warning - may support in future\n");
			break;
		case 'i':
			input_file = optarg;
			printf("evaluator: use input file %s\n", input_file);
			break;
		case 'o':
			oram_version = strtol(optarg, NULL, 10);
			printf("evaluator: use oram version %d\n", oram_version);
			break;
		case 'n':
			printf("evaluator: use the naive version of the program\n");
			sfe_naive = 1;
			break;
		case 'y':
			printf("evaluator: use the YAO hybrid model\n");
			hybrid_model = 1;
			yao_idx = strtol(optarg, NULL, 10);
			printf("evaluator: hybrid testing case %d\n", yao_idx);
			break;
		case 'h':
			/* fall through */
		default:
			usage();
			return -1;
		}
	}

	/* STM UT - should be disabled for real socket comm */
	if (eval_stm_ut) {
		printf("evaluator: UT enabled\n");
		goto INIT_TRANS;
	}

	/* Create the TCP socket */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		printf("evaluator: socket creation failed - %s\n", strerror(errno));
		goto DONE;
	}
	memset(&serv_addr, 0x0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port); 

	{
		int enable = 1;
		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
			printf("evaluator: setting SO_REUSEADDR failed - %s\n", strerror(errno));
			goto DONE;
		}
	}

	/* Get socket ready */
	if (bind(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("evaluator: socket binding failed - %s\n", strerror(errno));
		goto DONE;
	}
	if (listen(sock_fd, 10) < 0) {
		printf("evaluator: socket listen failed - %s\n", strerror(errno));
		goto DONE;
	}
	printf("evaluator: waiting for new connection...\n");

	{
		socklen_t socket_len = sizeof client_addr;
		conn_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &socket_len);
		if (conn_fd < 0) {
			printf("evaluator: socket accept failed - %s\n", strerror(errno));
			goto DONE;
		}

		// disable TCP buffering by the kernel. Affects fast runs
		if(!disable_nagle(conn_fd)) {
			printf("evaluator: failed to disable nagle\n");
			goto DONE;
		}
	}

	/* Dump the client info */
	host = gethostbyaddr((const char *)&client_addr.sin_addr, 
				sizeof(client_addr.sin_addr), AF_INET);
	if (!host)
		printf("evaluator: gethostbyaddr failed - %s\n", strerror(h_errno));
	host_addr = inet_ntoa(client_addr.sin_addr);
	if (!host_addr)
		printf("evaluator: inet_ntoa failed - %s\n", strerror(errno));
	printf("evaluator: established connection with %s (%s)\n", 
		(host ? host->h_name : "UNKNOWN"),
		(host_addr ? host_addr : "UNKNOWN"));

	/* Create recv buf */
	buf = (char *)malloc(SMC_SGX_BUF_LEN);
	if (!buf) {
		printf("evaluator: mem allocation failed - abort\n");
		goto DONE;
	}

	memset(buf, 0x0, SMC_SGX_BUF_LEN);

	/* Init the SGX */
	if (mysgx_init() != SMC_SGX_RTN_SUCCESS) {
		printf("evaluator: mysgx_init failed - abort\n");
		goto DONE;
	}
	eval_enclave = 1;
	printf("evaluator: sgx initialized\n");

INIT_TRANS:
	/* Init the STM */
	trans_init(conn_fd, eval_debug, eval_stm_ut,
		oram_version, sfe_naive, hybrid_model, yao_idx);

	/* Start UT */
	if (eval_stm_ut) {
		do_ut();
		goto DONE;
	}

	/* Read input */
	if (input_file || hybrid_model)
		trans_read_input(input_file);

	/* Recv */
	while (1) {

		size_t sizeLeft = sizeof(smcsgx_msghdr);
		size_t iter = 0;

		while(1) {
			if(eval_debug) {
				printf("eval-debug: receiving header %zu bytes (got %zu/%zu already)\n", sizeLeft, iter, iter+sizeLeft);
			}

			len = read(conn_fd, buf+iter, sizeLeft);

			if (len <= 0) {
				printf("evaluator: connection died during header recv - abort (got %zu bytes, expected %zu)\n", iter, sizeLeft + iter);
				goto DONE;
			}

			sizeLeft -= len;
			iter += len;

			if(sizeLeft == 0) {
				break;
			}
		}

		msg = (smcsgx_msghdr *)buf;

		if(eval_debug)
			printf("eval-debug: got message tag [%d], len [%d]\n",
				msg->tag, msg->len);

		if(msg->len < 0 || msg->len >= (int)(SMC_SGX_BUF_LEN-sizeof(smcsgx_msghdr)))
		{
			printf("evaluator: got INSANE message len. Bailing...\n");
			goto DONE;
		}

		iter = 0;
		sizeLeft = msg->len;

		while(1) {
			if(eval_debug) {
				printf("eval-debug: receiving msg %zu bytes (got %zu/%zu already)\n", sizeLeft, iter, iter+sizeLeft);
			}

			len = read(conn_fd, buf+iter+sizeof(smcsgx_msghdr),
					sizeLeft);

			if (len <= 0) {
				printf("evaluator: connection died during msg recv - abort (got %zu bytes, expected %zu)\n", iter, sizeLeft + iter);
				goto DONE;
			}

			sizeLeft -= len;
			iter += len;

			if(sizeLeft == 0) {
				break;
			}
		}

		if(eval_debug)
			printf("eval-debug: got completed message\n");

		/* Launch the STM trans */
		trans_main(msg);

		/* Check for quit */
		if (trans_fini())
			break;
	}

DONE:
	printf("evaluator: cleaning up\n");

	if (buf)
		free(buf);
	if (conn_fd > 0)
		close(conn_fd);
	if (sock_fd > 0)
		close(sock_fd);
	if (eval_enclave)
		mysgx_exit();
	return 0;
}

/* OCalls */
void debug_string(const char * str)
{
	printf("%s\n", str);
}
