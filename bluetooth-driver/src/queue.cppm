module;

#include <ntddk.h>
#include <wdf.h>

export module lesserjoy.bluetooth:queue;

import result;

extern "C" {

  EVT_WDF_REQUEST_COMPLETION_ROUTINE lj_queue_on_read_write_completion;
  EVT_WDF_IO_QUEUE_IO_WRITE          lj_queue_on_io_write;
  EVT_WDF_IO_QUEUE_IO_READ           lj_queue_on_io_read;
  EVT_WDF_IO_QUEUE_IO_STOP           lj_queue_on_io_stop;

} // extern "C"

export namespace lj {

  auto queue_on_read_write_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT                     context
) -> void;
  auto queue_on_io_write(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void;
  auto queue_on_io_read(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void;
  auto queue_on_io_stop(WDFQUEUE queue, WDFREQUEST request, ULONG action_flags) -> void;

} // namespace lj
