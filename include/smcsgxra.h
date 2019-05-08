/*
 * smcsgxra.h
 * Header file for SMC SGX Remote Attestation
 * Ref: /opt/intel/sgxsdk/SampleCode/RemoteAttestation/service_provider/ias_ra.h
 * Ref: /opt/intel/sgxsdk/SampleCode/RemoteAttestation/service_provider/ecp.h
 * Ref: /opt/intel/sgxsdk/SampleCode/RemoteAttestation/service_provider/remote_attestation_result.h
 * Ref: /opt/intel/sgxsdk/SampleCode/RemoteAttestation/service_provider/service_provider.h
 * Apr 19, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#ifndef _SMCSGXRA_H
#define _SMCSGXRA_H

#include <stdint.h>

#pragma once

#define SMC_SGX_MAC_SIZE             16  // Message Authentication Code

typedef uint8_t                     smcsgx_mac_t[SMC_SGX_MAC_SIZE];

#define SMC_SGX_FEBITSIZE        256
#define SMC_SGX_NISTP256_KEY_SIZE    (SMC_SGX_FEBITSIZE/ 8 /sizeof(uint32_t))

typedef struct smcsgx_ec_sign256_t
{
	uint32_t x[SMC_SGX_NISTP256_KEY_SIZE];
	uint32_t y[SMC_SGX_NISTP256_KEY_SIZE];
} smcsgx_ec_sign256_t;

#pragma pack(push,1)

#define SMC_SGX_SP_TAG_SIZE          16

typedef struct sp_aes_gcm_data_t {
	uint32_t        payload_size;       //  0: Size of the payload which is
		//     encrypted
	uint8_t         reserved[12];       //  4: Reserved bits
	uint8_t	        payload_tag[SMC_SGX_SP_TAG_SIZE];
		// 16: AES-GMAC of the plain text,
		//     payload, and the sizes
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
	uint8_t         payload[];          // 32: Ciphertext of the payload
		//     followed by the plain text
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} sp_aes_gcm_data_t;


#define ISVSVN_SIZE 2
#define PSDA_SVN_SIZE 4
#define GID_SIZE 4
#define PSVN_SIZE 18

// @TODO: Modify at production to use the values specified by the Production
// IAS API
typedef struct ias_platform_info_blob_t
{
	uint8_t smcsgx_epid_group_status;
	uint16_t smcsgx_tcb_evaluation_status;
	uint16_t pse_evaluation_status;
	uint8_t latest_equivalent_tcb_psvn[PSVN_SIZE];
	uint8_t latest_pse_isvsvn[ISVSVN_SIZE];
	uint8_t latest_psda_svn[PSDA_SVN_SIZE];
	uint8_t performance_rekey_gid[GID_SIZE];
	smcsgx_ec_sign256_t signature;
} ias_platform_info_blob_t;

typedef struct smcsgx_ra_att_result_msg_t {
	int			    result;	// daveti: att verification result
	ias_platform_info_blob_t    platform_info_blob;
	smcsgx_mac_t                mac;    // mac_smk(attestation_status)
	sp_aes_gcm_data_t           secret;
} smcsgx_ra_att_result_msg_t;

#pragma pack(pop)

#define SMC_SGX_ECP_KEY_SIZE                     (SMC_SGX_FEBITSIZE/8)

typedef struct smcsgx_ec_priv_t
{
	uint8_t r[SMC_SGX_ECP_KEY_SIZE];
} smcsgx_ec_priv_t;

typedef struct smcsgx_ec_dh_shared_t
{
	uint8_t s[SMC_SGX_ECP_KEY_SIZE];
}smcsgx_ec_dh_shared_t;

typedef uint8_t smcsgx_ec_key_128bit_t[16];

#define SMC_SGX_EC_MAC_SIZE 16

typedef enum _smcsgx_derive_key_type_t
{
	SMC_SGX_DERIVE_KEY_SMK = 0,
	SMC_SGX_DERIVE_KEY_SK,
	SMC_SGX_DERIVE_KEY_MK,
	SMC_SGX_DERIVE_KEY_VK,
} smcsgx_derive_key_type_t;

// These status should align with the definition in IAS API spec(rev 0.6)
typedef enum {
	IAS_QUOTE_OK,
	IAS_QUOTE_SIGNATURE_INVALID,
	IAS_QUOTE_GROUP_REVOKED,
	IAS_QUOTE_SIGNATURE_REVOKED,
	IAS_QUOTE_KEY_REVOKED,
	IAS_QUOTE_SIGRL_VERSION_MISMATCH,
	IAS_QUOTE_GROUP_OUT_OF_DATE,
	IAS_QUOTE_MEASUREMENT_OR_SIGNATURE_INVLAID,
} ias_quote_status_t;

// These status should align with the definition in IAS API spec(rev 0.6)
typedef enum {
	IAS_PSE_OK,
	IAS_PSE_DESC_TYPE_NOT_SUPPORTED,
	IAS_PSE_ISVSVN_OUT_OF_DATE,
	IAS_PSE_MISCSELECT_INVALID,
	IAS_PSE_ATTRIBUTES_INVALID,
	IAS_PSE_MRSIGNER_INVALID,
	IAS_PS_HW_GID_REVOKED,
	IAS_PS_HW_PRIVKEY_RLVER_MISMATCH,
	IAS_PS_HW_SIG_RLVER_MISMATCH,
	IAS_PS_HW_CA_ID_INVALID,
	IAS_PS_HW_SEC_INFO_INVALID,
	IAS_PS_HW_PSDA_SVN_OUT_OF_DATE,
} ias_pse_status_t;

// Revocation Reasons from RFC5280
typedef enum {
	IAS_REVOC_REASON_NONE,
	IAS_REVOC_REASON_KEY_COMPROMISE,
	IAS_REVOC_REASON_CA_COMPROMISED,
	IAS_REVOC_REASON_SUPERCEDED,
	IAS_REVOC_REASON_CESSATION_OF_OPERATION,
	IAS_REVOC_REASON_CERTIFICATE_HOLD,
	IAS_REVOC_REASON_PRIVILEGE_WITHDRAWN,
	IAS_REVOC_REASON_AA_COMPROMISE,
} ias_revoc_reason_t;

	// These status should align with the definition in IAS API spec(rev 0.6)
#define IAS_EPID_GROUP_STATUS_REVOKED_BIT_POS           0x00
#define IAS_EPID_GROUP_STATUS_REKEY_AVAILABLE_BIT_POS   0x01
#define IAS_TCB_EVAL_STATUS_CPUSVN_OUT_OF_DATE_BIT_POS  0x00
#define IAS_TCB_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS  0x01
#define IAS_PSE_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS  0x00
#define IAS_PSE_EVAL_STATUS_EPID_GROUP_REVOKED_BIT_POS  0x01
#define IAS_PSE_EVAL_STATUS_PSDASVN_OUT_OF_DATE_BIT_POS 0x02
#define IAS_PSE_EVAL_STATUS_SIGRL_OUT_OF_DATE_BIT_POS   0x03
#define IAS_PSE_EVAL_STATUS_PRIVRL_OUT_OF_DATE_BIT_POS  0x04

#define SMC_SGX_HASH_SIZE    32  // SHA256
#define SMC_SGX_MAC_SIZE     16  // Message Authentication Code

#define SMC_SGX_REPORT_DATA_SIZE         64

typedef uint8_t             smcsgx_measurement_t[SMC_SGX_HASH_SIZE];
typedef uint8_t             smcsgx_mac_t[SMC_SGX_MAC_SIZE];
typedef uint8_t             smcsgx_report_data_t[SMC_SGX_REPORT_DATA_SIZE];
typedef uint16_t            smcsgx_prod_id_t;

#define SMC_SGX_CPUSVN_SIZE  16

typedef uint8_t             smcsgx_cpu_svn_t[SMC_SGX_CPUSVN_SIZE];
typedef uint16_t            smcsgx_isv_svn_t;

typedef struct smcsgx_attributes_t
{
	uint64_t                flags;
	uint64_t                xfrm;
} smcsgx_attributes_t;

typedef struct smcsgx_report_body_t {
	smcsgx_cpu_svn_t        cpu_svn;        // (  0) Security Version of the CPU
	uint8_t                 reserved1[32];  // ( 16)
	smcsgx_attributes_t     attributes;     // ( 48) Any special Capabilities
						//       the Enclave possess
	smcsgx_measurement_t    mr_enclave;     // ( 64) The value of the enclave's
						//       ENCLAVE measurement
	uint8_t                 reserved2[32];  // ( 96)
	smcsgx_measurement_t    mr_signer;      // (128) The value of the enclave's
						//       SIGNER measurement
	uint8_t                 reserved3[32];  // (160)
	smcsgx_measurement_t    mr_reserved1;   // (192)
	smcsgx_measurement_t    mr_reserved2;   // (224)
	smcsgx_prod_id_t        isv_prod_id;    // (256) Product ID of the Enclave
	smcsgx_isv_svn_t        isv_svn;        // (258) Security Version of the
						//       Enclave
	uint8_t                 reserved4[60];  // (260)
	smcsgx_report_data_t    report_data;    // (320) Data provided by the user
} smcsgx_report_body_t;

#pragma pack(push, 1)


// This is a context data structure used in SP side
// @TODO: Modify at production to use the values specified by the Production
// IAS API
typedef struct _ias_att_report_t
{
	uint32_t                id;
	ias_quote_status_t      status;
	uint32_t                revocation_reason;
	ias_platform_info_blob_t    info_blob;
	ias_pse_status_t        pse_status;
	uint32_t                policy_report_size;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
	uint8_t                 policy_report[];// IAS_Q: Why does it specify a
						// list of reports?

#ifdef _MSC_VER
#pragma warning(pop)
#endif
} ias_att_report_t;

typedef uint8_t smcsgx_epid_group_id_t[4];

typedef struct smcsgx_spid_t
{
	uint8_t                 id[16];
} smcsgx_spid_t;

typedef struct smcsgx_basename_t
{
	uint8_t                 name[32];
} smcsgx_basename_t;

typedef struct smcsgx_quote_nonce_t
{
	uint8_t                 rand[16];
} smcsgx_quote_nonce_t;

#define SMC_SGX_QUOTE_UNLINKABLE_SIGNATURE 0
#define SMC_SGX_QUOTE_LINKABLE_SIGNATURE   1

typedef struct smcsgx_quote_t {
	uint16_t                version;        // 0
	uint16_t                sign_type;      // 2
	smcsgx_epid_group_id_t  epid_group_id;  // 4
	smcsgx_isv_svn_t        qe_svn;         // 8
	uint8_t                 reserved[6];    // 10
	smcsgx_basename_t       basename;       // 16
	smcsgx_report_body_t    report_body;    // 48
	uint32_t                signature_len;  // 432
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
	uint8_t                 signature[];    // 436
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} smcsgx_quote_t;

#pragma pack(pop)



typedef enum {
	SP_OK,
	SP_INTEGRITY_FAILED,
	SP_QUOTE_VERIFICATION_FAILED,
	SP_IAS_FAILED,
	SP_INTERNAL_ERROR,
	SP_PROTOCOL_ERROR,
} sp_ra_msg_status_t;

#pragma pack(push,1)

#define SMC_SGX_SP_TAG_SIZE       16
#define SMC_SGX_SP_IV_SIZE        12

typedef struct smcsgx_ec_pub_t
{
	uint8_t gx[SMC_SGX_ECP_KEY_SIZE];
	uint8_t gy[SMC_SGX_ECP_KEY_SIZE];
} smcsgx_ec_pub_t;

//fixed length to align with internal structure
typedef struct smcsgx_ps_sec_prop_desc_t
{
	uint8_t  smcsgx_ps_sec_prop_desc[256];
} smcsgx_ps_sec_prop_desc_t;

#pragma pack(pop)

typedef uint32_t                smcsgx_ra_context_t;
typedef uint8_t                 smcsgx_key_128bit_t[16];
typedef smcsgx_key_128bit_t     smcsgx_ra_key_128_t;

typedef struct smcsgx_ra_msg1_t
{
	smcsgx_ec_pub_t             g_a;        // the Endian-ness of Ga is
						// Little-Endian
	smcsgx_epid_group_id_t      gid;        // the Endian-ness of GID is
						// Little-Endian
} smcsgx_ra_msg1_t;

//Key Derivation Function ID : 0x0001  AES-CMAC Entropy Extraction and Key Expansion
const uint16_t SMC_SGX_AES_CMAC_KDF_ID = 0x0001;

typedef struct smcsgx_ra_msg2_t
{
	smcsgx_ec_pub_t             g_b;        // the Endian-ness of Gb is
						// Little-Endian
	smcsgx_spid_t               spid;
	uint16_t                    quote_type;  /* unlinkable Quote(0) or linkable Quote(0) in little endian*/
	uint16_t                    kdf_id;      /* key derivation function id in little endian. 
						0x0001 for AES-CMAC Entropy Extraction and Key Derivation */
	smcsgx_ec_sign256_t         sign_gb_ga; // In little endian
	smcsgx_mac_t                mac;        // mac_smk(g_b||spid||quote_type||
						//         sign_gb_ga)
	uint32_t                    sig_rl_size;
#ifdef _MSC_VER
#pragma warning(push)
// Disable warning that array payload has size 0
#ifdef __INTEL_COMPILER
#pragma warning ( disable:94 )
#else
#pragma warning ( disable: 4200 )
#endif
#endif
	uint8_t                  sig_rl[];
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} smcsgx_ra_msg2_t;

typedef struct smcsgx_ra_msg3_t
{
	smcsgx_mac_t                mac;        // mac_smk(g_a||ps_sec_prop||quote)
	smcsgx_ec_pub_t             g_a;        // the Endian-ness of Ga is
						// Little-Endian
	smcsgx_ps_sec_prop_desc_t   ps_sec_prop;
#ifdef _MSC_VER
#pragma warning(push)
// Disable warning that array payload has size 0
#ifdef __INTEL_COMPILER
#pragma warning ( disable:94 )
#else
#pragma warning ( disable: 4200 )
#endif
#endif
	uint8_t                  quote[];
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} smcsgx_ra_msg3_t;


#if !defined(SWAP_ENDIAN_DW)
    #define SWAP_ENDIAN_DW(dw)	((((dw) & 0x000000ff) << 24)                \
    | (((dw) & 0x0000ff00) << 8)                                            \
    | (((dw) & 0x00ff0000) >> 8)                                            \
    | (((dw) & 0xff000000) >> 24))
#endif
#if !defined(SWAP_ENDIAN_32B)
    #define SWAP_ENDIAN_32B(ptr)                                            \
{\
    unsigned int temp = 0;                                                  \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[0]);                       \
    ((unsigned int*)(ptr))[0] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[7]);  \
    ((unsigned int*)(ptr))[7] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[1]);                       \
    ((unsigned int*)(ptr))[1] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[6]);  \
    ((unsigned int*)(ptr))[6] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[2]);                       \
    ((unsigned int*)(ptr))[2] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[5]);  \
    ((unsigned int*)(ptr))[5] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[3]);                       \
    ((unsigned int*)(ptr))[3] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[4]);  \
    ((unsigned int*)(ptr))[4] = temp;                                       \
}
#endif


#endif
