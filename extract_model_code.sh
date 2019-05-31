#!/bin/sh

D=$1
echo "Running on '$D'"

MODEL_NAME=`echo $D | sed 's%/%_%g' | sed 's/models_//'`
echo "Model name: '$MODEL_NAME'"

EXEC_FUNCTION=`grep define "$D"/*.parse.ll | grep -o '[^"]*exec_wrap[^"]*'`
echo "Execute function: $EXEC_FUNCTION"
RUN_FUNCTION=`grep define "$D"/*.parse.ll | grep -o '[^"]*run_wrap[^"]*'`
echo "Run function: $RUN_FUNCTION"

if [ "x$LLVM_PREFIX" == "x" ]; then
	LLVM_PREFIX=/usr/local/llvm-git
fi

LLVM_LINK=$LLVM_PREFIX/bin/llvm-link
LLVM_OPT=$LLVM_PREFIX/bin/opt

# Link all .parse.ll files.
# Inline all functions, and internalize all but the call to exec_*.
$LLVM_LINK "$D"/*.parse.ll | $LLVM_OPT -forceattrs -force-attribute="$EXEC_FUNCTION:alwaysinline" -always-inline -internalize -internalize-public-api-list=$EXEC_FUNCTION,$RUN_FUNCTION -globaldce -stats -S -o "$MODEL_NAME.ll"
