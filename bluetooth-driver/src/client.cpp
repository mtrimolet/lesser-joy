module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <initguid.h>
#include <bthguid.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthddi.h>
#include <bthsdpddi.h>

#include <stormkit/core/macro.hpp>

module lesserjoy.bluetooth;

import utilities;

import :guids;
import :client;

#ifdef ALLOC_PRAGMA
  #pragma alloc_text (PAGE, lj_client_indication_callback)
  #pragma alloc_text (PAGE, lj_remote_connect_completion)
#endif

// extern "C" _IRQL_requires_max_(PASSIVE_LEVEL)
// auto lj_query_bluetooth_interfaces(lj::DeviceContext* context) -> NTSTATUS {
//   auto success = lj::query_bluetooth_interfaces(*context);
//   if (success) return STATUS_SUCCESS;
//   return success.error();
// }

namespace lj {

  auto query_bluetooth_interfaces(WDFDEVICE device) -> result<void, NTSTATUS> {
    auto& device_context = DeviceContext::get(device);

    NT_Try(WdfFdoQueryForInterface(
      device_context.header.device,
      &GUID_BTHDDI_PROFILE_DRIVER_INTERFACE,
      (PINTERFACE) (&device_context.header.profile_drv_interface),
      sizeof(device_context.header.profile_drv_interface), 
      BTHDDI_PROFILE_DRIVER_INTERFACE_VERSION_FOR_QI, 
      nullptr
    ));

    return {};
  }

} // namespace lj

// _IRQL_requires_max_(PASSIVE_LEVEL)
// auto lj_retrieve_server_bth_address(lj::DeviceContext* context) -> NTSTATUS {
//   auto success = lj::retrieve_server_bth_address(*context);
//   if (success) return STATUS_SUCCESS;
//   return success.error();
// }

namespace lj {

  auto request_reuse(WDFREQUEST request) -> void {
    // auto reuse_params = WDF_REQUEST_REUSE_PARAMS {};
    // WDF_REQUEST_REUSE_PARAMS_INIT(&reuse_params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    NT_ASSERT(NT_SUCCESS(WdfRequestReuse(request, nullptr)));
  }

  auto retrieve_server_bth_device_info(WDFDEVICE device) -> result<BTH_DEVICE_INFO, NTSTATUS> {
    auto server_device_info = BTH_DEVICE_INFO {};
    RtlZeroMemory(&server_device_info, sizeof(server_device_info));

    auto memory_desc = WDF_MEMORY_DESCRIPTOR {};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
      &memory_desc,
      &server_device_info,
      sizeof(server_device_info)
    );

    auto& device_context = DeviceContext::get(device);
    request_reuse(device_context.header.request);
    NT_Try(WdfIoTargetSendInternalIoctlSynchronously(
      device_context.header.io_target,
      device_context.header.request,
      IOCTL_INTERNAL_BTHENUM_GET_DEVINFO,
      nullptr,
      &memory_desc,
      nullptr,
      nullptr
    ));

    return server_device_info;
  }

  auto retrieve_server_bth_address(WDFDEVICE device) -> result<void, NTSTATUS> {
    auto server_device_info = Try(retrieve_server_bth_device_info(device));

    auto& device_context = DeviceContext::get(device);
    device_context.server_bth_address = server_device_info.address;

    return {};
  }

} // namespace lj

// extern "C" _IRQL_requires_max_(PASSIVE_LEVEL)
// auto lj_retrieve_server_sdp_record(lj::DeviceContext* context, BTH_SDP_STREAM_RESPONSE** server_sdp_record) -> NTSTATUS {
//   auto success = lj::retrieve_server_sdp_record(*context, server_sdp_record);
//   if (success) return STATUS_SUCCESS;
//   return success.error();
// }

namespace lj {
  auto request_create(WDFDEVICE device) -> result<WDFREQUEST, NTSTATUS> {
    auto& device_context = DeviceContext::get(device);

    auto request = WDFREQUEST {};
    NT_Try(WdfRequestCreate(nullptr, device_context.header.io_target, &request));

    return request;
  }

  auto sdp_connect(WDFDEVICE device, WDFREQUEST request) -> result<HANDLE_SDP, NTSTATUS> {
    auto& device_context = DeviceContext::get(device);

    auto connect_sdp = BTH_SDP_CONNECT {
      .bthAddress = device_context.server_bth_address,
      .fSdpConnect = 0,
      .requestTimeout = SDP_REQUEST_TO_DEFAULT,
    };

    auto in_memory_desc = WDF_MEMORY_DESCRIPTOR {};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&in_memory_desc, &connect_sdp, sizeof(connect_sdp));

    auto out_memory_desc = WDF_MEMORY_DESCRIPTOR {};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&out_memory_desc, &connect_sdp, sizeof(connect_sdp));

    NT_Try(WdfIoTargetSendIoctlSynchronously(
      device_context.header.io_target,
      request,
      IOCTL_BTH_SDP_CONNECT,
      &in_memory_desc,
      &out_memory_desc,
      nullptr,   //sendOptions
      nullptr    //bytesReturned
    ));

    return connect_sdp.hConnection;
  }
  
  auto sdp_disconnect(WDFDEVICE device, WDFREQUEST request, HANDLE_SDP connection) -> result<void, NTSTATUS> {
    auto disconnect_sdp = BTH_SDP_CONNECT {
      .hConnection = connection
    };

    auto in_memory_desc = WDF_MEMORY_DESCRIPTOR {};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&in_memory_desc, &disconnect_sdp, sizeof(disconnect_sdp));

    auto& device_context = DeviceContext::get(device);
    NT_Try(WdfIoTargetSendIoctlSynchronously(
      device_context.header.io_target,
      request,
      IOCTL_BTH_SDP_DISCONNECT,
      &in_memory_desc,
      nullptr,
      nullptr,
      nullptr
    ));

    return {};
  }

  auto allocate_server_sdp_record(ULONG request_size) -> result<BTH_SDP_STREAM_RESPONSE*, NTSTATUS> {
    NT_Try(RtlULongAdd(request_size, sizeof(BTH_SDP_STREAM_RESPONSE), &request_size));
    
    auto server_sdp_record = (BTH_SDP_STREAM_RESPONSE*)ExAllocatePoolZero(
      (enum _POOL_TYPE)POOL_FLAG_NON_PAGED,
      request_size,
      LESSERJOY_BLUETOOTH_POOLTAG
    );
    // *server_sdp_record = ExAllocatePool2(POOL_FLAG_NON_PAGED, request_size, LESSERJOY_BLUETOOTH_POOLTAG);
    Assert(server_sdp_record != nullptr, STATUS_INSUFFICIENT_RESOURCES);

    return server_sdp_record;
  }

  auto retrieve_server_sdp_record(
    WDFDEVICE device,
    WDFREQUEST request,
    HANDLE_SDP connection
  ) -> result<BTH_SDP_STREAM_RESPONSE*, NTSTATUS> {
    auto request_sdp = BTH_SDP_SERVICE_ATTRIBUTE_SEARCH_REQUEST {
      .hConnection = connection,
      .uuids = { { .u = { .uuid128 = NINTENDO_SW2 }, .uuidType = SDP_ST_UUID128 } },
      .range = { { .minAttribute = 0, .maxAttribute = 0xFFFF } },
    };

    auto response_sdp = BTH_SDP_STREAM_RESPONSE {};

    auto in_memory_desc = WDF_MEMORY_DESCRIPTOR {};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&in_memory_desc, &request_sdp, sizeof(request_sdp));

    auto out_memory_desc = WDF_MEMORY_DESCRIPTOR {};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&out_memory_desc, &response_sdp, sizeof(response_sdp));

    auto& device_context = DeviceContext::get(device);
    request_reuse(request);
    NT_Try(WdfIoTargetSendIoctlSynchronously(
      device_context.header.io_target,
      request,
      IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH,
      &in_memory_desc,
      &out_memory_desc,
      nullptr,   //sendOptions
      nullptr    //bytesReturned
    ));

    auto server_sdp_record = Try(allocate_server_sdp_record(response_sdp.requiredSize));

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&in_memory_desc, &request_sdp, sizeof(request_sdp));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&out_memory_desc, server_sdp_record, response_sdp.requiredSize);

    request_reuse(request);
    NT_Try(WdfIoTargetSendIoctlSynchronously(
      device_context.header.io_target,
      request,
      IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH,
      &in_memory_desc,
      &out_memory_desc,
      nullptr,   //sendOptions
      nullptr    //bytesReturned
    ));

    return server_sdp_record;
  }

  auto retrieve_server_sdp_record(WDFDEVICE device) -> result<BTH_SDP_STREAM_RESPONSE*, NTSTATUS> {
    auto& device_context = DeviceContext::get(device);
    
    auto request = Try(request_create(device));
    auto request_dispose = DisposeHandler([&request] {
      WdfObjectDelete(request);
    });

    auto connection = Try(sdp_connect(device, request));
    auto connection_dispose = DisposeHandler([&device, &request, &connection] {
      request_reuse(request);
      NT_ASSERT(sdp_disconnect(device, request, connection).has_value());
    });

    return Try(retrieve_server_sdp_record(device, request, connection));
  }

} // namespace lj

// _IRQL_requires_same_
// auto lj_open_remote_connection(lj::DeviceContext* context, WDFFILEOBJECT file_object, WDFREQUEST request) -> NTSTATUS {
//   auto success = lj::open_remote_connection(*context, file_object, request);
//   if (success) return STATUS_SUCCESS;
//   return success.error();
// }

namespace lj {

  auto retrieve_psm_from_sdp_record(
    BTHDDI_SDP_PARSE_INTERFACE& sdp_parse_interface,
    BTH_SDP_STREAM_RESPONSE& server_sdp_record
  ) -> result<USHORT, NTSTATUS> {
    // https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/host/service-discovery-protocol--sdp--specification.html
    
    auto next_element      = (UCHAR*) {};
    auto next_element_size = ULONG  {};
    sdp_parse_interface.SdpGetNextElement(
      &(server_sdp_record.response[0]),
      server_sdp_record.responseSize,
      nullptr,
      &next_element,
      &next_element_size
    );
    Assert(next_element_size != 0, STATUS_DEVICE_DATA_ERROR);

    auto sdp_tree = (SDP_TREE_ROOT_NODE*) {};
    NT_Try(sdp_parse_interface.SdpConvertStreamToTree(
      next_element,
      next_element_size,
      &sdp_tree,
      LESSERJOY_BLUETOOTH_POOLTAG
    ));
    auto sdp_tree_dispose = DisposeHandler([&sdp_parse_interface, &sdp_tree] {
      sdp_parse_interface.SdpFreeTree(sdp_tree);
    });

    auto node_proto_desc_list = (SDP_NODE*) {};
    NT_Try(sdp_parse_interface.SdpFindAttributeInTree(
      sdp_tree,
      (USHORT)SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST,
      &node_proto_desc_list
    ));
    Assert(node_proto_desc_list->hdr.Type == SDP_TYPE_SEQUENCE, STATUS_DEVICE_DATA_ERROR);

    Assert(node_proto_desc_list->u.sequence.Link.Flink != nullptr, STATUS_DEVICE_DATA_ERROR);
    auto node_proto_0 = CONTAINING_RECORD(node_proto_desc_list->u.sequence.Link.Flink, SDP_NODE, hdr.Link);
    Assert(node_proto_0->hdr.Type == SDP_TYPE_SEQUENCE, STATUS_DEVICE_DATA_ERROR);

    Assert(node_proto_0->u.sequence.Link.Flink != nullptr, STATUS_DEVICE_DATA_ERROR);
    auto node_proto_0_UUID = CONTAINING_RECORD(node_proto_0->u.sequence.Link.Flink, SDP_NODE, hdr.Link);
    Assert(node_proto_0_UUID->hdr.Type == SDP_TYPE_UUID, STATUS_DEVICE_DATA_ERROR);

    Assert(node_proto_0_UUID->hdr.Link.Flink != nullptr, STATUS_DEVICE_DATA_ERROR);
    auto node_proto_0_s_param_0 = CONTAINING_RECORD(node_proto_0_UUID->hdr.Link.Flink, SDP_NODE, hdr.Link);
    Assert(node_proto_0_s_param_0->hdr.SpecificType == SDP_ST_UINT16, STATUS_DEVICE_DATA_ERROR);

    auto psm = node_proto_0_s_param_0->u.uint16;

    return psm;
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_client_indication_callback(
    void* connection,
    INDICATION_CODE indication,
    INDICATION_PARAMETERS_ENHANCED* parameters
  ) -> void {
    lj::client_indication_callback(connection, indication, parameters);
  }

} // extern "C"

namespace lj {

/*++

Description:

    Indication callback passed to bth stack while sending open channel BRB
    Bth stack sends notification related to the connection.
    
Arguments:

    Context - We receive data connection as the context
    Indication - Type of indication
    Parameters - Parameters of indication

--*/
  auto client_indication_callback(
    void* connection,
    INDICATION_CODE indication,
    INDICATION_PARAMETERS_ENHANCED* parameters
  ) -> void {
    auto& connection_context = *(ConnectionContext*)connection;
    //
    // Only supporting connect and disconnect
    //

    switch(indication) {
      //
      // We don't add/release reference to anything because our connection
      // is scoped within file object lifetime
      //
      case IndicationAddReference:
      case IndicationReleaseReference:
        break;
      case IndicationRemoteConnect:
        //
        // We don't expect connection
        //
        NT_ASSERT(false);
        break;
      case IndicationRemoteDisconnect:
        //
        // This is an indication that server has disconnected
        // In response we disconnect from our end
        //

        connection_context.remote_disconnect(connection_context.get_header());

        break;
      default:
        break;
    }
  }

/*++
Description:

    This routine is invoked by remote_connect_completion
    function in client.cppm when opening a remote
    connection is completed.

    In this routine we set the file context to the connection passed in.

Arguments:

    FileObject - File object whose open resulted in open connection
    Connection - Our data strucutre to track open connection

Return Value:

    auto Status code.
--*/
  auto on_connection_state_connected(FileContext& file_context, ConnectionContext& connection_context) -> result<void, NTSTATUS> {
    file_context.connection = &connection_context;
    return {};
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_remote_connect_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT                     context
  ) -> void {
    lj::remote_connect_completion(request, target, params, context);
  }

}

namespace lj {

/*++
Description:

    Completion routine for Create request which we format as open
    channel BRB and send down the stack. We complete the Create request 
    in this routine.

    We receive open channel BRB as the context. This BRB
    is part of the request context and doesn't need to be freed
    explicitly.

    Connection is part of the context in the BRB.

Arguments:

    Request - Create request that we formatted with open channel BRB
    Target - Target to which we sent the request
    Params - Completion params
    Context - We receive BRB as the context          

Return Value:

    NTSTATUS Status code.
--*/
  auto remote_connect_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT                     context
  ) -> void {
    ([&] -> result<void, NTSTATUS> {
      NT_ASSERT(params != nullptr);

      auto& brb_l2ca = *(struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *) context;
      auto& connection_context = *(ConnectionContext*) brb_l2ca.Hdr.ClientContext[0];

      // TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT,
      //     "Connection completion, status: %!STATUS!", status);

      //
      // In the client we don't check for ConnectionStateDisconnecting state 
      // because only file close generates disconnect which
      // cannot happen before create completes. And we complete Create
      // only after we process this completion.
      //
      if (auto& status = params->IoStatus.Status; not NT_SUCCESS(status)) {
          connection_context.state = ConnectionContext::ConnectionState::CONNECTFAILED;
          return Unexpected{ status };
      }

      connection_context.out_mtu        = brb_l2ca.OutResults.Params.Mtu;
      connection_context.in_mtu         = brb_l2ca.InResults.Params.Mtu;
      connection_context.channel_handle = brb_l2ca.ChannelHandle;
      connection_context.remote_address = brb_l2ca.BtAddress;

      connection_context.state = ConnectionContext::ConnectionState::CONNECTED;

      // TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
      //     "Connection (0x%x) established to server", brb->OutResults.Params.RetransmissionAndFlow.Mode); 

      //
      // Call the function in device.c (BthEchoCliConnectionStateConnected)
      // for any post processing after connection has been established
      //
      auto& file_context   = FileContext::get(WdfRequestGetFileObject(request));
      auto& device_context = DeviceContext::get(WdfIoTargetGetDevice(target));

      return on_connection_state_connected(file_context, connection_context)
      .map_error([&connection_context, &device_context](auto status) {
        connection_context.remote_disconnect(device_context.get_header());
        return status;
      });
    })();
  }
/*++

Description:

    This routine is invoked by BthEchoCliEvtDeviceFileCreate.
    In this routine we send down open channel BRB.

    This routine allocates open channel BRB. If the request
    is sent down successfully completion routine needs to free
    this BRB.
    
Arguments:

    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx - 
    _In_ WDFFILEOBJECT FileObject - 
    _In_ WDFREQUEST Request - 

Return Value:

    NTSTATUS Status code.

--*/
  auto open_remote_connection(WDFDEVICE device, WDFREQUEST request, WDFFILEOBJECT file) -> result<void, NTSTATUS> {
    auto& device_context = DeviceContext::get(device);
    //
    // Create the connection object that would store information
    // about the open channel
    //
    // Set file object as the parent for this connection object
    //
    auto connection = Try(ConnectionContext::object_create(device_context.get_header(), file));

    auto& connection_context = ConnectionContext::get(connection);
    connection_context.state = ConnectionContext::ConnectionState::CONNECTING;

    auto& file_context = FileContext::get(file);

    auto& brb = RequestContext::get(request);
    //
    // Get the BRB from request context and initialize it as
    // BRB_L2CA_OPEN_CHANNEL BRB
    //
    device_context.header.profile_drv_interface.BthReuseBrb(&brb, BRB_L2CA_OPEN_ENHANCED_CHANNEL);
    auto& brb_l2ca = *(struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *)&brb;
    
    brb_l2ca.Hdr.ClientContext[0] = &connection_context;
    brb_l2ca.BtAddress            = device_context.server_bth_address;
    brb_l2ca.Psm                  = file_context.server_psm;

    brb_l2ca.ChannelFlags = CF_ROLE_EITHER;

    brb_l2ca.ConfigOut.Flags = CFG_ENHANCED;
    //
    // Open an ERTM channel if the local host supports it
    //
    if (device_context.header.local_features.Mask & BTH_HOST_FEATURE_ENHANCED_RETRANSMISSION_MODE) {
      brb_l2ca.ConfigOut.ModeConfig.Flags = CM_BASIC | CM_RETRANSMISSION_AND_FLOW;

      //
      // Mode is specified using Flags above and this should be 0.
      //
      brb_l2ca.ConfigOut.ModeConfig.RetransmissionAndFlow.Mode         = 0;
      brb_l2ca.ConfigOut.ModeConfig.RetransmissionAndFlow.MaxTransmit  = L2CAP_RAF_DEFAULT_MAXTRANSMIT;
      brb_l2ca.ConfigOut.ModeConfig.RetransmissionAndFlow.MaxPDUSize   = L2CAP_RAF_DEFAULT_MAX_PDU_SIZE;
      brb_l2ca.ConfigOut.ModeConfig.RetransmissionAndFlow.TxWindowSize = L2CAP_RAF_DEFAULT_TX_WINDOW_SIZE;
    }

    brb_l2ca.ConfigOut.Flags         |= CFG_MTU;
    brb_l2ca.ConfigOut.Mtu.Max        = L2CAP_DEFAULT_MTU;
    brb_l2ca.ConfigOut.Mtu.Min        = L2CAP_MIN_MTU;
    brb_l2ca.ConfigOut.Mtu.Preferred  = L2CAP_DEFAULT_MTU;

    brb_l2ca.ConfigIn.Flags         = CFG_MTU;
    brb_l2ca.ConfigIn.Mtu.Max       = brb_l2ca.ConfigOut.Mtu.Max;
    brb_l2ca.ConfigIn.Mtu.Min       = brb_l2ca.ConfigOut.Mtu.Min;
    brb_l2ca.ConfigIn.Mtu.Preferred = brb_l2ca.ConfigOut.Mtu.Max;

    //
    // Get notification about remote disconnect 
    //
    brb_l2ca.CallbackFlags = CALLBACK_DISCONNECT;

    brb_l2ca.Callback           = lj_client_indication_callback;
    brb_l2ca.CallbackContext    = &connection_context;
    brb_l2ca.ReferenceObject    = (void*) WdfDeviceWdmGetDeviceObject(device_context.header.device);
    brb_l2ca.IncomingQueueDepth = 50;

    return send_brb_async(
      device_context.header.io_target,
      request,
      (BRB&)brb_l2ca,
      sizeof(brb_l2ca),
      lj_remote_connect_completion,
      (WDFCONTEXT)&brb
    )
    .map_error([&connection_context](auto status) {
      connection_context.state = ConnectionContext::ConnectionState::CONNECTFAILED;
      return status;
    });
  }

} // namespace lj
