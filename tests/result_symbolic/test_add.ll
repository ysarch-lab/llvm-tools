; RUN: result-symbolic %s -f test -a 0 | FileCheck %s
; ModuleID = 'test_prob_add.ll'
source_filename = "llvm-link"
target triple = "unknown-unknown-unknown"

; CHECK: Running on function: test_add_const_rhs (2 args)
; CHECK-NEXT: Argument 0: double* %out
; CHECK-NEXT: Used in store:  store double %val, double* %out
; CHECK-NEXT: val: 3.0+a

define void @test_add_const_rhs(double* noalias nonnull %out, double* noalias nonnull %in) {
entry:
  %vali = load double, double * %in
  %val = fadd double %vali, 3.000
  store double %val, double* %out
  ret void
}

; CHECK: Running on function: test_add_const_lhs (2 args)
; CHECK-NEXT: Argument 0: double* %out
; CHECK-NEXT: Used in store:  store double %val, double* %out
; CHECK-NEXT: val: -4.0+c

define void @test_add_const_lhs(double* noalias nonnull %out, double* noalias nonnull %in) {
entry:
  %vali = load double, double * %in
  %val = fadd double -4.000, %vali
  store double %val, double* %out
  ret void
}

; CHECK: Running on function: test_add_var (3 args)
; CHECK-NEXT: Argument 0: double* %out
; CHECK-NEXT: Used in store:  store double %val, double* %out
; CHECK-NEXT: val: {{e\+f|f\+e}}

define void @test_add_var(double* noalias nonnull %out, double* noalias nonnull %ina, double* noalias nonnull %inb) {
entry:
  %vala = load double, double * %ina
  %valb = load double, double * %inb
  %val = fadd double %vala, %valb
  store double %val, double* %out
  ret void
}
