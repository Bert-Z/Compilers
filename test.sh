#!/bin/bash
#============some output color
SYS=$(uname -s)
if [[ $SYS == "Linux" ]]; then
    RED_COLOR='\E[1;31m'
    GREEN_COLOR='\E[1;32m'
    YELOW_COLOR='\E[1;33m'
    BLUE_COLOR='\E[1;34m'
    PINK='\E[1;35m'
    RES='\E[0m'
fi

BIN=tiger-compiler
PROJDIR=lab5
TESTCASEDIR=testcases
RUNTIMEPATH=../src/tiger/runtime/runtime.c
MERGECASEDIR=testcases/merge
REFOUTDIR=refs
MERGEREFDIR=refs/merge
DIFFOPTION="-w -B"
score=0

base_name=$(basename "$PWD")
if [[ ! $base_name =~ "tiger-compiler" ]]; then
    echo "[-_-]: Not in Lab Root Dir"
    echo "SCORE: 0"
    exit 1
fi

mkdir -p build
cd build
rm -f testcases refs _tmp.txt .tmp.txt __tmp.txt _ref.txt
ln -s ../testdata/$PROJDIR/testcases testcases
if [[ $? != 0 ]]; then
    echo "[-_-]$ite: Link Error"
    echo "TOTAL SCORE: 0"
    exit 123
fi

ln -s ../testdata/$PROJDIR/refs refs
if [[ $? != 0 ]]; then
    echo "[-_-]$ite: Link Error"
    echo "TOTAL SCORE: 0"
    exit 123
fi

cmake .. >&/dev/null
make ${BIN} -j >/dev/null

#echo $?
if [[ $? != 0 ]]; then
    echo -e "${RED_COLOR}[-_-]$ite: Compile Error${RES}"
    make clean >&/dev/null
    exit 123
fi
for tcase in $(ls $TESTCASEDIR/); do
    if [ ${tcase##*.} = "tig" ]; then
        tfileName=${tcase##*/}
        ./$BIN $TESTCASEDIR/$tfileName &> $TESTCASEDIR/s/${tfileName}.s
        # gcc -Wl,--wrap,getchar -m64 $TESTCASEDIR/${tfileName}.s $RUNTIMEPATH -o test.out &>/dev/null
    fi
done

