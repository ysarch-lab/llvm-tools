#!/bin/sh

D=$1
echo "Running on '$D'"

MODEL_NAME=`echo $D | sed 's%/%_%g' | sed 's/models_//'`
echo "Model name: '$MODEL_NAME'"

EXEC_FUNCTION=`grep define "$D"/*.parse.ll | grep -o '[^"]*exec_wrap[^"]*'`
echo "Execute function: $EXEC_FUNCTION"

RUN_FUNCTION=`grep define "$D"/*.parse.ll | grep -o '[^"]*run_wrap[^"]*'`
echo "Run function: $RUN_FUNCTION"

EXEC_SIM_FUNCTION=`grep define "$D"/*.parse.ll | grep -o '[^"]*exec_sim_wrap[^"]*'`
echo "Execute simulation function: $EXEC_SIM_FUNCTION"

RUN_SIM_FUNCTION=`grep define "$D"/*.parse.ll | grep -o '[^"]*run_sim_wrap[^"]*'`
echo "Run simulation function: $RUN_SIM_FUNCTION"

EVALUATE_FUNCTION=`grep define "$D"/*.parse.ll | grep -o '[^"]*eval[^"]*'`
echo "Evaluate function: $EVALUATE_FUNCTION"

if [ "x$LLVM_PREFIX" == "x" ]; then
	LLVM_PREFIX=`grep LLVM_DIR CMakeCache.txt | sed 's%.*=\(.*\)cmake/llvm%\1/../%'`
fi

LLVM_LINK=$LLVM_PREFIX/bin/llvm-link
LLVM_OPT=$LLVM_PREFIX/bin/opt

# Link all .parse.ll files.
# Internalize and eliminate all but exec_*.
$LLVM_LINK "$D"/*.parse.ll | $LLVM_OPT -internalize -internalize-public-api-list=$EXEC_FUNCTION,$RUN_FUNCTION,$EXEC_SIM_FUNCTION,$RUN_SIM_FUNCTION,$EVALUATE_FUNCTION -globaldce -stats -S -o "$MODEL_NAME.ll"

VARIANT=$(basename "$D")
if [ "x$VARIANT" == "xconst-input" ]; then
	RANGE_PARAM=$(grep -o 'raw_[^"]*' "$MODEL_NAME.ll" | sort -u | grep offset | head -n1)
	echo "Selected param: $RANGE_PARAM"
	cp $MODEL_NAME.ll $MODEL_NAME.pre-restrict.ll
	./param-restrict -p "$RANGE_PARAM" -l 0 -u 1 $MODEL_NAME.ll
fi


# Run optimization passes
# First try to get rid of alloca-s then inline everything other than PRNGs
$LLVM_OPT "${MODEL_NAME}.ll" -stats -S -sroa -mem2reg | $LLVM_OPT -force-attribute="$EXEC_FUNCTION:alwaysinline" -force-attribute="$EXEC_SIM_FUNCTION:alwaysinline" -force-attribute="$RUN_SIM_FUNCTION:alwaysinline"  -force-attribute="__pnl_builting_mt_rand_normal:noinline" -force-attribute="__pnl_builtin_mt_rand_double:noinline"  -always-inline -inline -O3 -disable-simplify-libcalls -sroa -mem2reg -S -o "${MODEL_NAME}.opt.ll"

$LLVM_OPT "${MODEL_NAME}.opt.ll" -S -stats --allow-unroll-and-jam --loop-unroll --loop-unroll-and-jam --loop-versioning --enable-unroll-and-jam --partial-unrolling-threshold=999999 --unroll-allow-partial --unroll-allow-peeling --unroll-allow-remainder --unroll-count=5 --unroll-runtime --unroll-remainder --unroll-runtime-multi-exit --enable-npm-unroll-and-jam --unroll-force-peel-count=5 --enable-gvn-memdep --enable-gvn-sink --gvn-hoist-max-bbs=-1 --gvn-hoist-max-chain-length=-1 --gvn-hoist-max-depth=-1 --gvn-max-num-deps=1000000 --gvn-max-recurse-depth=10000000 -O3 -disable-simplify-libcalls -o - | \
$LLVM_OPT -stats -O3 -disable-simplify-libcalls --enable-gvn-sink --gvn-hoist-max-bbs=-1 --gvn-hoist-max-chain-length=-1 --gvn-hoist-max-depth=-1 --gvn-max-num-deps=1000000 --gvn-max-recurse-depth=10000000 -S -o "${MODEL_NAME}.opt2.ll"

# Scalarize for easier analysis
$LLVM_OPT --scalarizer --scalarize-load-store -S -O1 -disable-simplify-libcalls "${MODEL_NAME}.opt2.ll"  -o "${MODEL_NAME}.opt2.scalarized.ll"
