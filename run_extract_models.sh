#!/bin/sh

SHARED="debug_info;llvm;const_data"
RM="rm -f"
MODEL_DIR=~/llvm-tools/models/

if [ "x$PNL_DIR" == "x" ]; then
	PNL_DIR=~/PsyNeuLink
fi

pushd $PNL_DIR
$RM -v *.ll
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

PNL_LLVM_DEBUG=$SHARED python3 -m pytest -n0 tests/models/test_greedy_agent.py -k 'predator_prey and LLVMRun'
$RM $MODEL_DIR/predator-prey/var-params/*.ll
mkdir -p $MODEL_DIR/predator-prey/var-params
mv *.ll $MODEL_DIR/predator-prey/var-params

PNL_LLVM_DEBUG="$SHARED;const_params" python3 -m pytest -n0 tests/models/test_greedy_agent.py -k 'predator_prey and LLVMRun'
$RM $MODEL_DIR/predator-prey/const-params/*.ll
mkdir -p $MODEL_DIR/predator-prey/const-params
mv *.ll $MODEL_DIR/predator-prey/const-params

PNL_LLVM_DEBUG="$SHARED;const_params;const_state" python3 -m pytest -n0 tests/models/test_greedy_agent.py -k 'predator_prey and LLVMRun'
$RM $MODEL_DIR/predator-prey/const-state/*.ll
mkdir -p $MODEL_DIR/predator-prey/const-state
mv *.ll $MODEL_DIR/predator-prey/const-state

PNL_LLVM_DEBUG="$SHARED;const_params;const_state;const_input=[[[1.1576537,  0.60782117]],[[-0.03479106, -0.47666293]],[[-0.60836214,  0.1760381 ]]]" python3 -m pytest -n0 tests/models/test_greedy_agent.py -k 'predator_prey and LLVMRun'
$RM $MODEL_DIR/predator-prey/const-input/*.ll
mkdir -p $MODEL_DIR/predator-prey/const-input
mv *.ll $MODEL_DIR/predator-prey/const-input

PNL_LLVM_DEBUG=$SHARED python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'simplified and LLVMRun'
$RM $MODEL_DIR/bi-percepts/var-params/*.ll
mkdir -p $MODEL_DIR/bi-percepts/var-params
mv *.ll $MODEL_DIR/bi-percepts/var-params

PNL_LLVM_DEBUG="$SHARED;const_params" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'simplified and LLVMRun'
$RM $MODEL_DIR/bi-percepts/const-params/*.ll
mkdir -p $MODEL_DIR/bi-percepts/const-params
mv *.ll $MODEL_DIR/bi-percepts/const-params

PNL_LLVM_DEBUG="$SHARED;const_params;const_state" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'simplified and LLVMRun'
$RM $MODEL_DIR/bi-percepts/const-state/*.ll
mkdir -p $MODEL_DIR/bi-percepts/const-state
mv *.ll $MODEL_DIR/bi-percepts/const-state

PNL_LLVM_DEBUG="$SHARED;const_params;const_state;const_input=[[[3127.65559899]], [[3610.74194658]],[[6468.6978669]], [[-4615.15074428]],[[-7369.73302025]], [[-11190.45001744]]]" python3 -m pytest -n0 tests/models/test_bi_percepts.py -k 'simplified and LLVMRun'
$RM $MODEL_DIR/bi-percepts/const-input/*.ll
mkdir -p $MODEL_DIR/bi-percepts/const-input
mv *.ll $MODEL_DIR/bi-percepts/const-input

popd
