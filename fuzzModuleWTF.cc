
#include "backend.h"
#include "targets.h"
#include "crash_detection_umode.h"
#include "handle_table.h"
#include "fshandle_table.h"
#include <fmt/format.h>

namespace fs = std::filesystem;


namespace jpgx {

bool InsertTestcase(const uint8_t *Buffer, const size_t BufferSize) {

    g_FsHandleTable.MapExistingGuestFile(
        uR"(\??\C:\Users\TEST\Desktop\JPG_1MB.jpg)", Buffer, BufferSize);
  
 // fmt::print("DONE! insert testcase\n");

  return true;
}



bool Init(const Options_t &Opts, const CpuState_t &) {

   //
   // Break at the end of the parser code
   //

   if (!g_Backend->SetBreakpoint(Gva_t(0x004A2CC9), [](Backend_t *Backend) {
       // fmt::print("DONE! reached end\n");
        Backend->Stop(Ok_t());
      })) {
    fmt::print("Failed to SetBreakpoint AfterParse\n");
    return false;
  }




    if (!g_Backend->SetBreakpoint(Gva_t(0x00436410), [](Backend_t *Backend) {
       // fmt::print("handling REG call\n");

        //
        // Set return value.
        //
        g_Backend->Rax(0);

        const uint64_t Stack = g_Backend->Rsp();
        const uint32_t SavedReturnAddress = g_Backend->VirtRead4(Gva_t(Stack));

        //
        // Eat up the saved return address.
        //

        g_Backend->Rsp(Stack + (4));
        g_Backend->Rip(SavedReturnAddress);

      })) {
    fmt::print("Failed to SetBreakpoint REG call handling\n");
    return false;
  }


    if (!SetupFilesystemHooks()) {
    fmt::print("Failed to SetupFilesystemHooks\n");
    return false;
  }

  //
  // Instrument the Windows user-mode exception dispatcher to catch access
  // violations

    SetupUsermodeCrashDetectionHooks();

    
    g_HandleTable.Save();

  return true;
}


bool Restore() {
  //
  // Restore the handle table and the fshooks.
  //

  g_HandleTable.Restore();
  return true;
}
//
// Register the target.
//

Target_t jpgx("jpgx", Init, InsertTestcase, Restore);

} // namespace jpgx
