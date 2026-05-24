module;

#include <ntddk.h>
#include <wdf.h>

export module log;

import string_view;

export namespace lj {
  auto debug_log(String_view str) -> void;
  auto debug_logln(String_view str) -> void;
} // namespace lj

module :private;

namespace lj {
  auto debug_log(String_view str) -> void {
    DbgPrint("lj: ");
    DbgPrint(str.data());
  }

  auto debug_logln(String_view str) -> void {
    debug_log(str);
    DbgPrint("\n");
  }
} // namespace lj
