#!/bin/sh

SHARED="debug_info;llvm;const_data"
RM="rm -f"
if [ "x$MODEL_DIR" == "x" ]; then
	MODEL_DIR=$(dirname "$0")/models/
fi

if [ "x$PNL_DIR" == "x" ]; then
	PNL_DIR=~/PsyNeuLink
fi

pushd $PNL_DIR
$RM -v *.ll

### Botvinnick Stroop Effect ###

PNL_LLVM_DEBUG=$SHARED python3 -m pytest -n0 tests/models/test_botvinick.py -k LLVMRun
$RM $MODEL_DIR/botvinick/var-params/*.ll
mkdir -p $MODEL_DIR/botvinick/var-params
mv *.ll $MODEL_DIR/botvinick/var-params

PNL_LLVM_DEBUG="$SHARED;const_params" python3 -m pytest -n0 tests/models/test_botvinick.py -k LLVMRun
$RM $MODEL_DIR/botvinick/const-params/*.ll
mkdir -p $MODEL_DIR/botvinick/const-params
mv *.ll $MODEL_DIR/botvinick/const-params

PNL_LLVM_DEBUG="$SHARED;const_params;const_state" python3 -m pytest -n0 tests/models/test_botvinick.py -k LLVMRun
$RM $MODEL_DIR/botvinick/const-state/*.ll
mkdir -p $MODEL_DIR/botvinick/const-state
mv *.ll $MODEL_DIR/botvinick/const-state

PNL_LLVM_DEBUG="$SHARED;const_params;const_state;const_input=[[[1,0,0]],[[0,1,0]],[[1,0]]]" python3 -m pytest -n0 tests/models/test_botvinick.py -k LLVMRun
$RM $MODEL_DIR/botvinick/const-input/*.ll
mkdir -p $MODEL_DIR/botvinick/const-input
mv *.ll $MODEL_DIR/botvinick/const-input

### Predator-Prey ###

PNL_LLVM_DEBUG=$SHARED python3 -m pytest -n0 tests/models/test_greedy_agent.py -k 'predator_prey and LLVMRun and 2'
$RM $MODEL_DIR/predator-prey/var-params/*.ll
mkdir -p $MODEL_DIR/predator-prey/var-params
mv *.ll $MODEL_DIR/predator-prey/var-params

PNL_LLVM_DEBUG="$SHARED;const_params" python3 -m pytest -n0 tests/models/test_greedy_agent.py -k 'predator_prey and LLVMRun and 2'
$RM $MODEL_DIR/predator-prey/const-params/*.ll
mkdir -p $MODEL_DIR/predator-prey/const-params
mv *.ll $MODEL_DIR/predator-prey/const-params

PNL_LLVM_DEBUG="$SHARED;const_params;const_state" python3 -m pytest -n0 tests/models/test_greedy_agent.py -k 'predator_prey and LLVMRun and 2'
$RM $MODEL_DIR/predator-prey/const-state/*.ll
mkdir -p $MODEL_DIR/predator-prey/const-state
mv *.ll $MODEL_DIR/predator-prey/const-state

PNL_LLVM_DEBUG="$SHARED;const_params;const_state;const_input=[[[1.1576537,  0.60782117]],[[-0.03479106, -0.47666293]],[[-0.60836214,  0.1760381 ]]]" python3 -m pytest -n0 tests/models/test_greedy_agent.py -k 'predator_prey and LLVMRun and 2'
$RM $MODEL_DIR/predator-prey/const-input/*.ll
mkdir -p $MODEL_DIR/predator-prey/const-input
mv *.ll $MODEL_DIR/predator-prey/const-input

### Simplified Necker Cube ###

PNL_LLVM_DEBUG=$SHARED python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_necker and LLVMRun-3'
$RM $MODEL_DIR/simplified-necker-cube/var-params/*.ll
mkdir -p $MODEL_DIR/simplified-necker-cube/var-params
mv *.ll $MODEL_DIR/simplified-necker-cube/var-params

PNL_LLVM_DEBUG="$SHARED;const_params" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_necker and LLVMRun-3'
$RM $MODEL_DIR/simplified-necker-cube/const-params/*.ll
mkdir -p $MODEL_DIR/simplified-necker-cube/const-params
mv *.ll $MODEL_DIR/simplified-necker-cube/const-params

PNL_LLVM_DEBUG="$SHARED;const_params;const_state" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_necker and LLVMRun-3'
$RM $MODEL_DIR/simplified-necker-cube/const-state/*.ll
mkdir -p $MODEL_DIR/simplified-necker-cube/const-state
mv *.ll $MODEL_DIR/simplified-necker-cube/const-state

PNL_LLVM_DEBUG="$SHARED;const_params;const_state;const_input=[[[3127.65559899]], [[3610.74194658]],[[6468.6978669]], [[-4615.15074428]],[[-7369.73302025]], [[-11190.45001744]]]" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_necker and LLVMRun-3'
$RM $MODEL_DIR/simplified-necker-cube/const-input/*.ll
mkdir -p $MODEL_DIR/simplified-necker-cube/const-input
mv *.ll $MODEL_DIR/simplified-necker-cube/const-input

### Full Necker Cube ###

PNL_LLVM_DEBUG=$SHARED python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_necker and LLVMRun-8'
$RM $MODEL_DIR/full-necker-cube/var-params/*.ll
mkdir -p $MODEL_DIR/full-necker-cube/var-params
mv *.ll $MODEL_DIR/full-necker-cube/var-params

PNL_LLVM_DEBUG="$SHARED;const_params" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_necker and LLVMRun-8'
$RM $MODEL_DIR/full-necker-cube/const-params/*.ll
mkdir -p $MODEL_DIR/full-necker-cube/const-params
mv *.ll $MODEL_DIR/full-necker-cube/const-params

PNL_LLVM_DEBUG="$SHARED;const_params;const_state" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_necker and LLVMRun-8'
$RM $MODEL_DIR/full-necker-cube/const-state/*.ll
mkdir -p $MODEL_DIR/full-necker-cube/const-state
mv *.ll $MODEL_DIR/full-necker-cube/const-state

# TODO: Add full necker cube input
#PNL_LLVM_DEBUG="$SHARED;const_params;const_state;const_input=[[[3127.65559899]], [[3610.74194658]],[[6468.6978669]], [[-4615.15074428]],[[-7369.73302025]], [[-11190.45001744]]]" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_necker and LLVMRun-8'
#$RM $MODEL_DIR/full-necker-cube/const-input/*.ll
#mkdir -p $MODEL_DIR/full-necker-cube/const-input
#mv *.ll $MODEL_DIR/full-necker-cube/const-input

### Vectorized Necker Cube ###

PNL_LLVM_DEBUG=$SHARED python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_vectorized_necker and LLVMRun'
$RM $MODEL_DIR/vector-necker-cube/var-params/*.ll
mkdir -p $MODEL_DIR/vector-necker-cube/var-params
mv *.ll $MODEL_DIR/vector-necker-cube/var-params

PNL_LLVM_DEBUG="$SHARED;const_params" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_vectorized_necker and LLVMRun'
$RM $MODEL_DIR/vector-necker-cube/const-params/*.ll
mkdir -p $MODEL_DIR/vector-necker-cube/const-params
mv *.ll $MODEL_DIR/vector-necker-cube/const-params

PNL_LLVM_DEBUG="$SHARED;const_params;const_state" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_vectorized_necker and LLVMRun'
$RM $MODEL_DIR/vector-necker-cube/const-state/*.ll
mkdir -p $MODEL_DIR/vector-necker-cube/const-state
mv *.ll $MODEL_DIR/vector-necker-cube/const-state

# TODO: Add vectorized necker cube input
#PNL_LLVM_DEBUG="$SHARED;const_params;const_state;const_input=[[[3127.65559899]], [[3610.74194658]],[[6468.6978669]], [[-4615.15074428]],[[-7369.73302025]], [[-11190.45001744]]]" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'test_vectorized_necker and LLVMRun'
#$RM $MODEL_DIR/vector-necker-cube/const-input/*.ll
#mkdir -p $MODEL_DIR/vector-necker-cube/const-input
#mv *.ll $MODEL_DIR/vector-necker-cube/const-input

### LCA ###

PNL_LLVM_DEBUG=$SHARED python3 -m pytest -n0 tests/mechanisms/test_lca.py -k 'DDM_equivalent and LLVMRun'
$RM $MODEL_DIR/lca/var-params/*.ll
mkdir -p $MODEL_DIR/lca/var-params
mv *.ll $MODEL_DIR/lca/var-params

PNL_LLVM_DEBUG="$SHARED;const_params" python3 -m pytest -n0 tests/mechanisms/test_lca.py -k 'DDM_equivalent and LLVMRun'
$RM $MODEL_DIR/lca/const-params/*.ll
mkdir -p $MODEL_DIR/lca/const-params
mv *.ll $MODEL_DIR/lca/const-params

PNL_LLVM_DEBUG="$SHARED;const_params;const_state" python3 -m pytest -n0 tests/mechanisms/test_lca.py -k 'DDM_equivalent and LLVMRun'
$RM $MODEL_DIR/lca/const-state/*.ll
mkdir -p $MODEL_DIR/lca/const-state
mv *.ll $MODEL_DIR/lca/const-state

PNL_LLVM_DEBUG="$SHARED;const_params;const_state;const_input=[[[1.0, -1.0]]]" python3 -m pytest -n0 tests/mechanisms/test_lca.py -k 'DDM_equivalent and LLVMRun'
$RM $MODEL_DIR/lca/const-input/*.ll
mkdir -p $MODEL_DIR/lca/const-input
mv *.ll $MODEL_DIR/lca/const-input

popd
