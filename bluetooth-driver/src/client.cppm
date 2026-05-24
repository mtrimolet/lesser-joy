module;

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthddi.h>
#include <bthsdpddi.h>

export module lesserjoy.bluetooth:client;

import result;

import :context;
import :connection;

extern "C" {

  auto lj_client_indication_callback(
    void*                           connection,
    INDICATION_CODE                 indication,
    INDICATION_PARAMETERS_ENHANCED* parameters
  ) -> void;
  EVT_WDF_REQUEST_COMPLETION_ROUTINE lj_remote_connect_completion;

} // extern "C"

export namespace lj {

  auto query_bluetooth_interfaces(WDFDEVICE device) -> result<void, NTSTATUS>;
  auto retrieve_server_bth_address(WDFDEVICE device) -> result<void, NTSTATUS>;
  auto retrieve_server_sdp_record(WDFDEVICE device) -> result<BTH_SDP_STREAM_RESPONSE*, NTSTATUS>;
  auto retrieve_psm_from_sdp_record(
    BTHDDI_SDP_PARSE_INTERFACE& sdp_parse_interface,
    BTH_SDP_STREAM_RESPONSE&    server_sdp_record
  ) -> result<USHORT, NTSTATUS>;

  auto client_indication_callback(
    void*                           connection,
    INDICATION_CODE                 indication,
    INDICATION_PARAMETERS_ENHANCED* parameters
  ) -> void;

  auto remote_connect_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT                     context
  ) -> void;

  auto open_remote_connection(WDFDEVICE device, WDFREQUEST request, WDFFILEOBJECT file) -> result<void, NTSTATUS>;

} // namespace lj
