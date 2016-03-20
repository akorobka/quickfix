/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) quickfixengine.org  All rights reserved.
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

#ifndef ITEM_ALLOCATOR_H
#define ITEM_ALLOCATOR_H

#include "Utility.h"

namespace FIX
{
  struct ItemAllocatorTraits
  {
    static const unsigned DefaultCapacity = 32;
  };

  // Cache-friendly allocator for node-based containers with optional aligned header area
  template <typename T, typename Header = void, std::size_t HeaderAlignment = 16, std::size_t ItemAlignment = 8>
  class ItemStore
  {
    template <typename O> struct SizeOf { // 0 for void
      static char f(void*);
      template <typename U> static char (&f(U*))[sizeof(U&) + 1];
      static const std::size_t value = sizeof(f((O*)0)) - 1;
    };
    static const std::size_t HeaderBytes = SizeOf<Header>::value;
    template<typename U, std::size_t HB, std::size_t HA, std::size_t IA>
    struct Options
    {
      union Entry {
        char m_v[sizeof(U) + (IA - sizeof(U) % IA) % IA];
        U*   m_p;
      };
      static const std::size_t AllowType = (HA != 0) && !(HA & (HA - 1)) && (IA != 0) && !(IA & (IA - 1));
      int improper_alignment : AllowType; // power-of-two alignment only
      static const std::size_t AllowCopy = HeaderBytes == HB && HeaderAlignment == HA &&
                                       sizeof(Entry) == sizeof(typename Options<T, HeaderBytes, HeaderAlignment, ItemAlignment>::Entry);
      int type_compatibility_for_item_store : AllowCopy;
    };
    public:
      class Buffer
      {
          typedef typename Options<T, HeaderBytes, HeaderAlignment, ItemAlignment>::Entry Entry;
          class Segment {
              unsigned m_count, m_num;
              Segment* m_next;
            public:
              Segment(unsigned num, Segment* next) : m_count(0), m_num(num), m_next(next) {}

              void reset(Segment* next = NULL) { m_count = 0; m_next = next; }
  
              T* PURE_DECL start() const { return (T*)(this + 1); }
              T* PURE_DECL end() const { return (T*)((Entry*)(this + 1) + m_num); }

              Segment* PURE_DECL next() const { return m_next; }
              Segment* next(Segment* p) { return m_next = p; }

              bool PURE_DECL space() const { return m_count < m_num; }
              T* reserve() { return start() + m_count++; }
              Segment* extend()
              {
                unsigned num = m_num << 1; // double the size for the next segment
                Segment* h = (Segment*)(ALLOCATOR<unsigned char>().allocate(num * sizeof(Entry) + sizeof(Segment)));
                return new (h) Segment(num, this);
              }
          };

          T*       m_pfree;
          unsigned m_shared, m_pad;
          Segment  m_seg; // must be the last member!

          Segment* PURE_DECL top() const { return m_seg.next(); }
          Segment* top(Segment* p) { return m_seg.next(p); } 

          void free_segments()
          {
            for (Buffer::Segment* n,* s = top(); s != &m_seg; s = n)
            {
              n = s->next();
              ALLOCATOR<unsigned char>().deallocate((unsigned char*)s, 0);
            }
          }

          Buffer(unsigned num, unsigned referenced, unsigned pad)
          : m_pfree(NULL), m_shared(referenced + 1), m_pad(pad), m_seg(num, &m_seg) {}
	  ~Buffer() {}

          template <typename B> struct Eval {
            static const std::size_t Bytes = HeaderBytes + sizeof(B) +
                                               ((HeaderBytes > 0 && HeaderAlignment < ItemAlignment) ? (ItemAlignment - HeaderAlignment) : 0);
            static const std::size_t Padding = Bytes + (ItemAlignment - Bytes % ItemAlignment) % ItemAlignment +
                                               ((HeaderBytes > 0 && HeaderAlignment >= ItemAlignment) ? HeaderAlignment : 0);
          };
        public:

          unsigned addRef() { return ++m_shared; }
          unsigned decRef() { return --m_shared; }
  
          static inline Buffer* create(unsigned num, unsigned referenced = 0)
          {
            unsigned char* p = ALLOCATOR<unsigned char>().allocate(Eval<Buffer>::Padding + num * sizeof(Entry));
            Buffer* h = (Buffer*) ((((uintptr_t)p + Eval<Buffer>::Padding) & ~(uintptr_t)(ItemAlignment - 1)) - sizeof(Buffer));
            return new (h) Buffer(num, referenced, (uintptr_t)h - (uintptr_t)p);
          }

          bool verify(T* p)
          {
            Buffer::Segment* s = top();
            do { if (p >= s->start() && p < s->end()) return true; s = s->next(); } while (s != top());
            return false;
          }

          T* acquire()
          {
            if ( LIKELY(!m_pfree) )
              return ( LIKELY(top()->space()) ? top() : top(top()->extend()) )->reserve();
            T* p = m_pfree;
            m_pfree = *(T**)p;
            return p;
          }
  
          void release(T* p)
          {
             *(T**)p = m_pfree;
             m_pfree = p;
          }

          void clear()
          {
            free_segments();
            m_seg.reset(&m_seg);
            m_pfree = NULL;
          }

          Header* header() // returns header space address
          { return HeaderBytes > 0 ? (Header*)(((uintptr_t)this - HeaderBytes) & ~(uintptr_t)(HeaderAlignment - 1)) : (Header*)NULL; }

          static std::size_t header_size() { return HeaderBytes; }
  
          static void destroy(Buffer* p)
          {
            p->free_segments();
            ALLOCATOR<unsigned char>().deallocate((unsigned char*)p - p->m_pad, 0);
          }
      };

      inline ItemStore(Buffer* b = NULL) : m_buffer( b ) {}
      inline ItemStore(ItemStore const& s) : m_buffer( s.m_buffer )
      { if ( m_buffer ) m_buffer->addRef(); }

      template <typename U, typename H, std::size_t HA, std::size_t IA>
      inline ItemStore(ItemStore<U, H, HA, IA> const& s, int = sizeof(Options<U, SizeOf<H>::value, HA, IA>))
      : m_buffer( (Buffer*)s.m_buffer )
      { if ( m_buffer ) m_buffer->addRef(); }

      template <typename U,  typename H, std::size_t HA, std::size_t IA>
      bool operator==(ItemStore<U, H, HA, IA> const& s) const
      { return (void*)m_buffer == (void*)s.m_buffer; }

      template <typename U, typename H, std::size_t HA, std::size_t IA> friend class ItemStore;

    protected:
      Buffer* m_buffer;
  };

  template<typename T, typename Header = void, std::size_t HeaderAlignment = 16>
  class ItemAllocator : public ItemStore<T, Header, HeaderAlignment>
  {
    public : 

      //    typedefs
      typedef T value_type;
      typedef value_type* pointer;
      typedef const value_type* const_pointer;
      typedef value_type& reference;
      typedef const value_type& const_reference;
      typedef std::size_t size_type;
      typedef std::ptrdiff_t difference_type;

      typedef ItemStore<T, Header, HeaderAlignment> item_store;
      typedef typename item_store::Buffer buffer_type;

    public : 
      //    convert an allocator<T> to allocator<U>
      template<typename U>
      struct rebind {
          typedef ItemAllocator<U> other;
      };
  
    public : 
      inline ItemAllocator() {}
      inline ItemAllocator(buffer_type* b) : item_store(b) {}

      inline explicit ItemAllocator(ItemAllocator const& a)
       : item_store(a) {}

      template<typename U, typename  H, std::size_t A>
      inline ItemAllocator(ItemAllocator<U, H, A> const& u)
       : item_store(u) {}

      inline ~ItemAllocator() // clean up here, not a virtual dtor
      {
        buffer_type* buf = this->m_buffer;
        if ( buf && buf->decRef() == 0 ) buffer_type::destroy(buf);
      }

      template<typename U, typename H, std::size_t A>
      inline bool operator==(ItemAllocator<U, H, A> const& u) const
      { return this->m_buffer ? static_cast<const item_store&>(*this) ==
                                static_cast<const typename ItemAllocator<U, H, A>::item_store&>(u) : false; }
      void clear()
      { if ( this->m_buffer ) this->m_buffer->clear(); }

      //    address
      inline pointer address(reference r) { return &r; }
      inline const_pointer address(const_reference r) { return &r; }
  
      //    memory allocation
      inline pointer HEAVYUSE allocate() // shortcut for one element
      { 
        buffer_type* buf = this->m_buffer;
        if ( LIKELY(NULL != buf) ) return buf->acquire();
        this->m_buffer = buffer_type::create(ItemAllocatorTraits::DefaultCapacity);
        return this->m_buffer->acquire();
      }

      inline pointer HEAVYUSE allocate(size_type cnt,
                     typename std::allocator<void>::const_pointer = 0)
      { return ( LIKELY(cnt == 1) ) ? allocate() : m_allocator.allocate(cnt); }

      inline void deallocate(pointer p) // shortcut for one element
      { this->m_buffer->release(p); }

      inline void deallocate(pointer p, size_type t)
      { (this->m_buffer && this->m_buffer->verify(p)) ? this->m_buffer->release(p) : m_allocator.deallocate(p, t); }
  
      //    size
      inline size_type max_size() const
      { return (std::numeric_limits<size_type>::max)() / sizeof(value_type); }
  
      //    construction/destruction
      inline void construct(pointer p, const value_type& t)
      { new(p) value_type(t); }
      inline void destroy(pointer p)
      { p->~value_type(); }
  
      inline bool operator==(ItemAllocator const&)
      { return true; }
      inline bool operator!=(ItemAllocator const& a)
      { return !operator==(a); }

      Header* header()
      { return this->m_buffer ? this->m_buffer->header() : (Header*)NULL; }

    private:
      typedef ALLOCATOR<value_type> Allocator;
      Allocator     m_allocator;
  };    //    end of class ItemAllocator 

} // namespace FIX

#endif
