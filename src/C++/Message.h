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

#ifndef FIX_MESSAGE
#define FIX_MESSAGE

#ifdef _MSC_VER
#pragma warning( disable: 4786 )
#endif

#include "FieldMap.h"
#include "FixFields.h"
#include "Group.h"
#include "SessionID.h"
#include "DataDictionary.h"
#include "Values.h"
#include <vector>
#include <memory>

namespace FIX
{
typedef FieldMap Header;
typedef FieldMap Trailer;

static int const headerOrder[] =
  {
    FIELD::BeginString,
    FIELD::BodyLength,
    FIELD::MsgType
  };

/**
 * Base class for all %FIX messages.
 *
 * A message consists of three field maps.  One for the header, the body,
 * and the trailer.
 */
class Message : public FieldMap
{
  friend class DataDictionary;
  friend class Session;

  enum status_type {
	tag_out_of_order,
	invalid_tag_format,
	incorrect_data_format,

	has_sender_comp_id = (sizeof(intptr_t) * 8) - 3,
	has_target_comp_id = (sizeof(intptr_t) * 8) - 2,
	serialized_once    = (sizeof(intptr_t) * 8) - 1
  };
  static const intptr_t status_error_mask =  (1 << tag_out_of_order) |
                                             (1 << invalid_tag_format) |
                                             (1 << incorrect_data_format);

  enum field_type { header, body, trailer };

  enum admin_trait
  {
    admin_none = 0,
    admin_session = 1, // TestRequest, Heartbeat, Reject
    admin_status = 2, // ResendRequest, SequenceReset, Logout
    admin_logon = 4  // Logon
  };

  class HeaderFieldSet : public Util::BitSet<1280>
  {
    static const int   m_fields[];

  public:

    HeaderFieldSet()
    {
      for(const int* p = m_fields; *p; p++)
      {
        if ((unsigned)*p < size()) set(*p);
      }
    }
  };

  static ALIGN_DECL_DEFAULT HeaderFieldSet headerFieldSet;

  class FieldReader : public Sequence {

    static const char* ErrDelimiter;
    static const char* ErrSOH;

    int  m_field, m_pos;
    int  m_length, m_csum;
    int  m_hdr, m_body, m_trl, m_grp;
    const char* m_start;
    const char* const m_end;

    inline void step() {
      m_start += m_pos + 1;
      m_pos = 0;
    }

  public:
    typedef String::value_type result_type;

    FieldReader ( const char* s, const char* e )
    : m_field(0), m_pos(0), m_length(0), m_csum(0),
      m_hdr(0), m_body(0), m_trl(0), m_grp(0),
      m_start(s), m_end(e) {}

    FieldReader ( const char* p, std::size_t n )
    : m_field(0), m_pos(0), m_length(0), m_csum(0),
      m_hdr(0), m_body(0), m_trl(0), m_grp(0),
      m_start(p), m_end(p + n) {}

    FieldReader ( const std::string& s )
    : m_field(0), m_pos(0), m_length(0), m_csum(0),
      m_hdr(0), m_body(0), m_trl(0), m_grp(0),
      m_start(String::c_str(s)),
      m_end(m_start + String::size(s)) {}

    FieldReader ( const std::string& s, std::string::size_type pos )
    : m_field(0), m_pos(0), m_length(0), m_csum(0),
      m_hdr(0), m_body(0), m_trl(0), m_grp(0),
      m_start(String::c_str(s) + pos),
      m_end(String::c_str(s) + String::size(s)) {}

    char getTagLength() const
    { return m_length + 1; }

    short getTagChecksum() const
    { return m_csum + (int)'='; }

    operator bool () const
    { return m_start + m_pos < m_end; }

    const char* scan ();
    void skip ();

    const char* pos () const
    { return m_start + m_pos; }

    const char* end () const
    { return m_end; }

    void pos(int pos)
    { m_pos = pos; }

    void rewind ( const char* p )
    { m_start = p; m_pos = 0; }

    void startGroupAt( int n = 0 ) { m_grp = n; }

    /// Store ordered header fields 
    const FieldBase* flushSpecHeaderField(FieldMap& map)
    {
      const FieldBase& r = Sequence::push_back_to_ordered( map, *this )->second;
      step();
      return &r;
    }

    const FieldBase* flushHeaderField(FieldMap& map)
    {
      const FieldBase& r = ( LIKELY(Sequence::header_compare(map, m_hdr, m_field)) )
                            ? m_hdr = m_field, Sequence::push_back_to_ordered( map, *this )->second
                            : Sequence::insert_into_ordered( map, *this )->second;
      step();
      return &r;
    }

    void HEAVYUSE flushField(FieldMap& map)
    {
      if ( LIKELY(m_body < m_field) )
      {
        Sequence::push_back_to( map, *this );
        m_body = m_field;
      }
      else
        Sequence::insert_into( map, *this );
      step();
    }

    void flushTrailerField(FieldMap& map)
    {
      if ( LIKELY(Sequence::trailer_compare(map, m_trl, m_field)) )
      {
        Sequence::push_back_to_ordered( map, *this );
        m_trl = m_field;
      }
      else
        Sequence::insert_into_ordered( map, *this );
      step();
    }

    int flushGroupField(FieldMap& map)
    {
      if ( LIKELY(Sequence::group_compare(map, m_grp, m_field) || m_grp == 0) )
      {
        Sequence::push_back_to( map, *this );
        m_grp = m_field;
      }
      else
        Sequence::insert_into( map, *this );
      step();
      return m_grp;
    }

    int getField() const { return m_field; }

    void assign_to(String::value_type& s) const
    {
      s.assign(m_start, m_pos);
    }
    operator String::value_type () const
    {
      return String::value_type(m_start, m_pos);
    }
  };

  class FieldCounter
  {
    int m_length, m_prefix;
    const int bodyLengthTag;
    const int checkSumTag;

    int countGroups(FieldMap::g_const_iterator git,
                          const FieldMap::g_const_iterator& gend);
    void countHeader(const FieldMap& fields);
    void countHeader(int beginStringTag, int bodyLengthDag,
                     const FieldMap& fields);
    int countBody(const FieldMap& fields);
    void countTrailer(const FieldMap& fields);

  public:

    FieldCounter(const Message& msg)
    : m_length(0), m_prefix(0),
      bodyLengthTag(FIELD::BodyLength), checkSumTag(FIELD::CheckSum)
    {
      countHeader(msg.m_header);
      m_length += countBody(msg);
      countTrailer(msg.m_trailer);
    }
    FieldCounter(const Message& msg,
      int beginStringField,
      int bodyLengthField = FIELD::BodyLength,
      int checkSumField = FIELD::CheckSum )
    : m_length(0), m_prefix(0),
      bodyLengthTag(bodyLengthField), checkSumTag(checkSumField)
    {
      countHeader(beginStringField, bodyLengthField, msg.m_header);
      m_length += countBody(msg);
      countTrailer(msg.m_trailer);
    }

    int getBodyLengthTag () const
    { return bodyLengthTag; }

    int getCheckSumTag () const
    { return checkSumTag; }

    int getBodyLength() const
    { return m_length; }

    int getBeginStringLength() const
    { return m_prefix; }

  };

  static inline bool isAdminMsg( const char* msg, std::size_t size )
  {
    if ( size > 5 )
    {
      Util::CharBuffer::Fixed<8> b = { { '0', '1', '3', '2', '4', '5', 'A', '\0' } };
      Util::CharBuffer::Fixed<4> const v = { { '\001', '3', '5', '=' } };
      const char* p = Util::CharBuffer::find( v, msg, size );
      return p && p[5] == '\001' && Util::CharBuffer::find( p[4], b ) < 7;
    }
    throw InvalidMessage();
  }

  static inline bool isAdminMsg( const std::string& msg )
  { return isAdminMsg( String::c_str(msg), String::size(msg) ); }

  void setString( const char* s, std::size_t n, bool validate,
                  const FIX::DataDictionary* pSessionDataDictionary,
                  const FIX::DataDictionary* pApplicationDataDictionary )
  throw( InvalidMessage );

  static const std::size_t HeaderFieldCountEstimate = 8;
  static const std::size_t TrailerFieldCountEstimate= 4;
  static inline std::size_t bodyFieldCountEstimate(std::size_t available = ItemAllocatorTraits::DefaultCapacity)
  { 
    return (available > (HeaderFieldCountEstimate + TrailerFieldCountEstimate))
           ? (available - HeaderFieldCountEstimate - TrailerFieldCountEstimate) : HeaderFieldCountEstimate;
  }

  Message( const char* p, std::size_t n,
           const DataDictionary& dataDictionary,
           FieldMap::allocator_type& a,
           bool validate )
  throw( InvalidMessage )
  : FieldMap(a),
    m_header( a, message_order( message_order::header ) ),
    m_trailer( a, message_order( message_order::trailer ) ),
    m_status( 0 )
  {
    setString( p, n, validate, &dataDictionary, &dataDictionary );
  }

  Message( const char* p, std::size_t n,
           const DataDictionary& sessionDataDictionary,
           const DataDictionary& applicationDataDictionary,
           FieldMap::allocator_type& a,
           bool validate )
  throw( InvalidMessage )
  : FieldMap(a),
    m_header( a, message_order( message_order::header ) ),
    m_trailer( a, message_order( message_order::trailer ) ),
    m_status( 0 )
  {
    if( isAdminMsg( p, n ) )
      setString( p, n, validate, &sessionDataDictionary, &sessionDataDictionary );
    else
      setString( p, n, validate, &sessionDataDictionary, &applicationDataDictionary );
  }

  std::string& toString( const FieldCounter&, std::string& ) const;

  template <typename S> typename S::buffer_type& toBuffer( S& s ) const
  {
    FieldCounter c( *this );
    int bodyLength = c.getBodyLength() + c.getBeginStringLength() + 
	  Sequence::set_in_ordered(m_header, BodyLength::Pack(c.getBodyLength()))->second.getLength();
    return m_trailer.serializeTo(
             FieldMap::serializeTo(
               m_header.serializeTo(
                 s.buffer(bodyLength +
                   Sequence::set_in_ordered(m_trailer, CheckSum::Pack(checkSum()))->second.getLength()
                 ) ) ) );
  }

protected:
  /// Constructor for derived classes
  template <typename Packed>
  Message( const BeginString::Pack& beginString, const Packed& msgType )
  : FieldMap( FieldMap::create_allocator() ),
    m_header( get_allocator(), message_order( message_order::header ) ),
    m_trailer( get_allocator(), message_order( message_order::trailer ) ),
    m_status( 0 )
  {
    Sequence::push_back_to_ordered(m_header, beginString );
    Sequence::push_back_to_ordered(m_header, msgType );
  }

public:

  enum SerializationHint
  {
    KeepFieldChecksum,
    SerializedOnce
  };

  Message();

  /// Construct a message with hints
  Message( SerializationHint, int hintFieldCount = ItemAllocatorTraits::DefaultCapacity );

  /// Construct a message from a string
  Message( const std::string& string, bool validate = true )
  throw( InvalidMessage );

  /// Construct a message from a string using a data dictionary
  Message( const std::string& string, const FIX::DataDictionary& dataDictionary,
           bool validate = true )
  throw( InvalidMessage );

  /// Construct a message from a string using a session and application data dictionary
  Message( const std::string& string, const FIX::DataDictionary& sessionDataDictionary,
           const FIX::DataDictionary& applicationDataDictionary, bool validate = true )
  throw( InvalidMessage );
  Message( const std::string& string, const FIX::DataDictionary& sessionDataDictionary,
           const FIX::DataDictionary& applicationDataDictionary,
           FieldMap::allocator_type& allocator, bool validate = true )
  throw( InvalidMessage );

  Message( const Message& copy )
  : FieldMap( FieldMap::create_allocator(), copy ),
    m_header( get_allocator(), message_order( message_order::header ) ),
    m_trailer( get_allocator(), message_order( message_order::trailer ) )
  {
    m_header = copy.m_header;
    m_trailer = copy.m_trailer;
    m_status = copy.m_status;
    m_status_data = copy.m_status_data;
  }

  /// Set global data dictionary for encoding messages into XML
  static bool InitializeXML( const std::string& string );

  FieldMap& addGroup( FIX::Group& group )
  { return FieldMap::addGroup( group.field(), group ); }

  void replaceGroup( unsigned num, const FIX::Group& group )
  { FieldMap::replaceGroup( num, group.field(), group ); }

  Group& getGroup( unsigned num, FIX::Group& group ) const throw( FieldNotFound )
  { group.clear();
    return static_cast < Group& >
      ( FieldMap::getGroup( num, group.field(), group ) );
  }

  void removeGroup( unsigned num, const FIX::Group& group )
  { FieldMap::removeGroup( num, group.field() ); }
  void removeGroup( const FIX::Group& group )
  { FieldMap::removeGroup( group.field() ); }

  bool hasGroup( const FIX::Group& group ) const
  { return FieldMap::hasGroup( group.field() ); }
  bool hasGroup( unsigned num, const FIX::Group& group ) const
  { return FieldMap::hasGroup( num, group.field() ); }

  /// Get a string representation without making a copy
  inline std::string& toString( std::string& str ) const
  {
    return toString( FieldCounter( *this ), str );
  }

  /// Get a string representation without making a copy
  inline std::string& toString( std::string& str,
                         int beginStringField,
                         int bodyLengthField = FIELD::BodyLength, 
                         int checkSumField = FIELD::CheckSum ) const
  {
    return toString( FieldCounter( *this,
                                   beginStringField,
                                   bodyLengthField,
                                   checkSumField ), str );
  }

  /// Get a string representation of the message
  inline std::string toString() const
  {
    std::string str;
    toString( FieldCounter( *this ), str );
    return str;
  }

  /// Get a string representation of the message
  inline std::string toString( int beginStringField,
                        int bodyLengthField = FIELD::BodyLength,
                        int checkSumField = FIELD::CheckSum ) const
  {
    std::string str;
    toString( FieldCounter( *this,
                            beginStringField,
                            bodyLengthField,
                            checkSumField ), str );
    return str;
  }

  /// Get a XML representation of the message
  std::string toXML() const;
  /// Get a XML representation without making a copy
  std::string& toXML( std::string& ) const;

  /**
   * Add header informations depending on a source message.
   * This can be used to add routing informations like OnBehalfOfCompID
   * and DeliverToCompID to a message.
   */
  void reverseRoute( const Header& );

  /**
   * Set a message based on a string representation
   * This will fill in the fields on the message by parsing out the string
   * that is passed in.  It will return true on success and false
   * on failure.
   */
  void setString( const std::string& string )
  {
    setString( String::c_str(string), String::length(string),
               true, 0, 0 );
  }
  void setString( const std::string& string, bool validate )
  {
    setString( String::c_str(string), String::length(string),
               validate, 0, 0 );
  }
  void setString( const std::string& string,
                  bool validate,
                  const FIX::DataDictionary* pDataDictionary )
  throw( InvalidMessage )
  {
    setString( String::c_str(string), String::length(string),
               validate, pDataDictionary, pDataDictionary );
  }
  void setString( const std::string& string,
                  bool validate,
                  const FIX::DataDictionary* pSessionDataDictionary,
                  const FIX::DataDictionary* pApplicationDataDictionary )
  throw( InvalidMessage )
  {
    setString( String::c_str(string), String::length(string),
               validate, pSessionDataDictionary, pApplicationDataDictionary );
  }

  void setGroup( const std::string& msg, const FieldBase& field,
                 const std::string& string, std::string::size_type& pos,
                 FieldMap& map, const DataDictionary& dataDictionary );

  /**
   * Set a messages header from a string
   * This is an optimization that can be used to get useful information
   * from the header of a FIX string without parsing the whole thing.
   */
  bool setStringHeader( const std::string& string );

  /// Getter for the message header
  const Header& getHeader() const { return m_header; }
  /// Mutable getter for the message header
  Header& getHeader() { return m_header; }
  /// Getter for the message trailer
  const Header& getTrailer() const { return m_trailer; }
  /// Mutable getter for the message trailer
  Trailer& getTrailer() { return m_trailer; }

  bool hasValidStructure(int& field) const
  {
    if( getStatusBit(tag_out_of_order) )
    {
      field = m_status_data;
      return false;
    }
    return true;
  }

  const char* hasInvalidTagFormat() const
  {
    return getStatusBit(invalid_tag_format) ? (const char*)m_status_data: NULL;
  }

  int bodyLength( int beginStringField = FIELD::BeginString, 
              int bodyLengthField = FIELD::BodyLength, 
              int checkSumField = FIELD::CheckSum ) const
  {
    return FieldCounter( *this, beginStringField,
                                bodyLengthField,
                                checkSumField ).getBodyLength();
  }

  int checkSum( int checkSumField = FIELD::CheckSum ) const
  { return ( m_header.calculateTotal(checkSumField)
             + calculateTotal(checkSumField)
             + m_trailer.calculateTotal(checkSumField) ) & 255;
  }

  inline bool NOTHROW isAdmin() const
  { 
    if( m_header.isSetField(FIELD::MsgType) )
    {
      const MsgType& msgType = FIELD_GET_REF( m_header, MsgType );
      return isAdminMsgType( msgType );
    }
    return false;
  }

  inline bool NOTHROW isApp() const
  { 
    if( m_header.isSetField(FIELD::MsgType) )
    {
      const MsgType& msgType = FIELD_GET_REF( m_header, MsgType );
      return !isAdminMsgType( msgType );
    }
    return false;
  }

  bool isEmpty()
  { return m_header.isEmpty() && FieldMap::isEmpty() && m_trailer.isEmpty(); }

  void clear()
  { 
    m_status_data = 0;
    m_status = 0;
    m_header.clear();
    FieldMap::clear();
    m_trailer.clear();
  }

  static inline bool isAdminMsgTypeValue( const char* value )
  {
    Util::CharBuffer::Fixed<8> b = { { '0', '1', '3', '2', '4', '5', 'A', '\0' } };
    char sym = value[0];
    return sym && value[1] == '\0' && Util::CharBuffer::find( sym, b ) < 7;
  }

  static inline bool NOTHROW isAdminMsgType( const MsgType& msgType )
  {
    return isAdminMsgTypeValue( msgType.forString( String::CstrFunc() ) );
  }

  static ApplVerID toApplVerID(const BeginString& value)
  {
    if( value == BeginString_FIX40 )
      return ApplVerID(ApplVerID_FIX40);
    if( value == BeginString_FIX41 )
      return ApplVerID(ApplVerID_FIX41);
    if( value == BeginString_FIX42 )
      return ApplVerID(ApplVerID_FIX42);
    if( value == BeginString_FIX43 )
      return ApplVerID(ApplVerID_FIX43);
    if( value == BeginString_FIX44 )
      return ApplVerID(ApplVerID_FIX44);
    if( value == BeginString_FIX50 )
      return ApplVerID(ApplVerID_FIX50);
    if( value == "FIX.5.0SP1" )
      return ApplVerID(ApplVerID_FIX50SP1);
    if( value == "FIX.5.0SP2" )
      return ApplVerID(ApplVerID_FIX50SP2);
    return ApplVerID(ApplVerID(value));
  }

  static BeginString toBeginString( const ApplVerID& applVerID )
  {
    if( applVerID == ApplVerID_FIX40 )
      return BeginString(BeginString_FIX40);
    else if( applVerID == ApplVerID_FIX41 )
      return BeginString(BeginString_FIX41);
    else if( applVerID == ApplVerID_FIX42 )
      return BeginString(BeginString_FIX42);
    else if( applVerID == ApplVerID_FIX43 )
      return BeginString(BeginString_FIX43);
    else if( applVerID == ApplVerID_FIX44 )
      return BeginString(BeginString_FIX44);
    else if( applVerID == ApplVerID_FIX50 )
      return BeginString(BeginString_FIX50);
    else if( applVerID == ApplVerID_FIX50SP1 )
      return BeginString(BeginString_FIX50);
    else if( applVerID == ApplVerID_FIX50SP2 )
      return BeginString(BeginString_FIX50);
    else
      return BeginString("");
  }


  static inline bool isHeaderField( int field,
				    const DataDictionary* pD = 0 )
  {
    return ( LIKELY((unsigned)field < headerFieldSet.size()) &&
	     headerFieldSet[field] ) || 
	   ( pD && pD->isHeaderField( field ) );
  }
  
  static inline bool isHeaderField( const FieldBase& field,
				    const DataDictionary* pD = 0 )
  {
    return isHeaderField( field.getField(), pD );
  }
  
  static inline bool isTrailerField( int field,
                                     const DataDictionary* pD = 0 )
  {
    return (field == FIELD::SignatureLength) ||
	   (field == FIELD::Signature) ||
	   (field == FIELD::CheckSum) ||
	   (pD && pD->isTrailerField( field ));
  }
  
  static inline bool isTrailerField( const FieldBase& field,
                                     const DataDictionary* pD = 0 )
  {
    return isTrailerField( field.getField(), pD );
  }

  /// Returns the session ID of the intended recipient
  SessionID getSessionID( const std::string& qualifier = "" ) const
  throw( FieldNotFound );
  /// Sets the session ID of the intended recipient
  void setSessionID( const SessionID& sessionID );

private:

  bool extractFieldDataLength( FieldReader& f, const Group* pGroup, int field );
  bool extractField ( FieldReader& f,
		      const DataDictionary* pSessionDD = 0,
		      const DataDictionary* pAppDD = 0,
		      const Group* pGroup = 0);

  void setGroup ( FieldReader& f, const DataDictionary::FieldPresenceMap::key_type& msg,
		  const int group, FieldMap& map, const DataDictionary& dataDictionary );

  void validate(const BodyLength* pBodyLength);
  std::string toXMLFields(const FieldMap& fields, int space) const;

  static inline int createStatus(status_type bit, bool v)
  {
    return (int)v << bit;
  }

  inline void setStatusBit(status_type bit)
  {
    m_status |= 1 << bit;
  }

  inline void setErrorStatusBit(status_type bit, int data)
  {
    if ( !(m_status & status_error_mask) )
    {
      m_status_data = data;
      m_status |= 1 << bit;
    }
  }

  inline void clearStatusBit(status_type bit)
  {
    m_status &= ~(1 << bit);
  }

  inline bool getStatusBit(status_type bit) const
  {
    return (m_status & (1 << bit)) != 0;
  }

  static inline admin_trait getAdminTrait( char msgType ) 
  {
    Util::CharBuffer::Fixed<8> b = { { '0', '1', '3', '2', '4', '5', 'A', '\0' } };
    std::size_t pos = Util::CharBuffer::find( msgType, b );
    return (admin_trait)( (pos < 7) << (pos & 7) / 3 );
  }

protected:
  mutable FieldMap m_header;
  mutable FieldMap m_trailer;
  intptr_t m_status;
  intptr_t m_status_data;

  static std::auto_ptr<DataDictionary> s_dataDictionary;
};
/*! @} */

inline const char* Message::FieldReader::scan()
{
  const char* b = m_start + m_pos;
  const char* p = Util::Tag::delimit(b, m_end - b);
  if ( LIKELY(p != NULL) )
  {
    m_length = p - b;
    if ( LIKELY(Util::Tag::parse(b, p, m_field, m_csum)) )
    {
      p++;
      b = (char*)::memchr( p, '\001', m_end - p );
      if ( LIKELY(b != NULL) )
      {
        m_start = p;
        m_pos = b - p;
        return NULL;
      }
      else
        b = ErrSOH;
    }
    else
      return b;
  }
  else
    b = ErrDelimiter;

  m_field = 0;
  throw InvalidMessage(b);
}

inline void Message::FieldReader::skip()
{
  const char* b = m_start + m_pos;
  const char* p = (const char*)::memchr( b, '\001', m_end - b );
  if ( LIKELY(p != NULL) )
  {
    m_start = b;
    m_pos = p - b + 1;
    return;
  }

  m_field = 0;
  throw InvalidMessage(ErrSOH);
}

inline void HEAVYUSE
Message::FieldCounter::countHeader( const FieldMap& fields )
{
  FieldMap::const_iterator it = fields.begin();
  const FieldMap::const_iterator end = fields.end();

  if( LIKELY(it != end) )
  {
    if( LIKELY(it->first == FIELD::BeginString) )
    {
      m_prefix = it->second.getLength();
      if( LIKELY(++it != end) )
        if( LIKELY(it->first == FIELD::BodyLength) )
          ++it;
    }
    else
    {
      if( it->first == FIELD::BodyLength )
        ++it;
    }
    for( ; it != end; ++it )
    {
      m_length += it->second.getLength();
    }
  }
  m_length += countGroups(fields.g_begin(), fields.g_end());
}

inline void HEAVYUSE
Message::FieldCounter::countHeader( int beginStringField,
                                    int bodyLengthField,
                                    const FieldMap& fields )
{
  const FieldMap::const_iterator end = fields.end();
  for( FieldMap::const_iterator it = fields.begin(); it != end; ++it )
  {
    int tag = it->first;
    if ( LIKELY(tag != bodyLengthField) )
    {
      if( LIKELY(tag != beginStringField) )
        m_length += it->second.getLength();
      else
        m_prefix += it->second.getLength();
    }
  }
  m_length += countGroups(fields.g_begin(), fields.g_end());
}

inline int HEAVYUSE
Message::FieldCounter::countBody(const FieldMap& fields)
{
  int result = 0;
  const FieldMap::const_iterator end = fields.end();
  for( FieldMap::const_iterator it = fields.begin();
       LIKELY(it != end); ++it )
  {
    result += it->second.getLength();
  }
  return result + countGroups(fields.g_begin(), fields.g_end());
}

inline void HEAVYUSE
Message::FieldCounter::countTrailer(const FieldMap& fields)
{
  const FieldMap::const_iterator end = fields.end();
  for ( FieldMap::const_iterator it = fields.begin();
        it != end; ++it )
  {
    if ( it->first != checkSumTag )
      m_length += it->second.getLength();
  }
  m_length += countGroups(fields.g_begin(), fields.g_end());
}

inline std::ostream& operator <<
( std::ostream& stream, const Message& message )
{
  std::string str;
  stream << message.toString( str );
  return stream;
}

inline SessionID Message::getSessionID( const std::string& qualifier ) const
throw( FieldNotFound )
{
  BeginString beginString;
  SenderCompID senderCompID;
  TargetCompID targetCompID;

  getHeader().getField( beginString );
  getHeader().getField( senderCompID );
  getHeader().getField( targetCompID );

  return SessionID( beginString, senderCompID, targetCompID, qualifier );
}

inline void Message::setSessionID( const SessionID& sessionID )
{
  Sequence::set_in_ordered( getHeader(), sessionID.getBeginString() );
  Sequence::set_in_ordered( getHeader(), sessionID.getSenderCompID() );
  Sequence::set_in_ordered( getHeader(), sessionID.getTargetCompID() );
}

/// Parse the type of a message from a string.
inline MsgType identifyType( const char* message, std::size_t length )
throw( MessageParseError )
{
  const char* p = Util::CharBuffer::memmem( message, length, "\00135=", 4 );
  if ( p != NULL )
  {
    p += 4;
    const char* e = (const char*)::memchr( p, '\001', length - (p - message) );
    if ( e != NULL )
      return MsgType( p, e - p );
  }
  throw MessageParseError();
}
inline MsgType identifyType( const std::string& message )
throw( MessageParseError )
{
  return identifyType( String::c_str(message), String::length(message) );
}
}

#endif //FIX_MESSAGE
