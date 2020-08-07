; RUN: result-range %s -f test -a 1 | FileCheck %s
; ModuleID = 'test_assue.ll'
source_filename = "llvm-link"
target triple = "unknown-unknown-unknown"

declare void @llvm.assume(i1)

; CHECK: Running on function: test_constant_olt
; CHECK: Used in store:  store double %val, double* %out
; CHECK: is known to be in: [-Inf, 0.99999999999999988]
define void @test_constant_olt(double* noalias nonnull %in,double* noalias nonnull %out) {
entry:
  %val = load double, double * %in
  %lt = fcmp olt double %val, 1.0
  call void @llvm.assume(i1 %lt)
  store double %val, double* %out
  ret void
}

; CHECK: Running on function: test_constant_ult
; CHECK: Used in store:  store double %val, double* %out
; CHECK: is known to be in: [-Inf, 0.99999999999999988] or NaN
define void @test_constant_ult(double* noalias nonnull %in,double* noalias nonnull %out) {
entry:
  %val = load double, double * %in
  %lt = fcmp ult double %val, 1.0
  call void @llvm.assume(i1 %lt)
  store double %val, double* %out
  ret void
}

; CHECK: Running on function: test_constant_ueq
; CHECK: Used in store:  store double %val, double* %out
; CHECK: is known to be in: [1, 1]* or NaN
define void @test_constant_ueq(double* noalias nonnull %in,double* noalias nonnull %out) {
entry:
  %val = load double, double * %in
  %lt = fcmp ueq double %val, 1.0
  call void @llvm.assume(i1 %lt)
  store double %val, double* %out
  ret void
}

; CHECK: Running on function: test_constant_oeq
; CHECK: Used in store:  store double %val, double* %out
; CHECK: is known to be in: [1, 1]
define void @test_constant_oeq(double* noalias nonnull %in,double* noalias nonnull %out) {
entry:
  %val = load double, double * %in
  %lt = fcmp oeq double %val, 1.0
  call void @llvm.assume(i1 %lt)
  store double %val, double* %out
  ret void
}

; CHECK: Running on function: test_constant_oeq_0
; CHECK: Used in store:  store double %val, double* %out
; CHECK: is known to be in: [-0, 0]
define void @test_constant_oeq_0(double* noalias nonnull %in,double* noalias nonnull %out) {
entry:
  %val = load double, double * %in
  %lt = fcmp oeq double %val, 0.0
  call void @llvm.assume(i1 %lt)
  store double %val, double* %out
  ret void
}

; CHECK: Running on function: test_constant_oeq_0_nsz
; CHECK: Used in store:  store double %val, double* %out
; CHECK: is known to be in: [0, 0]*
define void @test_constant_oeq_0_nsz(double* noalias nonnull %in,double* noalias nonnull %out) {
entry:
  %val = load double, double * %in
  %lt = fcmp nsz oeq double %val, 0.0
  call void @llvm.assume(i1 %lt)
  store double %val, double* %out
  ret void
}

; CHECK: Running on function: test_constant_i64
; CHECK: Used in store:  store i64 %val, i64* %out
; CHECK: is known to be in: [0,1)
define void @test_constant_i64(i64* noalias nonnull %in,i64* noalias nonnull %out) {
entry:
  %val = load i64, i64 * %in
  %lt = icmp ult i64 %val, 1
  call void @llvm.assume(i1 %lt)
  store i64 %val, i64* %out
  ret void
}
