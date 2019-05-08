#!/bin/bash
set -eu

build() {
	echo "Building sources..."
	echo "Params: $SFE_PARAMS"

	make clean > /dev/null 2>&1
	make -j -s > /dev/null 2>&1

	unset SFE_PARAMS
	echo "Build complete!"
}

set_mil_params() {
	PARAM1=$1

	echo "Millionare size $PARAM1"

	export SFE_PARAMS="-DSFE_PARAM_CFLAGS_MIL -Dsfe_mil_size=$PARAM1"
	CONFIG_NAME=mil-$PARAM1
}

deploy_mil_naive() {
	./deploy.sh $CONFIG_NAME-native "-s 0" "-n" mil
}

deploy_mil_normal() {
	./deploy.sh $CONFIG_NAME-normal "-s 0" "" mil
}

###############

set_dj_params() {
	PARAM1=$1

	echo "Dijkstra size $PARAM1"

	export SFE_PARAMS="-DSFE_PARAM_CFLAGS_DJ -Dsfe_dj_Nodes=$PARAM1 -Dsfe_dj_Edges_per_node=4"
	CONFIG_NAME=dj-$PARAM1
}

set_dj_hybrid_params() {
	NODE=$1
	SUPER=$2
	WIR=$3

	echo "Dijkstra hybrid nodes $NODE, supernodes $SUPER, wir $WIR"

	export SFE_PARAMS="-DSFE_PARAM_CFLAGS_DJ_HYBRID -Dyao_dj_Nodes=$NODE -Dyao_dj_SuperNodeSize=$SUPER -Dyao_dj_wir_file='\"$WIR\"'"
	CONFIG_NAME=dj-$NODE-$SUPER
}

deploy_dj_naive() {
	./deploy.sh $CONFIG_NAME-native "-s 2" "-n" dj
}

deploy_dj_normal() {
	./deploy.sh $CONFIG_NAME-normal "-s 2" "" dj
}

deploy_dj_hybrid() {
	./deploy.sh $CONFIG_NAME-hybrid "-y 2" "-y 2" none
}


###############

set_db_params() {
	SIZE=$1
	QR=$2

	echo "DB size $SIZE, queries $QR"

	export SFE_PARAMS="-DSFE_PARAM_CFLAGS_DB -Dsfe_or_queries=$QR -Dsfe_or_sizeOfDatabase=$SIZE"
	CONFIG_NAME=db-$SIZE-$QR
}

set_db_hybrid_params() {
	SIZE=$1
	QR=$2
	WIR=$3
	PERCENT=0.05

	SECRET=$(awk "BEGIN { pc=${PERCENT}*${SIZE}; i=int(pc); print (pc-i<0.5)?i:i+1 }")
	SIZE=$(( $SIZE - $SECRET ))

	echo "DB hybrid size $SIZE (${SECRET}), queries $QR, wir $WIR"

	export SFE_PARAMS="-DSFE_PARAM_CFLAGS_DB_HYBRID -Dyao_or_queries=$QR -Dyao_or_datasize=$SIZE -Dyao_or_secretsize=$SECRET -Dyao_or_wir_file='\"$WIR\"'"
	CONFIG_NAME=db-$1-$QR
}

set_db_hybrid_split_params() {
	SIZE=$1
	QR=$2
	WIR=$3
	PERCENT=0.05

	SECRET=$(awk "BEGIN { pc=${PERCENT}*${QR}; i=int(pc); print (pc-i<0.5)?i:i+1 }")

	echo "DB hybrid split db size $SIZE, queries $QR (${SECRET}), wir $WIR"

	export SFE_PARAMS="-DSFE_PARAM_CFLAGS_DB_HYBRID_SPLIT -Dyao_or_split_queries=$(($QR-$SECRET)) -Dyao_or_split_datasize=$SIZE -Dyao_or_split_secretqueries=$SECRET -Dyao_or_split_wir_file='\"$WIR\"'"
	CONFIG_NAME=db-split-$1-$QR
}

deploy_db_o0() {
	./deploy.sh $CONFIG_NAME-o0 "-s 3" "-o 0" or
}

deploy_db_o1() {
	./deploy.sh $CONFIG_NAME-o1 "-s 3" "-o 1" or
}

deploy_db_o2() {
	./deploy.sh $CONFIG_NAME-o2 "-s 3" "-o 2" or
}

deploy_db_hybrid() {
	./deploy.sh $CONFIG_NAME-hybrid "-y 1" "-y 1" none
}

deploy_db_hybrid_split() {
	./deploy.sh $CONFIG_NAME-hybrid-split "-y 3" "-y 3" none
}

###############


build_mil() {
	# Millionares
	for i in 32 128 512 8192; do
		set_mil_params $i
		build
		deploy_mil_naive
		deploy_mil_normal
		echo
	done
}

build_dj() {
	# Dijkstra
	for i in 20 50 100 200 250 1000 10000; do
		set_dj_params $i
		build
		deploy_dj_naive
		deploy_dj_normal
		echo
	done

	# Hybrid
	set_dj_hybrid_params 20 5 "./yao/TestPrograms/hybriddij20.wir"
	build
	deploy_dj_hybrid
	echo

	set_dj_hybrid_params 50 5 "./yao/TestPrograms/hybriddij20.wir"
	build
	deploy_dj_hybrid
	echo

	set_dj_hybrid_params 100 10 "./yao/TestPrograms/hybriddij25.wir"
	build
	deploy_dj_hybrid
	echo

	set_dj_hybrid_params 200 15 "./yao/TestPrograms/hybriddij50.wir"
	build
	deploy_dj_hybrid
	echo

	set_dj_hybrid_params 250 20 "./yao/TestPrograms/hybriddij100.wir"
	build
	deploy_dj_hybrid
	echo

	set_dj_hybrid_params 1000 25 "./yao/TestPrograms/hybriddij150.wir"
	build
	deploy_dj_hybrid
	echo

	set_dj_hybrid_params 10000 30 "./yao/TestPrograms/hybriddij250.wir"
	build
	deploy_dj_hybrid
	echo
}

build_uc() {
	echo "UC BUILD NOT IMPLEMENTED"
}

build_db() {
	# Database

	# Hybrid
	set_db_hybrid_params 500 2500 "./yao/TestPrograms/smalldb25.wir"
	build
	deploy_db_hybrid
	echo

	set_db_hybrid_params 1000 2500 "./yao/TestPrograms/smalldb50.wir"
	build
	deploy_db_hybrid
	echo

	set_db_hybrid_params 1500 5000 "./yao/TestPrograms/smalldb75.wir"
	build
	deploy_db_hybrid
	echo

	set_db_hybrid_params 5000 5000 "./yao/TestPrograms/smalldb250.wir"
	build
	deploy_db_hybrid
	echo

	set_db_hybrid_params 5000 25000 "./yao/TestPrograms/smalldb250.wir"
	build
	deploy_db_hybrid
	echo

	# Normal
	set_db_params 500 2500
	build
	deploy_db_o0
	deploy_db_o1
	deploy_db_o2
	echo

	set_db_params 1000 2500
	build
	deploy_db_o0
	deploy_db_o1
	deploy_db_o2
	echo

	set_db_params 1500 5000
	build
	deploy_db_o0
	deploy_db_o1
	deploy_db_o2
	echo

	set_db_params 5000 5000
	build
	deploy_db_o0
	deploy_db_o1
	deploy_db_o2
	echo

	set_db_params 5000 25000
	build
	deploy_db_o0
	deploy_db_o1
	deploy_db_o2
	echo
}

build_db_split() {
	# Hybrid DB query split
	set_db_hybrid_split_params 500 2500 "./yao/TestPrograms/hybriddb125-500.wir"
	build
	deploy_db_hybrid_split
	echo

	set_db_hybrid_split_params 1000 2500 "./yao/TestPrograms/hybriddb125-1000.wir"
	build
	deploy_db_hybrid_split
	echo

	set_db_hybrid_split_params 1500 5000 "./yao/TestPrograms/hybriddb250-1500.wir"
	build
	deploy_db_hybrid_split
	echo

	set_db_hybrid_split_params 5000 5000 "./yao/TestPrograms/hybriddb250-5000.wir"
	build
	deploy_db_hybrid_split
	echo

	set_db_hybrid_split_params 5000 25000 "./yao/TestPrograms/hybriddb1250-5000.wir"
	build
	deploy_db_hybrid_split
	echo
}

build_all() {
	build_mil
	build_dj
	build_uc
	build_db
	build_db_split
}

###############

if [ $# -le 0 ]; then
	echo "usage: $0 [mil, dj, uc, db, qs, all]"
	exit 0
fi

TARGET=$1
case $TARGET in
	mil) build_mil;;
	dj)  build_dj;;
	uc)  build_uc;;
	db)  build_db;;
	qs)  build_db_split;;
	all) build_all;;
	*) echo "Unknown build target '$TARGET'"; exit 1 ;;
esac

exit 0
