#ifndef EXEC_DETAILS_SPIN_LOCK_HINT_HPP
#define EXEC_DETAILS_SPIN_LOCK_HINT_HPP

#if defined(__x86_64__) || defined(__amd64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64)

#include <emmintrin.h>
#define EXEC_SPIN_LOCK_HINT() _mm_pause()

#elif defined(__arm__) || defined(_M_ARM) || defined(__aarch64__) || defined(_M_ARM64)

#if defined(__ARM_ACLE)

#include <arm_acle.h>
#define EXEC_SPIN_LOCK_HINT() __yield()

#else

#define EXEC_SPIN_LOCK_HINT() asm volatile("yield" ::: "memory")

#endif // !__ARM_ACLE

#else

#define EXEC_SPIN_LOCK_HINT()

#endif

#endif // !EXEC_DETAILS_SPIN_LOCK_HINT_HPP