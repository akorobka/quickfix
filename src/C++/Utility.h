/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef FIX_UTILITY_H
#define FIX_UTILITY_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#ifndef _MSC_VER
#include "Config.h"
#endif

#ifdef HAVE_STLPORT
  #define ALLOCATOR std::allocator
#elif ENABLE_DEBUG_ALLOCATOR
  #include <ext/debug_allocator.h>
  #define ALLOCATOR __gnu_cxx::debug_allocator
#elif ENABLE_NEW_ALLOCATOR
  #include <ext/new_allocator.h>
  #define ALLOCATOR __gnu_cxx::new_allocator
#elif ENABLE_BOOST_FAST_POOL_ALLOCATOR
  #include <boost/pool/pool_alloc.hpp>
  #define ALLOCATOR boost::fast_pool_allocator
#elif ENABLE_MT_ALLOCATOR
  #include <ext/mt_allocator.h>
  #define ALLOCATOR __gnu_cxx::__mt_alloc
#elif ENABLE_BOOST_POOL_ALLOCATOR
  #include <boost/pool/pool_alloc.hpp>
  #define ALLOCATOR boost::pool_allocator
#elif ENABLE_POOL_ALLOCATOR
  #include <ext/pool_allocator.h>
  #define ALLOCATOR __gnu_cxx::__pool_alloc
#elif ENABLE_BITMAP_ALLOCATOR
  #include <ext/bitmap_allocator.h>
  #define ALLOCATOR __gnu_cxx::bitmap_allocator
#elif ENABLE_TBB_ALLOCATOR
  #undef VERSION
  #include <tbb/scalable_allocator.h>
  #include <tbb/cache_aligned_allocator.h>
  #define ALLOCATOR tbb::scalable_allocator
#else
  #define ALLOCATOR std::allocator
#endif

#include <limits>
#include <string>
#include <stdexcept>

#ifdef _MSC_VER
/////////////////////////////////////////////
#include <stddef.h>
#include <Winsock2.h>
#include <process.h>
#include <malloc.h>
#include <direct.h>
#include <time.h>
#include <xmmintrin.h>
#if _MSC_VER < 1600
  typedef signed __int8     int8_t;
  typedef signed __int16    int16_t;
  typedef signed __int32    int32_t;
  typedef unsigned __int8   uint8_t;
  typedef unsigned __int16  uint16_t;
  typedef unsigned __int32  uint32_t;
  typedef signed __int64    int64_t;
  typedef unsigned __int64  uint64_t;
#else
#include <stdint.h>
#endif
typedef int socklen_t;
typedef int ssize_t;
/////////////////////////////////////////////
#else
/////////////////////////////////////////////
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#if defined(__GNUC__) && defined(__x86_64__)
#include <xmmintrin.h>
#ifdef __AVX__
#include <immintrin.h>
#endif
#endif
#if defined(__MACH__)
#include <mach/mach_time.h>
#endif
/////////////////////////////////////////////
#endif

#include <climits>
#include <cstring>
#include <cctype>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <memory>

#if defined(HAVE_STD_SHARED_PTR)
  namespace ptr = std;
#elif defined(HAVE_STD_TR1_SHARED_PTR)
  #include <tr1/memory>
  namespace ptr = std::tr1;
#else
  namespace ptr = std;
#endif

#ifdef HAVE_BOOST
#include <boost/thread/recursive_mutex.hpp>
#include <boost/scoped_array.hpp>
#include <boost/range.hpp>
#endif

#ifdef HAVE_TBB
#include <tbb/tick_count.h>
#include <tbb/spin_mutex.h>
#include <tbb/atomic.h>
#endif

#if defined(_MSC_VER)
#define HAVE_FSCANF_S 1
#define FILE_FSCANF fscanf_s
#else
#define FILE_FSCANF fscanf
#endif

#if defined(_MSC_VER)
#define HAVE_SNPRINTF_S 1
#define STRING_SNPRINTF sprintf_s
#else
#define STRING_SNPRINTF snprintf
#endif

#if defined(_MSC_VER)
#define ALIGN_DECL(x) __declspec(align(x))
#define NOTHROW __declspec(nothrow)
#define NOTHROW_PRE __declspec(nothrow)
#define NOTHROW_POST
#define HEAVYUSE
#define PREFETCH(addr, rw, longevity) _mm_prefetch(addr, longevity)
#define LIKELY(x) (x)
#define MAY_ALIAS
#define PURE_DECL
#define FORCE_INLINE
#elif defined(__GNUC__)
#define ALIGN_DECL(x) __attribute__ ((aligned(x)))
#define NOTHROW __attribute__ ((nothrow))
#define NOTHROW_PRE
#define NOTHROW_POST __attribute__ ((nothrow))
#define HEAVYUSE __attribute__((hot))
#define PREFETCH(addr, rw, longevity) __builtin_prefetch(addr, rw, longevity)
#define LIKELY(x) __builtin_expect((x),1)
#define MAY_ALIAS __attribute__ ((may_alias))
#define PURE_DECL __attribute__ ((pure))
#if defined(__INTEL_COMPILER)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE
#endif
#else
#define ALIGN_DECL(x)
#define NOTHROW
#define NOTHROW_PRE
#define NOTHROW_POST
#define HEAVYUSE
#define PREFETCH(addr, rw, longevity) while(false)
#define LIKELY(x) (x)
#define MAY_ALIAS
#define PURE_DECL
#define FORCE_INLINE
#endif

#define ALIGN_DECL_DEFAULT ALIGN_DECL(16)

namespace FIX
{
  void string_replace( const std::string& oldValue,
                       const std::string& newValue,
                       std::string& value );

  std::string string_toLower( const std::string& value );
  std::string string_toUpper( const std::string& value );
  std::string string_strip( const std::string& value );

  void socket_init();
  void socket_term();
  int socket_createAcceptor( int port, bool reuse = false );
  int socket_createConnector();
  int socket_connect( int s, const char* address, int port );
  int socket_accept( int s );
  ssize_t socket_send( int s, const char* msg, size_t length );
  void socket_close( int s );
  bool socket_fionread( int s, int& bytes );
  bool socket_disconnected( int s );
  int socket_setsockopt( int s, int opt );
  int socket_setsockopt( int s, int opt, int optval );
  int socket_getsockopt( int s, int opt, int& optval );
#ifndef _MSC_VER
  int socket_fcntl( int s, int opt, int arg );
  int socket_getfcntlflag( int s, int arg );
  int socket_setfcntlflag( int s, int arg );
#endif
  void socket_setnonblock( int s );
  bool socket_isValid( int socket );
#ifndef _MSC_VER
  bool socket_isBad( int s );
#endif
  void socket_invalidate( int& socket );
  short socket_hostport( int socket );
  const char* socket_hostname( int socket );
  const char* socket_hostname( const char* name );
  const char* socket_peername( int socket );
  std::pair<int, int> socket_createpair();

  tm time_gmtime( const time_t* t );
  tm time_localtime( const time_t* t );

  template<typename InputIterator, typename OutputIterator>
  OutputIterator copy_to_array(InputIterator first, InputIterator last, OutputIterator result, size_t result_size)
  {
#ifdef _MSC_VER
    stdext::checked_array_iterator<OutputIterator> checked_array(result, result_size);
    return std::copy(first, last, checked_array).base();
#else
    return std::copy(first, last, result);
#endif
  }

#ifdef _MSC_VER

#define ALIGNED_ALLOC(p, sz, alignment) (p = (CHAR*)_aligned_malloc(sz, alignment))
#define ALIGNED_FREE(p) _aligned_free(p)

  typedef unsigned int (_stdcall THREAD_START_ROUTINE)(void *);
#define  THREAD_PROC unsigned int _stdcall
  typedef unsigned thread_id;

  typedef HANDLE   FILE_HANDLE_TYPE;
#define INVALID_FILE_HANDLE_VALUE (INVALID_HANDLE_VALUE)
#define FILE_POSITION_SET (FILE_BEGIN)
#define FILE_POSITION_CUR (FILE_CURRENT)
#define FILE_POSITION_END (FILE_END)
  typedef LARGE_INTEGER FILE_OFFSET_TYPE;
#define FILE_OFFSET_TYPE_MOD	      "I64"
#define FILE_OFFSET_TYPE_SET(offt, value) ((offt).QuadPart = (value))
#define FILE_OFFSET_TYPE_VALUE(offt) ((offt).QuadPart)
#define FILE_OFFSET_TYPE_ADDR(offt) (&(offt).QuadPart)

#else /* !_MSC_VER */

#define ALIGNED_ALLOC(p, sz, alignment) posix_memalign(&(p), alignment, sz)
#define ALIGNED_FREE(p) free(p)

  extern "C" { typedef void * (THREAD_START_ROUTINE)(void *); }
#define THREAD_PROC void *
  typedef pthread_t thread_id;

  typedef int	    FILE_HANDLE_TYPE;
#define INVALID_FILE_HANDLE_VALUE (-1)
#define FILE_POSITION_SET (SEEK_SET)
#define FILE_POSITION_CUR (SEEK_CUR)
#define FILE_POSITION_END (SEEK_END)
  typedef off_t   FILE_OFFSET_TYPE;
#define FILE_OFFSET_TYPE_MOD		"L"
#define FILE_OFFSET_TYPE_SET(offt, value) ((offt) = (value))
#define FILE_OFFSET_TYPE_VALUE(offt) ((long long)offt)
#define FILE_OFFSET_TYPE_ADDR(offt) ((long long*)&(offt))
#endif /* _MSC_VER */

  FILE_HANDLE_TYPE file_handle_open( const char* path );
  void file_handle_close( FILE_HANDLE_TYPE );
  long file_handle_write( FILE_HANDLE_TYPE, const char* , std::size_t size );
  long file_handle_read_at( FILE_HANDLE_TYPE, char*, std::size_t size,
                         FILE_OFFSET_TYPE offset );
  FILE_OFFSET_TYPE file_handle_seek( FILE_HANDLE_TYPE,
                                     FILE_OFFSET_TYPE offset, int whence );

  bool thread_spawn( THREAD_START_ROUTINE func, void* var, thread_id& thread );
  bool thread_spawn( THREAD_START_ROUTINE func, void* var );
  void thread_join( thread_id thread );
  void thread_detach( thread_id thread );
  thread_id thread_self();

  void process_sleep( double s );

  std::string file_separator();
  void file_mkdir( const char* path );
  std::FILE* file_fopen( const char* path, const char* mode );
  void file_fclose( std::FILE* file );
  bool file_exists( const char* path );
  void file_unlink( const char* path );
  int file_rename( const char* oldpath, const char* newpath );
  std::string file_appendpath( const std::string& path, const std::string& file );

  namespace detail
  {
    template <size_t N> struct log_
    {
      enum { value = 1 + log_<N/2>::value };
    };

    template <> struct log_<1>
    {
      enum { value = 0 };
    };

    template <> struct log_<0>
    {
      enum { value = 0 };
    };

    struct bitop_base
    {
      static const int Mod67Position[];
    };

    template <typename W, std::size_t S = sizeof(W)>
    struct bitop : public bitop_base
    {
      static inline std::size_t bsf(W w)
      {
        register const W notfound = (W)(S << 3);
        std::size_t v = Mod67Position[((1 + ~(uint64_t)w) & (uint64_t)w) % 67];
        return (v != 64) ? v : notfound;
      }
    };

#if defined (__GNUC__)
#  if defined (__x86_64__)
    template <typename W> struct bitop<W, 8>
    {
      static inline std::size_t PURE_DECL HEAVYUSE bsf(W w)
      {
        register std::size_t v;
        register const W notfound = 64;
        __asm__ __volatile__ (
          "bsf %1, %0; "
          "cmovz %2, %0"
          : "=r" (v)
          : "rm" (w), "r"(notfound)
          : "cc");
        return v;
      }
    };
    
    template <typename W> struct bitop<W, 4>
    {
      static inline std::size_t PURE_DECL HEAVYUSE bsf(W w)
      {
        register unsigned v;
        register const W notfound = 32;
        __asm__ __volatile__ (
          "bsf %1, %0; "
          "cmovz %2, %0"
          : "=r" (v)
          : "rm" (w), "r"(notfound)
          : "cc");
        return v;
      }
    };
#  else
    template <typename W> struct bitop<W, 8>
    {
      static inline std::size_t PURE_DECL HEAVYUSE bsf(W w)
      {
        register std::size_t v = __builtin_ffsll((uint64_t)w);
        return v ? (v - 1) : 64;
      }
    };
    
    template <typename W> struct bitop<W, 4>
    {
      static inline std::size_t PURE_DECL HEAVYUSE bsf(W w)
      {
        register std::size_t v = __builtin_ffs((uint32_t)w);
        return v ? (v - 1) : 32;
      }
    };
#  endif
#elif defined(_MSC_VER)

    template <typename W> struct bitop<W, 4>
    {
      static inline std::size_t PURE_DECL HEAVYUSE bsf(W w)
      {
        unsigned long v;
        return _BitScanForward(&v, (unsigned long)w) ? v : 32;
      }
    };

#  if defined(_M_X64)

    template <typename W> struct bitop<W, 8>
    {
      static inline std::size_t PURE_DECL HEAVYUSE bsf(W w)
      {
        unsigned long v;
        return _BitScanForward64(&v, (unsigned __int64)w) ? v : 64;
      }
    };

#  elif defined(_M_IX86)

    template <typename W> struct bitop<W, 8>
    {
      static inline std::size_t PURE_DECL HEAVYUSE bsf(W w)
      {
        unsigned long v;
        return _BitScanForward( &v, (unsigned long) w ) ? v
             : _BitScanForward( &v, (unsigned long)(w >> 32) ) ? (v + 32) : 64;
      }
    };

#  endif
#endif

    template <typename W>
    static inline std::size_t PURE_DECL HEAVYUSE bit_scan(W w, std::size_t from = 0)
    {
      return bitop<W>::bsf( w & ((from < (sizeof(W) << 3)) ? (~(W)0 << from): (W)0) );
    }

  } // namespace detail

  namespace Util {

#ifdef HAVE_BOOST
    template <typename T> struct scoped_array {
        typedef boost::scoped_array<T> type;
    };
#else
    template <typename T> struct scoped_array {
      class type {
        T* p_;
      public:
        type(T* p = NULL) : p_(p) {}
        T* get() const { return p_; }
        void reset(T* p = NULL) { if (p_) delete [] p_; p_ = p; }
        ~type() { if (p_) delete [] p_; }
      };
    };
#endif

    struct Sys
    {
#ifdef HAVE_TBB

      typedef tbb::tick_count TickCount;

#elif defined(_MSC_VER)
      class TickCount
      {
        ALIGN_DECL(16) LARGE_INTEGER m_data;

        TickCount(LARGE_INTEGER data) : m_data(data) {}
        TickCount()
        {
	      ::QueryPerformanceCounter(&m_data);
        }
      public:
        static inline TickCount now()
        { return TickCount(); }

        TickCount operator-(const TickCount& rhs) const
        {
          LARGE_INTEGER diff;
	      diff.QuadPart = m_data.QuadPart - rhs.m_data.QuadPart;
	      return diff;
        }

        double seconds() const
        {
	      LARGE_INTEGER freq;
	      ::QueryPerformanceFrequency(&freq);
	      return (double)m_data.QuadPart/(double)freq.QuadPart;
        }
      };
#elif defined(__MACH__)
      class TickCount
      {
        uint64_t m_data;

        TickCount(uint64_t data) : m_data(data) {}
        TickCount()
        {
          m_data = ::mach_absolute_time();
        }
      public:
        static inline TickCount now()
        { return TickCount(); }

        TickCount operator-(const TickCount& rhs) const
        {
          return m_data - rhs.m_data;
        }

        double seconds() const
        {
          ::mach_timebase_info_data_t tb;
          ::mach_timebase_info(&tb);
          double v = (double)m_data * tb.numer / tb_denom;
          return v * (1.0E-9);
        }
      };
#else
      class TickCount
      {
        struct timespec m_data;

        TickCount(struct timespec data) : m_data(data) {}
        TickCount()
        {
          ::clock_gettime(CLOCK_REALTIME, &m_data);
        }
      public:
        static inline TickCount now()
        { return TickCount(); }

        TickCount operator-(const TickCount& rhs) const
        {
          struct timespec data;
          data.tv_sec = m_data.tv_sec - rhs.m_data.tv_sec;
          if( m_data.tv_nsec >= rhs.m_data.tv_nsec )
            data.tv_nsec = m_data.tv_nsec - rhs.m_data.tv_nsec;
          else
          {
            data.tv_sec -= 1;
            data.tv_nsec = 1000000000 + m_data.tv_nsec - rhs.m_data.tv_nsec;
          }
          return data;
        }

        double seconds() const
        {
          return (double)m_data.tv_sec + (double)m_data.tv_nsec / 1.0E-9;
        }
      };
#endif

#if defined(_MSC_VER)
      class Semaphore
      {
        HANDLE m_sem;
        Semaphore( const Semaphore & );
        Semaphore & operator = ( const Semaphore & );
      public:
        Semaphore(int count = 0) 
        {
          m_sem = ::CreateSemaphore(NULL, (std::numeric_limits<int>::max)(), count, NULL);
        }
        ~Semaphore()
        {
          ::CloseHandle( m_sem );
        }
        bool wait()
        {
          return WAIT_OBJECT_0 == ::WaitForSingleObject( m_sem, INFINITE );
        }
        bool try_wait()
        {
          return WAIT_OBJECT_0 == ::WaitForSingleObject( m_sem, 0L );
        }
        bool post()
        {
          return ::ReleaseSemaphore(m_sem, 1, NULL) != 0;
        }
      };
#else
      class Semaphore
      {
        sem_t m_sem;
        Semaphore( const Semaphore & );
        Semaphore & operator = ( const Semaphore & );
      public:
        Semaphore( int count = 0 ) 
        {
          ::sem_init( &m_sem, 0, count );
        }
        ~Semaphore()
        {
          ::sem_destroy( &m_sem );
        }
        bool wait()
        {
          return 0 == ::sem_wait( &m_sem );
        }
        bool try_wait()
        {
          return 0 == ::sem_trywait( &m_sem );
        }
        bool post()
        {
          return 0 == ::sem_post( &m_sem );
        }
      };
#endif

#if defined(_MSC_VER)
      static inline void SchedYield() { ::SwitchToThread(); }
#else
      static inline void SchedYield() { ::sched_yield(); }
#endif
    }; // Sys

#if defined(__GNUC__) && defined(__x86_64__)
    class IntBase {
      protected:
      struct Log2 {
        int32_t m_threshold;
        int32_t m_count;
      };
      static Log2 m_digits[32];
    };

    class Int : public IntBase {
      public:
      static inline int PURE_DECL HEAVYUSE numDigits( int32_t i )
      {
        register int32_t z = 0;
        register uint32_t log2;
        if (i < 0) i = -i;
        __asm__ __volatile__ (
	            "bsr %1, %0;"  \
	            "cmovz %2, %0;"\
	            : "=r" (log2)  \
	            : "rm" (i), "r"(z));
        register const Log2& e = m_digits[log2];
        return e.m_count + ( i > e.m_threshold );
      }
    };
    class PositiveInt : public IntBase {
      public:
      static inline int PURE_DECL HEAVYUSE numDigits( int32_t i )
      {
        register uint32_t log2;
        __asm__ __volatile__ (
                "bsr %1, %0; " \
                : "=r" (log2)  \
                : "rm" (i));
        register const Log2& e = m_digits[log2];
        return e.m_count + ( i > e.m_threshold );
      }
      static inline short PURE_DECL HEAVYUSE checkSum(int n)
      {
	short csum = 0;
        do
        {
          csum += (int)'0' + n % 10;
        } while ( (n /= 10) );
        return csum;
      }
    };
    class Long {
      struct Log2 {
        int64_t m_threshold;
        int64_t m_count;
      };
      static Log2 m_digits[64];
      public:
      static inline int PURE_DECL HEAVYUSE numDigits( int64_t i )
      {
        register const int64_t z = 0;
        register uint64_t log2;
        if (i < 0) i = -i;
        __asm__ __volatile__ (
                "bsr %1, %0;"  \
                "cmovz %2, %0;"\
                : "=r" (log2)  \
                : "rm" (i), "r"(z));
  
        register const Log2& e = m_digits[log2];
        return e.m_count + ( i > e.m_threshold );
      }
    };
#else
    class Int {
      public:
      static inline int PURE_DECL HEAVYUSE numDigits( int32_t i )
      {
        if (i < 0)  i = -i;
        if              (i < 100000) {
            if          (i < 1000) {
                if      (i < 10)         return 1;
                else if (i < 100)        return 2;
                else                     return 3;
            } else {
                if      (i < 10000)      return 4;
                else                     return 5;
            }
        } else {
            if          (i < 10000000) {
                if      (i < 1000000)    return 6;
                else                     return 7;
            } else {
                if      (i < 100000000)  return 8;
                else if (i < 1000000000) return 9;
                else                     return 10;
            }
        }
      }
    };
    class PositiveInt {
      public:
      static inline int PURE_DECL HEAVYUSE numDigits( int32_t i )
      {
        if              (i < 100000) {
            if          (i < 1000) {
                if      (i < 10)         return 1;
                else if (i < 100)        return 2;
                else                     return 3;
            } else {
                if      (i < 10000)      return 4;
                else                     return 5;
            }
        } else {
            if          (i < 10000000) {
                if      (i < 1000000)    return 6;
                else                     return 7;
            } else {
                if      (i < 100000000)  return 8;
                else if (i < 1000000000) return 9;
                else                     return 10;
            }
        }
      }
      static inline short PURE_DECL HEAVYUSE checkSum(int n)
      {
        short csum = 0;
        do
        {
          csum += (int)'0' + n % 10;
        } while ( (n /= 10) );
        return csum;
      }
    };
    class Long {
      public:
      static inline int PURE_DECL HEAVYUSE numDigits( int64_t v )
      {
        int r = 1;
        if (v < 0) v = -v;
        if (v >= 10000000000000000LL)
        {
          r += 16;
          v /= 10000000000000000LL;
        }
        if (v >= 100000000)
        {
          r += 8;
          v /= 100000000;
        }

        if (v >= 10000) {
          r += 4;
          v /= 10000;
        }

        if (v >= 100) {
          r += 2;
          v /= 100;
        }
        return r + (v >= 10);
      }
    };
#endif
    class ULong {
      public:
      static inline int PURE_DECL HEAVYUSE numFractionDigits( uint64_t v )
      {
        if            ( v % 100000000 ) {
          if          ( v % 10000 ) {
            if        ( v % 100 ) {
              if      ( v % 10 )                 return 16;
              else                               return 15;
            } else if ( v % 1000 )               return 14;
              else                               return 13;
          } else {
            if        ( v % 1000000 ) {
              if      ( v % 100000 )             return 12;
              else                               return 11;
            } else if ( v % 10000000 )           return 10;
                 else                            return 9;
          }
        } else {
          if          ( v % 1000000000000LL ) {
            if        ( v % 10000000000LL ) {
              if      ( v % 1000000000 )         return 8;
              else                               return 7;
            } else if ( v % 100000000000LL )     return 6;
                   else                          return 5;
          } else {
            if        ( v % 100000000000000LL ) {
              if      ( v % 10000000000000LL )   return 4;
              else                               return 3;
            } else if ( v % 1000000000000000LL ) return 2;
                   else                          return 1;
          }
        }
        return 0;
      }
    };

    struct Memory
    {
      template <typename T> static inline void shift_up_from(T* src, std::size_t count)
      {
        if (LIKELY(count < 8) ) while (count) { std::size_t c = count--; src[c] = src[count]; }
        else ::memmove(src + 1, src, count * sizeof(T));
      }
      template <typename T> static inline void shift_down_to(T* src, std::size_t count)
      {
        if (LIKELY(count < 8) ) while (count--) { T* dst = src++; *dst = *src; }
        else ::memmove(src, src + 1, count * sizeof(T));
      }
    };

    struct Char
    {
      static inline bool isspace( char x ) { return x == ' '; }
      static inline bool isdigit( char x ) { return (unsigned int)( x - '0' ) < 10; }
      static inline char toupper( char x ) { return ((x) >= 'a' && ((x) <= 'z') ? ((x)-('a'-'A')):(x)); }
    };

    struct CharBuffer
    {
      class reverse_iterator : public std::iterator<std::input_iterator_tag, char>
      {
          char* m_p;
        public:
          reverse_iterator(char* p) : m_p(p - 1) {}
          reverse_iterator operator++() { return --m_p; }
          reverse_iterator operator++(int) { return m_p--; }
          char operator*() const { return *m_p; }
          bool operator==( reverse_iterator it) const
          { return m_p == it.m_p; }
          bool operator!=( reverse_iterator it) const
          { return m_p != it.m_p; }
          difference_type operator-( reverse_iterator it) const
          { return it.m_p - m_p; }
      };

      static inline const char* memmem(const char* buf, std::size_t bsz,
                                       const char* str, std::size_t ssz)
      {
#if defined(__GNUC__)
	return (const char*)::memmem(buf, bsz, str, ssz);
#else
        if (bsz >= ssz)
        {
          const char* end = buf + bsz - ssz + 1;
		  char c = str[0];
          while (LIKELY(NULL != (buf = (const char*)::memchr (buf, c, end - buf))))
          {
            if (0 == ::memcmp (buf, str, ssz)) 
              return buf; 
            if (++buf >= end)
              break;
          }
        }
        return 0; 
#endif
      }

      static inline int32_t PURE_DECL HEAVYUSE checkSum(const char* p, int size)
      {
        register int32_t x = 0;
#if defined(__GNUC__) && defined(__x86_64__)
        register int32_t n = size;
        register const uint8_t* pu = (const uint8_t*)p;
        register const uint64_t mask = 0x0101010101010101ULL;

        if (n >= 8)
              __asm__ __volatile (
              " cmpl $16, %2;                      \n\t"
#ifdef __AVX__
              " vpxor %%xmm0, %%xmm0, %%xmm0;      \n\t"
              " vpxor %%xmm2, %%xmm2, %%xmm2;      \n\t"
              " jl 1f;                \n\t"
              " vmovq %3, %%xmm1;                  \n\t"
              " vpunpcklbw %%xmm0, %%xmm1, %%xmm1; \n\t"
              " .p2align 4,,10;                    \n\t"
              " .p2align 3;                        \n\t"
              // loop over 16-byte blocks
              "0:;                    \n\t"
              " vlddqu (%1), %%xmm4;               \n\t"
              " addl $-16, %2;                     \n\t"
              " vpunpcklbw %%xmm2, %%xmm4, %%xmm3; \n\t"
              " addq $16, %1;                      \n\t"
              " vpmaddwd %%xmm1, %%xmm3, %%xmm3;   \n\t"
              " cmpl $16, %2;                      \n\t"
              " vpunpckhbw %%xmm2, %%xmm4, %%xmm4; \n\t"
              " vpaddd %%xmm3, %%xmm0, %%xmm0;     \n\t"
              " vpmaddwd %%xmm1, %%xmm4, %%xmm4;   \n\t"
              " vpaddd %%xmm4, %%xmm0, %%xmm0;     \n\t"
              " jge 0b;               \n\t"
              " cmpl $4, %2;                       \n\t"
              " jl 2f;                \n\t"
              " .p2align 4,,10;                    \n\t"
              " .p2align 3;                        \n\t"
              // loop over 4-byte blocks
              "1:                     \n\t"
              " vmovd (%1), %%xmm3;                \n\t"
              " addq $4, %1;                       \n\t"
              " vpunpcklbw %%xmm2, %%xmm3, %%xmm3; \n\t"
              " addl $-4, %2;                      \n\t"
              " vpunpcklwd %%xmm2, %%xmm3, %%xmm3; \n\t"
              " cmpl $4, %2;                       \n\t"
              " vpaddd %%xmm3, %%xmm0, %%xmm0;     \n\t"
              " jge 1b;               \n\t"
              // accumulate
              "2:;                    \n\t"
              " vpsrldq $8, %%xmm0, %%xmm1;        \n\t"
              " vpaddd %%xmm1, %%xmm0, %%xmm0;     \n\t"
              " vpsrldq $4, %%xmm0, %%xmm2;        \n\t"
              " vpaddd %%xmm2, %%xmm0, %%xmm0;     \n\t"
              " vmovd %%xmm0, %0;                  \n\t"
#else
              " pxor %%xmm0, %%xmm0;      \n\t"
              " movdqa %%xmm0, %%xmm2;    \n\t"
              " jl 1f;                \n\t"
              " movq %3, %%xmm1;          \n\t"
              " punpcklbw %%xmm0, %%xmm1; \n\t"
              " .p2align 4,,10;           \n\t"
              " .p2align 3;               \n\t"
              // loop over 16-byte blocks
              "0:;                    \n\t"
              " movdqu (%1), %%xmm4;      \n\t"
              " movdqa %%xmm4, %%xmm3;    \n\t"
              " addl $-16, %2;            \n\t"
              " punpcklbw %%xmm2, %%xmm3; \n\t"
              " addq $16, %1;             \n\t"
              " pmaddwd %%xmm1, %%xmm3;   \n\t"
              " cmpl $16, %2;             \n\t"
              " punpckhbw %%xmm2, %%xmm4; \n\t"
              " paddd %%xmm3, %%xmm0;     \n\t"
              " pmaddwd %%xmm1, %%xmm4;   \n\t"
              " paddd %%xmm4, %%xmm0;     \n\t"
              " jge 0b;               \n\t"
              " cmpl $4, %2;              \n\t"
              " jl 2f;                \n\t"
              " .p2align 4,,10;           \n\t"
              " .p2align 3;               \n\t"
              // loop over 4-byte blocks
              "1:                     \n\t"
              " movd (%1), %%xmm3;        \n\t"
              " addq $4, %1;              \n\t"
              " punpcklbw %%xmm2, %%xmm3; \n\t"
              " addl $-4, %2;             \n\t"
              " punpcklwd %%xmm2, %%xmm3; \n\t"
              " cmpl $4, %2;              \n\t"
              " paddd %%xmm3, %%xmm0;     \n\t"
              " jge 1b;               \n\t"
              // accumulate
              "2:;                    \n\t"
              " movdqa %%xmm0, %%xmm1;    \n\t"
              " psrldq $8, %%xmm1;        \n\t"
              " paddd %%xmm1, %%xmm0;     \n\t"
              " movdqa %%xmm0, %%xmm2;    \n\t"
              " psrldq $4, %%xmm2;        \n\t"
              " paddd %%xmm2, %%xmm0;     \n\t"
              " movd %%xmm0, %0;          \n\t"
#endif
              : "+r"(x), "+r"(pu), "+r"(n)
              : "r" (mask)
              : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
              );

        while(n > 0)
        {
            // loop over remaining bytes
            __asm__ __volatile__ (
            " movzbl (%1), %%ecx; "
            " addl %%ecx, %0;   "
            : "+r" (x), "+r" (pu)
            :
            : "ecx" );
            pu++;
            n--;
        }
#else
        for (; size > 0; p++, size--)
          x += *(unsigned char*)p;
#endif
        return x;
      }

      template <std::size_t S> union Fixed
      {
        char data[S];
      };

      /// Scans f for character v, returns array index less than S on success
      /// or a value >= S on failure.
      template <std::size_t S>
      static inline std::size_t find(int v, const Fixed<S>& f)
      {
        const char* p = (const char*)::memchr(f.data, v, S);
        return p ? p - f.data : S;
      }

      /// Locates a substring f.
      template <std::size_t S>
      static inline const char* find(const Fixed<S>& f, const char* p, std::size_t size)
      {
        return CharBuffer::memmem( p, size, f.data, S );
      }

    }; // CharBuffer

    template <> union CharBuffer::Fixed<2>
    {
      char data[2];
      uint16_t value;
    };

    template <> union CharBuffer::Fixed<4>
    {
      char data[4];
      uint32_t value;
    };

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)

    template <>
    inline const char* CharBuffer::find<2>(const CharBuffer::Fixed<2>& f,
                                           const char* p, std::size_t size)
    {
      for (; size >= 2; size--, p++)
        if (*(const uint16_t*)p == f.value )
          return p;
      return NULL;
    }

    template <>
    inline const char* CharBuffer::find<4>(const CharBuffer::Fixed<4>& f,
                                           const char* p, std::size_t size)
    {
      for (; size >= 4; size--, p++)
        if (*(const uint32_t*)p == f.value )
          return p;
      return NULL;
    }

#endif

    template <> union CharBuffer::Fixed<8>
    {
      char data[8];
      uint64_t value;
    };

#if defined(__GNUC__) && defined(__x86_64__)

    template<>
    inline std::size_t PURE_DECL CharBuffer::find<8>(int v, const CharBuffer::Fixed<8>& f)
    {
      register unsigned at = 0;
      __asm__ __volatile (
#if defined(__AVX__)
        "vmovd %1, %%xmm1                  \n\t"
        "vpunpcklbw %%xmm1, %%xmm1, %%xmm1 \n\t"
        "vmovq %2, %%xmm0                  \n\t"
        "vpshuflw $0, %%xmm1, %%xmm1       \n\t"
        "movl $8, %1                       \n\t"
        "vpcmpeqb %%xmm1, %%xmm0, %%xmm0   \n\t"
        "vpmovmskb %%xmm0, %0              \n\t"
        "bsf %0, %0                        \n\t"
        "cmovz %1, %0                      \n\t"
        : "=a" (at), "+r" (v)
        : "r" (f.value)
        : "xmm0", "xmm1"
#else
        "movd %1, %%xmm1            \n\t"
        "punpcklbw %%xmm1, %%xmm1   \n\t"
        "movq %2, %%xmm0            \n\t"
        "pshuflw $0, %%xmm1, %%xmm1 \n\t"
        "movl $8, %1                \n\t"
        "pcmpeqb %%xmm1, %%xmm0     \n\t"
        "pmovmskb %%xmm0, %0        \n\t"
        "bsf %0, %0                 \n\t"
        "cmovz %1, %0               \n\t"
        : "=a" (at), "+r" (v)
        : "r" (f.value)
        : "xmm0", "xmm1"
#endif
      );
      return at;
    }

#endif
  
    class Tag
    {
      /* NOTE: Allow for tag value range of 1-99999999. */

#if defined(__GNUC__) && defined(__x86_64__)
      static struct ConvBits
      { 
        uint32_t div_10000[2];
        uint32_t mul_10000[2];
        uint16_t mul_10[8];
        uint16_t div_const[8];
        uint16_t shl_const[8];
        uint8_t  to_ascii[16];
      } s_Bits;
#endif

      class Convertor
      {
        static const std::size_t MaxValueSize = 8; // tag length limit

        uint32_t m_value;

        void NOTHROW HEAVYUSE generate(char* buf, int digits) const {

          uint32_t v, x = m_value;

#if defined(__GNUC__) && defined(__x86_64__)

          if( x <= 999 )
          {
	    if (x > 99) {
	      uint32_t mul100 = 42949673;
	      __asm__ __volatile__ (
		"mull    %2		\n"
		: "=d"(v), "+a"(mul100) : "r"(x) : "cc"
	      );
             *buf++ = '0' + v;
              x -= v * 100U;
	    }
	    uint32_t mul10 = 429496730;
	    __asm__ __volatile__ (
		"mull    %2			\n"
		: "=d"(v), "+a"(mul10) : "r"(x) : "cc"
	    );
	    if (digits > 1) {
	      *(uint16_t*)buf = 0x3030 + ((x - v * 10) << 8) + v;
	    } else
	      *buf = '0' + x;
          }
          else
          {
	    uint64_t packed;
              /* Based on utoa32_sse_2 routine by Wojciech Mula
               * http://wm.ite.pl/articles/sse-itoa.html
               */
            __asm__ __volatile__ (
#ifdef __AVX__
              "vmovd          %1, %%xmm1             \n"
              "vmovq          %2, %%xmm0             \n"
              "vmovq          %3, %%xmm3             \n"
              "vpmuludq       %%xmm1, %%xmm0, %%xmm0 \n"
              "vpsrlq         $45, %%xmm0, %%xmm0    \n"
              "vpmuludq       %%xmm3, %%xmm0, %%xmm0 \n"
              "vpaddd         %%xmm1, %%xmm0, %%xmm0 \n"
              "vpsllq         $2, %%xmm0, %%xmm0     \n"
              "vpunpcklwd     %%xmm0, %%xmm0, %%xmm0 \n"
              "vmovdqa        %4, %%xmm1             \n"
              "vpshufd        $5, %%xmm0, %%xmm0     \n"
              "vpmulhuw       %5, %%xmm0, %%xmm0     \n"
              "vpmulhuw       %6, %%xmm0, %%xmm0     \n"
              "vpmullw        %%xmm0, %%xmm1, %%xmm1 \n"
              "vpsllq         $16, %%xmm1, %%xmm1    \n"
              "vmovdqa        %7, %%xmm3             \n"
              "vpxor          %%xmm2, %%xmm2, %%xmm2 \n"
              "vpsubw         %%xmm1, %%xmm0, %%xmm0 \n"
              "vpackuswb      %%xmm2, %%xmm0, %%xmm0 \n"
              "vpaddb         %%xmm3, %%xmm0, %%xmm0 \n"
              "vmovq          %%xmm0, %0             \n"
#else
              "movd           %1, %%xmm1             \n"
              "movq           %2, %%xmm0             \n"
              "movq           %3, %%xmm3             \n"
              "pmuludq        %%xmm1, %%xmm0         \n"
              "psrlq          $45, %%xmm0            \n"
              "pmuludq        %%xmm3, %%xmm0         \n"
              "paddd          %%xmm1, %%xmm0         \n"
              "psllq          $2, %%xmm0             \n"
              "punpcklwd      %%xmm0, %%xmm0         \n"
              "movdqa         %4, %%xmm1             \n"
              "pshufd         $5, %%xmm0, %%xmm0     \n"
              "pmulhuw        %5, %%xmm0             \n"
              "pmulhuw        %6, %%xmm0             \n"
              "pmullw         %%xmm0, %%xmm1         \n"
              "psllq          $16, %%xmm1            \n"
              "movdqa         %7, %%xmm3             \n"
              "pxor           %%xmm2, %%xmm2         \n"
              "psubw          %%xmm1, %%xmm0         \n"
              "packuswb       %%xmm2, %%xmm0         \n"
              "paddb          %%xmm3, %%xmm0         \n"
              "movq           %%xmm0, %0             \n"
#endif
              : "=r"(packed) 
              : "r" (x), "m"(*s_Bits.div_10000),
                "m"(*s_Bits.mul_10000), "m"(*s_Bits.mul_10), "m"(*s_Bits.div_const),
                "m"(*s_Bits.shl_const), "m"(*s_Bits.to_ascii)
              : "xmm0", "xmm1", "xmm2", "xmm3"
            );
	    packed >>= (8 - digits) << 3;
	    if (digits > 4)
	      *(uint64_t*)buf = packed;	// can overwrite space reserved for '=', '\001' and '\000'
	    else
	      *(uint32_t*)buf = (uint32_t)packed;
          }
#else
          buf += digits - 1;
          do { 
	    v = x / 10;
	    *buf-- = "0123456789" [x - v * 10];
	  } while( (x = v) );
#endif
        }

      public:
        Convertor(int value) : m_value(value) {}

        void NOTHROW HEAVYUSE append_to(char* buf, int digits) const
        {
	  generate(buf, digits);
        }

        template <typename S> void HEAVYUSE append_to(S& s, int digits) const
        {
          char buf[MaxValueSize];
          generate(buf, digits);
	  s.append(buf, digits);
	}
      };

    public:

      struct Identity : public std::unary_function<int, std::size_t> {
        inline std::size_t operator()(int tag) const { return tag; }
      };

      template <typename S> static void generate(S& result, int value, int digits)
      {
        Convertor(value).append_to(result, digits);
      }

      static void NOTHROW HEAVYUSE generate(char* result, int value, int digits)
      {
        Convertor(value).append_to(result, digits);
      }

      static inline const char NOTHROW_PRE * PURE_DECL HEAVYUSE NOTHROW_POST delimit(const char* p, unsigned len)
      {
#if defined(__GNUC__) && defined(__x86_64__)
        if ( LIKELY(len > 1) )
        {
          register uint64_t v = 0x3d3d3d3d3d3d3d3dULL;
          register uint64_t pos = ~(uintptr_t)p;
          __asm__ __volatile__ (
#ifdef __AVX__
                "vmovq %0, %%xmm0                       \n\t"
                "vmovq %4, %%xmm1                       \n\t"
                "vpunpcklqdq %%xmm0, %%xmm0, %%xmm0     \n\t"
                "vpcmpeqb %%xmm1, %%xmm0, %%xmm0        \n\t"
                "movl %3, %%ecx                         \n\t"
                "vpmovmskb %%xmm0, %0                   \n\t"
#else
                "movq %0, %%xmm0                        \n\t"
                "movq %4, %%xmm1                        \n\t"
                "punpcklqdq %%xmm0, %%xmm0              \n\t"
                "pcmpeqb %%xmm1, %%xmm0                 \n\t"
                "movl %3, %%ecx                         \n\t"
                "pmovmskb %%xmm0, %0                    \n\t"
#endif
                "add $1, %2                             \n\t"
                "bsf %0, %0                             \n\t"
                "cmovnz %0, %%rcx                       \n\t"
                "cmpl %3, %%ecx                         \n\t"
                "cmovl %%rcx, %1                        \n\t"
                : "+r"(v), "+r"(pos), "+r"(p)
                : "r"(len), "m"(*(p + 1))
                : "cc", "rcx", "xmm0", "xmm1"
          );
          return p + pos;
        }
        return NULL;
#else
        switch (len)
        {
          default: if (LIKELY(*p != '=')) { p++;
                   if (LIKELY(*p != '=')) { p++;
          case 8:  if (LIKELY(*p != '=')) { p++;
          case 7:  if (LIKELY(*p != '=')) { p++;
          case 6:  if (LIKELY(*p != '=')) { p++;
          case 5:  if (LIKELY(*p != '=')) { p++;
          case 4:  if (LIKELY(*p != '=')) { p++;
          case 3:  if (LIKELY(*p != '=')) { p++;
          case 2:  if (LIKELY(*p != '=')) { p++;
                   if (LIKELY(*p != '=')) { p++;
          case 1:
          case 0:  return NULL;
                } } } } } } } } } }
        }
        return p;
#endif
      }

      // store correct accumulated value even in case of a stop at a non-digit character
      static inline bool HEAVYUSE NOTHROW parse(const char* b, const char* e, int& rv, int& rx)
      {
	bool valid = false;
	unsigned a, c, s = *b == '-';
        int v = 0, x = s ? '-' : 0;
	switch (e - (b += s))
	{
          case 8: a = (c = *b++) - '0'; if (LIKELY(a < 10)) { v = a; x = c; 
          case 7: a = (c = *b++) - '0'; if (LIKELY(a < 10)) { v = v * 10 + a; x += c; 
          case 6: a = (c = *b++) - '0'; if (LIKELY(a < 10)) { v = v * 10 + a; x += c; 
          case 5: a = (c = *b++) - '0'; if (LIKELY(a < 10)) { v = v * 10 + a; x += c; 
          case 4: a = (c = *b++) - '0'; if (LIKELY(a < 10)) { v = v * 10 + a; x += c; 
          case 3: a = (c = *b++) - '0'; if (LIKELY(a < 10)) { v = v * 10 + a; x += c; 
          case 2: a = (c = *b++) - '0'; if (LIKELY(a < 10)) { v = v * 10 + a; x += c; 
          case 1: a = (c = *b)   - '0'; if (LIKELY(a < 10)) { v = v * 10 + a; x += c;
                  valid = true; } } } } } } } }
	}
        rv = LIKELY(!s) ? v : -v; rx = x;
        return valid;
      }

      static inline char NOTHROW length(int field)
      {
        return Util::PositiveInt::numDigits(field) + 1;
      }

      static inline short NOTHROW checkSum(int field)
      {
        return Util::PositiveInt::checkSum(field) + (short)'=';
      }

    }; // Tag

    template <std::size_t N> class BitSet {

      typedef unsigned long word_type;

      static const std::size_t word_size = sizeof(word_type) * 8;
      static const std::size_t word_shift = detail::log_<sizeof(word_type) * 8>::value;
      static const std::size_t Width = N/word_size + ((N%word_size) ? 1 : 0);

      word_type m_bits[Width];

      std::size_t _Find_from( int word )
      {
        for(; word < Width; word++)
        {
          std::size_t b = detail::bit_scan(m_bits[word]);
          if (word_size != b)
            return ((word << word_shift) + b);
        }
        return this->size();
      }

    public:
      BitSet() { this->reset(); }
      BitSet( bool v ) { if ( v ) this->set(); else this->reset(); }

      BitSet NOTHROW_PRE & NOTHROW_POST reset()
      {
        ::memset( m_bits, 0, sizeof(m_bits) );
        return *this;
      }
      BitSet NOTHROW_PRE & NOTHROW_POST reset( int bit )
      {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
 	    __asm__ __volatile__ (
                "btr %1, %0; "
                : "+m" (*(volatile word_type*)m_bits)
                : "Ir" (bit)
                : "cc", "memory");   
#elif defined(_MSC_VER)
        _bittestandreset( (long*)m_bits, bit );
#else
        m_bits[bit >> word_shift] &= ~((word_type)1 << (bit & (word_size - 1)));
#endif
        return *this;
      }

      inline BitSet NOTHROW_PRE & NOTHROW_POST set()
      {
        ::memset( m_bits, 255, sizeof(m_bits) );
        if ( N%word_size )
        {
          word_type bits = (~(word_type)0) >> (word_size - N%word_size);
          m_bits[Width - 1] = bits;
        }
        return *this;
      }
      inline BitSet NOTHROW_PRE & NOTHROW_POST set( int bit )
      {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
        __asm__ __volatile__ (
                "bts %1, %0; "
                : "+m" (*(volatile word_type*)m_bits)
                : "Ir" (bit)
                : "cc", "memory");   
#elif defined(_MSC_VER)
        _bittestandset( (long*)m_bits, bit );
#else
        m_bits[bit >> word_shift] |= ((word_type)1 << (bit & (word_size - 1)));
#endif
        return *this;
      }
      inline BitSet NOTHROW_PRE & NOTHROW_POST set( int bit, bool value )
      {
        return value ? set( bit ) : reset( bit );
      }

      inline bool NOTHROW operator[]( int bit ) const
      {
        unsigned char nRet;
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
        __asm__ __volatile__ (
                "bt %1, %2; "
                "setc %b0 "                                     
                : "=r" (nRet)                                   
                : "Ir" (bit), "m"(*(volatile word_type*)m_bits) 
                : "cc");
        return nRet;
#elif defined(_MSC_VER)
        nRet = _bittest( (long*)m_bits, bit );
        return nRet != 0;
#else
        nRet = (m_bits[bit >> word_shift] &
               ((word_type)1 << (bit & (word_size - 1)))) ? 1 : 0;
        return nRet != 0;
#endif
      }

      inline int NOTHROW _Find_first()
      {
        return _Find_from(0);
      }

      int _Find_next( int b )
      {
        std::size_t i = b >> word_shift;
        std::size_t v = detail::bit_scan(m_bits[i], ++b);
        return (word_size != v) ? ((i << word_shift) + v) : _Find_from(++i);
      }

      inline BitSet NOTHROW_PRE & NOTHROW_POST operator <<=(int n)
      {
        if (n <= N)
        {
          const int offset = n / word_size;
          const int shift  = n - (offset * word_size);
          int dst = Width - 1, src = Width - 1 - offset;
          word_type rem = m_bits[src--] << shift;
          for ( ; src >= 0; dst--, src-- )
          {
            register word_type w = m_bits[src];
            m_bits[dst] = rem | (w >> (word_size - shift));
            rem = w << shift;
          }
          m_bits[dst] = rem;
          ::memset( m_bits, 0, offset * sizeof(word_type) );
          if ( N%word_size )
          {
            rem = (~(word_type)0) >> (word_size - N%word_size);
            m_bits[Width - 1] &= rem;
          }
          return *this;
        }
        return reset();
      }

      inline BitSet NOTHROW_PRE & NOTHROW_POST operator >>=(int n)
      {
        if (n <= N)
        {
          const int offset = n / word_size;
          const int shift  = n - (offset * word_size);
          int dst = 0, src = offset;
          word_type rem = m_bits[src++] >> shift;
          for ( ; src < Width; dst++, src++ )
          {
            register word_type w = m_bits[src];
            m_bits[dst] = rem | (w << (word_size - shift));
            rem = w >> shift;
          }
          m_bits[dst++] = rem;
          ::memset(m_bits + dst, 0, offset * sizeof(word_type) );
          return *this;
        }
        return reset();
      }

      std::size_t size() const { return N; }

    }; // template class BitSet

    template <> class BitSet<sizeof(uint64_t) * 8>
    {
      typedef uint64_t word_type;

      word_type m_bits;

    public:
      BitSet() { reset(); }
      BitSet( bool v ) { m_bits = v ? ~(word_type)0 : 0; }

      BitSet NOTHROW_PRE & NOTHROW_POST reset() { m_bits = 0; return *this; }
      BitSet NOTHROW_PRE & NOTHROW_POST reset( int bit )
      { 
        m_bits &= ~((word_type)1 << bit);
        return *this;
      }

      inline BitSet NOTHROW_PRE & NOTHROW_POST set()
      {
        m_bits = ~(word_type)0;
        return *this;
      }
      inline BitSet NOTHROW_PRE & NOTHROW_POST set( int bit )
      {
        m_bits |= ((word_type)1 << bit);
        return *this;
      }
      inline BitSet NOTHROW_PRE & NOTHROW_POST set( int bit, bool value )
      {
        return value ? set( bit ) : reset( bit );
      }

      inline bool NOTHROW operator[]( int bit ) const
      {
        return ((m_bits & ((word_type)1 << bit)) != 0);
      }

      inline int NOTHROW _Find_first()
      {
        return detail::bit_scan(m_bits);
      }

      int _Find_next( int b )
      {
        return detail::bit_scan(m_bits, ++b);
      }

      inline BitSet NOTHROW_PRE & NOTHROW_POST operator <<=(int n) { m_bits <<= n; return *this; }

      inline BitSet NOTHROW_PRE & NOTHROW_POST operator >>=(int n) { m_bits >>= n; return *this; }

      std::size_t size() const { return sizeof(word_type) << 3; }
    };

    template <typename Key, typename T, std::size_t N> class PtrCache {
      BitSet<N> m_mask;
      Key m_key[N];
      T* m_p[N];

    public:

      static const std::size_t Alignment = 1;

      PtrCache() : m_mask(true) {}

      static inline std::size_t capacity()
      { return N; }

      void clear()
      { m_mask.set(); }
  
      T* lookup(const Key& key)
      {
        for(std::size_t i = 0; i < N; i++)
          if (key == m_key[i] && !m_mask[i])
            return m_p[i];
        return NULL;
      }
      std::size_t insert(const Key& key, T* p)
      {
        std::size_t i = m_mask._Find_first();
        if (i != N)
        {
          m_key[i] = key;
          m_p[i] = p;
          m_mask.reset(i);
        }
        return i;
      }
      T* erase(const Key& key)
      {
        for (std::size_t i = 0; i < N; i++)
        {
          if (key == m_key[i] && !m_mask[i]) 
          {
            m_mask.set(i);
            return m_p[i];
          }
        }
        return false;
      }

      T* evict(std::size_t pos)
      { return (pos < N) ? m_mask.set(pos), m_p[pos] : NULL; }
    };

#if defined(__GNUC__) && defined(__x86_64__)

    namespace detail
    {
      template <std::size_t Size> struct int32_key_holder
      {
        union
        {
          __m128 m_simd[Size >> 2];
          struct
          {
            int32_t m_key[Size + 1];
            uint32_t m_mask;
          };
        } __attribute__((packed));
        int32_key_holder() : m_mask(((uint32_t)1 << Size) - 1) {}

        template <typename F> F* lookup_pos(uint32_t mask, F* m_p[Size + 1])
        {
          F* p = NULL;
          __asm__ __volatile__ (
          " bsf %1, %1;               \n\t"
          " cmovnz (%2,%q1,8), %0;    \n\t"
          : "+a"(p), "+r" (mask)
          : "r" (m_p)
          : "cc"
          );
          return p;
        }
        template <typename F> std::size_t insert_key(int32_t key, F* p, F* m_p[Size + 1]) {
          uint32_t i = Size;
          __asm__ __volatile__ (
          " bsf %1, %%edx;            \n\t"
          " cmovnz %%edx, %0;         \n\t"
          " btr %0, %1;               \n\t"
          : "+r"(i), "+m"(m_mask)
          : 
          : "edx", "cc"
          );
          m_key[i] = key;
          m_p[i] = p;
          return i;
        }
        template <typename F> F* erase_pos(uint32_t mask, F* m_p[Size + 1]) {
	  int32_t tmp;
          F* p = NULL;
          __asm__ __volatile__ (
          " bsf %0, %0;               \n\t"
          " cmovnz (%4,%q0,8), %3;    \n\t"
          " setnz %b1;                \n\t"
          " movzbl %b1, %1;           \n\t"
          " shl %b0, %1;              \n\t"
          " or %1, %2;                \n\t"
          : "+c"(mask), "=r" (tmp), "+m"(m_mask), "+r" (p)
          : "r" (m_p)
          : "cc"
          );
          return p;
        }
        template <typename F> F* evict_pos(std::size_t pos, F* m_p[Size + 1]) {
          std::size_t tmp = Size;
          F* p = NULL;
          __asm__ __volatile__ (
          " bts %2, %0                \n\t"
          " cmovc %4, %2              \n\t"
          " cmp %2, %4                \n\t"
          " cmovg (%3,%2,8), %1       \n\t"
          : "+m"(m_mask), "+r"(p)
          : "r"(pos), "r"(m_p), "r"(tmp)
          : "cc"
          );
          return p;
        }
      };

      template <std::size_t Size> struct int32_keys;

      template <> struct int32_keys<8> : public int32_key_holder<8>
      {
        inline uint32_t keymask(int32_t key)
        {
          uint32_t mask;
          __asm__ __volatile__ (
#ifdef __AVX__
          " vmovd   %1, %%xmm2;       \n\t"
          " vshufps $0, %%xmm2, %%xmm2, %%xmm2 \n\t"
          " vpcmpeqd %2, %%xmm2, %%xmm1; \n\t"
          " mov %4, %0;               \n\t"
          " vpcmpeqd %3, %%xmm2, %%xmm2; \n\t"
          " not %0;                   \n\t"
          " and $255, %0;             \n\t"
          " vpackssdw %%xmm2, %%xmm1, %%xmm1;  \n\t"
          " vpacksswb %%xmm1, %%xmm1, %%xmm1;  \n\t"
          " vpmovmskb %%xmm1, %1;     \n\t"
#else
          " movdqa %2, %%xmm1;        \n\t"
          " movd   %1, %%xmm2;        \n\t"
          " shufps $0, %%xmm2, %%xmm2 \n\t"
          " pcmpeqd %%xmm2, %%xmm1;   \n\t"
          " pcmpeqd %3, %%xmm2;       \n\t"
          " mov %4, %0;               \n\t"
          " not %0;                   \n\t"
          " and $255, %0;             \n\t"
          " packssdw %%xmm2, %%xmm1;  \n\t"
          " packsswb %%xmm1, %%xmm1;  \n\t"
          " pmovmskb %%xmm1, %1;      \n\t"
#endif
          " and %1, %0;               \n\t"
          : "=r" (mask), "+r" (key)
          : "m" (m_simd[0]), "m" (m_simd[1]), "m" (m_mask)
          : "xmm1", "xmm2", "cc"
          );
          return mask;
        }
      };

      template <> struct int32_keys<16> : public int32_key_holder<16>
      {
        inline uint32_t keymask(int32_t key)
        {
          uint32_t mask;
          __asm__ __volatile__ (
#ifdef __AVX__
          " vmovd   %1, %%xmm4;       \n\t"
          " vshufps $0, %%xmm4, %%xmm4, %%xmm4 \n\t"
          " vpcmpeqd %2, %%xmm4, %%xmm1; \n\t"
          " vpcmpeqd %3, %%xmm4, %%xmm2; \n\t"
          " vpcmpeqd %4, %%xmm4, %%xmm3; \n\t"
          " mov %6, %0;               \n\t"
          " vpackssdw %%xmm2, %%xmm1, %%xmm1;  \n\t"
          " vpcmpeqd %5, %%xmm4, %%xmm4; \n\t"
          " not %0;                   \n\t"
          " vpackssdw %%xmm4, %%xmm3, %%xmm3;  \n\t"
          " vpacksswb %%xmm3, %%xmm1, %%xmm1;  \n\t"
          " vpmovmskb %%xmm1, %1;     \n\t"
#else
          " movdqa %2, %%xmm1;        \n\t"
          " movdqa %3, %%xmm2;        \n\t"
          " movd   %1, %%xmm4;        \n\t"
          " shufps $0, %%xmm4, %%xmm4 \n\t"
          " pcmpeqd %%xmm4, %%xmm1;   \n\t"
          " movdqa %%xmm4, %%xmm3;    \n\t"
          " pcmpeqd %%xmm4, %%xmm2;   \n\t"
          " pcmpeqd %4, %%xmm3;       \n\t"
          " mov %6, %0;               \n\t"
          " packssdw %%xmm2, %%xmm1;  \n\t"
          " pcmpeqd %5, %%xmm4;       \n\t"
          " not %0;                   \n\t"
          " packssdw %%xmm4, %%xmm3;  \n\t"
          " packsswb %%xmm3, %%xmm1;  \n\t"
          " pmovmskb %%xmm1, %1;      \n\t"
#endif
          " and %1, %0;               \n\t"
          : "=r" (mask), "+r" (key)
          : "m" (m_simd[0]), "m" (m_simd[1]), "m" (m_simd[2]), "m" (m_simd[3]), "m" (m_mask)
          : "xmm1", "xmm2", "xmm3", "xmm4", "cc"
          );
          return mask;
        }
      };

      template <typename F, std::size_t N> class IntPtrCache : private int32_keys<N>
      {
        typedef int32_keys<N> Base;
        F* m_p[N + 1];
  
        public:

	static inline std::size_t capacity()
        { return N; }

	void clear()
        { this->m_mask = ((uint32_t)1 << N) - 1; }
  
        F* lookup(int32_t key)
        { return this->lookup_pos(Base::keymask(key), m_p); }

        std::size_t insert(int32_t key, F* p)
        { return this->insert_key(key, p, m_p); }

        F* erase(int32_t key) 
        { return this->erase_pos(Base::keymask(key), m_p); }

        F* evict(std::size_t pos)
        { return this->evict_pos((uint32_t)pos, m_p); }
      };
    }

    template <typename F> class PtrCache<int32_t, F, 16> : public detail::IntPtrCache<F, 16> {};
    template <typename F> class PtrCache<int32_t, F, 8> : public detail::IntPtrCache<F, 8> {};

#endif

  } // namespace FIX::Util

  struct Sg
  {

#ifdef _MSC_VER
    typedef WSABUF   sg_buf_t;
    typedef LPWSABUF sg_buf_ptr;
    typedef CHAR*    sg_ptr_t;

#define IOV_BUF_INITIALIZER(ptr, len) { (ULONG)(len), (CHAR*)(ptr) } 
#define IOV_BUF(b) ((b).buf)
#define IOV_LEN(b) ((b).len)

    static inline void NOTHROW append(sg_buf_t& buf, const char* s, std::size_t len)
    {
      char* pc = (char*)IOV_BUF(buf) + IOV_LEN(buf);
      IOV_LEN(buf) += len;
      while(len--) *pc++ = *s++;
    }

    static inline std::size_t NOTHROW send(SOCKET socket, sg_buf_ptr bufs, int n)
    {
      DWORD bytes;
      int i = ::WSASend(socket, bufs, n, &bytes, 0, NULL, NULL);
      return (i == 0) ? bytes : 0;
    }

    static inline int64_t NOTHROW writev(FILE_HANDLE_TYPE handle,
                                         sg_buf_ptr bufs, int n)
    {
      int64_t sz = 0;
      for( int i = 0; i < n; i++ )
      {
        DWORD written = 0;
        if( !::WriteFile( handle, IOV_BUF(bufs[i]),
                          IOV_LEN(bufs[i]), &written, NULL ) )
        {
          if( sz == 0 )
            return -1;
          break;
        }
        sz += (int64_t)written;
      }
      return sz;
    }

#else
    typedef struct iovec  sg_buf_t;
    typedef struct iovec* sg_buf_ptr;
    typedef void*         sg_ptr_t;

#define IOV_BUF_INITIALIZER(ptr, len) { (void*)(ptr), (size_t)(len) } 
#define IOV_BUF(b) ((b).iov_base)
#define IOV_LEN(b) ((b).iov_len)

    // optimize for short strings
    static inline void NOTHROW append(sg_buf_t& buf, const char* s, std::size_t len)
    {
      register char* p = (char*)IOV_BUF(buf) + IOV_LEN(buf);
      IOV_LEN(buf) += len;
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
      while (len > 3)
      {
        __asm__ __volatile__ (
          " movl (%1), %%ecx; \n\t"
          " subq $4, %2;      \n\t"
          " addq $4, %1;      \n\t"
          " movl %%ecx, (%0); \n\t"
          " addq $4, %0;      \n\t"
          : "+r"(p), "+r"(s), "+r"(len)
          :
          : "ecx", "memory" );
      }
      if (len > 0)
      {
        __asm__ __volatile__ (
	  "1:		     \n\t"
          " movb (%1), %%cl; \n\t"
          " incq %1;         \n\t"
          " movb %%cl, (%0); \n\t"
          " incq %0;         \n\t"
          " decq %2;         \n\t"
          " jne 1b;          \n\t"
          : "+r"(p), "+r"(s), "+r"(len)
          :
          : "ecx", "memory" );
/*
        __asm__ __volatile__ (
          " movb (%1), %%cl; \n\t"
          " incq %1;         \n\t"
          " movb %%cl, (%0); \n\t"
          " decq %2;         \n\t"
          " je 1f;           \n\t"
          " incq %0;         \n\t"
          " movb (%1), %%cl; \n\t"
          " incq %1;         \n\t"
          " movb %%cl, (%0); \n\t"
          " decq %2;         \n\t"
          " je 1f;           \n\t"
          " movb (%1), %%cl; \n\t"
          " incq %0;         \n\t"
          " movb %%cl, (%0); \n\t"
          "1:                \n\t"
          : "+r"(p), "+r"(s), "+r"(len)
          :
          : "ecx", "memory" );
*/
      }
#else
      while(len--) *p++ = *s++;
#endif
    }

    static inline std::size_t NOTHROW send(int socket, sg_buf_ptr bufs, int n)
    {
      struct msghdr m = { NULL, 0, bufs, (std::size_t)n, NULL, 0, 0 };
      ssize_t sz = ::sendmsg(socket, &m, MSG_NOSIGNAL);
      return sz >= 0 ? (std::size_t)sz : 0;
    }

    static inline int64_t NOTHROW writev(FILE_HANDLE_TYPE handle,
                                         sg_buf_ptr bufs, int n)
    {
      return ::writev(handle, bufs, n);
    }

#endif

    template <typename B>
    static std::size_t NOTHROW size( const B bufs, int n )
    {
      size_t sz = 0;
      for (int i = 0; i < n; i++)
       	sz += IOV_LEN(bufs[i]);
      return sz;
    }

    template <typename B>
    static std::size_t NOTHROW size( const B& buf )
    {
      return IOV_LEN(buf);
    }

    template <typename P, typename B>
    static P NOTHROW data( B buf )
    {
      return (P)IOV_BUF(buf);
    }

    template <typename B>
    static B split( B& buf, B by )
    {
      char* pe = (char*)IOV_BUF(by) + IOV_LEN(by);
      IOV_BUF(by) = IOV_BUF(buf);
      IOV_LEN(buf) -= pe - (char*)IOV_BUF(buf);
      IOV_BUF(buf) = pe;
      IOV_LEN(by) = pe - (char*)IOV_BUF(by);
      return by;
    }

    template <typename B>
    static std::string toString( const B bufs, int n )
    {
      std::string s;
      for (int i = 0; i < n; i++)
       	s.append((const char*)IOV_BUF(bufs[i]), IOV_LEN(bufs[i]));
      return s;
    }

    template <typename B>
    static std::string toString( B buf )
    {
      return std::string( (const char*)IOV_BUF(buf), IOV_LEN(buf) );
    }

  }; // struct Sg

} // namespace FIX

#if (!defined(_MSC_VER) || (_MSC_VER >= 1300)) && !defined(HAVE_STLPORT)
using std::abort;
using std::sprintf;
using std::atoi;
using std::atol;
using std::atof;
using std::isdigit;
using std::strcmp;
using std::strftime;
using std::strlen;
using std::abs;
using std::labs;
using std::memcpy;
using std::memset;
using std::exit;
using std::strtod;
using std::strtol;
using std::strerror;
#endif

#endif
