/*
 * Header for SFE naive program
 * May 16, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#ifndef _MYSFE_NAIVE_H
#define _MYSFE_NAIVE_H

#include "mysfe.h"

void sfe_naive_entry(unsigned int alice[sfe_mil_size],
		unsigned int bob[sfe_mil_size],
		unsigned int *alice_out, unsigned int *bob_out);

void sfe_naive_uc_entry(sfe_uc_input_program *prog,
		bool input[sfe_uc_numinputs],
		bool output[sfe_uc_numoutputs]);

void sfe_naive_dj_entry(sfe_dj_input_eval *evl_input,
		sfe_dj_input_send *gen_input,
		int output[sfe_dj_Nodes]);

#endif
