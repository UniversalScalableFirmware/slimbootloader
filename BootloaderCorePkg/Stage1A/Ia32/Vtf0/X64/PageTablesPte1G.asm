;------------------------------------------------------------------------------
; @file
; Emits Page Tables for 1:1 mapping of the addresses 0 - 0x8000000000 (512GB)
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
; Linear-Address Translation to a 1-GByte Page
;
;------------------------------------------------------------------------------

BITS    64

%define ALIGN_TOP_TO_4K_FOR_PAGING

%define PAGE_PRESENT            0x01
%define PAGE_READ_WRITE         0x02
%define PAGE_USER_SUPERVISOR    0x04
%define PAGE_WRITE_THROUGH      0x08
%define PAGE_CACHE_DISABLE      0x010
%define PAGE_ACCESSED           0x020
%define PAGE_DIRTY              0x040
%define PAGE_PAT                0x080
%define PAGE_GLOBAL             0x0100
%define PAGE_2M_MBO             0x080
%define PAGE_2M_PAT             0x01000
%define PAGE_1G                 0x80

%define PAGE_PDP_ATTR (PAGE_ACCESSED + \
                       PAGE_READ_WRITE + \
                       PAGE_PRESENT)

%define PAGE_PDP_1G_ATTR (PAGE_ACCESSED + \
                       PAGE_READ_WRITE + \
                       PAGE_PRESENT + \
                       PAGE_1G)

%define PGTBLS_OFFSET(x) ((x) - TopLevelPageDirectory)
%define PGTBLS_ADDR(x) (ADDR_OF(TopLevelPageDirectory) + (x))

%define PDP(offset) (ADDR_OF(TopLevelPageDirectory) + (offset) + \
                     PAGE_PDP_ATTR)

%define PDP_1G(x) ((x << 30) + PAGE_PDP_1G_ATTR)

ALIGN 16

TopLevelPageDirectory:

    ;
    ; Top level Page Directory Pointers (1 * 512GB entry)
    ;
    DQ      PDP(0x1000)


    TIMES 0x1000-PGTBLS_OFFSET($) DB 0
    ;
    ; Next level Page Directory Pointers (512 * 1GB entries => 512GB)
    ;
%assign i 0
%rep      512
    DQ    PDP_1G(i)
    %assign i i+1
%endrep


EndOfPageTables:
