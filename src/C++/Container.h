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

#ifndef FIX_CONTAINER_H
#define FIX_CONTAINER_H

#include <functional>
#include <iterator>
#include <utility>
#include <memory>

#include "Utility.h"

#ifdef HAVE_BOOST
#include <boost/pool/pool_alloc.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#endif

namespace FIX 
{
namespace Container {

// Minimal dictionary without erase capability
// based on open addressing hash table with power of 2 size,
// triangular sequence quadratic probing and cached key hashes.
// Optimized specializations for tag indices assume key tags to be non-negative
template <typename Dictionary, typename Traits> class DictionaryBase {
public:

  typedef typename Traits::value_type value_type;
  typedef typename Traits::key_type key_type;
  typedef typename Traits::mapped_type mapped_type;
  typedef typename Traits::hasher hasher;
  typedef typename Traits::key_equal key_equal;
  typedef value_type* pointer;
  typedef value_type const* const_pointer;
  typedef value_type& reference;
  typedef value_type const& const_reference;

protected:
  typedef typename Traits::hash_type hash_type;
  typedef typename Traits::stored_type stored_type;

  static const int MaxLoadPercentage = 70; 

  static inline stored_type* first(stored_type* p) {
    do { if (!Traits::empty(p)) return p; } while(!Traits::last(p++));
    return NULL;
  }

  template <typename P> static inline P* next(P* p) {
    while (!Traits::last(p)) { if (!Traits::empty(++p)) return p; }
    return NULL;
  }

  inline Dictionary* dictionary() { return static_cast<Dictionary*>(this); }
  inline const Dictionary* dictionary() const { return static_cast<const Dictionary*>(this); }

public:
  class iterator {

    stored_type* m_p;
    public:
      typedef std::forward_iterator_tag iterator_category;

      typedef typename Traits::value_type value_type;
      typedef value_type* pointer;
      typedef value_type& reference;
      typedef std::ptrdiff_t difference_type;

      iterator(stored_type* p = NULL) : m_p(p) {}
      reference operator*() { return Traits::value(m_p); }
      pointer operator->() { return &Traits::value(m_p); }

      iterator& operator++() { m_p = next(m_p); return *this; }
      iterator operator++(int) { iterator c(m_p); ++(*this); return c; }
      bool operator==( const iterator& rhs ) const { return m_p == rhs.m_p; }
      bool operator!=( const iterator& rhs ) const { return m_p != rhs.m_p; }
  };

  class const_iterator {

    const stored_type* m_p;
    public:
      typedef std::forward_iterator_tag iterator_category;

      typedef const typename Traits::value_type value_type;
      typedef value_type* pointer;
      typedef value_type& reference;
      typedef std::ptrdiff_t difference_type;

      const_iterator(stored_type* p = NULL) : m_p(p) {}
      reference operator*() const { return Traits::value(m_p); }
      pointer operator->() const { return &Traits::value(m_p); }

      const_iterator& operator++() { m_p = next(m_p); return *this; }
      const_iterator operator++(int) { const_iterator c(m_p); ++(*this); return c; }
      bool operator==( const const_iterator& rhs ) const { return m_p == rhs.m_p; }
      bool operator!=( const const_iterator& rhs ) const { return m_p != rhs.m_p; }
  };

  iterator begin() { return m_size ? first(dictionary()->stored()) : NULL; }
  iterator end() { return iterator(); }

  const_iterator begin() const { return m_size ? first(dictionary()->stored()) : NULL; }
  const_iterator end() const { return const_iterator(); }

  inline const_iterator find(const key_type& key) const { return find_this(key); }
  inline iterator find(const key_type& key) { return find_this(key); }

  std::pair<iterator, bool> insert(const value_type& value) {
    if ( m_size >= m_watermark ) {
      rehash( (m_max_index + 1) << 1 );
    }

    stored_type* p = dictionary()->stored();
    std::size_t i, step = 0;
    hash_type h = typename Traits::hasher()(Traits::value_key(value));
    for ( i = h & m_max_index; !Traits::empty(p + i); i = (i + ++step) & m_max_index ) {
      if ( h == Traits::value_attr(p + i) &&
           key_equal()(Traits::value_key(value), Traits::value_key(Traits::value(p + i))) )
        return std::pair<iterator, bool>(iterator(p + i), false);
    }
    new (&Traits::value(p + i)) value_type(value);
    Traits::assign_attr(p + i, h, i == m_max_index);
    m_size++;
    return std::pair<iterator, bool>(iterator(p + i), true);
  }

  mapped_type& operator[](const key_type& key) {
    iterator it = find_this(key);
    if (it != end()) return it->second;
    return insert(value_type(key, mapped_type())).first->second;
  }

  bool empty() const { return m_size == 0; }

  std::size_t size() const { return m_size; }

  protected:

  DictionaryBase()
  : m_max_index(0), m_size(0), m_watermark(0) {}

  stored_type* HEAVYUSE find_this(const key_type& k) const {
    stored_type* p = dictionary()->stored();
    if ( p ) {
      std::size_t i, step = 0;
      hash_type h = typename Traits::hasher()(k);
      for ( i = h & m_max_index; !Traits::empty(p + i); i = (i + ++step) & m_max_index ) {
        if ( h == Traits::value_attr(p + i) &&
             key_equal()(k, Traits::value_key(Traits::value(p + i))) )
          return p + i;
      }
    }
    return NULL;
  }

  void destroy() {
    if ( m_size ) {
      stored_type* p = dictionary()->stored();
      for (iterator it = first(p); it != end(); ++it) 
        (*it).~value_type();
      dictionary()->release(p, m_max_index + 1);
    }
  }

  void dup( stored_type* src, std::size_t capacity, std::size_t size, bool move = false ) {
    stored_type* p = dictionary()->reserve(capacity);
    m_size = 0;
    m_max_index = capacity - 1;
    m_watermark = capacity > 8 ? capacity * MaxLoadPercentage / 100 : capacity * 3 / 4 ;
    Traits::format_attr(p, m_max_index);
    if ( size ) {
      if ( move ) {
        for ( iterator it = first(src); it != end(); ++it ) {
          insert(*it);
          (*it).~value_type();
        }
      } else
        for ( iterator it = first(src); it != end(); ++it )
          insert(*it);
    }
  }

  void dup( stored_type* src, const DictionaryBase& other ) {
    if ( src )
      dup( src, other.m_max_index + 1, other.m_size );
    else
      m_max_index = m_size = m_watermark = 0;
  }

  private:

  void rehash( std::size_t target_capacity ) {
    stored_type* p = dictionary()->stored();
    std::size_t index = m_max_index;
    if ( target_capacity > index ) {
      std::size_t size = m_size;
      this->dup(p, target_capacity, size, true);
      if ( p )
        dictionary()->release(p, index + 1);
    }
  }

  std::size_t m_max_index;
  std::size_t m_size;
  std::size_t m_watermark;
};

template <typename Key, typename Hash>
struct TraitsBase {
  typedef Key key_type;
  typedef std::size_t hash_type;

  static const int HashBits = (sizeof(hash_type) << 3) - 2; // reserve two upper bits for markers
  static const std::size_t HashMask = ((hash_type)1 << (HashBits - 1)) - 1;
  static const std::size_t Terminal = (hash_type)1 << HashBits;
  static const std::size_t Empty = ((hash_type)1 << (HashBits + 1)); // top bit

  static const std::size_t PadSz = 0;

  struct hasher {
    hash_type operator()(const key_type& key) { return Hash()(key) & HashMask; }
  };
};

// Specialization for tag as key (positive integer)
template <>
struct TraitsBase<int, Util::Tag::Identity> {
  typedef int key_type;
  typedef int hash_type;

  static const int Empty = ((hash_type)1 << ((sizeof(hash_type) << 3) - 1)); // top bit
  static const int Terminal = ~0; // includes Empty bit

  static const std::size_t PadSz = 1;

  struct hasher {
    hash_type operator()(const int& key) { return key; }; // max tag value limited to 99999999
  };
};

template <typename Key, typename Mapped, typename Hash, typename Pred>
struct MapTraits : public TraitsBase<Key, Hash> {

  typedef TraitsBase<Key, Hash> base_type;
  typedef typename base_type::key_type key_type;
  typedef typename base_type::hash_type hash_type;

  typedef Mapped mapped_type;
  typedef std::pair<key_type, mapped_type> value_type;
  typedef std::pair<hash_type, value_type> stored_type;
  typedef Pred key_equal;

  static inline hash_type empty(const stored_type* p) { return base_type::Empty & p->first; }
  static inline hash_type last(const stored_type* p) { return base_type::Terminal & p->first; }

  static inline hash_type   value_attr(const stored_type* p) { return p->first & base_type::HashMask; }
  static inline value_type& value(stored_type* p) { return p->second; }
  static inline const value_type& value(const stored_type* p) { return p->second; }
  static inline const key_type& value_key(const value_type& v) { return v.first; }

  static inline void assign_attr(stored_type* p, hash_type h, bool last) {
    p->first = h | (last ? base_type::Terminal : 0);
  }
  static inline void format_attr(stored_type* p, std::size_t max_index) {
    for ( std::size_t i = 0; i < max_index; i++) p++->first = base_type::Empty;
    p->first = base_type::Empty | base_type::Terminal;
  }
};

template <typename Mapped>
struct MapTraits<int, Mapped, Util::Tag::Identity, std::equal_to<int> >
: public TraitsBase<int, Util::Tag::Identity> {

  typedef TraitsBase<int, Util::Tag::Identity> base_type;
  typedef base_type::key_type key_type;
  typedef base_type::hash_type hash_type;

  typedef Mapped mapped_type;
  typedef std::pair<hash_type, mapped_type> value_type;
  typedef value_type stored_type;
  typedef std::equal_to<int> key_equal;

  static inline bool empty(const stored_type* p) { return base_type::Empty == p->first; }
  static inline bool last(const stored_type* p) { return base_type::Terminal == p[1].first; }

  static inline hash_type   value_attr(const stored_type* p) { return p->first; }
  static inline value_type& value(stored_type* p) { return *p; }
  static inline const value_type& value(const stored_type* p) { return *p; }
  static inline key_type    value_key(const value_type& v) { return v.first; }

  static inline void assign_attr(stored_type* p, hash_type h, bool) {
    p->first = h;
  }
  static inline void format_attr(stored_type* p, std::size_t max_index) {
    for ( std::size_t i = 0; i <= max_index; i++) p++->first = base_type::Empty;
    p->first = base_type::Terminal;
  }
};

template <typename Key, typename Mapped, typename Hash,
          typename Pred = std::equal_to<Key>, 
          typename Alloc = std::allocator<std::pair<Key const, Mapped> > >
class DictionaryMap
: public DictionaryBase<
  DictionaryMap<Key, Mapped, Hash, Pred, Alloc>,
  MapTraits< Key, Mapped, Hash, Pred >
>
{
  template <typename D, typename T> friend class DictionaryBase;
  typedef MapTraits< Key, Mapped, Hash, Pred > Traits;
  typedef DictionaryBase<
            DictionaryMap<Key, Mapped, Hash, Pred, Alloc>,
            MapTraits< Key, Mapped, Hash, Pred >
          > base_type;
  typedef typename base_type::stored_type stored_type;
  public:

  typedef Alloc allocator_type;

  DictionaryMap()
  : m_values(NULL), m_alloc(allocator_type())
  {}

  DictionaryMap(allocator_type a) 
  : m_values(NULL), m_alloc(a)
  {}
  
  DictionaryMap(const DictionaryMap& other)
  : m_values(NULL), m_alloc(other.m_alloc) {
    this->dup(other.m_values, other );
  }

  ~DictionaryMap() { this->destroy(); }

  DictionaryMap& operator=(const DictionaryMap& other) {
    this->destroy();
    m_values = NULL;
    this->dup(other.m_values, other );
    return *this;
  }

  protected:

  stored_type* stored() const { return m_values; }
  stored_type* reserve(std::size_t capacity) {
    return (m_values = m_alloc.allocate( capacity + Traits::PadSz ));
  }
  void release(stored_type* p, std::size_t capacity) {
    m_alloc.deallocate( p, capacity + Traits::PadSz );
  }

  private:

  typedef typename Alloc::template rebind<stored_type>::other stored_allocator_type;
  
  stored_type* m_values;
  stored_allocator_type m_alloc;
};

template <typename Key, typename Hash, typename Pred>
struct SetTraits : public TraitsBase<Key, Hash> {

  typedef TraitsBase<Key, Hash> base_type;
  typedef typename base_type::key_type key_type;
  typedef typename base_type::hash_type hash_type;

  typedef key_type mapped_type;
  typedef key_type value_type;
  typedef std::pair<hash_type, value_type> stored_type;
  typedef Pred key_equal;

  static inline hash_type empty(const stored_type* p) { return base_type::Empty & p->first; }
  static inline hash_type last(const stored_type* p) { return base_type::Terminal & p->first; }

  static inline hash_type   value_attr(const stored_type* p) { return p->first & base_type::HashMask; }
  static inline value_type& value(stored_type* p) { return p->second; }
  static inline const value_type& value(const stored_type* p) { return p->second; }
  static inline const key_type& value_key(const value_type& v) { return v; }

  static inline void assign_attr(stored_type* p, hash_type h, bool last) {
    p->first = h | (last ? base_type::Terminal : 0);
  }
  static inline void format_attr(stored_type* p, std::size_t max_index) {
    for ( std::size_t i = 0; i < max_index; i++) p++->first = base_type::Empty;
    p->first = base_type::Empty | base_type::Terminal;
  }
};

template <>
struct SetTraits<int, Util::Tag::Identity, std::equal_to<int> >
: public TraitsBase<int, Util::Tag::Identity> {

  typedef TraitsBase<int, Util::Tag::Identity> base_type;

  typedef key_type mapped_type;
  typedef key_type value_type;
  typedef hash_type stored_type;
  typedef std::equal_to<int> key_equal;

  static inline bool empty(const stored_type* p) { return Empty == *p; }
  static inline bool last(const stored_type* p) { return Terminal == p[1]; }

  static inline hash_type   value_attr(const stored_type* p) { return *p; }
  static inline value_type& value(stored_type* p) { return *p; }
  static inline const value_type& value(const stored_type* p) { return *p; }
  static inline key_type    value_key(const value_type& v) { return v; }

  static inline void assign_attr(stored_type* p, hash_type h, bool) {
    *p = h;
  }
  static inline void format_attr(stored_type* p, std::size_t max_index) {
    for ( std::size_t i = 0; i <= max_index; i++) *p++ = base_type::Empty;
    *p = base_type::Terminal;
  }
};

template <typename Key, typename Hash,
          typename Pred = std::equal_to<Key>, 
          typename Alloc = std::allocator<Key> >
class DictionarySet
: public DictionaryBase<
  DictionarySet< Key, Hash, Pred, Alloc >,
  SetTraits< Key, Hash, Pred >
>
{
  template <typename D, typename T> friend class DictionaryBase;
  typedef SetTraits< Key, Hash, Pred > Traits;
  typedef DictionaryBase<
            DictionarySet< Key, Hash, Pred, Alloc >,
            SetTraits< Key, Hash, Pred >
          > base_type;
  typedef typename base_type::stored_type stored_type;
  public:

  typedef Alloc allocator_type;

  DictionarySet()
  : m_values(NULL), m_alloc(allocator_type())
  {}

  DictionarySet(allocator_type a) 
  : m_values(NULL), m_alloc(a)
  {}
  
  DictionarySet(const DictionarySet& other)
  : m_values(NULL), m_alloc(other.m_alloc) {
    this->dup(other.m_values, other );
  }

  ~DictionarySet() { this->destroy(); }

  DictionarySet& operator=(const DictionarySet& other) {
    this->destroy();
    m_values = NULL;
    this->dup(other.m_values, other );
    return *this;
  }

  protected:

  stored_type* stored() const { return m_values; }
  stored_type* reserve(std::size_t capacity) {
    return (m_values = m_alloc.allocate( capacity + Traits::PadSz ));
  }
  void release(stored_type* p, std::size_t capacity) {
    m_alloc.deallocate( p, capacity + Traits::PadSz );
  }

  private:

  typedef typename Alloc::template rebind<stored_type>::other stored_allocator_type;
  
  stored_type* m_values;
  stored_allocator_type m_alloc;
};

// Threaded AVL tree
struct avlNodeTraitTypes {

  enum side { left = 0, right = 1, parent = 2 }; // exactly this way
  enum balance
  {
    unbalanced_neg_t = -2, // exactly this way
    neg_t,
    zero_t,
    pos_t,
    unbalanced_pos_t
  };

  template <class B, class D> struct is_accessible_and_unique_base_of
  {
    typedef char yes_type[1]; typedef char no_type[2];
    static yes_type& test(B*); static no_type& test(...);
    static const bool value = (sizeof(test((D*)0)) == sizeof(yes_type));
  };
};

struct avlNode {

  avlNode* m_link[3];
  avlNode* m_next;
  union {
    avlNodeTraitTypes::balance m_balance;
    unsigned m_uint;
  };

}
#if defined(__GNUC__)
  __attribute__((packed))
#endif
;

// node traits
template <typename Node> struct avlNodeTraits : public avlNodeTraitTypes {

  typedef Node node_type;

  static inline node_type* uncast(const node_type* p)
  { return const_cast<node_type*>(p); }

  static inline node_type* get_parent(const node_type* n)
  { return n->m_link[parent]; }

  static inline void set_parent(node_type* n, node_type* p = NULL)
  { n->m_link[parent] = p; }

  static inline node_type* get_next(const node_type* n)
  { return n->m_next; }

  static inline void set_next(node_type* n, node_type* p = NULL)
  { n->m_next = p; }

  static inline node_type* get_side(const node_type* n, bool right_side)
  { return n->m_link[right_side]; }

  static inline node_type* get_link(const node_type* n, side dir) 
  { return n->m_link[dir]; }

  static inline node_type* get_left(const node_type* n)
  { return n->m_link[left]; }

  static inline node_type* get_right(const node_type* n)
  { return n->m_link[right]; }

  static inline void set_side(node_type* n, bool right_side, node_type* p = NULL)
  { n->m_link[right_side] = p; }
      
  static inline void set_side(node_type* n, side dir, node_type* p = NULL)
  { n->m_link[dir] = p; }

  static inline void set_link(node_type* n, side dir, node_type* p = NULL)
  { n->m_link[dir] = p; }

  static inline void set_left(node_type* n, node_type* l = NULL)
  { n->m_link[left] = l; }

  static inline void set_right(node_type* n, node_type* r = NULL)
  { n->m_link[right] = r; }

  static inline balance get_balance(const node_type* n)
  { return n->m_balance; }

  static inline balance add_balance(node_type* n, int v)
  { return (balance)(n->m_uint += v); }

  static inline void set_balance(node_type* n, balance b)
  { n->m_balance = b; }

  static inline balance invert_balance(balance b)
  { return (balance)-b; }

  static inline unsigned get_attribute(const node_type* n)
  { return n->m_uint; }

  static inline void set_attribute(node_type* n, unsigned a)
  { n->m_uint = a; }
};

// tree traits with the base class template
template <typename Node, bool> struct avlTreeTraits {

  typedef Node node_type;

  template <class Tree> class Base
  {
    node_type m_header; // sides are left- and right-most nodes, parent is root
    protected:

    node_type* header_ptr()
    { return &this->m_header; }

    const node_type* header_ptr() const
    { return &this->m_header; }
  };

  template <class NodeTraits> struct Algorithms
  {
    typedef Node node_type;
    typedef NodeTraits node_traits;

    static bool is_header(const node_type* n)
    { return node_traits::get_next(n) == n; }

    static void init_header(node_type* n)
    { 
      node_traits::set_parent(n);
      node_traits::set_left(n, n); node_traits::set_right(n, n); 
      node_traits::set_next(n, n); // header->m_next always points to itself
      node_traits::set_balance(n, node_traits::zero_t);
    }

    static void init_node(node_type* n, node_type* parent, node_type* next)
    {
      node_traits::set_left(n);
      node_traits::set_right(n);
      node_traits::set_parent(n, parent);
      node_traits::set_next(n, next);
      node_traits::set_balance(n, node_traits::zero_t);
    }
  
    static node_type* get_header(const node_type* n)
    {
      node_type* p = node_traits::get_parent(n);
      if (p)
      {
        while(!is_header(p))
          p = node_traits::get_parent(p);
        return p;
      }
      return node_traits::uncast(n);
    }
  
    static node_type* begin_node(const node_type* header)
    { return node_traits::get_left(header); }
  
    static node_type* end_node(const node_type* header)
    { return node_traits::uncast(header); }
  
    static inline node_type* maximum(node_type* n)
    {
      node_type* p;
      do { n = node_traits::get_right(p = n); } while (n);
      return p;
    }
  
    static node_type* next_node(const node_type* n)
    { return node_traits::get_next(n); }
  
    static node_type* prev_node_inner(const node_type* n)
    {
      node_type* p,* x = node_traits::get_left(n);
      if (x)
        p = maximum(x);
      else 
      {
        p = node_traits::get_parent(n);
        if (n == node_traits::get_left(p))
          do { p = node_traits::get_parent(x = p); } while (x == node_traits::get_left(p));
      }
      return p; // header if leftmost
    }
    static node_type* prev_node(const node_type* n)
    {
      return is_header(n) ? node_traits::get_right(n) : prev_node_inner(n);
    }
  };
};
// specialization to fail when the value type is not derived from the node type
template <typename Node> struct avlTreeTraits<Node, false> {
  template <class Tree, class Value_is_not_derived_from_BaseHook> class Base {};
};

// implementation class
template <typename Node,
          typename Compare = std::less<Node>,
          typename BaseHook = avlNode,
          typename NodeTraits = avlNodeTraits<BaseHook>,
          typename TreeTraits = avlTreeTraits<BaseHook, avlNodeTraitTypes::is_accessible_and_unique_base_of<BaseHook, Node>::value> >
class avlTree
: public TreeTraits::template Base< avlTree< Node, Compare, BaseHook, NodeTraits, TreeTraits > > {

  typedef BaseHook				node;
  typedef BaseHook*				node_ptr;
  typedef const BaseHook*			const_node_ptr;

  friend class TreeTraits::template Base<avlTree>;
  public:
  typedef typename TreeTraits::template Algorithms<NodeTraits> algorithms;
  typedef typename TreeTraits::template Base<avlTree> base_type;

  typedef avlTree				tree_type;
  typedef TreeTraits				tree_traits;
  typedef NodeTraits 				node_traits;

  typedef std::size_t				size_type;
  typedef Node					value_type;
  typedef Compare				value_compare;
  typedef Compare				key_compare;

  struct insert_commit_data
  {
    node_ptr prev, anchor;
    typename node_traits::side direction;
  };

  private:

  value_compare m_comp;

  template <class KeyValueCompare>
  struct KeyNodeCompare {
    KeyValueCompare m_comp;
    public:
    KeyNodeCompare(KeyValueCompare comp) : m_comp(comp) {}

    template <class T>
    const value_type& extract_key(const T& node, node_ptr) const 
    { return *static_cast<const value_type*>(node); }

    template <class T>
    const value_type& extract_key(const T& node, const_node_ptr) const 
    { return *static_cast<const value_type*>(node); }

    template <class T, class U>
    const T& extract_key(const T& key, const U&) const
    { return key; }

    template <class KeyA, class KeyB>
    bool operator()(const KeyA& a, const KeyB& b) const
    { return this->m_comp(this->extract_key(a, a), this->extract_key(b, b)); }
  };

  template <class Cloner>
  struct CloneInserter {
    tree_type& m_tree;
    Cloner m_cloner;
    public:
    typedef typename tree_type::value_type&	        reference;
    typedef const typename tree_type::value_type&       const_reference;
    typedef typename tree_type::value_type              value_type;

    CloneInserter(tree_type& t, Cloner c) : m_tree(t), m_cloner(c) {}

    void push_back(const value_type& v)
    { m_tree.push_back(*m_cloner(v)); }

    std::back_insert_iterator<CloneInserter> inserter()
    { return std::back_inserter<CloneInserter>(*this); }
  };

  static node_ptr rotate_single(const node_ptr & r, typename node_traits::side dir)
  {
    node_ptr n = node_traits::get_side(r, !dir);
    node_ptr nsa = node_traits::get_link(n, dir);
    node_traits::set_side(r, !dir, nsa);
    if (nsa) node_traits::set_parent(nsa, r);
    node_traits::set_side(n, dir, r);
    node_traits::set_parent(n, node_traits::get_parent(r));
    node_traits::set_parent(r, n);
    return n;
  }

  static node_ptr rotate_double(const node_ptr & r, typename node_traits::side dir)
  {
    node_ptr n = node_traits::get_side(r, !dir);
    node_ptr b = node_traits::get_link(n, dir);
    node_ptr bsa = node_traits::get_side(b, !dir);
    node_traits::set_side(n, dir, bsa);
    if (bsa) node_traits::set_parent(bsa, n);
    node_ptr bsb = node_traits::get_link(b, dir);
    node_traits::set_side(r, !dir, bsb);
    if (bsb) node_traits::set_parent(bsb, r);
    node_traits::set_side(b, !dir, n);
    node_traits::set_side(b, dir, r);
    node_traits::set_parent(b, node_traits::get_parent(r));
    node_traits::set_parent(r, b);
    node_traits::set_parent(n, b);
    return b;
  }

  template <class KeyType, class KeyNodePtrCompare>
  static node_ptr lower_bound( node_ptr x, node_ptr y, const KeyType& key, KeyNodePtrCompare comp)
  {
    while (x) {
      bool l = comp(x, key);
      y = l ? y : x;
      x = node_traits::get_side(x, l);
    }
    return y;
  }

  template <class KeyType, class KeyNodePtrCompare>
  static node_ptr upper_bound( node_ptr x, node_ptr y, const KeyType& key, KeyNodePtrCompare comp)
  {
    while (x) {
      bool leq = !comp(key, x);
      y = leq ? y : x;
      x = node_traits::get_side(x, leq);
    }
    return y;
  }

  template <class KeyType, class KeyNodePtrCompare>
  static node_ptr find( const const_node_ptr& header, const KeyType& key, KeyNodePtrCompare comp)
  {
    node_ptr end = node_traits::uncast(header);
    node_ptr y = lower_bound(node_traits::get_parent(header), end, key, comp);
    return (y == end || comp(key, y)) ? end : y;
  }

  static inline node_ptr
  relink_parent(const const_node_ptr& header, node_ptr n, typename node_traits::side dir)
  {
    node_ptr u = node_traits::get_parent(n);
    node_traits::set_link(u, u != header ? dir : node_traits::parent, n);
    return u;
  }

  static inline node_ptr
  relink_parent(const const_node_ptr& header, node_ptr n, const const_node_ptr& prev)
  {
    node_ptr u = node_traits::get_parent(n);
    if (u != header)
      node_traits::set_side(u, prev != node_traits::get_left(u), n);
    else
      node_traits::set_parent(u, n);
    return u;
  }

  template <typename node_traits::side dir>
  struct direction_traits
  {
    static const int balance_change = 
     (dir == node_traits::right) ? node_traits::pos_t : node_traits::neg_t;
    static const typename node_traits::side opposite_dir =
     (dir == node_traits::right) ? node_traits::left : node_traits::right;

    static const typename node_traits::balance off_by_one =
     (dir == node_traits::right) ? node_traits::pos_t : node_traits::neg_t;
    static const typename node_traits::balance unbalanced =
     (dir == node_traits::right) ? node_traits::unbalanced_pos_t : node_traits::unbalanced_neg_t;
  };

  template <typename node_traits::side dir>
  static void do_push(node_ptr header, node_ptr n)
  {
    node_ptr p = node_traits::get_side(header, dir);
    node_traits::set_side(header, dir, n);
    algorithms::init_node(n, p, dir == node_traits::right ? header : p);
    node_ptr u = node_traits::get_parent(p);
    if (u)
    {
      if (dir == node_traits::right)
        node_traits::set_next(p, n);
      node_traits::set_side(p, dir, n);

      typename node_traits::balance b = node_traits::add_balance(p, direction_traits<dir>::balance_change);
      if (b != node_traits::zero_t)
      {
        for (; u != header; u = node_traits::get_parent(u))
        {
          b = node_traits::add_balance(u, direction_traits<dir>::balance_change);
          if (b == direction_traits<dir>::off_by_one)
            continue;
          if (b == direction_traits<dir>::unbalanced)
          {
            p = rotate_single(u, direction_traits<dir>::opposite_dir);
            node_traits::set_balance(p, node_traits::zero_t);
            node_traits::set_balance(u, node_traits::zero_t);
            relink_parent(header, p, dir);
          }
          break;
        }
      }
    } 
    else
    {
      // parent pointer not set, empty tree
      node_traits::set_parent(header, n);
      node_traits::set_side(header, direction_traits<dir>::opposite_dir, n);
    }
  }

  template <class iterator_type>
  static std::pair<iterator_type, bool> as_iterator_pair(std::pair<node_ptr, bool> p)
  { return std::pair<iterator_type, bool>(iterator_type(p.first), p.second); }

  static inline std::pair<node_ptr, bool> init_commit_data(node_ptr prev, const const_node_ptr& hint, insert_commit_data& data)
  {
    data.direction = (prev && node_traits::get_left(hint)) ? node_traits::right : node_traits::left;
    data.anchor = data.direction == node_traits::right ? prev : node_traits::uncast(hint);
    data.prev = prev;
    return std::pair<node_ptr, bool>((node_ptr)0, true);
  }

  template <class KeyType, class KeyNodePtrCompare>
  static std::pair<node_ptr, bool> insert_unique_check(const const_node_ptr& header, const const_node_ptr& hint, 
                                          const KeyType& key, KeyNodePtrCompare comp, insert_commit_data& data)
  {
    node_ptr anchor = node_traits::uncast(header), prev = NULL;
    // check if the insertion point comes immediatelly before the hint
    if (hint && (hint == header || comp(key, hint))
             && (hint == algorithms::begin_node(header) || comp(prev = algorithms::prev_node(hint), key)))
      return init_commit_data(prev, hint, data);

    typename node_traits::side side = node_traits::parent;
    for (node_ptr x = node_traits::get_parent(anchor); x; x = node_traits::get_link(x, side))
      side = comp(key, anchor = x) ? node_traits::left : (prev = x, node_traits::right);

    bool not_found = !prev || comp(prev, key);
    if (not_found)
    {
      data.direction = side;
      data.anchor = anchor;
      data.prev = prev;
    }
    return std::pair<node_ptr, bool>(prev, not_found);
  }

  template <typename NodePtrCompare>
  static void insert_equal_check(const const_node_ptr& header, const const_node_ptr& hint,
                                       node_ptr n, NodePtrCompare comp, insert_commit_data& data)
  {
    typename node_traits::side side = node_traits::parent;
    node_ptr anchor = node_traits::uncast(header), prev = NULL;
    if (hint && (hint == header || !comp(hint, n)))
    {
      if (hint == algorithms::begin_node(header) || !comp(n, (prev = algorithms::prev_node(hint))))
      {
        init_commit_data(prev, hint, data);
        return;
      }
      else
        for (node_ptr x = node_traits::get_parent(anchor); x; x = node_traits::get_link(x, side))
          side = comp(n, anchor = x) ? node_traits::left : (prev = x, node_traits::right);
    }
    else
      for (node_ptr x = node_traits::get_parent(anchor); x; x = node_traits::get_link(x, side))
        side = !comp(anchor = x, n) ? node_traits::left : (prev = x, node_traits::right);

    data.direction = side;
    data.anchor = anchor;
    data.prev = prev;
  }

  static inline void insert_commit(node_ptr header, node_ptr n, node_ptr anchor, node_ptr prev, typename node_traits::side side)
  {
    if (anchor != header)
    {
      if (prev)
      {
        algorithms::init_node(n, anchor, node_traits::get_next(prev));
        node_traits::set_next(prev, n);
      }
      else
        algorithms::init_node(n, anchor,  node_traits::get_left(header));

      node_traits::set_link(anchor, side, n);
      if (node_traits::get_link(header, side) == anchor)
        node_traits::set_link(header, side, n);

      typename node_traits::balance b = node_traits::add_balance(anchor, side == node_traits::right ? node_traits::pos_t : node_traits::neg_t);
      if (b != node_traits::zero_t)
      {
        prev = anchor;
        for (n = node_traits::get_parent(prev); n != header; n = node_traits::get_parent(prev = n))
        {
          typename node_traits::balance disbalance = prev != node_traits::get_left(n) ? node_traits::pos_t : node_traits::neg_t;
          b = node_traits::add_balance(n, disbalance);
          if (b == disbalance)
            continue;
          if (b ==(disbalance + disbalance)) // unbalanced
          {
            side = disbalance == node_traits::pos_t ? node_traits::left : node_traits::right;
            if (node_traits::get_balance(prev) == disbalance)
            {
              node_traits::set_balance(n, node_traits::zero_t);
              node_traits::set_balance(prev, node_traits::zero_t);
              prev = rotate_single(n, side);
            }
            else
            {
              typename node_traits::balance negbalance = node_traits::invert_balance(disbalance);
              node_ptr pprev = node_traits::get_link(prev, side);
              b = node_traits::get_balance(pprev);
              node_traits::set_balance(n, b == disbalance ? negbalance : node_traits::zero_t);
              node_traits::set_balance(pprev, node_traits::zero_t);
              node_traits::set_balance(prev, b == negbalance ? disbalance : node_traits::zero_t);
              prev = rotate_double(n, side);
            }
            relink_parent(header, prev, n);
          }
          break;
        }
      }
    }
    else
    {
      algorithms::init_node(n, anchor, header);
      node_traits::set_parent(header, n);
      node_traits::set_right(header, n);
      node_traits::set_left(header, n);
    }
  }

  template <class NodePtrCompare>
  static node_ptr insert_equal_upper_bound(const const_node_ptr& header, node_ptr n, NodePtrCompare comp)
  {
    typename node_traits::side side = node_traits::parent;
    node_ptr anchor = node_traits::uncast(header), prev = NULL;
    for (node_ptr x = node_traits::get_parent(anchor); x; x = node_traits::get_link(x, side))
      side = comp(n, anchor = x) ? node_traits::left : (prev = x, node_traits::right);
    insert_commit(node_traits::uncast(header), n, anchor, prev, side);
    return n;
  }

  static node_ptr do_erase(const const_node_ptr& header, const const_node_ptr& n)
  {
    node_ptr x, y, parent = node_traits::get_parent(n);
    typename node_traits::side dir, side = node_traits::get_left(parent) == n ? node_traits::left : node_traits::right;
    node_ptr l = node_traits::get_left(n);
    node_ptr r = node_traits::get_right(n);

    if (!l || !r)
    {
      x = l ? l : r;
      if (parent != header)
      {
        if (node_traits::get_link(header, side) == n)
        {
          y = node_traits::get_next(n);
          if (side == node_traits::left) // n is leftmost
            node_traits::set_left(node_traits::uncast(header), y);
          else // n is rightmost
          {
            r = x ? algorithms::maximum(x) : parent;
            node_traits::set_next(r, y);
            node_traits::set_right(node_traits::uncast(header), r);
          }
        }
        else
          node_traits::set_next(algorithms::prev_node_inner(n), node_traits::get_next(n));

        node_traits::set_link(parent, side, x);
        if (x) node_traits::set_parent(x, parent);
        dir = side;
      }
      else // empty or only x left
      {
        node_traits::set_parent(parent, x);
        if (x)
        {
          node_traits::set_parent(x, parent);
          node_traits::set_next(x, parent);
          node_traits::set_side(parent, x == r ? node_traits::left : node_traits::right, x);
        }
        else
        {
          node_traits::set_left(parent, parent);
          node_traits::set_right(parent, parent);
        }
        return node_traits::uncast(n);
      }
    }
    else // two sides
    {
      node_ptr n_parent = parent;
      y = node_traits::get_next(n); // y has an empty left side
      x = node_traits::get_right(y);

      node_traits::set_next(algorithms::maximum(l), y);
      node_traits::set_parent(l, y);
      node_traits::set_left(y, l);
      if (y != r)
      {
        dir = node_traits::left;
        node_traits::set_right(y, r);
        node_traits::set_parent(r, y);
        parent = node_traits::get_parent(y);
        if (x) node_traits::set_parent(x, parent);
        node_traits::set_left(parent, x);
      }
      else
      {
        dir = node_traits::right;
        parent = y;
      }

      // link y in place of n
      node_traits::set_parent(y, n_parent);
      node_traits::set_link(n_parent, header != n_parent ? side : node_traits::parent, y);
      node_traits::set_balance(y, node_traits::get_balance(n));
    }

    // root == parent, dir = side which held erased node
    typename node_traits::balance disbalance, negbalance, b;
    while(parent != header) 
    {
      disbalance = dir == node_traits::right ? node_traits::neg_t : node_traits::pos_t;
      b = node_traits::add_balance(parent, disbalance);
      if (b == disbalance)
        break;
      if (b ==(disbalance + disbalance))
      {
        y = node_traits::get_side(parent, dir == node_traits::left ? node_traits::right : node_traits::left);
        disbalance = dir == node_traits::left ? node_traits::neg_t : node_traits::pos_t;
        if (node_traits::get_balance(y) == -disbalance)
        {
          node_traits::set_balance(parent, node_traits::zero_t);
          node_traits::set_balance(y, node_traits::zero_t);
          x = rotate_single(parent, dir);
        }
        else if (node_traits::get_balance(y) == disbalance)
        {
          negbalance = node_traits::invert_balance(disbalance);
          x = node_traits::get_link(y, dir);
          b = node_traits::get_balance(x);
          node_traits::set_balance(parent, b == negbalance ? disbalance : node_traits::zero_t);
          node_traits::set_balance(y, b == disbalance ? negbalance : node_traits::zero_t);
          node_traits::set_balance(x, node_traits::zero_t);
          x = rotate_double(parent, dir);
        }
        else
        {
          node_traits::set_balance(parent, node_traits::invert_balance(disbalance));         
          node_traits::set_balance(y, disbalance);
          x = rotate_single(parent, dir);
          relink_parent(header, x, parent);
          break;
        }
        parent = relink_parent(header, x, parent);
      }
      else
        parent = node_traits::get_parent(x = parent);
      dir = node_traits::get_left(parent) == x ? node_traits::left : node_traits::right;
    }
    return node_traits::uncast(n);
  }

  public:

  template <class T, class NodePtr>
  class tree_iterator
  {
    protected:

    NodePtr m_node;

    public:
	typedef std::bidirectional_iterator_tag	iterator_category;
	typedef std::ptrdiff_t			difference_type;
	typedef T				value_type;
	typedef T&				reference;
	typedef T*				pointer;

    tree_iterator()
      : m_node(NULL) {}

    explicit tree_iterator(const NodePtr& n)
      : m_node(n) {}

    tree_iterator(const tree_iterator<Node, node_ptr>& other)
      : m_node(other.pointed_node()) {}

    NodePtr pointed_node() const
    { return m_node; }

    tree_iterator& operator=(const node_ptr& node_ptr)
    { m_node = node_ptr; return *this; }

    tree_iterator& operator++()
    { m_node = algorithms::next_node(m_node); return *this; }

    tree_iterator operator++(int)
    { tree_iterator copy(*this); m_node = algorithms::next_node(m_node); return copy; }

    tree_iterator& operator--()
    { m_node = algorithms::prev_node(m_node); return *this; }

    tree_iterator operator--(int)
    { tree_iterator copy(*this); m_node = algorithms::prev_node(m_node); return copy; }

    friend bool operator==(const tree_iterator& l, const tree_iterator& r)
    { return l.pointed_node() == r.pointed_node(); }

    friend bool operator!=(const tree_iterator& l, const tree_iterator& r)
    { return l.pointed_node() != r.pointed_node(); }

    reference operator*() const
    { return *operator->(); }

    pointer operator->() const
    { return static_cast<pointer>(m_node); }

  };

  typedef value_type&					reference;
  typedef const value_type&				const_reference;
  typedef tree_iterator<Node, node_ptr> 		iterator;
  typedef tree_iterator<const Node, const_node_ptr>	const_iterator;

  avlTree( const value_compare& comp = value_compare() )
    : m_comp(comp)
  { algorithms::init_header(this->header_ptr()); }

  key_compare key_comp() const
  { return m_comp; }

  value_compare value_comp() const
  { return m_comp; }

  bool empty() const
  { return node_traits::get_parent(this->header_ptr()) == 0; }

  size_type size() const
  {
    size_type n = 0;
    for (node_ptr p = algorithms::begin_node(this->header_ptr());
         p != algorithms::end_node(this->header_ptr()); p = algorithms::next_node(p)) n++;
    return n;
  }

  iterator begin()
  { return iterator(algorithms::begin_node(this->header_ptr())); }

  const_iterator begin() const
  { return const_iterator(algorithms::begin_node(this->header_ptr())); }

  const_iterator cbegin() const
  { return const_iterator(algorithms::begin_node(this->header_ptr())); }

  iterator end()
  { return iterator(algorithms::end_node(this->header_ptr())); }

  const_iterator end() const
  { return const_iterator(algorithms::end_node(this->header_ptr())); }

  const_iterator cend() const
  { return const_iterator(algorithms::end_node(this->header_ptr())); }

  iterator find(const_reference value)
  { return this->find(value, this->m_comp); }

  const_iterator find(const_reference value) const
  { return this->find(value, this->m_comp); }

  template <class KeyType, class KeyValueCompare>
  iterator find(const KeyType& key, KeyValueCompare comp)
  { return iterator(this->find(this->header_ptr(), key, KeyNodeCompare<KeyValueCompare>(comp))); }

  template <class KeyType, class KeyValueCompare>
  const_iterator find(const KeyType& key, KeyValueCompare comp) const
  { return const_iterator(this->find(this->header_ptr(), key, KeyNodeCompare<KeyValueCompare>(comp))); }

  iterator lower_bound(const_reference value)
  { return this->lower_bound(value, this->m_comp); }

  const_iterator lower_bound(const_reference value) const
  { return this->lower_bound(value, this->m_comp); }

  template <class KeyType, class KeyValueCompare>
  iterator lower_bound(const KeyType& key, KeyValueCompare comp) 
  {
     return iterator(lower_bound(node_traits::get_parent(this->header_ptr()),
				 node_traits::uncast(this->header_ptr()), key, KeyNodeCompare<KeyValueCompare>(comp)));
  }

  template <class KeyType, class KeyValueCompare>
  const_iterator lower_bound(const KeyType& key, KeyValueCompare comp) const
  {
     return const_iterator(lower_bound(node_traits::get_parent(this->header_ptr()),
				 node_traits::uncast(this->header_ptr()), key, KeyNodeCompare<KeyValueCompare>(comp)));
  }

  iterator upper_bound(const_reference value)
  { return this->upper_bound(value, this->m_comp); }

  const_iterator upper_bound(const_reference value) const
  { return this->upper_bound(value, this->m_comp); }

  template <class KeyType, class KeyValueCompare>
  iterator upper_bound(const KeyType& key, KeyValueCompare comp) 
  {
     return iterator(upper_bound(node_traits::get_parent(this->header_ptr()),
				 node_traits::uncast(this->header_ptr()), key, KeyNodeCompare<KeyValueCompare>(comp)));
  }

  template <class KeyType, class KeyValueCompare>
  const_iterator upper_bound(const KeyType& key, KeyValueCompare comp) const
  {
     return const_iterator(upper_bound(node_traits::get_parent(this->header_ptr()),
				 node_traits::uncast(this->header_ptr()), key, KeyNodeCompare<KeyValueCompare>(comp)));
  }

  void push_front(reference value)
  { do_push<node_traits::left>(this->header_ptr(), &value); }

  void push_back(reference value)
  { do_push<node_traits::right>(this->header_ptr(), &value); }

  template <class KeyType, class KeyValueCompare>
  std::pair<iterator, bool>
  insert_unique_check(const KeyType& key, KeyValueCompare comp, insert_commit_data& data)
  {
    return as_iterator_pair<iterator>(insert_unique_check(this->header_ptr(),
                               NULL, key, KeyNodeCompare<KeyValueCompare>(comp), data));
  }

  template <class KeyType, class KeyValueCompare>
  std::pair<iterator, bool>
  insert_unique_check(const_iterator hint, const KeyType& key, KeyValueCompare comp, insert_commit_data& data)
  {
    return as_iterator_pair<iterator>(insert_unique_check(this->header_ptr(),
                               hint.pointed_node(), key, KeyNodeCompare<KeyValueCompare>(comp), data));
  }

  iterator insert_unique_commit(reference value, const insert_commit_data& data)
  { 
    insert_commit(this->header_ptr(), &value, data.anchor, data.prev, data.direction);
    return iterator(&value);
  }

  std::pair<iterator, bool> insert_unique(reference value)
  {
    insert_commit_data data;
    std::pair<iterator, bool> ret = this->insert_unique_check(value, this->m_comp, data);
    return ret.second ? std::pair<iterator, bool>(this->insert_unique_commit(value, data), true) : ret;
  }

  std::pair<iterator, bool> insert_unique(const_iterator hint, reference value)
  {
    insert_commit_data data;
    std::pair<iterator, bool> ret = this->insert_unique_check(value, hint, this->m_comp, data);
    return ret.second ? std::pair<iterator, bool>(this->insert_unique_commit(value, data), true) : ret;
  }

  iterator insert_equal(reference value)
  { return iterator(this->insert_equal_upper_bound(this->header_ptr(), &value, KeyNodeCompare<Compare>(this->m_comp))); }

  iterator insert_equal(const_iterator hint, reference value)
  {
    insert_commit_data data;
    insert_equal_check(this->header_ptr(), hint.pointed_node(), &value, KeyNodeCompare<Compare>(this->m_comp), data);
    insert_commit(this->header_ptr(), &value, data.anchor, data.prev, data.direction);
    return iterator(&value);
  }

  iterator erase(const_iterator i)
  {
    iterator ret(node_traits::get_next(i.pointed_node()));
    do_erase(this->header_ptr(), i.pointed_node());
    return ret;
  }

  template <class Disposer>
  iterator erase_and_dispose(const_iterator i, Disposer disposer)
  {
    iterator ret(node_traits::get_next(i.pointed_node()));
    disposer(static_cast<value_type*>(do_erase(this->header_ptr(), i.pointed_node())));
    return ret;
  }

  size_type erase(const_reference value)
  { return this->erase(value, this->m_comp); }

  template <class KeyType, class KeyValueCompare>
  size_type erase(const KeyType& key, KeyValueCompare comp)
  {
    const_iterator i = this->find(key, comp);
    if (i != this->end())
    {
      size_type erased = 0;
      for (const_iterator e = this->upper_bound(key, comp); i != e; i = this->erase(i)) erased++;
      return erased;
    } 
    return 0;
  }

  template <class KeyType, class KeyValueCompare, class Disposer>
  size_type erase_and_dispose(const KeyType& key, KeyValueCompare comp, Disposer disposer)
  {
    const_iterator i = this->find(key, comp);
    if (i != this->end())
    {
      size_type erased = 0;
      for (const_iterator e = this->upper_bound(key, comp); i != e; i = this->erase_and_dispose(i, disposer)) erased++;
      return erased;
    } 
    return 0;
  }

  void clear()
  { algorithms::init_header(this->header_ptr()); }

  template <class Disposer>
  void clear_and_dispose(Disposer disposer)
  {
    node_ptr n = node_traits::get_left(this->header_ptr());
    while (n != this->header_ptr())
    {
      node_ptr d = n;
      n = node_traits::get_next(d);
      disposer(static_cast<value_type*>(d));
    }
    algorithms::init_header(this->header_ptr());
  }

  template <class Cloner, class Disposer>
  void clone_from(const tree_type& src, Cloner cloner, Disposer disposer)
  {
    this->clear_and_dispose(disposer);
    this->m_comp = src.value_comp();
    std::copy(src.begin(), src.end(), CloneInserter<Cloner>(*this, cloner).inserter());
  }
};

// Flat set implementation of an ordered container with the same "intrusive" semantics as above except the lack of iterator stability
template <typename T,
          typename Compare = std::less<T>,
          typename Allocator = std::allocator<T*>,
	  std::size_t DefaultCapacity = 16>
class flatSet {
  typedef T*				node_ptr;
  typedef const T*			const_node_ptr;
  typedef flatSet			set_type;

  typedef typename Allocator::template rebind<node_ptr>::other store_allocator_type;
  class store_type {

    node_ptr*   m_start;
    node_ptr*   m_allocated;
    std::size_t m_size, m_capacity;

    node_ptr* do_erase(const node_ptr* p, std::size_t n)
    {
      std::size_t extent = m_size-- - (p + n - m_start);
      if (extent)
        ::memmove(const_cast<node_ptr*>(p), p + n, extent * sizeof(node_ptr));
      return const_cast<node_ptr*>(p);
    }

    void do_allocated(node_ptr* p, std::size_t capacity)
    {
      if (m_allocated) store_allocator_type().deallocate(m_allocated, m_capacity);
      m_allocated = m_start = p;
      m_capacity = capacity;
    }

    public:

    typedef std::ptrdiff_t difference_type;
    typedef node_ptr* iterator;
    typedef const node_ptr* const_iterator;

    store_type(node_ptr* buffer, std::size_t capacity)
    : m_start(buffer), m_allocated(NULL), m_size(0), m_capacity(capacity)
    {
      if (m_start != NULL) return;
      m_allocated = m_start = store_allocator_type().allocate(capacity);
    }

    store_type(std::size_t capacity)
    : m_allocated(store_allocator_type().allocate(capacity)), m_size(0), m_capacity(capacity)
    { m_start = m_allocated; }

    store_type()
    : m_allocated(store_allocator_type().allocate(DefaultCapacity)), m_size(0), m_capacity(DefaultCapacity)
    { m_start = m_allocated; }

    ~store_type()
    { if (m_allocated) store_allocator_type().deallocate(m_start, m_capacity); }

    bool empty() const
    { return m_size == 0; }

    std::size_t size() const
    { return m_size; }

    void clear()
    { m_size = 0; }

    void reserve(std::size_t sz)
    {
      if (sz > m_capacity)
      {
        node_ptr* d = store_allocator_type().allocate(sz);
        ::memcpy(d, m_start, m_size * sizeof(node_ptr));
        do_allocated(d, sz);
      }
    }

    void push_back(node_ptr p)
    {
      if (m_size < m_capacity)
      {
        m_start[m_size++] = p;
        return;
      }
      std::size_t sz = m_capacity << 1;
      node_ptr* d = store_allocator_type().allocate(sz);
      ::memcpy(d, m_start, m_size * sizeof(node_ptr));
      d[m_size++] = p;
      do_allocated(d, sz);
    }

    iterator insert(iterator it, node_ptr p)
    {
      if (it == m_start + m_size)
      {
        push_back(p);
        return m_start + m_size - 1;
      }
      if (m_size < m_capacity)
        Util::Memory::shift_up_from(it, m_size - (it - m_start));
        //::memmove(it + 1, it, (m_size - (it - m_start)) * sizeof(node_ptr));
      else
      {
        std::size_t sz = m_capacity << 1;
        std::size_t off = it - m_start;
        node_ptr* d = store_allocator_type().allocate(sz);
        ::memcpy(d, m_start, off * sizeof(node_ptr));
        it = d + off;
        ::memcpy(it + 1, m_start + off, (m_size - off) * sizeof(node_ptr));
        do_allocated(d, sz);
      }
      *it = p;
      m_size++;
      return it;
    }

    iterator erase(const_iterator i)
    { return do_erase(i, 1); }

    iterator erase(const_iterator b, const_iterator e)
    { return do_erase(b, e - b); }

    iterator begin()
    { return m_start; }

    const_iterator begin() const
    { return m_start; }

    const_iterator cbegin() const
    { return m_start; }

    iterator end()
    { return m_start + m_size; }

    const_iterator end() const
    { return m_start + m_size; }

    const_iterator cend() const
    { return m_start + m_size; }
  };

  // typedef std::vector<node_ptr, store_allocator_type> store_type;
  store_type m_data;

  public:

  typedef std::size_t				size_type;
  typedef T					value_type;
  typedef Compare				value_compare;
  typedef Compare				key_compare;
  typedef value_type&                           reference;
  typedef const value_type&                     const_reference;

  struct insert_commit_data { typename store_type::iterator next; };

  private:

  value_compare m_comp;

  template <class KeyValueCompare>
  struct KeyPtrCompare {
    KeyValueCompare m_comp;
    public:
    KeyPtrCompare(KeyValueCompare comp) : m_comp(comp) {}

    template <class V>
    const value_type& extract_key(const V& node, node_ptr) const 
    { return *node; }

    template <class V>
    const value_type& extract_key(const V& node, const_node_ptr) const 
    { return *node; }

    template <class V, class U>
    const V& extract_key(const V& key, const U&) const
    { return key; }

    template <class KeyA, class KeyB>
    bool operator()(const KeyA& a, const KeyB& b) const
    { return this->m_comp(this->extract_key(a, a), this->extract_key(b, b)); }
  };

  template <class Cloner>
  struct CloneInserter {
    set_type& m_set;
    Cloner m_cloner;
    public:
    typedef typename set_type::value_type&              reference;
    typedef const typename set_type::value_type&        const_reference;
    typedef typename set_type::value_type               value_type;

    CloneInserter(set_type& t, Cloner c) : m_set(t), m_cloner(c) {}

    void push_back(const value_type& v)
    { m_set.push_back(*m_cloner(v)); }

    std::back_insert_iterator<CloneInserter> inserter()
    { return std::back_inserter<CloneInserter>(*this); }
  };

  template <class KeyType, class KeyCompare>
  static typename store_type::iterator find( store_type& data, const KeyType& key, KeyCompare comp)
  {
    typename store_type::iterator e = data.end(), it = std::lower_bound( data.begin(), e, key, comp );
    return (it != e && !comp(key, *it)) ? it : e;
  }

  template <class KeyType, class KeyCompare>
  static typename store_type::const_iterator find( const store_type& data, const KeyType& key, KeyCompare comp)
  {
    const typename store_type::const_iterator e = data.end(), it = std::lower_bound( data.begin(), e, key, comp );
    return (it != e && !comp(key, *it)) ? it : e;
  }

  template <class iterator_type>
  static std::pair<iterator_type, bool> as_iterator_pair(std::pair<typename store_type::iterator, bool> p)
  { return std::pair<iterator_type, bool>(iterator_type(p.first), p.second); }

  static inline std::pair<typename store_type::iterator, bool>
  init_commit_data(store_type& data, typename store_type::difference_type distance, insert_commit_data& cdata)
  {
    cdata.next = data.begin() + distance;
    return std::pair<typename store_type::iterator, bool>(data.end(), true);
  }

  template <class KeyType, class KeyNodePtrCompare>
  static std::pair<typename store_type::iterator, bool>
  insert_unique_check(store_type& data, const KeyType& key, KeyNodePtrCompare comp, insert_commit_data& cdata)
  {
    typename store_type::iterator prev = data.begin();
    typename store_type::iterator next = std::upper_bound(prev, data.end(), key, comp);
    bool not_found = next == prev || comp(*(prev = (next - 1)), key);
    if (not_found) cdata.next = next;
    return std::pair<typename store_type::iterator, bool>(prev, not_found);
  }

  template <class KeyType, class KeyNodePtrCompare>
  static std::pair<typename store_type::iterator, bool>
  insert_unique_check(store_type& data, typename store_type::const_iterator hint, 
                      const KeyType& key, KeyNodePtrCompare comp, insert_commit_data& cdata)
  {
    // check if the insertion point comes immediatelly before the hint
    if ( (hint == data.end() || comp(key, *hint)) && (hint == data.begin() || comp(*(hint - 1), key)))
      return init_commit_data(data, hint - data.cbegin(), cdata);
    return insert_unique_check(data, key, comp, cdata);
  }

  template <class NodePtrCompare>
  static void insert_equal_check(store_type& data, typename store_type::const_iterator hint,
                                 reference value, NodePtrCompare comp, insert_commit_data& cdata)
  {
    if ((hint == data.end() || !comp(*hint, value)) && (hint == data.begin() || !comp(value, *(hint - 1))))
      init_commit_data(data, hint - data.begin(), cdata);
    else
      cdata.next = std::upper_bound(data.begin(), data.end(), value, comp);
  }

  public:

  template <class V, class iterator_type>
  class set_iterator
  {
    protected:

    iterator_type m_it;

    public:
	typedef std::bidirectional_iterator_tag iterator_category;
	typedef std::ptrdiff_t                  difference_type;
	typedef V				value_type;
	typedef V&				reference;
	typedef V*				pointer;

    set_iterator() {}

    explicit set_iterator(iterator_type it)
      : m_it(it) {}

    set_iterator(const set_iterator<T, typename store_type::iterator>& other)
      : m_it(static_cast<typename store_type::iterator>(other)) {}

    operator iterator_type() const
    { return m_it; }

    set_iterator& operator=(const iterator_type& it)
    { m_it = it; return *this; }

    set_iterator& operator++()
    { ++m_it; return *this; }

    set_iterator operator++(int)
    { set_iterator copy(*this); ++m_it; return copy; }

    set_iterator& operator--()
    { --m_it; return *this; }

    set_iterator operator--(int)
    { set_iterator copy(*this); --m_it; return copy; }

    friend bool operator==(const set_iterator& l, const set_iterator& r)
    { return l.m_it == r.m_it; }

    friend bool operator!=(const set_iterator& l, const set_iterator& r)
    { return l.m_it != r.m_it; }

    reference operator*() const
    { return *operator->(); }

    pointer operator->() const
    { return *m_it; }

  };

  typedef set_iterator<T, typename store_type::iterator>             iterator;
  typedef set_iterator<const T, typename store_type::const_iterator> const_iterator;

  flatSet( const value_compare& comp = value_compare() )
    : m_comp(comp) 
  {}

  flatSet(std::size_t capacity, const value_compare& comp )
    : m_data(capacity), m_comp(comp)
  {}

  flatSet(node_ptr* buffer, std::size_t capacity, const value_compare& comp )
    : m_data(buffer, capacity), m_comp(comp)
  {}

  key_compare key_comp() const
  { return m_comp; }

  value_compare value_comp() const
  { return m_comp; }

  bool empty() const
  { return m_data.empty(); }

  size_type size() const
  { return m_data.size(); }

  iterator begin()
  { return iterator(m_data.begin()); }

  const_iterator begin() const
  { return const_iterator(m_data.begin()); }

  const_iterator cbegin() const
  { return const_iterator(m_data.cbegin()); }

  iterator end()
  { return iterator(m_data.end()); }

  const_iterator end() const
  { return const_iterator(m_data.end()); }

  const_iterator cend() const
  { return const_iterator(m_data.cend()); }

  iterator find(const_reference value)
  { return iterator(this->find( m_data, value, KeyPtrCompare<Compare>(m_comp))); }

  const_iterator find(const_reference value) const
  { return const_iterator(this->find( m_data, value, KeyPtrCompare<Compare>(m_comp))); }

  template <class KeyType, class KeyValueCompare>
  iterator find(const KeyType& key, KeyValueCompare comp)
  { return iterator(this->find(this->m_data, key, KeyPtrCompare<KeyValueCompare>(comp))); }

  template <class KeyType, class KeyValueCompare>
  const_iterator find(const KeyType& key, KeyValueCompare comp) const
  { return const_iterator(this->find(this->m_data, key, KeyPtrCompare<KeyValueCompare>(comp))); }

  iterator lower_bound(const_reference value)
  { return iterator(std::lower_bound(this->m_data.begin(), this->m_data.end(), value, KeyPtrCompare<Compare>(this->m_comp))); }

  const_iterator lower_bound(const_reference value) const
  { return const_iterator(std::lower_bound(this->m_data.begin(), this->m_data.end(), value, KeyPtrCompare<Compare>(this->m_comp))); }

  template <class KeyType, class KeyValueCompare>
  iterator lower_bound(const KeyType& key, KeyValueCompare comp) 
  { return iterator(std::lower_bound(this->m_data.begin(), this->m_data.end(), key, KeyPtrCompare<KeyValueCompare>(comp))); }

  template <class KeyType, class KeyValueCompare>
  const_iterator lower_bound(const KeyType& key, KeyValueCompare comp) const
  { return const_iterator(std::lower_bound(this->m_data.begin(), this->m_data.end(), key, KeyPtrCompare<KeyValueCompare>(comp))); }

  iterator upper_bound(const_reference value)
  { return iterator(std::upper_bound(this->m_data.begin(), this->m_data.end(), value, KeyPtrCompare<Compare>(this->m_comp))); }

  const_iterator upper_bound(const_reference value) const
  { return const_iterator(std::upper_bound(this->m_data.begin(), this->m_data.end(), value, KeyPtrCompare<Compare>(this->m_comp))); }

  template <class KeyType, class KeyValueCompare>
  iterator upper_bound(const KeyType& key, KeyValueCompare comp) 
  { return iterator(std::upper_bound(this->m_data.begin(), this->m_data.end(), key, KeyPtrCompare<KeyValueCompare>(comp))); }

  template <class KeyType, class KeyValueCompare>
  const_iterator upper_bound(const KeyType& key, KeyValueCompare comp) const
  { return const_iterator(std::upper_bound(this->m_data.begin(), this->m_data.end(), key, KeyPtrCompare<KeyValueCompare>(comp))); }

  void push_front(reference value)
  { this->m_data.insert(this->m_data.begin(), &value); }

  void push_back(reference value)
  { this->m_data.push_back(&value); }

  template <class KeyType, class KeyValueCompare>
  std::pair<iterator, bool>
  insert_unique_check(const KeyType& key, KeyValueCompare comp, insert_commit_data& data)
  { return as_iterator_pair<iterator>(insert_unique_check(this->m_data, key, KeyPtrCompare<KeyValueCompare>(comp), data)); }

  template <class KeyType, class KeyValueCompare>
  std::pair<iterator, bool>
  insert_unique_check(const_iterator hint, const KeyType& key, KeyValueCompare comp, insert_commit_data& data)
  {
    return as_iterator_pair<iterator>(insert_unique_check(this->m_data, static_cast<typename store_type::const_iterator>(hint),
                                                                        key, KeyPtrCompare<KeyValueCompare>(comp), data));
  }

  iterator insert_unique_commit(reference value, const insert_commit_data& cdata)
  { return iterator(this->m_data.insert(cdata.next, &value)); }

  std::pair<iterator, bool> insert_unique(reference value)
  {
    insert_commit_data data;
    std::pair<iterator, bool> ret = this->insert_unique_check(value, this->m_comp, data);
    return ret.second ? std::pair<iterator, bool>(this->insert_unique_commit(value, data), true) : ret;
  }

  std::pair<iterator, bool> insert_unique(const_iterator hint, reference value)
  {
    insert_commit_data data;
    std::pair<iterator, bool> ret = this->insert_unique_check(value, hint, this->m_comp, data);
    return ret.second ? std::pair<iterator, bool>(this->insert_unique_commit(value, data), true) : ret;
  }

  iterator insert_equal(reference value)
  { return iterator(this->m_data.insert(std::upper_bound(this->m_data.begin(), this->m_data.end(), value, KeyPtrCompare<Compare>(this->m_comp)), &value)); }


  iterator insert_equal(const_iterator hint, reference value)
  {
    insert_commit_data cdata;
    insert_equal_check(this->m_data, static_cast<typename store_type::const_iterator>(hint), value, KeyPtrCompare<Compare>(this->m_comp), cdata);
    return iterator(this->m_data.insert(cdata.next, &value));
  }

  iterator erase(const_iterator i)
  { return iterator(this->m_data.erase(i)); }

  template <class Disposer>
  iterator erase_and_dispose(const_iterator i, Disposer disposer)
  {
    disposer(const_cast<T*>(*static_cast<typename store_type::const_iterator>(i)));
    return iterator(this->m_data.erase(i));
  }

  size_type erase(const_reference value)
  { return this->erase(value, this->m_comp); }

  template <class KeyType, class KeyValueCompare>
  size_type erase(const KeyType& key, KeyValueCompare comp)
  {
    iterator i = this->find(key, comp);
    if (i != this->end())
    {
      iterator e = this->upper_bound(key, comp);
      size_type erased = e - i;
      this->erase(i, e);
      return erased;
    } 
    return 0;
  }

  template <class KeyType, class KeyValueCompare, class Disposer>
  size_type erase_and_dispose(const KeyType& key, KeyValueCompare comp, Disposer disposer)
  {
    iterator i = this->find(key, comp);
    if (i != this->end())
    {
      iterator e = this->upper_bound(key, comp);
      size_type erased = e - i;
      for (iterator it = i; it != e; ++it) disposer(*it);
      this->erase(i, e);
      return erased;
    } 
    return 0;
  }

  void clear()
  { this->m_data.clear(); }

  template <class Disposer>
  void clear_and_dispose(Disposer disposer)
  {
    std::for_each(this->m_data.begin(), this->m_data.end(), disposer);
    this->clear();
  }

  template <class Cloner, class Disposer>
  void clone_from(const set_type& src, Cloner cloner, Disposer disposer)
  {
    this->clear_and_dispose(disposer);
    this->m_comp = src.value_comp();
    this->m_data.reserve(src.size());
    std::copy(src.begin(), src.end(), CloneInserter<Cloner>(*this, cloner).inserter());
  }
};

} // Container
}

#endif //FIX_CONTAINER_H

