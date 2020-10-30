/** @file

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <BootFw.h>

const
FSPT_UPD TempRamInitParams = {
  .FspUpdHeader = {
    .Signature = FSPT_UPD_SIGNATURE,
    .Revision  = 1,
    .Reserved  = {0},
  },
  .FsptCommonUpd = {
    .Revision              = 1,
    .MicrocodeRegionBase   = 0,
    .MicrocodeRegionLength = 0,
    .CodeRegionBase        = 0xFF000000,
    .CodeRegionLength      = 0x00000000,
  },
  .UpdTerminator = 0x55AA,
};

const FSP_INIT_PHASE mNotify[] = {
  EnumInitPhaseAfterPciEnumeration,
  EnumInitPhaseReadyToBoot,
  EnumInitPhaseEndOfFirmware
};

// SplitFspBin.py rebase  -f  FspRel.bin -c t m s -b 0xFFFC7000 0xFFFA5000 0xFFF90000
const UINT32 FsptBaseAddr = 0xFFFC7000;
const UINT32 FspmBaseAddr = 0xFFFA5000;
const UINT32 FspsBaseAddr = 0xFFF90000;

//
// Global Descriptor Table (GDT)
//
STATIC
CONST IA32_SEGMENT_DESCRIPTOR
mGdtEntries[STAGE_GDT_ENTRY_COUNT] = {
  /* selector { Global Segment Descriptor                              } */
  /* 0x00 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, //null descriptor
  /* 0x08 */  {{0xffff, 0,  0,  0x2,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //linear data segment descriptor
  /* 0x10 */  {{0xffff, 0,  0,  0xb,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //linear code segment descriptor
  /* 0x18 */  {{0xffff, 0,  0,  0x3,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //system data segment descriptor
  /* 0x20 */  {{0xffff, 0,  0,  0xb,  1,  0,  1,  0xf,  0,  1, 0,  1,  0}}, //linear code (64-bit) segment descriptor
  /* 0x28 */  {{0xffff, 0,  0,  0xb,  1,  0,  1,  0x0,  0,  0, 0,  0,  0}}, //16-bit code segment descriptor
  /* 0x30 */  {{0xffff, 0,  0,  0x2,  1,  0,  1,  0x0,  0,  0, 0,  0,  0}}, //16-bit data segment descriptor
};

//
// IA32 Gdt register
//
STATIC
CONST IA32_DESCRIPTOR mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN) mGdtEntries
  };


/**

  Entry point to the C language phase of Stage1A.

  After the Stage1A assembly code has initialized some temporary memory and set
  up the stack, control is transferred to this function.
  - Initialize the global data
  - Do post TempRaminit board initialization.
  - Relocate by itself stage1A code to temp memory and execute.
  - CPU halted if relocation fails.

  @param[in] Params            Pointer to stage specific parameters.

**/
VOID
EFIAPI
SecStartup (
  IN VOID  *Params
  )
{
  LOADER_GLOBAL_DATA        LdrGlobalData;
  STAGE_IDT_TABLE           IdtTable;
  STAGE_GDT_TABLE           GdtTable;
  LOADER_GLOBAL_DATA       *LdrGlobal;
  LOADER_GLOBAL_DATA       *OldLdrGlobal;
  STAGE1A_ASM_PARAM        *Stage1aAsmParam;
  UINT32                    StackTop;
  UINT32                    PageTblSize;
  UINT64                    TimeStamp;
  VOID                     *HobList;
  UINT32                    FspReservedMemBase;
  UINT64                    FspReservedMemSize;
  UINT32                    MemPoolStart;
  UINT32                    MemPoolEnd;
  UINT32                    MemPoolCurrTop;
  UINT32                    LoaderReservedMemSize;
  UINT32                    LoaderHobStackSize;
  EFI_STATUS                Status;
  STAGE_IDT_TABLE          *IdtTablePtr;
  STAGE_GDT_TABLE          *GdtTablePtr;


  TimeStamp   = ReadTimeStamp ();
  Stage1aAsmParam = (STAGE1A_ASM_PARAM *)Params;

  // Init global data
  PageTblSize = IS_X64 ? 8 * EFI_PAGE_SIZE : 0;
  LdrGlobal = &LdrGlobalData;
  ZeroMem (LdrGlobal, sizeof (LOADER_GLOBAL_DATA));
  StackTop = (UINT32)(UINTN)Params + sizeof (STAGE1A_ASM_PARAM);
  LdrGlobal->Signature             = LDR_GDATA_SIGNATURE;
  LdrGlobal->LoaderStage           = LOADER_STAGE_1A;
  LdrGlobal->StackTop              = StackTop;
  LdrGlobal->MemPoolEnd            = StackTop + 0xE000 - PageTblSize;
  LdrGlobal->MemPoolStart          = StackTop;
  LdrGlobal->MemPoolCurrTop        = LdrGlobal->MemPoolEnd;
  LdrGlobal->MemPoolCurrBottom     = LdrGlobal->MemPoolStart;
  LdrGlobal->DebugPrintErrorLevel  = 0x8000004F;
  LdrGlobal->PerfData.PerfIndex    = 2;
  LdrGlobal->PerfData.FreqKhz      = GetTimeStampFrequency ();
  LdrGlobal->PerfData.TimeStamp[0] = Stage1aAsmParam->TimeStamp | 0x1000000000000000ULL;
  LdrGlobal->PerfData.TimeStamp[1] = TimeStamp  | 0x1010000000000000ULL;
  // Set the Loader features to default here.
  // Any platform (board init lib) can update these according to
  // the config data passed in or these defaults remain
  LdrGlobal->LdrFeatures           = FEATURE_MEASURED_BOOT | FEATURE_ACPI;

  LoadGdt (&GdtTable, (IA32_DESCRIPTOR *)&mGdt);
  LoadIdt (&IdtTable, (UINT32)(UINTN)LdrGlobal);
  SetLoaderGlobalDataPointer (LdrGlobal);

  DEBUG ((DEBUG_INFO, "\n============= FSP-R Entry =============\n\n"));
  SetBootMode (BOOT_WITH_FULL_CONFIGURATION);

  DEBUG ((DEBUG_INIT, "Memory Init\n"));
  Status = CallFspMemoryInit (FspmBaseAddr, &HobList);
  ASSERT_EFI_ERROR (Status);

  // Need to switch to new stack
  FspReservedMemBase = (UINT32)GetFspReservedMemoryFromGuid (
                         HobList,
                         &FspReservedMemSize,
                         &gFspReservedMemoryResourceHobGuid
                         );
  ASSERT (FspReservedMemBase > 0);

  // Prepare Global Data structure
  LoaderReservedMemSize = 0x100000;
  LoaderHobStackSize    = 0x001000;

  OldLdrGlobal   = LdrGlobal;
  MemPoolStart   = FspReservedMemBase - LoaderReservedMemSize;
  MemPoolEnd     = FspReservedMemBase - LoaderHobStackSize;
  MemPoolCurrTop = ALIGN_DOWN (MemPoolEnd - sizeof (LOADER_GLOBAL_DATA), 0x10);
  LdrGlobal      = (LOADER_GLOBAL_DATA *)(UINTN)MemPoolCurrTop;
  MemPoolCurrTop = ALIGN_DOWN (MemPoolCurrTop - sizeof (STAGE_IDT_TABLE), 0x10);
  IdtTablePtr    = (STAGE_IDT_TABLE *)(UINTN)MemPoolCurrTop;
  MemPoolCurrTop = ALIGN_DOWN (MemPoolCurrTop - sizeof (STAGE_GDT_TABLE), 0x10);
  GdtTablePtr    = (STAGE_GDT_TABLE *)(UINTN)MemPoolCurrTop;
  CopyMem (LdrGlobal, OldLdrGlobal, sizeof (LOADER_GLOBAL_DATA));

  LdrGlobal->FspHobList        = HobList;
  LdrGlobal->LdrHobList        = NULL;
  LdrGlobal->StackTop          = FspReservedMemBase;
  LdrGlobal->MemPoolEnd        = MemPoolEnd;
  LdrGlobal->MemPoolStart      = MemPoolStart;
  LdrGlobal->MemPoolCurrTop    = MemPoolCurrTop;
  LdrGlobal->MemPoolCurrBottom = MemPoolStart;
  LdrGlobal->MemUsableTop      = (UINT32)(FspReservedMemBase + FspReservedMemSize);

  // Setup global data in memory
  LoadGdt (GdtTablePtr, NULL);
  LoadIdt (IdtTablePtr, (UINT32)(UINTN)LdrGlobal);
  SetLoaderGlobalDataPointer (LdrGlobal);
  DEBUG ((DEBUG_INFO, "Loader global data @ 0x%08X\n", (UINT32)(UINTN)LdrGlobal));

  // Setup new stack and continue
  StackTop  = LdrGlobal->StackTop;
  StackTop  = ALIGN_DOWN (StackTop, 0x100);
  DEBUG ((DEBUG_INFO, "Switch to memory stack @ 0x%08X\n", StackTop));
  SwitchStack (ContinueFunc, HobList, NULL, (VOID *)(UINTN)StackTop);
}



/**
  Continue Stage 1B execution.

  This function will continue Stage1B execution with a new memory-based stack.

  @param[in]  Context1        Pointer to STAGE1B_PARAM in main memory.
  @param[in]  Context2        Unused.

**/
VOID
EFIAPI
ContinueFunc (
  IN VOID                      *Context1,
  IN VOID                      *Context2
  )
{
  UINT32       Idx;
  EFI_STATUS   Status;
  VOID        *OemEntry;
  VOID        *HobList;

  HobList = Context1;

  Status = CallFspTempRamExit (FspmBaseAddr, NULL);
  ASSERT_EFI_ERROR (Status);

  Status = CallFspSiliconInit ();
  ASSERT_EFI_ERROR (Status);

  for (Idx = 0; Idx < 3; Idx++) {
    Status = CallFspNotifyPhase (mNotify[Idx]);
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "HobList is located at 0x%08x\n", HobList));
  DEBUG ((DEBUG_INFO, "\n============= FSP-R Exit =============\n"));

  DEBUG ((DEBUG_INFO, "\nJump into OEM entry\n"));

  OemEntry = (VOID *)(*(UINT32 *)0xFFF80000);
  JumpToOemEntry (OemEntry, HobList, 0, 0x80000);

  while (1);
}