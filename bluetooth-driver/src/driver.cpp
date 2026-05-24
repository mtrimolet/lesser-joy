module;

#include <ntddk.h>
#include <wdf.h>

#include <stormkit/core/macro.hpp>

module lesserjoy.bluetooth;

import log;
import utilities;

import :device;

#ifdef ALLOC_PRAGMA
  #pragma alloc_text (PAGE, lj_driver_exit)
#endif

namespace lj {

  auto driver_create(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> result<WDFDRIVER, NTSTATUS> {
    auto driver_config = WDF_DRIVER_CONFIG {};
    WDF_DRIVER_CONFIG_INIT(&driver_config, lj_device_add);
    driver_config.DriverInitFlags = WdfDriverInitNoDispatchOverride;

    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
  
    auto driver = WDFDRIVER {};
    NT_Try(WdfDriverCreate(driver_object, registry_path, &attributes, &driver_config, &driver));

    return driver;
  }

  auto driver_main(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> NTSTATUS {
    return ([&] -> result<void, NTSTATUS> {
      debug_logln("Initalizing bluetooth driver...");

      auto driver = Try(driver_create(driver_object, registry_path));

      driver_object->DriverUnload = lj_driver_exit;

      debug_logln("Initialized bluetooth driver!");

      return {};
    })()
    .map_error([](auto status) {
      KdPrint(("Failed to initialize lesser-joy bluetooth driver! status: 0x%x\n", status));
      return status;
    })
    .and_then([](auto res) static -> result<void, NTSTATUS> {
      return Unexpected{ STATUS_SUCCESS };
    })
    .error();
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_driver_exit(DRIVER_OBJECT* driver_object) -> void {
    lj::driver_exit(driver_object);
  }

} // extern "C"

namespace lj {

  auto driver_exit(DRIVER_OBJECT* driver_object) -> void {
    debug_logln("Uninitialized bluetooth driver!");
  }

} // namespace lj
