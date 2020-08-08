; RUN: opt -analyze -scalar-evolution %s | FileCheck %s
; ModuleID = 'loop-f32-range.ll'
source_filename = "loop-f32-range.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; CHECK: Classifying expressions for: @f
; CHECK-NEXT:   %.01 = phi float [ 0.000000e+00, %7 ], [ %12, %10 ]
; CHECK-NEXT:   -->  {0.000000e+00,+,%1}<%8> S: full-set-fp		Exits: ((1.000000e+01 /u %1) * %1) S: [6.66666651, 15] or NaN		LoopDispositions: { %8: Computable }
; CHECK-NEXT:   %.0 = phi float [ 0.000000e+00, %7 ], [ %11, %10 ]
; CHECK-NEXT:   -->  {0.000000e+00,+,1.000000e+00}<%8> S: full-set-fp		Exits: (1.000000e+01 /u %1) S: [3.33333325, 5] or NaN		LoopDispositions: { %8: Computable }
; CHECK-NEXT:   %11 = fadd float %.0, 1.000000e+00
; CHECK-NEXT:   -->  {1.000000e+00,+,1.000000e+00}<%8> S: full-set-fp		Exits: (1.000000e+00 + (1.000000e+01 /u %1)) S: [4.33333302, 6] or NaN		LoopDispositions: { %8: Computable }
; CHECK-NEXT:   %12 = fadd float %.01, %1
; CHECK-NEXT:   -->  {%1,+,%1}<%8> S: full-set-fp		Exits: ((1.000000e+00 + (1.000000e+01 /u %1)) * %1) S: [8.66666603, 18] or NaN		LoopDispositions: { %8: Computable }
; CHECK-NEXT: Determining loop execution counts for: @f
; CHECK-NEXT: Loop %8: backedge-taken count is (1.000000e+01 /u %1)
; CHECK-NEXT: Loop %8: max backedge-taken count is 5
; CHECK-NEXT: Loop %8: min backedge-taken count is 4
; CHECK-NEXT: Loop %8: Predicated backedge-taken count is (1.000000e+01 /u %1)
; CHECK-NEXT:  Predicates:
; CHECK: Loop %8: Trip multiple is 1

; Function Attrs: noinline nounwind uwtable
define dso_local void @f(float* %0, float %1) #0 {
  %3 = fcmp ogt float %1, 3.000000e+00
  br i1 %3, label %6, label %4

4:                                                ; preds = %2
  %5 = fcmp olt float %1, 2.000000e+00
  br i1 %5, label %6, label %7

6:                                                ; preds = %4, %2
  br label %14

7:                                                ; preds = %4
  br label %8

8:                                                ; preds = %10, %7
  %.01 = phi float [ 0.000000e+00, %7 ], [ %12, %10 ]
  %.0 = phi float [ 0.000000e+00, %7 ], [ %11, %10 ]
  %9 = fcmp olt float %.01, 1.000000e+01
  br i1 %9, label %10, label %13

10:                                               ; preds = %8
  %11 = fadd float %.0, 1.000000e+00
  %12 = fadd float %.01, %1
  br label %8

13:                                               ; preds = %8
  store float %.0, float* %0, align 4
  br label %14

14:                                               ; preds = %13, %6
  ret void
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0 "}
