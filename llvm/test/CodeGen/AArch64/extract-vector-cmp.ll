; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 5
; RUN: llc -mattr=+sve < %s | FileCheck %s

target triple = "aarch64-unknown-linux-gnu"


define i1 @extract_icmp_v4i32_const_splat_rhs(<4 x i32> %a) {
; CHECK-LABEL: extract_icmp_v4i32_const_splat_rhs:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, v0.s[1]
; CHECK-NEXT:    cmp w8, #5
; CHECK-NEXT:    cset w0, lo
; CHECK-NEXT:    ret
  %icmp = icmp ult <4 x i32> %a, splat (i32 5)
  %ext = extractelement <4 x i1> %icmp, i32 1
  ret i1 %ext
}

define i1 @extract_icmp_v4i32_const_splat_lhs(<4 x i32> %a) {
; CHECK-LABEL: extract_icmp_v4i32_const_splat_lhs:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, v0.s[1]
; CHECK-NEXT:    cmp w8, #7
; CHECK-NEXT:    cset w0, hi
; CHECK-NEXT:    ret
  %icmp = icmp ult <4 x i32> splat(i32 7), %a
  %ext = extractelement <4 x i1> %icmp, i32 1
  ret i1 %ext
}

define i1 @extract_icmp_v4i32_const_vec_rhs(<4 x i32> %a) {
; CHECK-LABEL: extract_icmp_v4i32_const_vec_rhs:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, v0.s[1]
; CHECK-NEXT:    cmp w8, #234
; CHECK-NEXT:    cset w0, lo
; CHECK-NEXT:    ret
  %icmp = icmp ult <4 x i32> %a, <i32 5, i32 234, i32 -1, i32 7>
  %ext = extractelement <4 x i1> %icmp, i32 1
  ret i1 %ext
}

define i1 @extract_fcmp_v4f32_const_splat_rhs(<4 x float> %a) {
; CHECK-LABEL: extract_fcmp_v4f32_const_splat_rhs:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov s0, v0.s[1]
; CHECK-NEXT:    fmov s1, #4.00000000
; CHECK-NEXT:    fcmp s0, s1
; CHECK-NEXT:    cset w0, lt
; CHECK-NEXT:    ret
  %fcmp = fcmp ult <4 x float> %a, splat(float 4.0e+0)
  %ext = extractelement <4 x i1> %fcmp, i32 1
  ret i1 %ext
}

define void @vector_loop_with_icmp(ptr nocapture noundef writeonly %dest) {
; CHECK-LABEL: vector_loop_with_icmp:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    index z0.d, #0, #1
; CHECK-NEXT:    mov w8, #4 // =0x4
; CHECK-NEXT:    mov w9, #16 // =0x10
; CHECK-NEXT:    dup v2.2d, x8
; CHECK-NEXT:    add x8, x0, #8
; CHECK-NEXT:    mov w10, #1 // =0x1
; CHECK-NEXT:    mov z1.d, z0.d
; CHECK-NEXT:    add z1.d, z1.d, #2 // =0x2
; CHECK-NEXT:    b .LBB4_2
; CHECK-NEXT:  .LBB4_1: // %pred.store.continue18
; CHECK-NEXT:    // in Loop: Header=BB4_2 Depth=1
; CHECK-NEXT:    add v1.2d, v1.2d, v2.2d
; CHECK-NEXT:    add v0.2d, v0.2d, v2.2d
; CHECK-NEXT:    subs x9, x9, #4
; CHECK-NEXT:    add x8, x8, #16
; CHECK-NEXT:    b.eq .LBB4_10
; CHECK-NEXT:  .LBB4_2: // %vector.body
; CHECK-NEXT:    // =>This Inner Loop Header: Depth=1
; CHECK-NEXT:    fmov x11, d0
; CHECK-NEXT:    cmp x11, #14
; CHECK-NEXT:    b.hi .LBB4_4
; CHECK-NEXT:  // %bb.3: // %pred.store.if
; CHECK-NEXT:    // in Loop: Header=BB4_2 Depth=1
; CHECK-NEXT:    stur w10, [x8, #-8]
; CHECK-NEXT:  .LBB4_4: // %pred.store.continue
; CHECK-NEXT:    // in Loop: Header=BB4_2 Depth=1
; CHECK-NEXT:    mov x11, v0.d[1]
; CHECK-NEXT:    cmp x11, #14
; CHECK-NEXT:    b.hi .LBB4_6
; CHECK-NEXT:  // %bb.5: // %pred.store.if5
; CHECK-NEXT:    // in Loop: Header=BB4_2 Depth=1
; CHECK-NEXT:    stur w10, [x8, #-4]
; CHECK-NEXT:  .LBB4_6: // %pred.store.continue6
; CHECK-NEXT:    // in Loop: Header=BB4_2 Depth=1
; CHECK-NEXT:    fmov x11, d1
; CHECK-NEXT:    cmp x11, #14
; CHECK-NEXT:    b.hi .LBB4_8
; CHECK-NEXT:  // %bb.7: // %pred.store.if7
; CHECK-NEXT:    // in Loop: Header=BB4_2 Depth=1
; CHECK-NEXT:    str w10, [x8]
; CHECK-NEXT:  .LBB4_8: // %pred.store.continue8
; CHECK-NEXT:    // in Loop: Header=BB4_2 Depth=1
; CHECK-NEXT:    mov x11, v1.d[1]
; CHECK-NEXT:    cmp x11, #14
; CHECK-NEXT:    b.hi .LBB4_1
; CHECK-NEXT:  // %bb.9: // %pred.store.if9
; CHECK-NEXT:    // in Loop: Header=BB4_2 Depth=1
; CHECK-NEXT:    str w10, [x8, #4]
; CHECK-NEXT:    b .LBB4_1
; CHECK-NEXT:  .LBB4_10: // %for.cond.cleanup
; CHECK-NEXT:    ret
entry:
  br label %vector.body

vector.body:
  %index = phi i64 [ 0, %entry ], [ %index.next, %pred.store.continue18 ]
  %vec.ind = phi <4 x i64> [ <i64 0, i64 1, i64 2, i64 3>, %entry ], [ %vec.ind.next, %pred.store.continue18 ]
  %0 = icmp ult <4 x i64> %vec.ind, <i64 15, i64 15, i64 15, i64 15>
  %1 = extractelement <4 x i1> %0, i64 0
  br i1 %1, label %pred.store.if, label %pred.store.continue

pred.store.if:
  %2 = getelementptr inbounds i32, ptr %dest, i64 %index
  store i32 1, ptr %2, align 4
  br label %pred.store.continue

pred.store.continue:
  %3 = extractelement <4 x i1> %0, i64 1
  br i1 %3, label %pred.store.if5, label %pred.store.continue6

pred.store.if5:
  %4 = or disjoint i64 %index, 1
  %5 = getelementptr inbounds i32, ptr %dest, i64 %4
  store i32 1, ptr %5, align 4
  br label %pred.store.continue6

pred.store.continue6:
  %6 = extractelement <4 x i1> %0, i64 2
  br i1 %6, label %pred.store.if7, label %pred.store.continue8

pred.store.if7:
  %7 = or disjoint i64 %index, 2
  %8 = getelementptr inbounds i32, ptr %dest, i64 %7
  store i32 1, ptr %8, align 4
  br label %pred.store.continue8

pred.store.continue8:
  %9 = extractelement <4 x i1> %0, i64 3
  br i1 %9, label %pred.store.if9, label %pred.store.continue18

pred.store.if9:
  %10 = or disjoint i64 %index, 3
  %11 = getelementptr inbounds i32, ptr %dest, i64 %10
  store i32 1, ptr %11, align 4
  br label %pred.store.continue18

pred.store.continue18:
  %index.next = add i64 %index, 4
  %vec.ind.next = add <4 x i64> %vec.ind, <i64 4, i64 4, i64 4, i64 4>
  %24 = icmp eq i64 %index.next, 16
  br i1 %24, label %for.cond.cleanup, label %vector.body

for.cond.cleanup:
  ret void
}


; Negative tests

define i1 @extract_icmp_v4i32_splat_rhs(<4 x i32> %a, i32 %b) {
; CHECK-LABEL: extract_icmp_v4i32_splat_rhs:
; CHECK:       // %bb.0:
; CHECK-NEXT:    dup v1.4s, w0
; CHECK-NEXT:    cmhi v0.4s, v1.4s, v0.4s
; CHECK-NEXT:    xtn v0.4h, v0.4s
; CHECK-NEXT:    umov w8, v0.h[1]
; CHECK-NEXT:    and w0, w8, #0x1
; CHECK-NEXT:    ret
  %ins = insertelement <4 x i32> poison, i32 %b, i32 0
  %splat = shufflevector <4 x i32> %ins, <4 x i32> poison, <4 x i32> zeroinitializer
  %icmp = icmp ult <4 x i32> %a, %splat
  %ext = extractelement <4 x i1> %icmp, i32 1
  ret i1 %ext
}

define i1 @extract_icmp_v4i32_splat_rhs_mul_use(<4 x i32> %a, ptr %p) {
; CHECK-LABEL: extract_icmp_v4i32_splat_rhs_mul_use:
; CHECK:       // %bb.0:
; CHECK-NEXT:    movi v1.4s, #235
; CHECK-NEXT:    adrp x9, .LCPI6_0
; CHECK-NEXT:    mov x8, x0
; CHECK-NEXT:    ldr q2, [x9, :lo12:.LCPI6_0]
; CHECK-NEXT:    cmhi v0.4s, v1.4s, v0.4s
; CHECK-NEXT:    xtn v1.4h, v0.4s
; CHECK-NEXT:    and v0.16b, v0.16b, v2.16b
; CHECK-NEXT:    addv s0, v0.4s
; CHECK-NEXT:    umov w9, v1.h[1]
; CHECK-NEXT:    fmov w10, s0
; CHECK-NEXT:    and w0, w9, #0x1
; CHECK-NEXT:    strb w10, [x8]
; CHECK-NEXT:    ret
  %icmp = icmp ult <4 x i32> %a, splat(i32 235)
  %ext = extractelement <4 x i1> %icmp, i32 1
  store <4 x i1> %icmp, ptr %p, align 4
  ret i1 %ext
}

define i1 @extract_icmp_v4i32_splat_rhs_unknown_idx(<4 x i32> %a, i32 %c) {
; CHECK-LABEL: extract_icmp_v4i32_splat_rhs_unknown_idx:
; CHECK:       // %bb.0:
; CHECK-NEXT:    sub sp, sp, #16
; CHECK-NEXT:    .cfi_def_cfa_offset 16
; CHECK-NEXT:    movi v1.4s, #127
; CHECK-NEXT:    add x8, sp, #8
; CHECK-NEXT:    // kill: def $w0 killed $w0 def $x0
; CHECK-NEXT:    bfi x8, x0, #1, #2
; CHECK-NEXT:    cmhi v0.4s, v1.4s, v0.4s
; CHECK-NEXT:    xtn v0.4h, v0.4s
; CHECK-NEXT:    str d0, [sp, #8]
; CHECK-NEXT:    ldrh w8, [x8]
; CHECK-NEXT:    and w0, w8, #0x1
; CHECK-NEXT:    add sp, sp, #16
; CHECK-NEXT:    ret
  %icmp = icmp ult <4 x i32> %a, splat(i32 127)
  %ext = extractelement <4 x i1> %icmp, i32 %c
  ret i1 %ext
}
