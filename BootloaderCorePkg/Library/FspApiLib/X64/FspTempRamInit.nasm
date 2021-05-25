;------------------------------------------------------------------------------
;
; Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;
; Module Name:
;
;  FspTempRamInit.nasm
;
; Abstract:
;
;  This is the code that will call into FSP TempRamInit API
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text
;---------------------
; FSP API offset
%define FSP_HEADER_TEMPRAMINIT_OFFSET 0x30

extern  ASM_PFX(PcdGet32(PcdFSPTBase))
extern  ASM_PFX(TempRamInitParams)

global  ASM_PFX(FspTempRamInit)
ASM_PFX(FspTempRamInit):
        ;
        ; This hook is called to initialize temporay RAM
        ; ESI, EDI need to be preserved
        ; ESP contains return address
        ; ECX, EDX return the temprary RAM start and end
        ;

        ;
        ; Get FSP-T base in EAX
        ;
        mov     rbp, rsp
        lea     rax, [ASM_PFX(PcdGet32(PcdFSPTBase))]
        mov     eax, dword [rax]

        ;
        ; Find the fsp info header
        ; Jump to TempRamInit API
        ;
        add     eax, dword [rax + 094h + FSP_HEADER_TEMPRAMINIT_OFFSET]
        lea     rcx, [ASM_PFX(TempRamInitParams)]
        lea     rsp, [TempRamInitStack]
        jmp     rax

TempRamInitDone:
        mov     rsp, rbp
        jmp     rsp

align 16
TempRamInitStack:
        DQ      TempRamInitDone
