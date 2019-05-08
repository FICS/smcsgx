#ifndef _SMCSGXSFE_H
#define _SMCSGXSFE_H

// To allow for command line input through -D define
#ifndef SFE_PARAM_CFLAGS_MIL

// millionare (-s 0)
#define sfe_mil_size			32

#endif

#ifndef SFE_PARAM_CFLAGS_UC

// universal circuit (-s 1)
#define sfe_uc_gatecount		1000
#define sfe_uc_poolsize			350
#define sfe_uc_numinputs		250
#define sfe_uc_numoutputs		1000

#endif

// we dont need to ever change this
#define sfe_bool_size			sizeof(bool)

#ifndef SFE_PARAM_CFLAGS_DJ

// dijkstra (-s 2)
#define sfe_dj_Nodes			6
#define sfe_dj_Edges_per_node		4

#endif

// we dont need to change these
#define sfe_dj_MaxIterations		sfe_dj_Nodes
#define sfe_dj_MaxWeight		0x7FFFFFF

#ifndef SFE_PARAM_CFLAGS_DB

// database (oram) (-s 3)
#define sfe_or_queries			900
#define sfe_or_sizeOfDatabase		900

#endif

#ifndef SFE_PARAM_CFLAGS_DB_HYBRID

// database hybrid (db protection)
#define yao_or_secretsize		25 // should be 5% of the datasize
#define yao_or_queries			2500
#define yao_or_datasize			(500-yao_or_secretsize)
#define yao_or_wir_file			"./yao/TestPrograms/smalldb25.wir"

#endif

#ifndef SFE_PARAM_CFLAGS_DB_HYBRID_SPLIT
// database hybrid split (query protection)
#define yao_or_split_datasize		100
#define yao_or_split_secretqueries	5 // should be 5% of the total queries
#define yao_or_split_queries            (50-yao_or_split_secretqueries)
#define yao_or_split_wir_file		"./yao/TestPrograms/hybriddb5-100.wir"

#endif

#ifndef SFE_PARAM_CFLAGS_DJ_HYBRID

// dijkstra hybrid
#define yao_dj_Nodes			50
#define yao_dj_SuperNodeSize		5
//#define yao_dj_wir_file			"./yao/TestPrograms/hybriddij20.wir"
//daveti: for dj testing, Jul 28, 2016
#define yao_dj_wir_file			"./yao/TestPrograms/hybridtest.wir"	

#endif

#endif
