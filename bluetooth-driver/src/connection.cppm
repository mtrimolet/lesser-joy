module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthioctl.h>
#include <bthddi.h>

export module lesserjoy.bluetooth:connection;

import :context;

extern "C" {

  EVT_WDF_REQUEST_COMPLETION_ROUTINE lj_reader_pending_read_completion;
  KDEFERRED_ROUTINE                  lj_reader_resubmit_read_dpc;

  EVT_WDF_OBJECT_CONTEXT_CLEANUP     lj_connection_cleanup;
  EVT_WDF_REQUEST_COMPLETION_ROUTINE lj_connection_disconnect_completion;

  struct lj_ConnectionContext;

  struct lj_RepeatReader {
    struct _BRB_L2CA_ACL_TRANSFER transfer_request;

    WDFREQUEST pending_read_request;
    WDFMEMORY  pending_read_memory;

    KDPC resubmit;

    LONG   stopping;
    KEVENT on_stop;

    lj_ConnectionContext* connection;
  };

  struct lj_ContinuousReader {
    static constexpr auto AMOUNT = 2uz;

    lj_RepeatReader repeat_readers[AMOUNT];
    DWORD initialized_count;

    using ReadCompleteCallback = void (*)(void* buffer, size_t buffer_size);
    ReadCompleteCallback on_read_complete;
    using FailedCallback = void (*)();
    FailedCallback on_failed;
  };

  enum struct lj_ConnectionState {
    UNITIALIZED = 0,
    INITIALIZED,
    CONNECTING,
    CONNECTED,
    CONNECTFAILED,
    DISCONNECTING,
    DISCONNECTED
  };
  // L2Ca connection
  struct lj_ConnectionContext {
    // List entry for connection list maintained at device level
    LIST_ENTRY list_entry;

    lj_ContextHeader* header;

    lj_ConnectionState state;
    WDFSPINLOCK        state_lock;

    USHORT out_mtu, in_mtu;

    L2CAP_CHANNEL_HANDLE channel_handle;
    BTH_ADDR             remote_address;

    struct _BRB connect_disconnect_brb;
    WDFREQUEST  connect_disconnect_request;
    KEVENT      disconnect_event; // active when connection_state != ConnectionState::DISCONNECTING

    // Continuous readers (used only by server)
    // PLEASE NOTE that KMDF USB Pipe Target uses a single continuous reader
    lj_ContinuousReader continuous_reader;
  };
  WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(lj_ConnectionContext, lj_get_connection_context)

  struct lj_FileContext {
    // Connection to server opened for this file
    lj_ConnectionContext* connection;

    USHORT server_psm;
  };
  WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(lj_FileContext, lj_get_file_context)

} // extern "C"

export namespace lj {
  struct ConnectionContext;
  
  struct RepeatReader : lj_RepeatReader {
    auto get_connection() -> ConnectionContext& { return (ConnectionContext&) connection; }

    static auto pending_read_completion(
      WDFREQUEST                      request,
      WDFIOTARGET                     target,
      WDF_REQUEST_COMPLETION_PARAMS*  params,
      WDFCONTEXT                      context
    ) -> void;

    auto pending_read_completion(NTSTATUS status) -> void;

    static auto resubmit_read_dpc(
      struct _KDPC  *dpc,
      void*  deferred_context,
      void*  system_argument_1,
      void*  system_argument_2
    ) -> void;

    auto submit(ContextHeader& header) -> result<void, NTSTATUS>;
    auto uninitialize() -> void;
    auto init(ConnectionContext& connection, size_t buffer_size) -> result<void, NTSTATUS>;
    auto cancel() -> void;
    auto wait_for_stop() -> void;

  private:
    auto init_pending_read(size_t buffer_size) -> result<void, NTSTATUS>;
  };

  struct ContinuousReader : lj_ContinuousReader {
    auto get_reader(size_t r) -> RepeatReader& { return *(RepeatReader*)&repeat_readers[r]; }

    auto init(
      ConnectionContext& connection,
      ReadCompleteCallback on_read_complete_cb,
      FailedCallback on_failed_cb,
      size_t buffer_size
    ) -> result<void, NTSTATUS>;

    auto submit(ContextHeader& header) -> result<void, NTSTATUS>; 

    auto join() -> void;
    auto cancel() -> void;
  };

  // L2Ca connection
  struct ConnectionContext : lj_ConnectionContext {
    using ConnectionState = lj_ConnectionState;
    
    static auto get(WDFOBJECT object) -> ConnectionContext& { return *(ConnectionContext*)lj_get_connection_context(object); }
    
    auto get_header() -> ContextHeader& { return *(ContextHeader*) header; }

    auto get_continuous_reader() -> ContinuousReader& { return (ContinuousReader&) continuous_reader; };

    // auto on_reader_read_complete(void* buffer, size_t buffer_size) -> void;
    // auto on_reader_failed() -> void;

    static auto cleanup(WDFOBJECT) -> void;
    auto cleanup() -> void;

    auto init(ContextHeader& header) -> result<void, NTSTATUS>;
    static auto object_create(ContextHeader& header, WDFOBJECT parent) -> result<WDFOBJECT, NTSTATUS>;

    static auto disconnect_completion(
      WDFREQUEST                     request,
      WDFIOTARGET                    target,
      WDF_REQUEST_COMPLETION_PARAMS* params,
      WDFCONTEXT                     context
    ) -> void;
    auto disconnect_completion() -> void;

    // _IRQL_requires_max_(DISPATCH_LEVEL)
    auto remote_disconnect(ContextHeader& _header) -> bool;
    // _IRQL_requires_max_(DISPATCH_LEVEL)
    auto remote_disconnect_sync(ContextHeader& _header) -> void;

    auto format_request_with_brb(
      WDFIOTARGET io_target,
      WDFREQUEST request,
      BRB& brb,
      size_t brb_size
    ) -> result<void, NTSTATUS>;

    auto format_request_for_l2ca_transfer(
        WDFREQUEST request,
        struct _BRB_L2CA_ACL_TRANSFER& brb_l2ca,
        WDFMEMORY memory,
        ULONG transfer_flags //flags include direction of transfer
    ) -> result<void, NTSTATUS>;
  };

  struct FileContext : lj_FileContext {
    static auto get(WDFFILEOBJECT file) -> FileContext& { return *(FileContext*)lj_get_file_context(file); }

    // Connection to server opened for this file
    auto get_connection() -> ConnectionContext& { return *(ConnectionContext*) connection; }

    USHORT server_psm;
  };
} // namespace lj
