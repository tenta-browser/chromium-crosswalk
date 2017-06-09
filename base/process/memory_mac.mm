// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/process/memory.h"

#include "base/allocator/allocator_interception_mac.h"
#include "base/allocator/allocator_shim.h"
#include "base/allocator/features.h"
#include "build/build_config.h"

namespace base {

namespace {
void oom_killer_new() {
  TerminateBecauseOutOfMemory(0);
}
}  // namespace

void EnableTerminationOnHeapCorruption() {
#if !ARCH_CPU_64_BITS
  DLOG(WARNING) << "EnableTerminationOnHeapCorruption only works on 64-bit";
#endif
}

CFAllocatorContext* ContextForCFAllocator(CFAllocatorRef allocator) {
  if (base::mac::IsOSMavericks() || base::mac::IsOSYosemite() ||
      base::mac::IsOSElCapitan()) {
    ChromeCFAllocatorLions* our_allocator =
        const_cast<ChromeCFAllocatorLions*>(
            reinterpret_cast<const ChromeCFAllocatorLions*>(allocator));
    return &our_allocator->_context;
  } else {
    return NULL;
  }
}

CFAllocatorAllocateCallBack g_old_cfallocator_system_default;
CFAllocatorAllocateCallBack g_old_cfallocator_malloc;
CFAllocatorAllocateCallBack g_old_cfallocator_malloc_zone;

void* oom_killer_cfallocator_system_default(CFIndex alloc_size,
                                            CFOptionFlags hint,
                                            void* info) {
  void* result = g_old_cfallocator_system_default(alloc_size, hint, info);
  if (!result)
    TerminateBecauseOutOfMemory(alloc_size);
  return result;
}

void* oom_killer_cfallocator_malloc(CFIndex alloc_size,
                                    CFOptionFlags hint,
                                    void* info) {
  void* result = g_old_cfallocator_malloc(alloc_size, hint, info);
  if (!result)
    TerminateBecauseOutOfMemory(alloc_size);
  return result;
}

void* oom_killer_cfallocator_malloc_zone(CFIndex alloc_size,
                                         CFOptionFlags hint,
                                         void* info) {
  void* result = g_old_cfallocator_malloc_zone(alloc_size, hint, info);
  if (!result)
    TerminateBecauseOutOfMemory(alloc_size);
  return result;
}

#endif  // !defined(ADDRESS_SANITIZER)

// === Cocoa NSObject allocation ===

typedef id (*allocWithZone_t)(id, SEL, NSZone*);
allocWithZone_t g_old_allocWithZone;

id oom_killer_allocWithZone(id self, SEL _cmd, NSZone* zone)
{
  id result = g_old_allocWithZone(self, _cmd, zone);
  if (!result)
    TerminateBecauseOutOfMemory(0);
  return result;
}

}  // namespace

bool UncheckedMalloc(size_t size, void** result) {
  return allocator::UncheckedMallocMac(size, result);
}

bool UncheckedCalloc(size_t num_items, size_t size, void** result) {
  return allocator::UncheckedCallocMac(num_items, size, result);
}

void EnableTerminationOnOutOfMemory() {
  // Step 1: Enable OOM killer on C++ failures.
  std::set_new_handler(oom_killer_new);

// Step 2: Enable OOM killer on C-malloc failures for the default zone (if we
// have a shim).
#if BUILDFLAG(USE_EXPERIMENTAL_ALLOCATOR_SHIM)
  allocator::SetCallNewHandlerOnMallocFailure(true);
#endif

  // Step 3: Enable OOM killer on all other malloc zones (or just "all" without
  // "other" if shim is disabled).
  allocator::InterceptAllocationsMac();
}

}  // namespace base
