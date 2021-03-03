// Copyright (c) 2017 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "umc_semaphore.h"

namespace UMC
{

Semaphore::Semaphore(void)
{
    vm_semaphore_set_invalid(&m_handle);

} // Semaphore::Semaphore(void)

Semaphore::~Semaphore(void)
{
    if (vm_semaphore_is_valid(&m_handle))
    {
        vm_semaphore_post(&m_handle);
        vm_semaphore_destroy(&m_handle);
    }

} // Semaphore::~Semaphore(void)

Status Semaphore::Init(int32_t iInitCount)
{
    if (vm_semaphore_is_valid(&m_handle))
    {
        vm_semaphore_post(&m_handle);
        vm_semaphore_destroy(&m_handle);
    }
    return vm_semaphore_init(&m_handle, iInitCount);

} // Status Semaphore::Init(int32_t iInitCount)

Status Semaphore::Init(int32_t iInitCount, int32_t iMaxCount)
{
    if (vm_semaphore_is_valid(&m_handle))
    {
        vm_semaphore_post(&m_handle);
        vm_semaphore_destroy(&m_handle);
    }
    return vm_semaphore_init_max(&m_handle, iInitCount, iMaxCount);

} // Status Semaphore::Init(int32_t iInitCount, int32_t iMaxCount)

} // namespace UMC



#include <atomic>

static_assert(sizeof(std::atomic<uint16_t>) == sizeof(uint16_t), "incorrect using of atomic");
static_assert(sizeof(std::atomic<uint32_t>) == sizeof(uint32_t), "incorrect using of atomic");

/* Thread-safe 16-bit variable incrementing */
extern "C" uint16_t vm_interlocked_inc16(volatile uint16_t *pVariable) {
    std::atomic<uint16_t> * var = reinterpret_cast<std::atomic<uint16_t>*>((uint16_t*)pVariable);
    std::atomic_fetch_add(var, (uint16_t)1);
    return *var;
}

/* Thread-safe 16-bit variable decrementing */
extern "C" uint16_t vm_interlocked_dec16(volatile uint16_t *pVariable) {
    std::atomic<uint16_t> * var = reinterpret_cast<std::atomic<uint16_t>*>((uint16_t*)pVariable);
    std::atomic_fetch_sub(var, (uint16_t)1);
    return *var;
}

/* Thread-safe 32-bit variable incrementing */
extern "C" uint32_t vm_interlocked_inc32(volatile uint32_t *pVariable) {
    std::atomic<uint32_t> * var = reinterpret_cast<std::atomic<uint32_t>*>((uint32_t*)pVariable);
    std::atomic_fetch_add(var, (uint32_t)1);
    return *var;
}

/* Thread-safe 32-bit variable decrementing */
extern "C" uint32_t vm_interlocked_dec32(volatile uint32_t *pVariable) {
    std::atomic<uint32_t> * var = reinterpret_cast<std::atomic<uint32_t>*>((uint32_t*)pVariable);
    std::atomic_fetch_sub(var, (uint32_t)1);
    return *var;
}

/* Thread-safe 32-bit variable comparing and storing */
extern "C" uint32_t vm_interlocked_cas32(volatile uint32_t *pVariable, uint32_t with, uint32_t cmp) {
    std::atomic<uint32_t> * var = reinterpret_cast<std::atomic<uint32_t>*>((uint32_t*)pVariable);
    std::atomic_compare_exchange_strong(var, &cmp, with);
    return *var;
}

/* Thread-safe 32-bit variable exchange */
extern "C" uint32_t vm_interlocked_xchg32(volatile uint32_t *pVariable, uint32_t val) {
    std::atomic<uint32_t> * var = reinterpret_cast<std::atomic<uint32_t>*>((uint32_t*)pVariable);
    std::atomic_exchange(var, val);
    return *var;
}
