module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <bthioctl.h>
#include <bthddi.h>

export module lesserjoy.bluetooth:context;

import result;

export namespace lj {

  // see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-exallocatepool2
  constexpr auto LESSERJOY_BLUETOOTH_POOLTAG = ULONG{ 'tbjl' };

} // namespace lj

extern "C" {

  struct lj_ContextHeader {
    // Framework device this context is associated with
    WDFDEVICE   device;
    WDFIOTARGET io_target;

    // Profile driver interface which contains profile driver DDI
    BTH_PROFILE_DRIVER_INTERFACE profile_drv_interface;
    BTH_ADDR                     local_bth_addr;
    // Features supported by the local stack
    BTH_HOST_FEATURE_MASK        local_features;

    // Preallocated request to be reused during initialization/deinitialzation phase
    // Access to this reqeust is not synchronized
    WDFREQUEST request;
  };

  struct lj_DeviceContext {
    lj_ContextHeader header;
    BTH_ADDR         server_bth_address;
  };
  WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(lj_DeviceContext, lj_get_device_context);

  using lj_RequestContext = BRB;
  WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(lj_RequestContext, lj_get_request_context);

} // extern "C"

export namespace lj {
  struct ContextHeader : lj_ContextHeader {
    auto init(WDFDEVICE device) -> result<void, NTSTATUS>;

    auto retrieve_local_info() -> result<void, NTSTATUS>;
    auto get_host_supported_features() -> result<void, NTSTATUS>;
  };

  struct DeviceContext : lj_DeviceContext {
    static auto get(WDFDEVICE device) -> DeviceContext& { return (DeviceContext&)*lj_get_device_context(device); }
    auto get_header() -> ContextHeader& { return (ContextHeader&)header; }

    auto init(WDFDEVICE device) -> result<void, NTSTATUS>;
  };

  struct RequestContext : lj_RequestContext {
    static auto get(WDFREQUEST request) -> RequestContext& { return (RequestContext&)*lj_get_request_context(request); }
  };

  auto send_brb_async(
    WDFIOTARGET                        io_target,
    WDFREQUEST                         request,
    BRB&                               brb,
    size_t                             brb_size,
    PFN_WDF_REQUEST_COMPLETION_ROUTINE compl_routine,
    WDFCONTEXT                         context
  ) -> result<void, NTSTATUS>;

  auto send_brb_sync(WDFIOTARGET io_target, WDFREQUEST request, BRB& brb, size_t brb_size) -> result<void, NTSTATUS>;

} // namespace lj
