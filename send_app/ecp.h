/**
 *
 * Copyright(c) 2011-2016 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and treaty
 * provisions. No part of the Material may be used, copied, reproduced, modified,
 * published, uploaded, posted, transmitted, distributed, or disclosed in any
 * way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel(R) in writing.
 *
 */
#ifndef _ECP_H
#define _ECP_H

#include <stdint.h>
#include <stdbool.h>
#include "../include/smcsgxra.h"

bool derive_key(
	const smcsgx_ec_dh_shared_t *p_shared_key,
	uint8_t key_id,
	smcsgx_ec_key_128bit_t *derived_key);

bool verify_cmac128(
	smcsgx_ec_key_128bit_t mac_key,
	const uint8_t *p_data_buf,
	uint32_t buf_size,
	const uint8_t *p_mac_buf);

int ias_verify_attestation_evidence(
	smcsgx_quote_t *p_isv_quote,
	uint8_t* pse_manifest,
	ias_att_report_t* p_attestation_verification_report);
#endif

