module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <bthioctl.h>
#include <bthddi.h>

#include <stormkit/core/macro.hpp>

#define LESSERJOY_BLUETOOTH_POOLTAG  'tbjl'

module lesserjoy.bluetooth;

import log;
import utilities;

import :context;

namespace lj {
  auto DeviceContext::init(WDFDEVICE device) -> result<void, NTSTATUS> {
    return get_header().init(device);
  }

/*++

Description:

    Initializes the common context header between server and client

Arguments:

    Header - Contex header
    Device - Framework device object

Return Value:

    NTSTATUS Status code.

--*/
  auto ContextHeader::init(WDFDEVICE _device) -> result<void, NTSTATUS> {
    debug_logln("Initializing bluetooth device context header...");
    device    = _device;
    io_target = WdfDeviceGetIoTarget(device);

    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    NT_Try(WdfRequestCreate(&attributes, io_target, &request));

    debug_logln("Initialized bluetooth device context header!");

    return {};
  }

/*++

Description:

    Retrieves the local bth address.
    This address is burnt into device hence doesn't change.

    It also retrieves the local host supported features if available.

Arguments:

    Header - Contex header

Return Value:

    NTSTATUS Status code.

--*/
  auto ContextHeader::retrieve_local_info() -> result<void, NTSTATUS> {
    auto brb = profile_drv_interface.BthAllocateBrb(
      BRB_HCI_GET_LOCAL_BD_ADDR,
      LESSERJOY_BLUETOOTH_POOLTAG
    );
    Assert(brb != nullptr, STATUS_INSUFFICIENT_RESOURCES);

    {
      auto brb_dispose = DisposeHandler([this, brb] {
        profile_drv_interface.BthFreeBrb(brb);
      });

      auto& brb_get_addr = *(struct _BRB_GET_LOCAL_BD_ADDR*)brb;

      Try(send_brb_sync(io_target, request, (BRB&)brb_get_addr, sizeof(brb_get_addr)));

      local_bth_addr = brb_get_addr.BtAddress;

      Try(get_host_supported_features());
    }

    return {};
  }

/*++

Routine Description:

    This routine synchronously checks the local stack's supported features

Arguments:

    DevCtxHdr - Information about the local device

Return Value:

    NTSTATUS Status code.

--*/
  auto ContextHeader::get_host_supported_features() -> result<void, NTSTATUS> {
    local_features.Mask = 0;

    auto out_mem_desc = WDF_MEMORY_DESCRIPTOR {};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&out_mem_desc, &local_features, sizeof(local_features));

    NT_Try(WdfIoTargetSendIoctlSynchronously(
      io_target,
      nullptr,
      IOCTL_BTH_GET_HOST_SUPPORTED_FEATURES,
      nullptr,
      &out_mem_desc,
      nullptr,
      nullptr
    ));

    return {};
  }

/*++

Routine Description:

    This routine formats a request with brb and sends it asynchronously

Arguments:

    IoTarget - Target to send the brb to
    Request - request object to be formatted with brb                
    Brb - Brb to be sent
    BrbSize - size of the Brb data structure
    ComplRoutine - WDF completion routine for the request
                   This must be specified because we are formatting the request
                   and hence not using SEND_AND_FORGET flag
    Context - (optional) context to be passed in to the completion routine

Return Value:

    Success implies that request was sent correctly and completion routine will be called
    for it,
    failure implies it was not sent and caller should complete the request

Notes:

    This routine does not call WdfRequestReuse on the Request passed in.
    Caller must do so before passing in the request, if it is reusing the request.

    This routine does not complete the request in case of failure.
    Caller must complete the request in case of failure.

--*/
  auto send_brb_async(
    WDFIOTARGET                        io_target,
    WDFREQUEST                         request,
    BRB&                               brb,
    size_t                             brb_size,
    PFN_WDF_REQUEST_COMPLETION_ROUTINE compl_routine,
    WDFCONTEXT                         context
  ) -> result<void, NTSTATUS> {
    Assert(brb_size > 0, STATUS_INVALID_PARAMETER);

    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = request;

    auto memory = WDFMEMORY {};
    NT_Try(WdfMemoryCreatePreallocated(&attributes, &brb, brb_size, &memory));

    NT_Try(WdfIoTargetFormatRequestForInternalIoctlOthers(
      io_target,
      request,
      IOCTL_INTERNAL_BTH_SUBMIT_BRB,
      memory,
      nullptr, //OtherArg1Offset
      nullptr, //OtherArg2
      nullptr, //OtherArg2Offset
      nullptr, //OtherArg4
      nullptr  //OtherArg4Offset
    ));

    WdfRequestSetCompletionRoutine(request, compl_routine, context);
    Assert(WdfRequestSend(request, io_target, nullptr), WdfRequestGetStatus(request));

    return {};
  }

/*++

Routine Description:

    This routine formats a request with brb and sends it synchronously

Arguments:

    IoTarget - Target to send the brb to
    Request - request object to be formatted with brb
    Brb - Brb to be sent
    BrbSize - size of the Brb data structure

Return Value:

    NTSTATUS Status code.

Notes:

    This routine does calls WdfRequestReuse on the Request passed in.
    Caller need not do so before passing in the request.

    This routine does not complete the request in case of failure.
    Caller must complete the request in case of failure.

--*/
  auto send_brb_sync(WDFIOTARGET io_target, WDFREQUEST request, BRB& brb, size_t brb_size) -> result<void, NTSTATUS> {
    Assert(brb_size > 0, STATUS_INVALID_PARAMETER);

    auto reuse_params = WDF_REQUEST_REUSE_PARAMS {};
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuse_params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    NT_Try(WdfRequestReuse(request, &reuse_params));

    auto memory_desc = WDF_MEMORY_DESCRIPTOR {};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memory_desc, &brb, brb_size);

    NT_Try(WdfIoTargetSendInternalIoctlOthersSynchronously(
      io_target,
      request,
      IOCTL_INTERNAL_BTH_SUBMIT_BRB,
      &memory_desc,
      nullptr, //OtherArg2
      nullptr, //OtherArg4
      nullptr, //RequestOptions
      nullptr  //BytesReturned
    ));

    return {};
  }
} // namespace lj
