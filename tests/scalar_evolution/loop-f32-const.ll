; RUN: opt -analyze -scalar-evolution %s | FileCheck %s
; ModuleID = 'loop-f32-const.ll'
source_filename = "loop-f32-const.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; CHECK: Classifying expressions for: @f
; CHECK-NEXT:   %.01 = phi float [ 0.000000e+00, %1 ], [ %6, %4 ]
; CHECK-NEXT:   -->  {0.000000e+00,+,2.500000e+00}<%2> S: full-set-fp		Exits: 1.000000e+01 S: [10, 10]*		LoopDispositions: { %2: Computable }
; CHECK-NEXT:   %.0 = phi float [ 0.000000e+00, %1 ], [ %5, %4 ]
; CHECK-NEXT:   -->  {0.000000e+00,+,1.000000e+00}<%2> S: full-set-fp		Exits: 4.000000e+00 S: [4, 4]*		LoopDispositions: { %2: Computable }
; CHECK-NEXT:   %5 = fadd float %.0, 1.000000e+00
; CHECK-NEXT:   -->  {1.000000e+00,+,1.000000e+00}<%2> S: full-set-fp		Exits: 5.000000e+00 S: [5, 5]*		LoopDispositions: { %2: Computable }
; CHECK-NEXT:   %6 = fadd float %.01, 2.500000e+00
; CHECK-NEXT:   -->  {2.500000e+00,+,2.500000e+00}<%2> S: full-set-fp		Exits: 1.250000e+01 S: [12.5, 12.5]*		LoopDispositions: { %2: Computable }
; CHECK-NEXT: Determining loop execution counts for: @f
; CHECK-NEXT: Loop %2: backedge-taken count is 4
; CHECK-NEXT: Loop %2: max backedge-taken count is 4
; CHECK-NEXT: Loop %2: min backedge-taken count is 4
; CHECK-NEXT: Loop %2: Predicated backedge-taken count is 4
; CHECK-NEXT:  Predicates:
; CHECK: Loop %2: Trip multiple is 5 

; Function Attrs: noinline nounwind uwtable
define dso_local void @f(float* %0) #0 {
  br label %2

2:                                                ; preds = %4, %1
  %.01 = phi float [ 0.000000e+00, %1 ], [ %6, %4 ]
  %.0 = phi float [ 0.000000e+00, %1 ], [ %5, %4 ]
  %3 = fcmp olt float %.01, 1.000000e+01
  br i1 %3, label %4, label %7

4:                                                ; preds = %2
  %5 = fadd float %.0, 1.000000e+00
  %6 = fadd float %.01, 2.500000e+00
  br label %2

7:                                                ; preds = %2
  store float %.0, float* %0, align 4
  ret void
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0 "}
