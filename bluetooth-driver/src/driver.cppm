module;

#include <ntddk.h>

export module lesserjoy.bluetooth;

import result;

extern "C" {

  DRIVER_UNLOAD lj_driver_exit;

} // extern "C"

export namespace lj {

  auto driver_main(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> NTSTATUS;
  auto driver_exit(DRIVER_OBJECT* driver_object) -> void;

} // namespace lj
