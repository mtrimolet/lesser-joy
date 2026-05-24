module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <bthguid.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthsdpddi.h>

#include <stormkit/core/macro.hpp>

module lesserjoy.bluetooth;

import log;
import utilities;

import :device;

import :guids;
import :connection;
import :client;
import :queue;

#ifdef ALLOC_PRAGMA
  #pragma alloc_text (PAGE, lj_device_add)
  #pragma alloc_text (PAGE, lj_device_cleanup)
  #pragma alloc_text (PAGE, lj_device_on_self_managed_io_init)
  #pragma alloc_text (PAGE, lj_device_on_self_managed_io_cleanup)
  #pragma alloc_text (PAGE, lj_device_on_file_create)
  #pragma alloc_text (PAGE, lj_device_on_file_close)
#endif

extern "C" {

  _Use_decl_annotations_
  NTSTATUS lj_device_add(WDFDRIVER driver, WDFDEVICE_INIT* device_init) {
    return lj::device_add(driver, device_init);
  }

} // extern "C"

namespace lj {

  auto device_create(WDFDRIVER driver, WDFDEVICE_INIT* device_init) -> result<WDFDEVICE, NTSTATUS> {
    auto pnp_power_callbacks = WDF_PNPPOWER_EVENT_CALLBACKS {};
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnp_power_callbacks);
    pnp_power_callbacks.EvtDeviceSelfManagedIoInit    = lj_device_on_self_managed_io_init;
    pnp_power_callbacks.EvtDeviceSelfManagedIoCleanup = lj_device_on_self_managed_io_cleanup;
  
    WdfDeviceInitSetPnpPowerEventCallbacks(device_init, &pnp_power_callbacks);

    auto fileobject_config = WDF_FILEOBJECT_CONFIG {};
    WDF_FILEOBJECT_CONFIG_INIT(&fileobject_config, lj_device_on_file_create, lj_device_on_file_close, WDF_NO_EVENT_CALLBACK);

    // Inform framework to create context area in every fileobject
    // so that we can track information per open handle by the
    // application.

    auto file_attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&file_attributes, lj_FileContext);

    WdfDeviceInitSetFileObjectConfig(device_init, &fileobject_config, &file_attributes);

    // Inform framework to create context area in every request object.
    //
    // We make BRB as the context since we need BRB for all the requests
    // we handle (Create, Read, Write).

    auto request_attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&request_attributes, lj_RequestContext);

    WdfDeviceInitSetRequestAttributes(device_init, &request_attributes);

    auto device_attributes  = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&device_attributes, lj_DeviceContext);

    auto device = WDFDEVICE {};
    NT_Try(WdfDeviceCreate(&device_init, &device_attributes, &device));

    return device;
  }

  auto queue_create(WDFDEVICE device) -> result<WDFQUEUE, NTSTATUS> {
    auto io_queue_config = WDF_IO_QUEUE_CONFIG {};
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&io_queue_config, WdfIoQueueDispatchParallel);

    io_queue_config.EvtIoRead  = lj_queue_on_io_read;
    io_queue_config.EvtIoWrite = lj_queue_on_io_write;
    io_queue_config.EvtIoStop  = lj_queue_on_io_stop;

    auto queue = WDFQUEUE {};
    NT_Try(WdfIoQueueCreate(device, &io_queue_config, WDF_NO_OBJECT_ATTRIBUTES, &queue));

    return queue;
  }

  auto device_add(WDFDRIVER driver, WDFDEVICE_INIT* device_init) -> NTSTATUS {
    return ([&] -> result<void, NTSTATUS> {
      debug_logln("Initializing bluetooth device...");

      auto device = Try(device_create(driver, device_init));
      debug_logln("...device created");

      Try(DeviceContext::get(device).init(device));
      debug_logln("...device context initialized");

      Try(query_bluetooth_interfaces(device));
      debug_logln("...queried bluetooth interfaces");

      auto queue = Try(queue_create(device));
      debug_logln("...device queue created");

      NT_Try(WdfDeviceCreateDeviceInterface(device, &LESSERJOY_BLUETOOTH_DEVIFACE_GUID, nullptr));
      debug_logln("...device interface created");

      debug_logln("Initialized bluetooth device!");
      return {};
    })()
    .and_then([] (auto res) static -> result<void, NTSTATUS> {
      return Unexpected{ STATUS_SUCCESS };
    })
    .error();
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_device_cleanup(WDFOBJECT object) -> void {
    lj::device_cleanup(object);
  }

} // extern "C"

namespace lj {

  auto device_cleanup(WDFOBJECT object) -> void {
    debug_logln("Uninitialized bluetooth device!");
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_device_on_self_managed_io_init(WDFDEVICE device) -> NTSTATUS {
    return lj::device_on_self_managed_io_init(device);
  }

} // extern "C"

namespace lj {
/*++

Description:

    This routine is called by the framework only once
    and hence we use it for our one time initialization.

    Bth addresses for both our local device and server device
    do not change, hence we retrieve them here.

    Features that the local device supports also do not change,
    so checking for local L2cap support is done here and saved in the
    device context header

    Please note that retrieveing server bth address does not
    require presence of the server. It is remembered from
    the installation time when the client gets installed for a
    specific server.

Arguments:

    Device - Framework device object

Return Value:

    auto Status code.

--*/
  auto device_on_self_managed_io_init(WDFDEVICE device) -> NTSTATUS {
    return ([&] -> result<void, NTSTATUS> {
      debug_logln("Initializing bluetooth device self-managed io...");

      Try(DeviceContext::get(device).get_header().retrieve_local_info());
      debug_logln("...retrieved local info");

      Try(retrieve_server_bth_address(device));
      debug_logln("...retrieved server bluetooth address");

      debug_logln("Initialized bluetooth device self-managed io!");
      return {};
    })()
    .and_then([] (auto res) static -> lj::result<void, NTSTATUS> {
      return lj::Unexpected{ STATUS_SUCCESS };
    })
    .error();
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_device_on_self_managed_io_cleanup(WDFDEVICE device) -> void {
    lj::device_on_self_managed_io_cleanup(device);
  }

} // extern "C"

namespace lj {

  auto device_on_self_managed_io_cleanup(WDFDEVICE device) -> void {
    debug_logln("Uninitialized bluetooth device self-managed io!");
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_device_on_file_create(WDFDEVICE device, WDFREQUEST request, WDFFILEOBJECT file) -> void {
    lj::device_on_file_create(device, request, file);
  }

} // extern "C"

namespace lj {

  auto device_on_file_create(WDFDEVICE device, WDFREQUEST request, WDFFILEOBJECT file) -> void {
    ([&] -> result<void, NTSTATUS> {
      debug_logln("Initializing bluetooth device connection file...");

      auto& device_context = DeviceContext::get(device);

      //
      // We need to obtained PSM from the server
      // For this, first we retrieve the SDP record
      //
      auto server_sdp_record         = Try(retrieve_server_sdp_record(device));
      auto server_sdp_record_dispose = DisposeHandler([server_sdp_record] {
        ExFreePoolWithTag(server_sdp_record, LESSERJOY_BLUETOOTH_POOLTAG);
      });
      debug_logln("...retrieved server_sdp_record");

      auto sdp_parse_interface = BTHDDI_SDP_PARSE_INTERFACE {};
      NT_Try(WdfFdoQueryForInterface(
        device_context.header.device,
        &GUID_BTHDDI_SDP_PARSE_INTERFACE,
        (INTERFACE*) &sdp_parse_interface,
        sizeof(sdp_parse_interface), 
        BTHDDI_SDP_PARSE_INTERFACE_VERSION_FOR_QI, 
        nullptr
      ));
      debug_logln("...queried sdp parse interface");

      //
      // Once we retrieved the server SDP record, we retrieve
      // PSM from this record.
      //
      // We store this Psm in our file context and use it in open
      // and close channel BRBs.
      //
      auto& file_context = FileContext::get(file);
      file_context.server_psm = Try(retrieve_psm_from_sdp_record(sdp_parse_interface, *server_sdp_record));
      debug_logln("...retrieved psm from sdp record");

      Try(open_remote_connection(device, request, file));
      debug_logln("...opened connection");

      debug_logln("Initialized bluetooth device connection file!");
      return {};
    })()
    .map_error([&request] (auto status) {
      KdPrint(("Failed to initialize lesser-joy bluetooth device connection file! status: 0x%x\n", status));
      WdfRequestComplete(request, status);
      return status;
    });
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_device_on_file_close(WDFFILEOBJECT file) -> void {
    lj::device_on_file_close(file);
  }

} // extern "C"

namespace lj {

  auto device_on_file_close(WDFFILEOBJECT file) -> void {
    debug_logln("Uninitializing bluetooth device connection file...");

    auto& device_context = DeviceContext::get(WdfFileObjectGetDevice(file));
    auto& file_context   = FileContext::get(file);

    file_context.get_connection().remote_disconnect_sync(device_context.get_header());

    debug_logln("Uninitialized bluetooth device connection file!");
  }

} // namespace lj
