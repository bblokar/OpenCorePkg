#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "GUI.h"
#include "BmfLib.h"
#include "GuiApp.h"

extern unsigned char Cursor_bmp[];
extern unsigned int  Cursor_bmp_len;

extern unsigned char IconHd_bmp[];
extern unsigned int  IconHd_bmp_len;

extern unsigned char IconHdExt_bmp[];
extern unsigned int  IconHdExt_bmp_len;

extern unsigned char IconSelected_bmp[];
extern unsigned int  IconSelected_bmp_len;

extern unsigned char Selector_bmp[];
extern unsigned int  Selector_bmp_len;

GLOBAL_REMOVE_IF_UNREFERENCED BOOT_PICKER_GUI_CONTEXT mGuiContext = { { { 0 } } };

STATIC
VOID
InternalSafeFreePool (
  IN CONST VOID  *Memory
  )
{
  if (Memory != NULL) {
    FreePool ((VOID *)Memory);
  }
}

STATIC
VOID
InternalContextDestruct (
  IN OUT BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  InternalSafeFreePool (Context->Cursor.Buffer);
  InternalSafeFreePool (Context->EntryBackSelected.Buffer);
  InternalSafeFreePool (Context->EntrySelector.BaseImage.Buffer);
  InternalSafeFreePool (Context->EntrySelector.HoldImage.Buffer);
  InternalSafeFreePool (Context->EntryIconInternal.Buffer);
  InternalSafeFreePool (Context->EntryIconInternal.Buffer);
  InternalSafeFreePool (Context->EntryIconExternal.Buffer);
  InternalSafeFreePool (Context->EntryIconExternal.Buffer);
  InternalSafeFreePool (Context->FontContext.FontImage.Buffer);
}

RETURN_STATUS
InternalContextConstruct (
  OUT BOOT_PICKER_GUI_CONTEXT  *Context
  )
{
  STATIC CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL HighlightPixel = {
    0xAF, 0xAF, 0xAF, 0x32
  };

  RETURN_STATUS Status;
  BOOLEAN       Result;

  ASSERT (Context != NULL);

  Context->BootEntry = NULL;

  Status  = GuiBmpToImage (&Context->Cursor,            Cursor_bmp,       Cursor_bmp_len);
  Status |= GuiBmpToImage (&Context->EntryBackSelected, IconSelected_bmp, IconSelected_bmp_len);
  Status |= GuiBmpToClickImage (&Context->EntrySelector,     Selector_bmp,     Selector_bmp_len,  &HighlightPixel);
  Status |= GuiBmpToImage (&Context->EntryIconInternal, IconHd_bmp,       IconHd_bmp_len);
  Status |= GuiBmpToImage (&Context->EntryIconExternal, IconHdExt_bmp,    IconHdExt_bmp_len);
  if (RETURN_ERROR (Status)) {
    InternalContextDestruct (Context);
    return RETURN_UNSUPPORTED;
  }

  Result = GuiInitializeFontHelvetica (&Context->FontContext);
  if (!Result) {
    DEBUG ((DEBUG_WARN, "BMF: Helvetica failed\n"));
    InternalContextDestruct (Context);
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

CONST GUI_IMAGE *
InternalGetCursorImage (
  IN OUT GUI_SCREEN_CURSOR  *This,
  IN     VOID               *Context
  )
{
  CONST BOOT_PICKER_GUI_CONTEXT *GuiContext;

  ASSERT (This != NULL);
  ASSERT (Context != NULL);

  GuiContext = (CONST BOOT_PICKER_GUI_CONTEXT *)Context;
  return &GuiContext->Cursor;
}

RETURN_STATUS
InternalBootPickerEntriesAddDummies (
  IN CONST BOOT_PICKER_GUI_CONTEXT  *GuiContext
  )
{
  RETURN_STATUS Status;
  
  ASSERT (GuiContext != NULL);

  Status  = BootPickerEntriesAdd (GuiContext, L"Macintosh SSD",  NULL, FALSE, TRUE);
  Status |= BootPickerEntriesAdd (GuiContext, L"Macintosh HD",   NULL, FALSE, FALSE);
  Status |= BootPickerEntriesAdd (GuiContext, L"Recovery HD",    NULL, FALSE, FALSE);
  Status |= BootPickerEntriesAdd (GuiContext, L"TimeMachine HD", NULL, TRUE,  FALSE);
  if (RETURN_ERROR (Status)) {
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

EFI_STATUS
EFIAPI
OcBootstrapMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  GUI_DRAWING_CONTEXT DrawContext;

  OcBootstrapMain (ImageHandle, SystemTable);

  Status = GuiLibConstruct (0, 0);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = InternalContextConstruct (&mGuiContext);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = BootPickerViewInitialize (
             &DrawContext,
             &mGuiContext,
             InternalGetCursorImage
             );
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = InternalBootPickerEntriesAddDummies (&mGuiContext);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  GuiDrawLoop (&DrawContext);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiUnload (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  GuiLibDestruct ();
  return EFI_SUCCESS;
}