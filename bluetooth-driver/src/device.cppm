module;

#include <ntddk.h>
#include <wdf.h>

export module lesserjoy.bluetooth:device;

import result;

import :context;
import :connection;

extern "C" {

  EVT_WDF_DRIVER_DEVICE_ADD      lj_device_add;
  EVT_WDF_OBJECT_CONTEXT_CLEANUP lj_device_cleanup;
  
  EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT    lj_device_on_self_managed_io_init;
  EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP lj_device_on_self_managed_io_cleanup;

  EVT_WDF_DEVICE_FILE_CREATE lj_device_on_file_create;
  EVT_WDF_FILE_CLOSE         lj_device_on_file_close;

} // extern "C"

export namespace lj {

  auto device_add(WDFDRIVER driver, WDFDEVICE_INIT* device_init) -> NTSTATUS;
  auto device_cleanup(WDFOBJECT object) -> void;
  
  auto device_on_self_managed_io_init(WDFDEVICE device) -> NTSTATUS;
  auto device_on_self_managed_io_cleanup(WDFDEVICE device) -> void;
  
  auto device_on_file_create(WDFDEVICE device, WDFREQUEST request, WDFFILEOBJECT file) -> void;
  auto device_on_file_close(WDFFILEOBJECT file) -> void;

} // namespace lj
