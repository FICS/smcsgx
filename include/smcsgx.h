/*
 * smcsgx.h
 * A common header file shared by all SMC SGX participants
 * Apr 4, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#ifndef _SMCSGX_H
#define _SMCSGX_H

/* Tags */
#define SMC_SGX_MSG_TAG_ATT_CHALLENGE		0
#define SMC_SGX_MSG_TAG_ATT_S1			1
#define SMC_SGX_MSG_TAG_ATT_S2			2
#define SMC_SGX_MSG_TAG_ATT_S3			3
#define SMC_SGX_MSG_TAG_ATT_RESULT		4
#define SMC_SGX_MSG_TAG_INPUT			5
#define SMC_SGX_MSG_TAG_RESULT			6
#define SMC_SGX_MSG_TAG_YAO_INPUT		7	/* Only used by Yao hybrid model */
#define SMC_SGX_MSG_TAG_ANY			9
#define SMC_SGX_MSG_TAG_STUB			10
#define SMC_SGX_MSG_TAG_FINI			11	/* max number of tags */
#define SMC_SGX_MSG_TAG_NUM_MAX			(SMC_SGX_MSG_TAG_FINI+1)

/* STMs */
#define SMC_SGX_STM_INIT			0	/* used by all */
#define SMC_SGX_STM_ATT_STARTED			1	/* used by the sender */
#define SMC_SGX_STM_ATT_S1_RECVD		2	/* used by the sender */
#define SMC_SGX_STM_ATT_S2_SENT			3	/* used by the sender */
#define SMC_SGX_STM_ATT_S3_RECVD		4	/* used by the sender */
#define SMC_SGX_STM_ATT_RES_SENT		5	/* used by the sender */
#define SMC_SGX_STM_INPUT_SENT			6	/* used by the sender */
#define SMC_SGX_STM_RES_RECVD			7	/* used by the sender */
#define SMC_SGX_STM_ATT_RECVD			8	/* used by the evaluator */
#define SMC_SGX_STM_ATT_S1_SENT			9	/* used by the evaluator */
#define SMC_SGX_STM_ATT_S2_RECVD		10	/* used by the evaluator */
#define SMC_SGX_STM_ATT_S3_SENT			11	/* used by the evaluator */
#define SMC_SGX_STM_ATT_RES_RECVD		12	/* used by the evaluator */
#define SMC_SGX_STM_INPUT_WAIT			13	/* used by the evaluator */
#define SMC_SGX_STM_INPUT_RECVD			14	/* used by the evaluator */
#define SMC_SGX_STM_RES_SENT			15	/* used by the evaluator */
#define SMC_SGX_STM_ERROR                       17      /* used by all */
#define SMC_SGX_STM_ANY                         18      /* used by all */
#define SMC_SGX_STM_FINI			19	/* max number of states */
#define SMC_SGX_STM_NUM_MAX			(SMC_SGX_STM_FINI+1)

#define MAKE_STRING(NAME) "" #NAME ""

static const char * smc_sgx_stm_name[] = {
	MAKE_STRING(SMC_SGX_STM_INIT),
	MAKE_STRING(SMC_SGX_STM_ATT_STARTED),
	MAKE_STRING(SMC_SGX_STM_ATT_S1_RECVD),
	MAKE_STRING(SMC_SGX_STM_ATT_S2_SENT),
	MAKE_STRING(SMC_SGX_STM_ATT_S3_RECVD),
	MAKE_STRING(SMC_SGX_STM_ATT_RES_SENT),
	MAKE_STRING(SMC_SGX_STM_INPUT_SENT),
	MAKE_STRING(SMC_SGX_STM_RES_RECVD),
	MAKE_STRING(SMC_SGX_STM_ATT_RECVD),
	MAKE_STRING(SMC_SGX_STM_ATT_S1_SENT),
	MAKE_STRING(SMC_SGX_STM_ATT_S2_RECVD),
	MAKE_STRING(SMC_SGX_STM_ATT_S3_SENT),
	MAKE_STRING(SMC_SGX_STM_ATT_RES_RECVD),
	MAKE_STRING(SMC_SGX_STM_INPUT_WAIT),
	MAKE_STRING(SMC_SGX_STM_INPUT_RECVD),
	MAKE_STRING(SMC_SGX_STM_RES_SENT),
	MAKE_STRING(SMC_SGX_STM_ERROR),
	MAKE_STRING(SMC_SGX_STM_ANY),
	MAKE_STRING(SMC_SGX_STM_FINI)
};

#undef MAKE_STRING

/* Others */
#define SMC_SGX_IP_MAX_LEN			sizeof("255.255.255.255")
#define SMC_SGX_BUF_LEN				1024*1024*10
#define SMC_SGX_KEY_LEN				2048
#define SMC_SGX_NONCE_LEN			20
#define SMC_SGX_SECRET_LEN_MAX			16
#define SMC_SGX_SECRET_LEN_NOW			8	/* daveti: current value in use */
#define SMC_SGX_DEFAULT_PORT			55555
#define SMC_SGX_RTN_SUCCESS			0
#define SMC_SGX_RTN_FAILURE			-1
#define SMC_SGX_ATT_RES_GOOD			0
#define SMC_SGX_ATT_RES_BAD			1
#define SMC_SGX_UT_LEN				777	/* daveti likes this number */
#define SMC_SGX_UT_EVAL_CASE_NUM		4
#define SMC_SGX_UT_SEND_CASE_NUM		3
#define SMC_SGX_LOCAL_IP_ADDR			"127.0.0.1"
#define SMC_SGX_INPUT_MSG_YAO			"YAODF"
#define SMC_SGX_INPUT_MSG_YAO_OR		"YAOOR"
#define SMC_SGX_INPUT_MSG_YAO_DJ		"YAODJ"
#define SMC_SGX_INPUT_MSG_YAO_OR_SPLIT		"YAOQS"
#define SMC_SGX_INPUT_MSG_SFE			"SFEMIL"
#define SMC_SGX_INPUT_MSG_SFE_UC		"SFEUC"
#define SMC_SGX_INPUT_MSG_SFE_DJ		"SFEDJ"
#define SMC_SGX_INPUT_MSG_SFE_OR		"SFEOR"
#define SMC_SGX_INPUT_LEN_MAX			1024*1024	/* ORAM case requires at least 3K */
#define SMC_SGX_SHARED_SECRET_LEN		16	/* daveti: needed for secure comm using the shared key */
#define SMC_SGX_EVAL_ENCLAVE_HASH_FILE		"eval_enclave_hash.hex"	/* Generated automatically by make */
#define SMC_SGX_SHA256_LEN			32
#define SMC_SGX_SFE_TEST_NUM_MAX		4
#define SMC_SGX_YAO_TEST_NUM_MAX		4
#define SMC_SGX_MBM_SEC_IN_USEC			1000000         /* micro benchmark */
#define SMC_SGX_MBM_SUB_TV(s, e)		\
	((e.tv_sec*SMC_SGX_MBM_SEC_IN_USEC+e.tv_usec) - \
	(s.tv_sec*SMC_SGX_MBM_SEC_IN_USEC+s.tv_usec))

/* Enclave file names */
#if defined(_MSC_VER)
#define SMC_SGX_EVAL_TOKEN_FILENAME		"Eval_enclave.token"
#define SMC_SGX_EVAL_ENCLAVE_FILENAME		"Eval_enclave.signed.dll"
#define SMC_SGX_SEND_TOKEN_FILENAME		"Send_enclave.token"
#define SMC_SGX_SEND_ENCLAVE_FILENAME		"Send_enclave.signed.dll"
#elif defined(__GNUC__)
#define SMC_SGX_EVAL_TOKEN_FILENAME		"eval_enclave.token"
#define SMC_SGX_EVAL_ENCLAVE_FILENAME		"eval_enclave.signed.so"
#define SMC_SGX_SEND_TOKEN_FILENAME		"send_enclave.token"
#define SMC_SGX_SEND_ENCLAVE_FILENAME		"send_enclave.signed.so"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


/* Socket msg header */
typedef struct _smcsgx_msghdr {
	int	tag;
	int	len;
	char	val[0];
} smcsgx_msghdr;

/* STM helpers */
typedef int (*trans_func) (int, char *);
typedef struct _smcsgx_stm_trans {
	int		current_state;
	int		event;
	int		new_state;
	trans_func	transition;
} smcsgx_stm_trans;
typedef struct _smcsgx_stm_trans_entry {
	int		new_state;
	trans_func	transition;
} smcsgx_stm_trans_entry;

/* STM debugging function */
const char * smcsgx_stm_name(int state);

/* Common helpers */
int smcsgx_read_input(char *path, char *buf, int buf_len);

#endif
