;------------------------------------------------------------------------------
;
; Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;
; Module Name:
;
;  SecEntry.nasm
;
; Abstract:
;
;  This is the code that goes from real-mode to protected mode.
;  It consumes the reset vector.
;
;------------------------------------------------------------------------------

SECTION .text

extern  ASM_PFX(SecStartup)

; FSP API offset
%define FSP_HEADER_TEMPRAMINIT_OFFSET 0x30

extern  ASM_PFX(FsptBaseAddr)
extern  ASM_PFX(TempRamInitParams)

align 16
TempRamInitStack:
        DD      TempRamInitDone
        DD      ASM_PFX(TempRamInitParams)

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
        mov     ebp, esp
        mov     eax, dword [ASM_PFX(FsptBaseAddr)]

        ;
        ; Find the fsp info header
        ; Jump to TempRamInit API
        ;
        add     eax, dword [eax + 094h + FSP_HEADER_TEMPRAMINIT_OFFSET]
        mov     esp, TempRamInitStack
        jmp     eax

TempRamInitDone:
        mov     esp, ebp
        jmp     esp



global  ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
        movd    mm0, eax

        ;
        ; Read time stamp
        ;
        rdtsc
        mov     esi, eax
        mov     edi, edx

        mov     esp, FspTempRamInitRet
        jmp     ASM_PFX(FspTempRamInit)

FspTempRamInitRet:
        cmp     eax, 8000000Eh      ;Check if EFI_NOT_FOUND returned. Error code for Microcode Update not found.
        je      FspApiSuccess       ;If microcode not found, don't hang, but continue.

        cmp     eax, 0              ;Check if EFI_SUCCESS returned.
        jz      FspApiSuccess

        ; FSP API failed:
        jmp     $

FspApiSuccess:
        ;
        ; Setup stack
        ; ECX: Bootloader stack base
        ; EDX: Bootloader stack top
        ;

        ; Setup BIST
        xor     ebx,  ebx

        ;
        ; CpuBist error check
        ;
        movd    eax, mm0
        emms                         ; Exit MMX Instruction
        cmp     eax, 0
        jz      CheckStatusDone

        ;
        ; Error in CpuBist
        ;
        bts     ebx, 0               ; Set BIT0 in Status

CheckStatusDone:
        mov     esp,  ecx
        add     esp,  0x10000

        xor     eax,  eax
        push    ebx                  ; BIST status
        push    eax                  ; HobList, not used
        push    edi                  ; TimeStamp[0] [63:32]
        push    esi                  ; TimeStamp[0] [31:0]
        push    edx                  ; CarTop
        push    ecx                  ; CarBase

        push    esp
        call    ASM_PFX(SecStartup)  ; Jump to C code
        jmp     $


global  ASM_PFX(JumpToOemEntry)
ASM_PFX(JumpToOemEntry):
        xor     eax, eax         ; BIST
        mov     ebx, [esp+ 4]    ; OEM entry point
        mov     ecx, [esp+12]    ; CarBase
        mov     edx, [esp+16]    ; CarTop
        mov     esp, [esp+ 8]    ; Hoblist
        jmp     ebx

