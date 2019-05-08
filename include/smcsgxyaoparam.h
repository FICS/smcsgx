/*
 * Common header shared by both app and enclave
 * Used to tune values for the hybrid model
 * May 20, 2016
 * daveti
 */
#ifndef _SMC_SGX_YAO_PARAM_H
#define _SMC_SGX_YAO_PARAM_H

#include "smcsgxsfeparam.h"

#define yao_or_NOTPRESENT		0x7FFFFFFF
#define HYBRID_OR_SECRET_LEN		(yao_or_secretsize*sizeof(unsigned int))
#define HYBRID_OR_DATABASE_LEN		(yao_or_datasize*sizeof(unsigned int))
#define HYBRID_OR_QUERY_LEN		(yao_or_queries*sizeof(unsigned int))
#define HYBRID_OR_SGX_OUTPUT_LEN	(yao_or_queries*sizeof(unsigned long))
#define HYBRID_OR_MAGIC_J		10
#define HYBRID_OR_GENGC_LEN		(yao_or_secretsize*2*HYBRID_OR_MAGIC_J)
#define HYBRID_OR_EVLGC_LEN		(yao_or_secretsize*HYBRID_OR_MAGIC_J)
#define HYBRID_OR_PERMUTES_LEN		yao_or_secretsize
#define HYBRID_OR_GEN_OUTPUT_LEN	(HYBRID_OR_SGX_OUTPUT_LEN+HYBRID_OR_EVLGC_LEN+HYBRID_OR_PERMUTES_LEN)
#define HYBRID_OR_GEN_INPUT_FILE	"./input/hybridain.txt"
#define HYBRID_OR_EVL_INPUT_FILE	"./input/hybridbin.txt"
#define HYBRID_OR_CIRCUIT_FILE		yao_or_wir_file
#define HYBRID_OR_YAO_FILE		"./input/inp.txt"

/* for db query split */
#define yao_or_split_NOTPRESENT		0x7FFFFFFFFFFFFFFF
#define HYBRID_OR_SPLIT_SECRET_LEN	(yao_or_split_secretqueries*sizeof(unsigned int))
#define HYBRID_OR_SPLIT_DATABASE_LEN	(yao_or_split_datasize*sizeof(unsigned int))
#define HYBRID_OR_SPLIT_QUERY_LEN	(yao_or_split_queries*sizeof(unsigned int))
#define HYBRID_OR_SPLIT_SGX_OUTPUT_LEN	(yao_or_split_queries*sizeof(unsigned long))
#define HYBRID_OR_SPLIT_MAGIC_J		10
#define HYBRID_OR_SPLIT_GENGC_LEN	(yao_or_split_datasize*64*2*HYBRID_OR_SPLIT_MAGIC_J)
#define HYBRID_OR_SPLIT_EVLGC_LEN	(yao_or_split_datasize*64*HYBRID_OR_SPLIT_MAGIC_J)
#define HYBRID_OR_SPLIT_PERMUTES_LEN	(yao_or_split_datasize*64)
#define HYBRID_OR_SPLIT_GEN_OUTPUT_LEN	(HYBRID_OR_SPLIT_SGX_OUTPUT_LEN+HYBRID_OR_SPLIT_EVLGC_LEN+HYBRID_OR_SPLIT_PERMUTES_LEN)
#define HYBRID_OR_SPLIT_GEN_INPUT_FILE	"./input/hybriddbsplitain.txt"
#define HYBRID_OR_SPLIT_EVL_INPUT_FILE	"./input/hybriddbsplitbin.txt"
#define HYBRID_OR_SPLIT_CIRCUIT_FILE	yao_or_split_wir_file
#define HYBRID_OR_SPLIT_YAO_FILE	"./input/dbsplitinp.txt"

#define yao_dj_Edges_per_node		4
#define yao_dj_MaxIterations		yao_dj_Nodes
#define yao_dj_MaxWeight		0x7FFFFFF
#define yao_dj_dijsgxinput		32
#define HYBRID_DJ_EDGES_NUM		(yao_dj_Nodes*yao_dj_Edges_per_node)
#define HYBRID_DJ_CONNS_NUM		(yao_dj_Nodes*yao_dj_Edges_per_node)
#define HYBRID_DJ_EDGES_LEN		(HYBRID_DJ_EVL_EDGES_NUM*sizeof(int))
#define HYBRID_DJ_CONNS_LEN		(HYBRID_DJ_EVL_CONNS_NUM*sizeof(int))
#define HYBRID_DJ_EVL_INPUT_NUM		2
#define HYBRID_DJ_EVL_INPUT_LEN		(HYBRID_DJ_EVL_INPUT_NUM*sizeof(unsigned int))
#define HYBRID_DJ_GEN_INPUT_NUM		(yao_dj_Nodes*yao_dj_Edges_per_node*2)
#define HYBRID_DJ_GEN_INPUT_LEN		(HYBRID_DJ_GEN_INPUT_NUM*sizeof(unsigned int))
#define HYBRID_DJ_SGX_OUTPUT_LEN	(yao_dj_Nodes*sizeof(int))
#define HYBRID_DJ_MAGIC_J		10
#define HYBRID_DJ_GENGC_LEN		(yao_dj_dijsgxinput*2*HYBRID_DJ_MAGIC_J)
#define HYBRID_DJ_EVLGC_LEN		(yao_dj_dijsgxinput*HYBRID_DJ_MAGIC_J)
#define HYBRID_DJ_PERMUTES_LEN		yao_dj_dijsgxinput
#define HYBRID_DJ_GEN_OUTPUT_LEN	(HYBRID_DJ_EVLGC_LEN+HYBRID_DJ_PERMUTES_LEN)
#define HYBRID_DJ_GEN_INPUT_FILE	"./input/hybriddijain.txt"
#define HYBRID_DJ_EVL_INPUT_FILE	"./input/hybriddijbin.txt"
#define HYBRID_DJ_CIRCUIT_FILE		yao_dj_wir_file
#define HYBRID_DJ_YAO_FILE		"./input/dijinp.txt"

/* daveti: change this for different hybrid cases! */
//#define HYBRID_YAO_BACKEND_FILE	HYBRID_DJ_YAO_FILE
//#define HYBRID_YAO_BACKEND_FILE	HYBRID_OR_YAO_FILE
#define HYBRID_YAO_BACKEND_FILE		HYBRID_OR_SPLIT_YAO_FILE
#define HYBRID_YAO_SERVER_IP		"10.245.44.45" // NOTE: this means SGX evaluator should use this IP!


#endif
