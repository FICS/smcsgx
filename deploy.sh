#!/bin/bash
set -eu

template() {
	FILENAME=$1
	NEW_FILENAME=$2
	TEMPLATE=$3
	REPLACE=$4

	if [ $FILENAME = $NEW_FILENAME ]; then
		sed -i 's,'"$TEMPLATE"','"$REPLACE"',' $FILENAME
	else
		sed 's,'"$TEMPLATE"','"$REPLACE"',' $FILENAME > $NEW_FILENAME
	fi
}

FILE_LIST="sender send_enclave.so send_enclave.signed.so evaluator eval_enclave.so \
	eval_enclave.signed.so eval_enclave_hash.hex eval_enclave_signature.hex libyao.so \
	CCS.params input/ send_bm.sh eval_bm.sh sample_libcrypto/libsample_libcrypto.so"

if [ $# -eq 0 ]; then
	read -p "Run configuration name: " archive
	read -p "Sender command line (-s): " sendercmd
	read -p "Evaluator command line (-n, -o): " evalcmd
	read -p "Input code (mil, uc, dj, or): " inputcode
elif [ $# -eq 4 ]; then
	archive=$1
	sendercmd=$2
	evalcmd=$3
	inputcode=$4
else
	echo "usage: $0 [config] [sendercmd] [evalcmd] [inputcode]"
	exit 1
fi

TMPDIR=$(mktemp -d)

clean() {
	rm -rf $TMPDIR
}
trap clean EXIT

SENDER_TEMPLATE_FILE=perf_eval/send_bm.sh.template
EVAL_TEMPLATE_FILE=perf_eval/eval_bm.sh.template

TIME=$(date +"%Y-%m-%d_%H-%M")
ARCHIVE_FULL_NAME=${archive}_$TIME
ARCHIVE=${ARCHIVE_FULL_NAME}.tar.gz
ARCHIVE_PATH=${PWD}/${ARCHIVE_FULL_NAME}.tar.gz
ARCHIVE_DIR=$TMPDIR/${ARCHIVE_FULL_NAME}

case $inputcode in
	mil|uc|dj|or)
		inputcode_evl="-i ./input/sfe_${inputcode}_evl.txt"
		inputcode_gen="-i ./input/sfe_${inputcode}_gen.txt"
		;;
	none) inputcode_evl=""; inputcode_gen="";;
	*) echo "Unknown input code"; exit 1; ;;
esac

echo "Templating files..."
template $SENDER_TEMPLATE_FILE send_bm.sh "@@SEND_ARGS@@" "$sendercmd"
template send_bm.sh send_bm.sh "@@CONFIG_NAME@@" "$archive" -i
template send_bm.sh send_bm.sh "@@INPUT_FILE@@" "$inputcode_gen" -i
chmod +x send_bm.sh

template $EVAL_TEMPLATE_FILE eval_bm.sh "@@EVAL_ARGS@@" "$evalcmd"
template eval_bm.sh eval_bm.sh "@@CONFIG_NAME@@" "$archive" -i
template eval_bm.sh eval_bm.sh "@@INPUT_FILE@@" "$inputcode_evl" -i
chmod +x eval_bm.sh

mkdir -p ${ARCHIVE_DIR}
echo "Copying files..."

for i in ${FILE_LIST}; do
	cp -r $i "${ARCHIVE_DIR}/"
done

mkdir "${ARCHIVE_DIR}/yao/"
cp -r "yao/TestPrograms" "${ARCHIVE_DIR}/yao/"
cp -r "yao/original" "${ARCHIVE_DIR}/yao/"

cat << EOF > ${ARCHIVE_DIR}/deploy-info.txt
Deployed at $(date) by $(whoami) on $(hostname)
Archive name: ${ARCHIVE}
Git hash: $(git rev-parse --short HEAD)

Configuration name: $archive
Sender command line: $sendercmd
Evaluator command line: $evalcmd
Deploy command line: "$archive" "$sendercmd" "$evalcmd" "$inputcode" 
EOF

echo "Creating $ARCHIVE"

pushd $TMPDIR > /dev/null 2>&1
tar czvf $ARCHIVE_PATH ${ARCHIVE_FULL_NAME}
popd > /dev/null 2>&1

echo
echo "******** Archive ready at $ARCHIVE"
echo

cat ${ARCHIVE_DIR}/deploy-info.txt
