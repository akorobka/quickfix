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

#ifndef FIX_FIELDMAP
#define FIX_FIELDMAP

#ifdef _MSC_VER
#pragma warning( disable: 4786 )
#endif

#include "Field.h"
#include "MessageSorters.h"
#include "Exceptions.h"
#include "ItemAllocator.h"
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>

#include "Container.h"

namespace FIX
{
/**
 * Stores and organizes a collection of Fields.
 *
 * This is the basis for a message, header, and trailer.  This collection
 * class uses a sorter to keep the fields in a particular order.
 */
class FieldMap
{

  struct stored_type
#ifndef ENABLE_FLAT_FIELDMAP
  : public Container::avlNode
#endif
  {

    int first;
    FieldBase second;

    struct compare_type : public message_order::comparator {
      typedef message_order::comparator base_type;
      compare_type( const message_order& order ) : base_type( order ) {}
      bool operator()(const stored_type& a, const stored_type& b) const
      { return base_type::operator()(a.first, b.first); }
      bool operator()(int a, const stored_type& b) const 
      { return base_type::operator()(a, b.first); }
      bool operator()(const stored_type& a, int b) const 
      { return base_type::operator()(a.first, b); }
    };

    template <typename A> struct disposer_type {
      A& m_allocator;
      disposer_type(A& a) : m_allocator(a) {}
      void operator()(stored_type* p) const {
        p->~stored_type();
        m_allocator.deallocate(p);
      }
    };

    template <typename A> struct cloner_type {
      A& m_allocator;
      cloner_type(A& a) : m_allocator(a) {}
      stored_type* operator()(const stored_type& r) {
        return new (m_allocator.allocate()) stored_type(r);
      }
    };

    template <typename Arg> stored_type( const Arg& arg ) : first(arg.getField()), second( arg ) {}
    template <typename Arg> stored_type( int tag, const Arg& arg ) : first(tag), second( arg ) {}
    stored_type( int tag, const std::string& value ) : first(tag), second( tag, value ) {}
  };

#if defined(ENABLE_FLAT_FIELDMAP)

  class RefBuffers {
    stored_type* m_buf[ItemAllocatorTraits::DefaultCapacity];
    std::size_t m_size;
    public:
    void clear() { m_size = 0; }
    stored_type** buffer(std::size_t count)
    { return (m_size + count <= ItemAllocatorTraits::DefaultCapacity) ? m_size += count, m_buf + m_size - count : NULL; }
  };

  typedef ItemAllocator< stored_type, RefBuffers > field_allocator_type;
  typedef Container::flatSet
          <stored_type,
           stored_type::compare_type,
           ALLOCATOR<stored_type*> > store_type;

  typedef store_type::const_iterator field_iterator;

  inline field_iterator f_begin() const { return m_fields.begin(); }
  inline field_iterator f_end() const { return m_fields.end(); }

  static inline store_type::const_iterator to_store_iterator(const field_iterator it) { return it; }
  static inline field_iterator link_next(const store_type::const_iterator it) { return it; }

#elif !defined(ENABLE_RELAXED_ORDERING)

  typedef ItemAllocator< stored_type > field_allocator_type;
  typedef Container::avlTree
          <stored_type,
           stored_type::compare_type> store_type;

  typedef store_type::const_iterator field_iterator;

  inline field_iterator f_begin() const { return m_fields.begin(); }
  inline field_iterator f_end() const { return m_fields.end(); }

  static inline store_type::const_iterator to_store_iterator(const field_iterator it) { return it; }
  static inline field_iterator link_next(const store_type::const_iterator it) { return it; }

#else

  typedef Container::avlNodeTraits<Container::avlNode> NodeTraits;
  template <typename Parent, typename MemberTraits, typename MemberType> static inline Parent* parent_from_member(MemberType* const pm)
  { return (Parent*)((uintptr_t)pm + 1 - (uintptr_t)MemberTraits::ptr_to_member((Parent*)1)); }

  class NodeList {

    typedef Container::avlNode node_type;
    typedef Util::PtrCache<int, node_type, ENABLE_RELAXED_ORDERING> cache_type;

    cache_type m_cache;
    node_type* m_head, ** m_pnext;

    public:

    struct Traits {
      static inline node_type** ptr_to_member(node_type* p) { return &p->m_next; }
      static inline node_type* node_from_next(node_type** const ppn) { return parent_from_member<node_type, Traits>(ppn); }

      static inline node_type** get_next_ptr(node_type* p) { return &p->m_next; }

      static inline void set_next(node_type* p, node_type* pn) { NodeTraits::set_next(p, pn); }
      static inline node_type* get_next(const node_type* p) { return NodeTraits::get_next(p); }

      static inline void set_prev(node_type* p, node_type* pp) { NodeTraits::set_right(p, pp); }
      static inline node_type* get_prev(const node_type* p) { return NodeTraits::get_right(p); }

      static inline void set_rank(node_type* p, std::size_t r) { NodeTraits::set_attribute(p, r); }
      static inline std::size_t get_rank(const node_type* p) { return NodeTraits::get_attribute(p); }

      static inline void set_parent(node_type* p) { NodeTraits::set_parent(p); }
    };
    typedef Traits traits_type;

    NodeList(node_type* header = NULL)
    : m_head(header), m_pnext(&m_head)
    {}
 
    NodeList* extend(node_type* next)
    { *m_pnext = next; return this; }

    bool empty() const
    { return &m_head == m_pnext; }

    template <class Disposer>
    void clear_and_dispose(Disposer disposer)
    {
      node_type *next = m_head;
      for(node_type *p = next, **pn = &m_head; pn != m_pnext; p = next)
      {
        next = traits_type::get_next(p);
        pn = traits_type::get_next_ptr(p);
        disposer(static_cast<stored_type*>(p));
      }
      m_head = next;
      m_pnext = &m_head;
      m_cache.clear();
    }

    stored_type* push_back(stored_type* p)
    {
      std::size_t i = m_cache.insert(p->first, p);
      if (i < m_cache.capacity())
      {
        node_type** pp = m_pnext;
        node_type* pn = *pp;
        *pp = p;
        m_pnext = traits_type::get_next_ptr(p);
        traits_type::set_parent(p);
        traits_type::set_next(p, pn);
        traits_type::set_prev(p, &m_head != pp ? traits_type::node_from_next(pp) : NULL);
        traits_type::set_rank(p, i);
        return p;
      }
      return NULL;
    }
    node_type* erase(const stored_type* n)
    {
      node_type* p = m_cache.evict(traits_type::get_rank(n));
      if (p) unlink(p);
      return p;
    }
    node_type* erase(int key)
    {
      node_type* p = m_cache.erase(key);
      if (p) unlink(p);
      return p;
    }
    node_type* find(int key)
    {
        return m_cache.lookup(key);
    }
    node_type* begin() { return m_head; }
    node_type* end() { return *m_pnext; }
    node_type* rbegin() { return traits_type::node_from_next(m_pnext); }

    private:

    void unlink(node_type* p)
    {
      node_type* pp = traits_type::get_prev(p);
      node_type* pn = traits_type::get_next(p);
      node_type** pnext = pp ? (traits_type::set_next(pp, pn), traits_type::get_next_ptr(pp)) : (m_head = pn, &m_head);
      if (m_pnext == traits_type::get_next_ptr(p))
        m_pnext = pnext;
    }
  };

  struct TreeTraits {

    typedef NodeList::Traits list_traits;
    typedef Container::avlNode node_type;

    // custom header holder
    template <class Tree> class Base {

      NodeList* m_plist;
      node_type m_header; // sides are left- and right-most nodes, parent is root

      protected:
      Base() : m_plist(NULL) {}

      node_type* header_ptr()
      { return &this->m_header; }

      const node_type* header_ptr() const
      { return &this->m_header; }

      public:

      struct HeaderTraits 
      {
        static inline node_type* ptr_to_member(Base* p)
        { return &p->m_header; }
      };

      static inline NodeList* list_for_header(node_type* header)
      { return parent_from_member<Base, HeaderTraits>(header)->m_plist; }

      NodeList* list() const
      { return this->m_plist; }

      void attach(NodeList* p)
      { this->m_plist = new (p) NodeList(&m_header); } // reset the header
    };

    // custom algorithms
    template <class NodeTraits> struct Algorithms : public Container::avlTreeTraits<node_type, true>::template Algorithms<NodeTraits>
    {
      typedef Container::avlTreeTraits<node_type, true>::template Algorithms<NodeTraits> algorithms_base;
      static bool is_enlisted(const node_type* p)
      { return !algorithms_base::node_traits::get_parent(p) && !algorithms_base::is_header(p); }
    };

  };

  typedef Container::avlTree<stored_type,
                             stored_type::compare_type,
                             TreeTraits::node_type, NodeTraits, TreeTraits> store_type;
  typedef ItemAllocator< stored_type, NodeList > field_allocator_type;

  class field_iterator : public store_type::const_iterator
  {
    static store_type::tree_traits::node_type* dec(const store_type::tree_traits::node_type* n)
    {
      store_type::tree_traits::node_type* p =  store_type::node_traits::get_parent(n); 
      if (!p)
      {
        if ((p = store_type::tree_traits::list_traits::get_prev(n)) || !store_type::algorithms::is_header(n)) return p;
      }
      else
      {
        if ((p = store_type::algorithms::prev_node_inner(n)) && !store_type::algorithms::is_header(p)) return p;
      }

      NodeList* l = store_type::base_type::list_for_header(p);
      if (l && !l->empty()) return l->rbegin();
      return p;
    }
    public:
    field_iterator() {}
    field_iterator(store_type::const_iterator i) : store_type::const_iterator(i) {}
    explicit field_iterator(const TreeTraits::node_type* p) : store_type::const_iterator(p) {}
    field_iterator& operator--()
    {
      m_node = dec(m_node);
      return *this;
    }
    field_iterator operator--(int)
    {
      field_iterator copy(*this);
      m_node = dec(m_node);
      return copy;
    }
  };

  inline field_iterator f_begin() const
  { NodeList* p = m_fields.list(); return p ? field_iterator(p->begin()) : m_fields.begin(); }
  inline field_iterator f_end() const { return m_fields.end(); }

  inline store_type::const_iterator to_store_iterator(const field_iterator it) { return it; }
  inline field_iterator link_next(const store_type::const_iterator it)
  { NodeList* p = m_fields.list(); if (p && m_fields.begin() == it) p->extend(const_cast<stored_type*>(&*it)); return it; }

#endif // ENABLE_RELAXED_ORDERING

  field_allocator_type m_allocator;

  public:

  typedef field_allocator_type allocator_type;
  allocator_type get_allocator() { return m_allocator; }

  private:

  typedef std::vector < FieldMap *, ALLOCATOR < FieldMap* > > GroupItem;
  typedef std::map < int, GroupItem, std::less<int>, 
                          ALLOCATOR < std::pair<const int, GroupItem> > > Groups;

  void g_clear()
  {
    Groups::const_iterator i, end = m_groups.end();
    for ( i = m_groups.begin(); i != end; ++i )
    {
      GroupItem::const_iterator j, jend = i->second.end();
      for ( j = i->second.begin(); j != jend; ++j )
        delete *j;
    }
    m_groups.clear();
  }

  void g_copy(const FieldMap& src)
  {
    Groups::const_iterator i, end = src.m_groups.end();
    for ( i = src.m_groups.begin(); i != end; ++i )
    {
      GroupItem::const_iterator j, jend = i->second.end();
      for ( j = i->second.begin(); j != jend; ++j )
      {
        FieldMap * pGroup = new FieldMap( FieldMap::create_allocator(), **j );
        m_groups[ i->first ].push_back( pGroup );
      }
    }
  }

  template <typename Arg> static inline void assign_value(store_type::iterator& it, const Arg& arg)
  { it->second.setPacked(arg); }
  void assign_value(store_type::iterator& it, const FieldBase& field)
  { it->second = field; }
  void assign_value(store_type::iterator& it, const std::string& value)
  { it->second.setString( value ); }

  inline void HEAVYUSE f_clear()
  {
    m_fields.clear_and_dispose(stored_type::disposer_type<allocator_type>(m_allocator));
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    NodeList* p = m_fields.list();
    if (p)
    {
      p->extend(&*m_fields.begin());
      p->clear_and_dispose(stored_type::disposer_type<allocator_type>(m_allocator));
    }
#endif
  }

  inline void f_copy(const FieldMap& from)
  {
    const store_type& src = from.m_fields;
    for (store_type::const_iterator it = src.begin(), e = src.end(); it != e; ++it)
      m_fields.push_back( *new (m_allocator.allocate()) stored_type(*it) );
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    NodeList* p = m_fields.list(), *po = from.m_fields.list();
    if (p)
    {
      if (po)
        for (field_iterator it(po->begin()), e(po->end()); it != e; ++it)
          p->push_back( new (m_allocator.allocate()) stored_type(*it) );
      p->extend(&*m_fields.begin());
    }
    else if (po)
      for (field_iterator it(po->begin()), e(po->end()); it != e; ++it)
        m_fields.insert_equal(*new (m_allocator.allocate()) stored_type(*it));
#endif
  }

  inline void f_clone(const FieldMap& from) {
    m_fields.clone_from( from.m_fields, stored_type::cloner_type<allocator_type>(m_allocator),
                                        stored_type::disposer_type<allocator_type>(m_allocator));
    m_order = from.m_order;
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    NodeList* p = m_fields.list(), *po = from.m_fields.list();
    if (p)
    {
      p->extend(&*m_fields.begin());
      if (po)
      {
        p->clear_and_dispose(stored_type::disposer_type<allocator_type>(m_allocator));
        for (field_iterator it(po->begin()), e(po->end()); it != e; ++it)
          p->push_back( new (m_allocator.allocate()) stored_type(*it) );
      }
    }
    else if (po)
      for (field_iterator it(po->begin()), e(po->end()); it != e; ++it)
        m_fields.insert_equal(*new (m_allocator.allocate()) stored_type(*it));
#endif
  }

  inline void erase(int tag) {
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    stored_type* f;
    NodeList* p = m_fields.list();
    if (p && (f = static_cast<stored_type*>(p->erase(tag))))
    {
      (stored_type::disposer_type<allocator_type>(m_allocator))(f);
      return;
    }
#endif
    store_type::iterator it = m_fields.find( tag, m_fields.value_comp() );
    if ( it != m_fields.end() ) m_fields.erase_and_dispose(it, stored_type::disposer_type<allocator_type>(m_allocator));
  }
  inline store_type::iterator erase(store_type::iterator& it) {
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    stored_type* f,* n;
    NodeList* p = m_fields.list();
    if (p && store_type::algorithms::is_enlisted((f = &*it)) && (f = static_cast<stored_type*>(p->erase(f))))
    {
      n = static_cast<stored_type*>(store_type::tree_traits::list_traits::get_next(f));
      (stored_type::disposer_type<allocator_type>(m_allocator))(f);
      return store_type::iterator(n);
    }
#endif
    return m_fields.erase_and_dispose( it, stored_type::disposer_type<allocator_type>(m_allocator));
  }

  template <typename Arg> stored_type& HEAVYUSE push_front_ordered(const Arg& arg) {
    stored_type* f = new (m_allocator.allocate()) stored_type( arg );
    m_fields.push_front(*f);
    return *f;
  }
  template <typename Arg> stored_type& HEAVYUSE push_back_ordered(const Arg& arg) {
    stored_type* f = new (m_allocator.allocate()) stored_type( arg );
    m_fields.push_back(*f);
    return *f;
  }
  template <typename Arg> stored_type& HEAVYUSE push_back(const Arg& arg) {
    stored_type* f = new (m_allocator.allocate()) stored_type( arg );
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    NodeList* p = m_fields.list();
    if (p && p->push_back(f)) return *f;
#endif
    m_fields.push_back(*f);
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    if (p && f == &*m_fields.begin()) p->extend(f);
#endif
    return *f;
  }

  inline field_iterator HEAVYUSE find(int tag) const {
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    store_type::tree_traits::node_type* f;
    NodeList* p = m_fields.list();
    if (p && (f = p->find(tag))) return field_iterator(f);
#endif
    return m_fields.find( tag, m_fields.value_comp() );
  }

  template <typename Arg> field_iterator HEAVYUSE add_ordered(const Arg& arg) {
    return link_next(m_fields.insert_equal(*new (m_allocator.allocate()) stored_type( arg )));
  }
  template <typename Arg> field_iterator HEAVYUSE add(const Arg& arg) {
    stored_type* f = new (m_allocator.allocate()) stored_type( arg );
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    NodeList* p = m_fields.list();
    if (p && p->push_back(f)) return field_iterator(f);
#endif
    return link_next(m_fields.insert_equal(*f));
  }
  template <typename Arg> field_iterator HEAVYUSE add(field_iterator hint, const Arg& arg) {
    stored_type* f = new (m_allocator.allocate()) stored_type( arg );
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    NodeList* p = m_fields.list();
    if (p && p->push_back(f)) return field_iterator(f);
#endif
    return link_next(m_fields.insert_equal(to_store_iterator(hint), *f));
  }

  template <typename Arg> store_type::iterator HEAVYUSE assign_ordered(int tag, const Arg& arg) {
    store_type::insert_commit_data data;
    std::pair<store_type::iterator, bool> r = m_fields.insert_unique_check( tag, m_fields.value_comp(), data);
    if (r.second) link_next(r.first = m_fields.insert_unique_commit( *new (m_allocator.allocate()) stored_type( tag, arg ), data));
    else assign_value(r.first, arg );
    return r.first;
  }
  template <typename Arg> store_type::iterator HEAVYUSE assign(int tag, const Arg& arg) {
    store_type::insert_commit_data data;
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    store_type::tree_traits::node_type* f;
    NodeList* p = m_fields.list();
    if (p && (f = p->find(tag)))
    {
      store_type::iterator it(f);
      assign_value(it, arg);
      return it;
    }
#endif
    std::pair<store_type::iterator, bool> r = m_fields.insert_unique_check( tag, m_fields.value_comp(), data);
    if (r.second)
    {
      stored_type* f = new (m_allocator.allocate()) stored_type( tag, arg );
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
      if (p && p->push_back(f)) return store_type::iterator(f);
#endif
      link_next(r.first = m_fields.insert_unique_commit( *f, data ));
    }
    else assign_value(r.first, arg );
    return r.first;
  }

protected:

  struct Options {
    const std::size_t expected_field_count;
    const bool strictly_ordered;
    Options(std::size_t fc, bool ordering = true) : expected_field_count(fc), strictly_ordered(ordering) {}
  };

  struct Sequence {
    template <typename Packed>
    static inline field_iterator push_front_to_ordered( FieldMap& map, const Packed& packed ) // in sequence, ordered container
    { return field_iterator(&map.push_front_ordered( packed )); }
    template <typename Packed>
    static inline field_iterator push_back_to_ordered( FieldMap& map, const Packed& packed ) // in sequence, ordered container
    { return field_iterator(&map.push_back_ordered( packed )); }
    template <typename Packed>
    static inline field_iterator push_back_to( FieldMap& map, const Packed& packed ) // in sequence
    { return field_iterator(&map.push_back( packed )); }

    template <typename Packed>
    static inline const store_type::value_type* insert_into_ordered( FieldMap& map, const Packed& packed ) // out of sequence, ordered container
    { return &*map.add_ordered( packed ); }
    template <typename Packed>
    static inline const store_type::value_type* insert_into( FieldMap& map, const Packed& packed ) // out of sequence
    { return &*map.add( packed ); }

    template <typename Packed>
    static inline store_type::value_type* set_in_ordered( FieldMap& map, const Packed& packed,
                                                                typename Packed::result_type* = NULL ) // out of sequence, ordered container
    { return &*map.assign_ordered( packed.getField(), packed ); }
    static inline store_type::value_type* set_in_ordered( FieldMap& map, const FieldBase& field)
    { return &*map.assign_ordered( field.getField(), field ); }
    template <typename Packed>
    static inline const store_type::value_type* set_in( FieldMap& map, const Packed& packed ) // out of sequence
    { return &*map.assign( packed.getField(), packed ); }

    static inline bool header_compare( const FieldMap&, int x, int y )
    { return message_order::header_compare( x, y ); }
    static inline bool trailer_compare( const FieldMap&, int x, int y )
    { return message_order::trailer_compare( x, y ); }
    static inline bool group_compare( const FieldMap& map, int x, int y )
    { return map.m_order.group_compare( x, y ); }
  };

public:

  typedef store_type Fields;

  typedef field_iterator         iterator;
  typedef field_iterator         const_iterator;

  typedef Groups::const_iterator g_iterator;
  typedef g_iterator             g_const_iterator;
  typedef Groups::mapped_type::const_iterator g_item_iterator;
  typedef g_item_iterator        g_item_const_iterator;

  FieldMap( const message_order& order =
            message_order( message_order::normal ) )
  : m_order( order ), m_fields( order ) {}

  FieldMap( const int order[] )
  : m_order( order ), m_fields( m_order ) {}

  FieldMap( const FieldMap& src )
  : m_order( src.m_order ), m_fields( src.m_fields.value_comp() )
  {
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    if (src.m_fields.list()) m_fields.attach(m_allocator.header());
#endif
    f_copy(src); g_copy(src);
  }

  FieldMap( const allocator_type& a, const message_order& order =
            message_order( message_order::normal ) )
  : m_allocator( a ), m_order( order ), m_fields( order ) {}

  FieldMap( const allocator_type& a, const FieldMap& src )
  : m_allocator( a ), m_order( src.m_order ), m_fields( src.m_fields.value_comp() )
  {
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    if (src.m_fields.list()) m_fields.attach(m_allocator.header());
#endif
    f_copy(src); g_copy(src);
  }

#ifdef ENABLE_FLAT_FIELDMAP
  FieldMap( const allocator_type& a, const message_order& order, const Options& opt )
  : m_allocator( a ), m_order( order ),
    m_fields( m_allocator.header()->buffer(opt.expected_field_count), opt.expected_field_count, order )
  {}
#else
  FieldMap( const allocator_type& a, const message_order& order, const Options& opt )
  : m_allocator( a ), m_order( order ), m_fields( order )
  {
#ifdef ENABLE_RELAXED_ORDERING
    if (!opt.strictly_ordered) m_fields.attach(m_allocator.header());
#endif
  }
#endif

  virtual ~FieldMap();

  FieldMap& operator=( const FieldMap& rhs );

  /// Adds a field without type checking by constructing in place
  template <typename Packed>
  iterator addField( const Packed& packed,
                     typename Packed::result_type* = NULL )
  { return add( end(), packed ); }

  iterator addField( const FieldBase& field )
  { return add( end(), field ); }

  template <typename Packed>
  iterator addField( FieldMap::iterator hint, const Packed& packed,
                     typename Packed::result_type* = NULL )
  { return add( hint, packed ); }

  iterator addField( FieldMap::iterator hint, const FieldBase& field )
  { return add( hint, field ); }

  /// Overwrite a field without type checking by constructing in place
  template <typename Packed>
  FieldBase& setField( const Packed& packed,
                       typename Packed::result_type* = NULL)
  { return assign( packed.getField(), packed )->second; }

  /// Set a field without type checking
  void setField( const FieldBase& field, bool overwrite = true )
  throw( RepeatedTag )
  {
    if ( overwrite )
      assign( field.getTag(), field );
    else
      add( field );
  }

  /// Set a field without a field class
  void setField( int tag, const std::string& value )
  throw( RepeatedTag, NoTagValue )
  { assign( tag, value ); }

  /// Get a field if set
  bool getFieldIfSet( FieldBase& field ) const
  {
    Fields::const_iterator iter = find( field.getTag() );
    if ( iter == m_fields.end() )
      return false;
    field = iter->second;
    return true;
  }

  /// Get a field without type checking
  FieldBase& getField( FieldBase& field )
  const throw( FieldNotFound )
  {
    field = getFieldRef( field.getField() );
    return field;
  }

  /// Get a field without a field class
  const std::string& getField( int tag )
  const throw( FieldNotFound )
  { return getFieldRef( tag ).getString(); }

  /// Get direct access to a field through a reference
  const FieldBase& getFieldRef( int tag )
  const throw( FieldNotFound )
  {
    Fields::const_iterator iter = find( tag );
    if ( iter == m_fields.end() )
      throw FieldNotFound( tag );
    return iter->second;
  }

  /// Get direct access to a field through a pointer
  const FieldBase* getFieldPtr( int tag )
  const throw( FieldNotFound )
  { return &getFieldRef( tag ); }

  /// Get direct access to a field through a pointer
  const FieldBase* getFieldPtrIfSet( int tag ) const
  {
    Fields::const_iterator iter = find( tag );
    return ( iter != m_fields.end() ) ? &iter->second : NULL;
  }

  /// Check to see if a field is set
  bool isSetField( const FieldBase& field ) const
  { return find( field.getTag() ) != m_fields.end(); }
  /// Check to see if a field is set by referencing its number
  bool isSetField( int tag ) const
  { return find( tag ) != m_fields.end(); }

  /// Remove a field. If field is not present, this is a no-op.
  void removeField( int tag )
  { erase( tag ); }

  /// Remove a range of ordered fields 
  template <typename TagIterator>
  void removeFields(TagIterator tb, TagIterator te)
  {
    if( tb < te )
    {
      int tag, last = *(tb + ((te - tb) - 1));
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
      NodeList* p = m_fields.list();
      if (p)
      {
        stored_type* f;
        for( TagIterator t = tb; t < te; ++t )
          while( (f = static_cast<stored_type*>(p->erase(tag = *t))) )
            (stored_type::disposer_type<allocator_type>(m_allocator))(f);
      }
#endif
      store_type::iterator it = m_fields.lower_bound( *tb, m_fields.value_comp() );
      while( it != m_fields.end() && (tag = it->first) <= last )
      {
        for( ; *tb < tag; ++tb )
          ;
        if( tag == *tb )
          it = erase( it );
        else
          ++it;
      }
    }
  }

  /// Add a group and return a reference to the added object.
  FieldMap& addGroup( int tag, const FieldMap& group, bool setCount = true );

  /// Acquire ownership of Group object
  void addGroupPtr( int tag, FieldMap* group, bool setCount = true );

  /// Replace a specific instance of a group.
  bool replaceGroup( int num, int tag, const FieldMap& group );

  /// Get a specific instance of a group.
  FieldMap& getGroup( int num, int tag, FieldMap& group ) const
  throw( FieldNotFound )
  { return group = getGroupRef( num, tag ); }

  /// Get direct access to a field through a reference
  FieldMap& getGroupRef( int num, int tag ) const
  throw( FieldNotFound )
  {
    Groups::const_iterator i = m_groups.find( tag );
    if( i == m_groups.end() ) throw FieldNotFound( tag );
    if( num <= 0 ) throw FieldNotFound( tag );
    if( i->second.size() < (unsigned)num ) throw FieldNotFound( tag );
    return *( *(i->second.begin() + (num-1) ) );
  }

  /// Get direct access to a field through a pointer
  FieldMap* getGroupPtr( int num, int tag ) const
  throw( FieldNotFound )
  { return &getGroupRef( num, tag ); }

  /// Remove a specific instance of a group.
  void removeGroup( int num, int tag );
  /// Remove all instances of a group.
  void removeGroup( int tag );

  /// Check to see any instance of a group exists
  bool hasGroup( int tag ) const;
  /// Check to see if a specific instance of a group exists
  bool hasGroup( int num, int tag ) const;
  /// Count the number of instance of a group
  size_t groupCount( int tag ) const;

  /// Clear all fields from the map
  void clear()
  {
    f_clear();
    g_clear();
  }

  /// Check if map contains any fields
  bool isEmpty()
  { 
#if !defined(ENABLE_FLAT_FIELDMAP) && defined(ENABLE_RELAXED_ORDERING)
    NodeList* p = m_fields.list();
    return (!p || p->empty()) && m_fields.empty();
#else
    return m_fields.empty();
#endif
  }

  size_t totalFields() const;

  std::string& calculateString( std::string&, bool clear = true ) const;

  int calculateLength( int beginStringField = FIELD::BeginString,
                       int bodyLengthField = FIELD::BodyLength,
                       int checkSumField = FIELD::CheckSum ) const;

  int calculateTotal( int checkSumField = FIELD::CheckSum ) const;

  iterator begin() const { return f_begin(); }
  iterator end() const { return f_end(); }
  g_iterator g_begin() const { return m_groups.begin(); }
  g_iterator g_end() const { return m_groups.end(); }

  template <typename S> S& serializeTo( S& sink ) const
  {
    iterator i;
    for ( i = begin(); i != end(); ++i )
    {
      i->second.pushValue(sink);

      if( m_groups.empty() ) continue;
      Groups::const_iterator j = m_groups.find( i->first );
      if ( j == m_groups.end() ) continue;
      GroupItem::const_iterator ke = j->second.end();
      for ( GroupItem::const_iterator k = j->second.begin(); k != ke; ++k )
        ( *k ) ->serializeTo( sink );
    }
    return sink;
  }

protected:
  static inline
  allocator_type create_allocator(std::size_t n = ItemAllocatorTraits::DefaultCapacity)
  {
    allocator_type a( allocator_type::buffer_type::create(n) );
#ifdef ENABLE_FLAT_FIELDMAP
    a.header()->clear();
#endif
    return a;
  } 

  static inline
  void reset_allocator( allocator_type& a )
  {
#ifdef ENABLE_FLAT_FIELDMAP
    a.header()->clear();
#endif
    a.clear();
  }

  message_order m_order; // must precede field container
private:
  store_type m_fields;
  Groups m_groups;
};
/*! @} */
}

#define FIELD_SET( MAP, FIELD )           \
bool isSet( const FIELD& field ) const    \
{ return (MAP).isSetField(field); }       \
void set( const FIELD& field )            \
{ (MAP).setField(field); }                \
void set( const FIELD::Pack& packed )     \
{ (MAP).setField(packed); }               \
FIELD& get( FIELD& field ) const          \
{ return (FIELD&)(MAP).getField(field); } \
bool getIfSet( FIELD& field ) const       \
{ return (MAP).getFieldIfSet(field); }

#define FIELD_GET_PTR( MAP, FLD ) \
(const FIX::FLD*)MAP.getFieldPtr( FIX::FIELD::FLD )
#define FIELD_GET_REF( MAP, FLD ) \
(const FIX::FLD&)MAP.getFieldRef( FIX::FIELD::FLD )
#define FIELD_THROW_IF_NOT_FOUND( MAP, FLD ) \
if( !(MAP).isSetField( FIX::FIELD::FLD) ) \
  throw FieldNotFound( FIX::FIELD::FLD )

#endif //FIX_FIELDMAP

