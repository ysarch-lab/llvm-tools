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
$LLVM_LINK "$D"/*.parse.ll | $LLVM_OPT -always-inline -internalize -internalize-public-api-list=$EXEC_FUNCTION,$RUN_FUNCTION -globaldce -stats -S -o "$MODEL_NAME.ll"

# Run optimization passes
$LLVM_OPT "${MODEL_NAME}.ll" -stats -S -O3 -disable-simplify-libcalls -sroa -mem2reg | $LLVM_OPT -force-attribute="$EXEC_FUNCTION:alwaysinline" -always-inline -O3 -S -o "${MODEL_NAME}.opt.ll"

$LLVM_OPT "${MODEL_NAME}.opt.ll" -S -stats --allow-unroll-and-jam --loop-unroll --loop-unroll-and-jam --loop-versioning --enable-unroll-and-jam --partial-unrolling-threshold=999999 --unroll-allow-partial --unroll-allow-peeling --unroll-allow-remainder --unroll-count=5 --unroll-runtime --unroll-remainder --unroll-runtime-multi-exit --enable-npm-unroll-and-jam --unroll-force-peel-count=5 -O3 -o "${MODEL_NAME}.opt2.ll"
