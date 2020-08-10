; RUN: opt -analyze -scalar-evolution %s | FileCheck %s
; ModuleID = 'ddm.ll'
source_filename = "ddm.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: Printing analysis 'Scalar Evolution Analysis' for function 'ddm':
; CHECK-NEXT: Classifying expressions for: @ddm
; CHECK-NEXT:   %.02 = phi float [ %0, %43 ], [ %52, %46 ]
; CHECK-NEXT:   -->  {%0,+,((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5)}<%44> S: full-set-fp		Exits: (((1.000000e+00 + (-1.000000e+00 * %2)) * (((-1.000000e+00 * %0) + %1) /u ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5)) * %3 * %4 * %5) + %0) S: [0.226666629, 36.1000061]		LoopDispositions: { %44: Computable }
; CHECK-NEXT:   %.01 = phi float [ 0.000000e+00, %43 ], [ %47, %46 ]
; CHECK-NEXT:   -->  {0.000000e+00,+,1.000000e+00}<%44> S: full-set-fp		Exits: (((-1.000000e+00 * %0) + %1) /u ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5)) S: [25.3333302, 480.000031]		LoopDispositions: { %44: Computable }
; CHECK-NEXT:   %47 = fadd float %.01, 1.000000e+00
; CHECK-NEXT:   -->  {1.000000e+00,+,1.000000e+00}<%44> S: full-set-fp		Exits: (1.000000e+00 + (((-1.000000e+00 * %0) + %1) /u ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5))) S: [26.3333302, 481.000031]		LoopDispositions: { %44: Computable }
; CHECK-NEXT:   %48 = fsub float 1.000000e+00, %2
; CHECK-NEXT:   -->  (1.000000e+00 + (-1.000000e+00 * %2)) S: [0.5, 0.5]*		Exits: (1.000000e+00 + (-1.000000e+00 * %2)) S: [0.5, 0.5]*		LoopDispositions: { %44: Invariant }
; CHECK-NEXT:   %49 = fmul float %48, %3
; CHECK-NEXT:   -->  ((1.000000e+00 + (-1.000000e+00 * %2)) * %3) S: [0.100000001, 0.75]	Exits: ((1.000000e+00 + (-1.000000e+00 * %2)) * %3) S: [0.100000001, 0.75]	LoopDispositions: { %44: Invariant }
; CHECK-NEXT:   %50 = fmul float %49, %4
; CHECK-NEXT:   -->  ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4) S: [0.0500000007, 0.75]	Exits: ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4) S: [0.0500000007, 0.75]	LoopDispositions: { %44: Invariant }
; CHECK-NEXT:   %51 = fmul float %50, %5
; CHECK-NEXT:   -->  ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5) S: [0.00499999989, 0.075000003]		Exits: ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5) S: [0.00499999989, 0.075000003]		LoopDispositions: { %44: Invariant }
; CHECK-NEXT:   %52 = fadd float %.02, %51
; CHECK-NEXT:   -->  {(((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5) + %0),+,((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5)}<%44> S: full-set-fp		Exits: (((1.000000e+00 + (-1.000000e+00 * %2)) * (1.000000e+00 + (((-1.000000e+00 * %0) + %1) /u ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5))) * %3 * %4 * %5) + %0) S: [0.231666639, 36.1750069]		LoopDispositions: { %44: Computable }
; CHECK-NEXT:   %.0 = phi float [ %.01, %53 ], [ -1.000000e+00, %41 ], [ -1.000000e+00, %35 ], [ -1.000000e+00, %29 ], [ -1.000000e+00, %23 ], [ -1.000000e+00, %17 ], [ -1.000000e+00, %11 ]
; CHECK-NEXT:   -->  %.0 S: full-set-fp
; CHECK-NEXT: Determining loop execution counts for: @ddm
; CHECK-NEXT: Loop %44: backedge-taken count is (((-1.000000e+00 * %0) + %1) /u ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5))
; CHECK-NEXT: Loop %44: max backedge-taken count is 481
; CHECK-NEXT: Loop %44: min backedge-taken count is 26
; CHECK-NEXT: Loop %44: Predicated backedge-taken count is (((-1.000000e+00 * %0) + %1) /u ((1.000000e+00 + (-1.000000e+00 * %2)) * %3 * %4 * %5))


; Function Attrs: noinline nounwind uwtable
define dso_local float @ddm(float %0, float %1, float %2, float %3, float %4, float %5, float* %6) #0 {
  %8 = fcmp ole float %0, 0x3FB99999A0000000
  br i1 %8, label %9, label %11

9:                                                ; preds = %7
  %10 = fcmp oge float %0, 0x3FB99999A0000000
  br i1 %10, label %12, label %11

11:                                               ; preds = %9, %7
  br label %54

12:                                               ; preds = %9
  br label %13

13:                                               ; preds = %12
  %14 = fcmp ole float %1, 2.500000e+00
  br i1 %14, label %15, label %17

15:                                               ; preds = %13
  %16 = fcmp oge float %1, 2.000000e+00
  br i1 %16, label %18, label %17

17:                                               ; preds = %15, %13
  br label %54

18:                                               ; preds = %15
  br label %19

19:                                               ; preds = %18
  %20 = fcmp ole float %4, 1.000000e+00
  br i1 %20, label %21, label %23

21:                                               ; preds = %19
  %22 = fcmp oge float %4, 5.000000e-01
  br i1 %22, label %24, label %23

23:                                               ; preds = %21, %19
  br label %54

24:                                               ; preds = %21
  br label %25

25:                                               ; preds = %24
  %26 = fcmp ole float %2, 5.000000e-01
  br i1 %26, label %27, label %29

27:                                               ; preds = %25
  %28 = fcmp oge float %2, 5.000000e-01
  br i1 %28, label %30, label %29

29:                                               ; preds = %27, %25
  br label %54

30:                                               ; preds = %27
  br label %31

31:                                               ; preds = %30
  %32 = fcmp ole float %5, 0x3FB99999A0000000
  br i1 %32, label %33, label %35

33:                                               ; preds = %31
  %34 = fcmp oge float %5, 0x3FB99999A0000000
  br i1 %34, label %36, label %35

35:                                               ; preds = %33, %31
  br label %54

36:                                               ; preds = %33
  br label %37

37:                                               ; preds = %36
  %38 = fcmp ole float %3, 1.500000e+00
  br i1 %38, label %39, label %41

39:                                               ; preds = %37
  %40 = fcmp oge float %3, 0x3FC99999A0000000
  br i1 %40, label %42, label %41

41:                                               ; preds = %39, %37
  br label %54

42:                                               ; preds = %39
  br label %43

43:                                               ; preds = %42
  br label %44

44:                                               ; preds = %46, %43
  %.02 = phi float [ %0, %43 ], [ %52, %46 ]
  %.01 = phi float [ 0.000000e+00, %43 ], [ %47, %46 ]
  %45 = fcmp olt float %.02, %1
  br i1 %45, label %46, label %53

46:                                               ; preds = %44
  %47 = fadd float %.01, 1.000000e+00
  %48 = fsub float 1.000000e+00, %2
  %49 = fmul float %48, %3
  %50 = fmul float %49, %4
  %51 = fmul float %50, %5
  %52 = fadd float %.02, %51
  br label %44

53:                                               ; preds = %44
  store float %.01, float* %6, align 4
  br label %54

54:                                               ; preds = %53, %41, %35, %29, %23, %17, %11
  %.0 = phi float [ %.01, %53 ], [ -1.000000e+00, %41 ], [ -1.000000e+00, %35 ], [ -1.000000e+00, %29 ], [ -1.000000e+00, %23 ], [ -1.000000e+00, %17 ], [ -1.000000e+00, %11 ]
  ret float %.0
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0 (Fedora 10.0.0-2.fc32)"}
