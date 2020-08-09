; RUN: result-symbolic %s -f test -a 0 | FileCheck %s
; ModuleID = 'test_prob_add.ll'
source_filename = "llvm-link"
target triple = "unknown-unknown-unknown"

; CHECK: Running on function: test_load (2 args)
; CHECK-NEXT: Argument 0: double* %out
; CHECK-NEXT: Used in store:  store double %val, double* %out
; CHECK-NEXT: val: a

define void @test_load(double* noalias nonnull %out, double* noalias nonnull %in) {
entry:
  %val = load double, double * %in
  store double %val, double* %out
  ret void
}
