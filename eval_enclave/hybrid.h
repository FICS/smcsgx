/*
 * Header for the hybrid model
 * May 20, 2016
 * daveti
 */
#ifndef _HYBRID_H
#define _HYBRID_H

void yao_or_init_db(unsigned int *input);
void yao_or_get_sgx_output(unsigned long *output, unsigned int *queries);
void yao_or_get_bit_input(char *input);
int yao_or_gen_program_input(char *gengc, char *evlgc, char *permutes);
void yao_dj_entry(unsigned int *alicein, unsigned int *bobin, int *sgxout,
		char *gengc, char *evlgc, char *permutes);
void yao_or_split_entry(unsigned int *db,
                        unsigned long *output,
                        unsigned int *queries,
                        char *gengc, char *evlgc, char *permutes);
#endif
