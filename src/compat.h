#ifndef COMPAT_H
#define COMPAT_H

#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS
    #define STRSAFE_NO_DEPRECATE

    #pragma warning(push)
    #pragma warning(disable: 4996) // deprecated functions
    #pragma warning(disable: 4244) // conversion warnings
    #pragma warning(disable: 4267) // size_t conversion

    #include <BaseTsd.h>
    #include <windows.h>

    // MSVC doesn't define ssize_t
    #ifndef ssize_t
        typedef SSIZE_T ssize_t;
    #endif

    // Inline keyword compatibility for pre-C99 MSVC
    #if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
        #define inline __inline
    #endif

    // Atomic operations - try C++, then C11, fallback to Interlocked
    #if defined(__cplusplus)
        #include <atomic>
        template<class T> using compat_atomic = std::atomic<T>;
        using atomic_bool = compat_atomic<bool>;
        using atomic_size_t = compat_atomic<size_t>;
        #define atomic_load(p) ((p)->load())
        #define atomic_store(p, v) ((p)->store((v)))
        #define atomic_fetch_add(p, v) ((p)->fetch_add((v)))
        #define atomic_compare_exchange_strong(p, e, d) ((p)->compare_exchange_strong(*(e),(d)))
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
        #ifdef __has_include
            #if __has_include(<stdatomic.h>) && _MSC_VER >= 1935
                #include <stdatomic.h>
            #else
                #define COMPAT_USE_INTERLOCKED
            #endif
        #else
            #define COMPAT_USE_INTERLOCKED
        #endif
    #else
        #define COMPAT_USE_INTERLOCKED
    #endif

    #ifdef COMPAT_USE_INTERLOCKED
        typedef volatile LONG atomic_bool;

        #ifdef _WIN64
            typedef volatile LONG64 atomic_size_t;

            static inline LONG64 compat_atomic_load64(volatile LONG64* p) {
                return InterlockedOr64((volatile LONG64*)p, 0);
            }

            static inline void compat_atomic_store64(volatile LONG64* p, LONG64 v) {
                InterlockedExchange64((volatile LONG64*)p, v);
            }

            static inline int compat_cas64(volatile LONG64* p, LONG64* expected, LONG64 desired) {
                LONG64 observed = InterlockedCompareExchange64((volatile LONG64*)p, desired, *expected);
                if (observed == *expected) return 1;
                *expected = observed;
                return 0;
            }

            #define atomic_load(p) \
                (sizeof(*(p)) == 8 ? (size_t)compat_atomic_load64((volatile LONG64*)(p)) \
                                   : (LONG)InterlockedOr((volatile LONG*)(p), 0))

            #define atomic_store(p, v) \
                (sizeof(*(p)) == 8 ? compat_atomic_store64((volatile LONG64*)(p), (LONG64)(v)) \
                                   : (void)InterlockedExchange((volatile LONG*)(p), (LONG)(v)))

            #define atomic_fetch_add(p, v) \
                (sizeof(*(p)) == 8 ? (size_t)InterlockedExchangeAdd64((volatile LONG64*)(p), (LONG64)(v)) \
                                   : (LONG)InterlockedExchangeAdd((volatile LONG*)(p), (LONG)(v)))

            #define atomic_compare_exchange_strong(p, e, d) \
                (sizeof(*(p)) == 8 ? compat_cas64((volatile LONG64*)(p), (LONG64*)(e), (LONG64)(d)) \
                                   : ({ \
                                       volatile LONG* _p = (volatile LONG*)(p); \
                                       LONG* _e = (LONG*)(e); \
                                       LONG _obs = InterlockedCompareExchange(_p, (LONG)(d), *_e); \
                                       if (_obs == *_e) return 1; \
                                       *_e = _obs; \
                                       return 0; \
                                     }))
        #else
            typedef volatile LONG atomic_size_t;

            static inline LONG compat_atomic_load32(volatile LONG* p) {
                return InterlockedOr((volatile LONG*)p, 0);
            }

            static inline void compat_atomic_store32(volatile LONG* p, LONG v) {
                InterlockedExchange((volatile LONG*)p, v);
            }

            static inline int compat_cas32(volatile LONG* p, LONG* expected, LONG desired) {
                LONG observed = InterlockedCompareExchange((volatile LONG*)p, desired, *expected);
                if (observed == *expected) return 1;
                *expected = observed;
                return 0;
            }

            #define atomic_load(p) (compat_atomic_load32((volatile LONG*)(p)))
            #define atomic_store(p, v) (compat_atomic_store32((volatile LONG*)(p), (LONG)(v)))
            #define atomic_fetch_add(p, v) (InterlockedExchangeAdd((volatile LONG*)(p), (LONG)(v)))
            #define atomic_compare_exchange_strong(p, e, d) (compat_cas32((volatile LONG*)(p), (LONG*)(e), (LONG)(d)))
        #endif
    #endif

    #pragma warning(pop)
#else
    // GCC/Clang - standard C11 atomics
    #define STRSAFE_NO_DEPRECATE
    #include <stdatomic.h>
#endif

#endif