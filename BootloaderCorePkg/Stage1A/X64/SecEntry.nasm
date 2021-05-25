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

DEFAULT  REL
SECTION .text

extern  ASM_PFX(SecStartup)
extern  ASM_PFX(PcdGet32(PcdStage1StackSize))
extern  ASM_PFX(PcdGet32(PcdStage1DataSize))
extern  ASM_PFX(PcdGet32(PcdStage1StackBaseOffset))
extern  ASM_PFX(EarlyBoardInit)
extern  ASM_PFX(FspTempRamInit)
extern  ASM_PFX(mPageTableLength)

%define FSP_HEADER_TEMPRAMINIT_OFFSET 0x30
%define PAGE_REGION_SIZE              0x6000

%define PAGE_PRESENT                  0x01
%define PAGE_READ_WRITE               0x02
%define PAGE_USER_SUPERVISOR          0x04
%define PAGE_WRITE_THROUGH            0x08
%define PAGE_CACHE_DISABLE            0x010
%define PAGE_ACCESSED                 0x020
%define PAGE_DIRTY                    0x040
%define PAGE_PAT                      0x080
%define PAGE_GLOBAL                   0x0100
%define PAGE_2M_MBO                   0x080
%define PAGE_2M_PAT                   0x01000

%define PAGE_2M_PDE_ATTR (PAGE_2M_MBO + \
                          PAGE_ACCESSED + \
                          PAGE_DIRTY + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)

%define PAGE_PDP_ATTR    (PAGE_ACCESSED + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)


PreparePagingTable:
        ; Input:
        ;   ECX:  Page table base, need 6 pages
        ; Modify:
        ;   ECX

        ;
        ; Set up identical paging table for x64
        ;
        push    rsi
        push    rdx

        mov     esi, ecx
        mov     ecx, PAGE_REGION_SIZE / 4
        xor     eax, eax
        xor     edx, edx
clearPageTablesMemoryLoop:
        mov     dword[ecx * 4 + esi - 4], eax
        loop    clearPageTablesMemoryLoop

        ;
        ; Top level Page Directory Pointers (1 * 512GB entry)
        ;
        lea     eax, [esi + (0x1000) + PAGE_PDP_ATTR]
        mov     dword[esi + (0)], eax
        mov     dword[esi + (4)], edx

        ;
        ; Next level Page Directory Pointers (4 * 1GB entries => 4GB)
        ;
        lea     eax, [esi + (0x2000) + PAGE_PDP_ATTR]
        mov     dword[esi + (0x1000)], eax
        mov     dword[esi + (0x1004)], edx
        lea     eax, [esi + (0x3000) + PAGE_PDP_ATTR]
        mov     dword[esi + (0x1008)], eax
        mov     dword[esi + (0x100C)], edx
        lea     eax, [esi + (0x4000) + PAGE_PDP_ATTR]
        mov     dword[esi + (0x1010)], eax
        mov     dword[esi + (0x1014)], edx
        lea     eax, [esi + (0x5000) + PAGE_PDP_ATTR]
        mov     dword[esi + (0x1018)], eax
        mov     dword[esi + (0x101C)], edx

        ;
        ; Page Table Entries (2048 * 2MB entries => 4GB)
        ;
        mov     ecx, 0x800
pageTableEntriesLoop:
        mov     eax, ecx
        dec     eax
        shl     eax, 21
        add     eax, PAGE_2M_PDE_ATTR
        mov     [ecx * 8 + esi + (0x2000 - 8)], eax
        mov     [(ecx * 8 + esi + (0x2000 - 8)) + 4], edx
        loop    pageTableEntriesLoop

        pop     rdx
        pop     rsi
        xor     rax, rax
        ret


global  ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
        movd    mm0, eax

        ;
        ; Read time stamp
        ;
        rdtsc
        mov     rsi, rax
        mov     rdi, rdx

        ;
        ; Early board hooks
        ;
        mov     rsp, EarlyBoardInitRet
        jmp     ASM_PFX(EarlyBoardInit)

EarlyBoardInitRet:
        mov     rsp, FspTempRamInitRet
        jmp     ASM_PFX(FspTempRamInit)

FspTempRamInitRet:
        cmp     rax, 0x800000000000000EULL ;Check if EFI_NOT_FOUND returned. Error code for Microcode Update not found.
        je      FspApiSuccess           ;If microcode not found, don't hang, but continue.

        cmp     rax, 0              ;Check if EFI_SUCCESS returned.
        jz      FspApiSuccess

        ; FSP API failed:
        jmp     $

FspApiSuccess:
        ;
        ; Setup stack
        ; ECX: Bootloader stack base
        ; EDX: Bootloader stack top
        ;
        mov     rsp, rcx
        lea     rax, [ASM_PFX(PcdGet32(PcdStage1StackBaseOffset))]
        add     esp, dword [rax]
        lea     rax, [ASM_PFX(PcdGet32(PcdStage1StackSize))]
        add     esp, dword [rax]

        mov     rcx, rsp
        lea     rax, [ASM_PFX(PcdGet32(PcdStage1DataSize))]
        add     ecx, dword [rax]
        sub     rcx, [ASM_PFX(mPageTableLength)]

        push    rcx
        call    PreparePagingTable
        pop     rax
        mov     cr3, rax

        xor     rbx, rbx             ; Use EBX for Status
        ;
        ; Check stage1 stack base offset
        ;
        lea     rax, [ASM_PFX(PcdGet32(PcdStage1DataSize))]
        mov     eax, dword [rax]
        add     rax, rsp
        cmp     rax, rdx
        jle     CheckStackRangeDone

        ;
        ; Error in stack range
        ;
        bts     ebx, 1               ; Set BIT1 in Status
        lea     rax, [ASM_PFX(PcdGet32(PcdStage1StackBaseOffset))]
        sub     esp, dword [rax]

CheckStackRangeDone:
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
        ; Setup HOB
        push    rbx                  ; Status
        push    rdi                  ; TimeStamp[0] [63:0]
        shl     rdx, 32              ; Move CarTop to high 32bit
        add     rdx, rcx             ; Add back CarBase
        push    rdx
        mov     rcx, rsp             ; Argument 1

        sub     rsp, 32              ; 32 bytes shadow store for x64
        and     esp, 0xfffffff0      ; Align stack to 16 bytes
        call    ASM_PFX(SecStartup)  ; Jump to C code
        jmp     $