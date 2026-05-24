module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthioctl.h>
#include <bthddi.h>

#include <stormkit/core/macro.hpp>

#define LESSERJOY_BLUETOOTH_POOLTAG  'tbjl'

module lesserjoy.bluetooth;

import utilities;

import :connection;

#ifdef ALLOC_PRAGMA
  #pragma alloc_text (PAGE, lj_reader_pending_read_completion)
  #pragma alloc_text (PAGE, lj_reader_resubmit_read_dpc)
  #pragma alloc_text (PAGE, lj_connection_cleanup)
  #pragma alloc_text (PAGE, lj_connection_disconnect_completion)
  // #pragma alloc_text (PAGE, lj_connection_remote_disconnect_sync)
#endif

namespace lj {

  auto RepeatReader::uninitialize() -> void {
    if (pending_read_request == nullptr)
      return;

    pending_read_request = nullptr;
    stopping = 0;
  }

  auto RepeatReader::init_pending_read(size_t buffer_size) -> result<void, NTSTATUS> {
    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = WdfObjectContextGetObject(connection);

    NT_Try(WdfRequestCreate(&attributes, connection->header->io_target, &pending_read_request));

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = pending_read_request;

    NT_Try(WdfMemoryCreate(
      &attributes,
      NonPagedPoolNx,
      LESSERJOY_BLUETOOTH_POOLTAG,
      buffer_size,
      &pending_read_memory,
      nullptr
    ));

    return {};
  }

  auto RepeatReader::init(ConnectionContext& _connection, size_t buffer_size) -> result<void, NTSTATUS> {
    connection = &_connection;

    Assert(buffer_size <= 0, STATUS_INVALID_PARAMETER);

    Try(init_pending_read(buffer_size)
    .map_error([this] (auto status) {
      uninitialize();
      return status;
    }));

    KeInitializeDpc(&resubmit, lj_reader_resubmit_read_dpc, connection->header);
    KeInitializeEvent(&on_stop, NotificationEvent, true);

    return {};
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_reader_pending_read_completion(
    WDFREQUEST                      request,
    WDFIOTARGET                     target,
    WDF_REQUEST_COMPLETION_PARAMS*  params,
    WDFCONTEXT                      context
  ) -> void {
    lj::RepeatReader::pending_read_completion(request, target, params, context);
  }

} // extern "C"

namespace lj {

  auto RepeatReader::pending_read_completion(
    WDFREQUEST                      request,
    WDFIOTARGET                     target,
    WDF_REQUEST_COMPLETION_PARAMS*  params,
    WDFCONTEXT                      context
  ) -> void {
    NT_ASSERT(params != nullptr);
    
    auto& reader = *(RepeatReader*)context;
    reader.pending_read_completion(params->IoStatus.Status);
  }

  auto RepeatReader::pending_read_completion(NTSTATUS status) -> void {
    auto& cont_reader = get_connection().continuous_reader;

    if (not NT_SUCCESS(status)) {
      if (status != STATUS_CANCELLED) {
        cont_reader.on_failed();
      }
      KeSetEvent(&on_stop, 0, false);
      return;
    }

    cont_reader.on_read_complete(transfer_request.Buffer, transfer_request.BufferSize);
    NT_ASSERT(KeInsertQueueDpc(&resubmit, this, nullptr));
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_reader_resubmit_read_dpc(
    struct _KDPC  *dpc,
    void*  deferred_context,
    void*  system_argument_1,
    void*  system_argument_2
  ) -> void {
    lj::RepeatReader::resubmit_read_dpc(dpc, deferred_context, system_argument_1, system_argument_2);
  }

} // extern "C"

namespace lj {
  auto RepeatReader::resubmit_read_dpc(
    struct _KDPC  *dpc,
    void*  deferred_context,
    void*  system_argument_1,
    void*  system_argument_2
  ) -> void {
    if (deferred_context == nullptr or system_argument_1 == nullptr)
      return;

    auto& reader = *(RepeatReader*) system_argument_1;
    auto& header = *(ContextHeader*) deferred_context;

    reader.submit(header);
  }

  auto RepeatReader::submit(ContextHeader& _header) -> result<void, NTSTATUS> {
    if (stopping) {
      KeSetEvent(&on_stop, 0, false);
      return {};
    }

    return ([this, &_header] -> result<void, NTSTATUS> {
      _header.profile_drv_interface.BthReuseBrb((BRB*)&transfer_request, BRB_L2CA_ACL_TRANSFER);

      NT_ASSERT(NT_SUCCESS(WdfRequestReuse(pending_read_request, nullptr)));

      Try(get_connection().format_request_for_l2ca_transfer(
        pending_read_request,
        transfer_request,
        pending_read_memory,
        ACL_TRANSFER_DIRECTION_IN | ACL_SHORT_TRANSFER_OK
      ));

      WdfRequestSetCompletionRoutine(pending_read_request, lj_reader_pending_read_completion, this);
      KeClearEvent(&on_stop);
      Assert(
        WdfRequestSend(pending_read_request, _header.io_target, nullptr),
        WdfRequestGetStatus(pending_read_request)
      );

      return {};
    })().map_error([this] (auto status) {
      get_connection().continuous_reader.on_failed();
      KeSetEvent(&on_stop, 0, false);
      return status;
    });
  }

  auto RepeatReader::cancel() -> void {
    InterlockedIncrement(&stopping);
    WdfRequestCancelSentRequest(pending_read_request);
  }

  auto RepeatReader::wait_for_stop() -> void {
    KeWaitForSingleObject(&on_stop, Executive, KernelMode, false, nullptr);
  }
} // namespace lj

namespace lj {
  auto ContinuousReader::init(
      ConnectionContext&   connection,
      ReadCompleteCallback on_read_complete_cb,
      FailedCallback       on_failed_cb,
      size_t buffer_size
  ) -> result<void, NTSTATUS> {
    on_read_complete = on_read_complete_cb;
    on_failed        = on_failed_cb;

    for (auto r = 0; r < initialized_count; r++) {
      Try(get_reader(r).init(connection, buffer_size)
      .map_error([this] (auto status) {
        cancel();
        return status;
      }));
    }

    return {};
  }

  auto ContinuousReader::submit(ContextHeader& header) -> result<void, NTSTATUS> {
    NT_ASSERT(initialized_count <= AMOUNT);

    for (auto r = 0; r < initialized_count; r++) {
      Try(get_reader(r).submit(header)
      .map_error([this] (auto status) {
        cancel();
        return status;
      }));
    }

    return {};
  }

  auto ContinuousReader::cancel() -> void {
    NT_ASSERT(initialized_count <= AMOUNT);

    for (auto r = 0; r < initialized_count; r++) {
      get_reader(r).cancel();
    }
  }

  auto ContinuousReader::join() -> void {
    NT_ASSERT(initialized_count <= AMOUNT);

    for (auto r = 0; r < initialized_count; r++) {
      get_reader(r).wait_for_stop();
      get_reader(r).uninitialize();
    }
  }

} // namespace lj

extern "C" {

  _Use_decl_annotations_
  auto lj_connection_cleanup(WDFOBJECT connection) -> void {
    lj::ConnectionContext::cleanup(connection);
  }

} // extern "C"

namespace lj {

  auto ConnectionContext::cleanup(WDFOBJECT object) -> void {
    ConnectionContext::get(object).cleanup();
  }

  auto ConnectionContext::cleanup() -> void {
    get_continuous_reader().join();
    KeWaitForSingleObject(&disconnect_event, Executive, KernelMode, false, nullptr);
    WdfObjectDelete(connect_disconnect_request);
  }

  auto ConnectionContext::init(ContextHeader& _header) -> result<void, NTSTATUS> {
    get_header() = _header;
    state  = ConnectionState::INITIALIZED; // <- INITIALIZING ?

    NT_Try(WdfSpinLockCreate(nullptr, &state_lock));

    NT_Try(WdfRequestCreate(nullptr, _header.io_target, &connect_disconnect_request));

    KeInitializeEvent(&disconnect_event, NotificationEvent, true);
    InitializeListHead(&list_entry);

    state = ConnectionState::INITIALIZED;

    return {};
  }

  auto ConnectionContext::object_create(ContextHeader& _header, WDFOBJECT parent) -> result<WDFOBJECT, NTSTATUS> {
    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, lj_ConnectionContext);
    attributes.ParentObject       = parent;
    attributes.EvtCleanupCallback = lj_connection_cleanup;
    
    auto object = WDFOBJECT {};
    NT_Try(WdfObjectCreate(&attributes, &object));

    Try(ConnectionContext::get(object).init(_header)
    .map_error([&object](auto status) {
      WdfObjectDelete(object);
      return status;
    }));

    return object;
  }
}

extern "C" {

  _Use_decl_annotations_
  auto lj_connection_disconnect_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT                     context
  ) -> void {
    lj::ConnectionContext::disconnect_completion(request, target, params, context);
  }

}

namespace lj {

  auto ConnectionContext::disconnect_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT                     context
  ) -> void {
    auto connection_context = *(ConnectionContext*)context;
    connection_context.disconnect_completion();
  }

  auto ConnectionContext::disconnect_completion() -> void {
    WdfSpinLockAcquire(state_lock);
    state = ConnectionState::DISCONNECTED;
    WdfSpinLockRelease(state_lock);

    KeSetEvent(&disconnect_event, 0, false);
  }

/*++

Description:

    This routine sends a disconnect BRB for the connection

Arguments:

    DevCtxHdr - Device context header
    Connection - Connection which is to be disconnected

Return Value:

    TRUE is this call initiates the disconnect.
    FALSE if the connection was already disconnected.

--*/
  auto ConnectionContext::remote_disconnect(ContextHeader& _header) -> bool {
    WdfSpinLockAcquire(state_lock);
    {
      auto state_lock_release = DisposeHandler([this] {
        WdfSpinLockRelease(state_lock);
      });

      switch (state) {
        case ConnectionState::CONNECTING:
          state = ConnectionState::DISCONNECTING;
          KeClearEvent(&disconnect_event);
          return true;

        case ConnectionState::CONNECTED:
          state = ConnectionState::DISCONNECTING;
          KeClearEvent(&disconnect_event);
          break;

        default:
          return false;
      }
    }

    _header.profile_drv_interface.BthReuseBrb(&connect_disconnect_brb, BRB_L2CA_CLOSE_CHANNEL);
    auto& disconnect_brb = *(struct _BRB_L2CA_CLOSE_CHANNEL *) &connect_disconnect_brb;

    disconnect_brb.BtAddress     = remote_address;
    disconnect_brb.ChannelHandle = channel_handle;

    auto result = send_brb_async(
      _header.io_target,
      connect_disconnect_request,
      (BRB&)disconnect_brb,
      sizeof(disconnect_brb),
      &lj_connection_disconnect_completion,
      this
    );
    // if (not result) {
    //   NT_ASSERT(result.error() == STATUS_DEVICE_DISCONNECT);
    // }

    return true;
  }

  auto ConnectionContext::remote_disconnect_sync(ContextHeader& _header) -> void {
    remote_disconnect(_header);
    KeWaitForSingleObject(&disconnect_event, Executive, KernelMode, false, nullptr);
  }

/*++

Description:

    This routine formats are WDFREQUEST with the passed in BRB

Arguments:

    IoTarget - Target to which request will be sent
    Request - Request to be formattted
    Brb - BRB to format the request with
    BrbSize - size of the BRB

--*/
  auto ConnectionContext::format_request_with_brb(
    WDFIOTARGET io_target,
    WDFREQUEST request,
    BRB& brb,
    size_t brb_size
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

    return {};
  }
/*++

Description:

    Formats a request for L2Ca transfer

Arguments:

    Connection - Connection on which L2Ca transfer will be made
    Request - Request to be formatted
    Brb - If a Brb is passed in, it will be used, otherwise
          this routine will allocate the Brb and return in this parameter
    Memory - Memory object which has the buffer for transfer
    TransferFlags - Transfer flags which include direction of the transfer

Return Value:

    NTSTATUS Status code.

--*/
  auto ConnectionContext::format_request_for_l2ca_transfer(
    WDFREQUEST request,
    struct _BRB_L2CA_ACL_TRANSFER& brb_l2ca,
    WDFMEMORY memory,
    ULONG transfer_flags //flags include direction of transfer
  ) -> result<void, NTSTATUS> {
    WdfSpinLockAcquire(state_lock);
    {
      auto state_lock_release = DisposeHandler([this] {
        WdfSpinLockRelease(state_lock);
      });

      Assert(state == ConnectionState::CONNECTED, STATUS_CONNECTION_DISCONNECTED);
    }

    brb_l2ca.BtAddress = remote_address;
    brb_l2ca.BufferMDL = nullptr;

    auto buffer_size = size_t {};
    brb_l2ca.Buffer = WdfMemoryGetBuffer(memory, &buffer_size);
    Assert(buffer_size > -1, STATUS_BUFFER_OVERFLOW);
    brb_l2ca.BufferSize = buffer_size;

    brb_l2ca.ChannelHandle = channel_handle;
    brb_l2ca.TransferFlags = transfer_flags;

    return format_request_with_brb(get_header().io_target, request, (BRB&)brb_l2ca, sizeof(brb_l2ca));
  }

} // namespace lj
