module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <bthddi.h>

#include <stormkit/core/macro.hpp>

module lesserjoy.bluetooth;

import log;
import utilities;

import :queue;

import :context;
import :connection;

#ifdef ALLOC_PRAGMA
  #pragma alloc_text (PAGE, lj_queue_on_read_write_completion)
  #pragma alloc_text (PAGE, lj_queue_on_io_write)
  #pragma alloc_text (PAGE, lj_queue_on_io_read)
  #pragma alloc_text (PAGE, lj_queue_on_io_stop)
#endif

extern "C" {

  _Use_decl_annotations_
  auto lj_queue_on_read_write_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT                     context
  ) -> void {
    lj::queue_on_read_write_completion(request, target, params, context);
  }

} // extern "C"

namespace lj {

/*++
Description:

    Completion routine for read/write requests

    We receive l2ca transfer BRB as the context. This BRB
    is part of the request context and doesn't need to be freed
    explicitly.
    
Arguments:

    Request - Request that got completed
    Target - Target to which request was sent
    Params - Completion parameters for the request
    Context - We receive BRB as the context

--*/
  auto queue_on_read_write_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT                     context
  ) -> void {
    debug_logln("Completing bluetooth device read/write...");

    auto brb = (struct _BRB_L2CA_ACL_TRANSFER *) context;
    NT_ASSERT((brb != nullptr));

    auto& bytes_transfered = brb->BufferSize;
    WdfRequestCompleteWithInformation(request, params->IoStatus.Status, bytes_transfered);

    debug_logln("Completed bluetooth device read/write!");
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_queue_on_io_write(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void {
    lj::queue_on_io_write(queue, request, length);
  }

} // extern "C"

namespace lj {

/*++
Description:

    This routine is invoked by the framework to deliver a
    Write request to the driver.

Arguments:

    Queue - Queue delivering the request
    Request - Write request
    Length - Length of write

--*/
  auto queue_on_io_write(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void {
    ([&] -> result<void, NTSTATUS> {
      debug_logln("Starting bluetooth device write...");

      auto& device_context = DeviceContext::get(WdfIoQueueGetDevice(queue));

      auto memory = WDFMEMORY {};
      NT_Try(WdfRequestRetrieveInputMemory(request, &memory));
      debug_logln("...retrieved input memory");

      //
      // Get the BRB from request context and initialize it as
      // BRB_L2CA_ACL_TRANSFER BRB
      //
      auto& brb = RequestContext::get(request);
      device_context.header.profile_drv_interface.BthReuseBrb(&brb, BRB_L2CA_ACL_TRANSFER);
      auto& brb_lc2a = *(struct _BRB_L2CA_ACL_TRANSFER *)&brb;

      //
      // Format the Write request for L2Ca OUT transfer
      //
      // This routine allocates a BRB which is returned to us
      // in brb parameter
      //
      // This BRB is freed by the completion routine if we send the
      // request successfully, else it is freed by this routine.
      //
      auto& connection_context = FileContext::get(WdfRequestGetFileObject(request)).get_connection();
      Try(connection_context.format_request_for_l2ca_transfer(request, brb_lc2a, memory, ACL_TRANSFER_DIRECTION_OUT));
      debug_logln("...formatted lc2a acl transfer request");

      WdfRequestSetCompletionRoutine(request, queue_on_read_write_completion, &brb_lc2a);
      Assert(
        WdfRequestSend(request, device_context.header.io_target, nullptr),
        WdfRequestGetStatus(request)
      );

      debug_logln("...sent write request");

      debug_logln("Started bluetooth device write!");

      return {};
    })()
    .map_error([&request](auto status) {
      WdfRequestComplete(request, status);
      return status;
    });
  }

extern "C" {

  _Use_decl_annotations_
  auto lj_queue_on_io_read(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void {
    lj::queue_on_io_read(queue, request, length);
  }

} // extern "C"


/*++
Description:

    This routine is invoked by the framework to deliver a
    Read request to the driver.

Arguments:

    Queue - Queue delivering the request
    Request - Read request
    Length - Length of Read

--*/
  auto queue_on_io_read(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void {
    ([&] -> result<void, NTSTATUS> {
      debug_logln("Starting bluetooth device read...");

      auto& device_context = DeviceContext::get(WdfIoQueueGetDevice(queue));

      auto memory = WDFMEMORY {};
      NT_Try(WdfRequestRetrieveOutputMemory(request, &memory));
      debug_logln("...retrieved output memory");

      auto& brb = RequestContext::get(request);
      device_context.get_header().profile_drv_interface.BthReuseBrb(&brb, BRB_L2CA_ACL_TRANSFER);
      auto& brb_lc2a = *(struct _BRB_L2CA_ACL_TRANSFER *)&brb;

      auto& connection_context = FileContext::get(WdfRequestGetFileObject(request)).get_connection();
      Try(connection_context.format_request_for_l2ca_transfer(
        request,
        brb_lc2a,
        memory,
        ACL_TRANSFER_DIRECTION_IN | ACL_SHORT_TRANSFER_OK
      ));
      debug_logln("...formatted lc2a acl transfer request");

      WdfRequestSetCompletionRoutine(request, lj_queue_on_read_write_completion, &brb_lc2a);
      Assert(
        WdfRequestSend(request, device_context.header.io_target, nullptr),
        WdfRequestGetStatus(request)
      );
      debug_logln("...sent read request");

      debug_logln("Started bluetooth device read!");

      return {};
    })()
    .map_error([&request](auto status) {
      KdPrint(("Failed to start lesser-joy bluetooth device read! status: 0x%x\n", status));
      WdfRequestComplete(request, status);
      return status;
    });
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_queue_on_io_stop(WDFQUEUE queue, WDFREQUEST request, ULONG action_flags) -> void {
    lj::queue_on_io_stop(queue, request, action_flags);
  }

} // extern "C"

namespace lj {

/*++
Description:

    This routine is invoked by Framework when Queue is being stopped

    We implement this routine to cancel any requests owned by our driver.
    
    Without this Queue stop would wait indefinitely for requests to complete
    during surprise remove.

Arguments:

    Queue - Framework queue being stopped
    Request - Request owned by the driver
    ActionFlags - Action flags

--*/
  auto queue_on_io_stop(WDFQUEUE queue, WDFREQUEST request, ULONG action_flags) -> void {
    debug_logln("Stopping bluetooth device queue...");

    WdfRequestCancelSentRequest(request);

    debug_logln("Stopped bluetooth device queue!");
  }

} // namespace lj
