;------------------------------------------------------------------------------
; @file
; Main routine of the pre-SEC code up through the jump into SEC
;
; Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    16

;
; Modified:  EBX, ECX, EDX, EBP
;
; @param[in,out]  RAX/EAX  Initial value of the EAX register
;                          (BIST: Built-in Self Test)
; @param[in,out]  DI       'BP': boot-strap processor, or
;                          'AP': application processor
; @param[out]     RBP/EBP  Address of Boot Firmware Volume (BFV)
;
; @return         None  This routine jumps to SEC and does not return
;
Main16:
    OneTimeCall EarlyInit16

    ;
    ; Transition the processor from 16-bit real mode to 32-bit flat mode
    ;
    OneTimeCall TransitionFromReal16To32BitFlat

BITS    32

%ifdef ARCH_IA32
    ;
    ; Get BFV Entrypoint
    ;
    mov     eax, 0FFFFFFFCh
    mov     eax, dword [eax]
    mov     esi, dword [eax]
    ; Restore the BIST value to EAX register
    movd    eax, mm0
    jmp     esi
%endif

%ifdef ARCH_X64
    ;
    ; Transition the processor from 32-bit flat mode to 64-bit flat mode
    ;
    OneTimeCall Transition32FlatTo64Flat

BITS    64
    ;
    ; Get BFV Entrypoint
    ;
    mov     rax, 0FFFFFFFCh
    mov     eax, dword [rax]
    mov     esi, dword [rax]

    ; Restore the BIST value to EAX register
    movd    eax, mm0

    ;
    ; Jump to the 64-bit SEC entry point
    ;
    jmp     rsi

%endif


