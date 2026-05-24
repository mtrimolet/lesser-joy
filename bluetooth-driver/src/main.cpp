#include <ntddk.h>

import lesserjoy.bluetooth;

extern "C" {

  DRIVER_INITIALIZE DriverEntry;

}

#ifdef ALLOC_PRAGMA
  #pragma alloc_text (INIT, DriverEntry)
#endif

extern "C" {

  _Use_decl_annotations_
  auto DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> NTSTATUS {
    return lj::driver_main(driver_object, registry_path);
  }

}
