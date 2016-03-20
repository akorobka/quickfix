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

#ifndef FIX_SESSIONID_H
#define FIX_SESSIONID_H

#include "Fields.h"

namespace FIX
{
/// Unique session id consists of BeginString, SenderCompID and TargetCompID.
class SessionID
{
  static inline std::string toString( Sg::sg_buf_t beginString,
                                      Sg::sg_buf_t senderCompID,
                                      Sg::sg_buf_t targetCompID,
                                      Sg::sg_buf_ptr sessionQualifier )
  {
    std::string str;
    str.reserve( Sg::size(beginString) + 1 +
                 Sg::size(senderCompID) + 2 +
                 Sg::size(targetCompID) +
               ( sessionQualifier ? (Sg::size(*sessionQualifier) + 1) :  0 ) );
    String::append(
      String::append(
        String::append(
          str,
          beginString
        ).append(":", 1),
        senderCompID
      ).append("->", 2),
      targetCompID
    );
    if( sessionQualifier )
      String::append( str.append(":", 1), *sessionQualifier );
    return str;
  }

  static inline bool toFIXT( Sg::sg_buf_t beginString )
  {
    Util::CharBuffer::Fixed<4> tag = { { 'F', 'I', 'X', 'T' } };
    return NULL != Util::CharBuffer::find( tag, Sg::data<const char*>(beginString), 4 );
  }

public:
  SessionID()
  {
        toString(m_frozenString);
  }

  SessionID( const std::string& beginString,
             const std::string& senderCompID,
             const std::string& targetCompID,
             const std::string& sessionQualifier = "" )
  : m_beginString( String::c_str(beginString), String::length(beginString) ),
    m_senderCompID( String::c_str(senderCompID), String::length(senderCompID) ),
    m_targetCompID( String::c_str(targetCompID), String::length(targetCompID) ),
    m_sessionQualifier( sessionQualifier ),
    m_isFIXT(false)
  {
    toString(m_frozenString);
    if( beginString.substr(0, 4) == "FIXT" )
      m_isFIXT = true;
  }

  SessionID( Sg::sg_buf_t beginString,
             Sg::sg_buf_t senderCompID,
             Sg::sg_buf_t targetCompID,
             Sg::sg_buf_ptr sessionQualifier = NULL )
  : m_beginString( Sg::toString(beginString) ),
    m_senderCompID( Sg::toString(senderCompID) ),
    m_targetCompID( Sg::toString(targetCompID) ),
    m_sessionQualifier( sessionQualifier ? Sg::toString(*sessionQualifier) : std::string() ),
    m_isFIXT( toFIXT( beginString ) ),
    m_frozenString( toString(beginString, senderCompID, targetCompID, sessionQualifier) )
  {}

  const BeginString& getBeginString() const
    { return m_beginString; }
  const SenderCompID& getSenderCompID() const
    { return m_senderCompID; }
  const TargetCompID& getTargetCompID() const
    { return m_targetCompID; }
  const std::string& getSessionQualifier() const
    { return m_sessionQualifier; }
  bool isFIXT() const
    { return m_isFIXT; }

  /// Get a string representation of the SessionID
  std::string toString() const
  {
    return m_frozenString;
  }
    
  // Return a reference for a high-performance scenario
  const std::string& toStringFrozen() const
  {
    return m_frozenString;
  }

  /// Build from string representation of SessionID
  void fromString( const std::string& str )
  {
    std::string::size_type first =
      str.find_first_of(':');
    std::string::size_type second =
      str.find("->");
    std::string::size_type third =
      str.find_last_of(':');
    if( first == std::string::npos )
      return;
    if( second == std::string::npos )
      return;
    m_beginString.setPacked(
      BeginString::Pack( str.substr(0, first).data(), first ) );
    m_senderCompID.setPacked(
      SenderCompID::Pack( str.substr(first+1, second - first - 1).data(),
                                              second - first - 1) );
    if( first == third )
    {
      m_targetCompID.setPacked(
        TargetCompID::Pack( str.substr(second+2).data(),
                            str.length() - second - 2 ) );
      m_sessionQualifier = "";
    }
    else
    {
      m_targetCompID.setPacked(
        TargetCompID::Pack( str.substr(second+2, third - second - 2).data(),
                                                 third - second - 2) );
      m_sessionQualifier = str.substr(third+1);
    }
    toString(m_frozenString);
  }

  /// Get a string representation without making a copy
  std::string& toString( std::string& str ) const
  {
    Sg::sg_buf_t beginStringBuf = m_beginString.getSgBuf();
    Sg::sg_buf_t senderCompIDBuf = m_senderCompID.getSgBuf();
    Sg::sg_buf_t targetCompIDBuf = m_targetCompID.getSgBuf();
    Sg::sg_buf_t qualifier = String::toBuffer( m_sessionQualifier );
    str = toString( beginStringBuf, senderCompIDBuf, targetCompIDBuf,
                    Sg::size( qualifier ) ? &qualifier : NULL );
    return str;
  }

  friend bool operator<( const SessionID&, const SessionID& );
  friend bool operator==( const SessionID&, const SessionID& );
  friend bool operator!=( const SessionID&, const SessionID& );
  friend std::ostream& operator<<( std::ostream&, const SessionID& );
  friend std::ostream& operator>>( std::ostream&, const SessionID& );

  SessionID operator~() const
  {
    Sg::sg_buf_t qualifier = String::toBuffer( m_sessionQualifier );
    return SessionID( m_beginString.getSgBuf(),
                      m_targetCompID.getSgBuf(),
                      m_senderCompID.getSgBuf(),
                      Sg::size( qualifier ) ? &qualifier : NULL );
  }

private:
  BeginString m_beginString;
  SenderCompID m_senderCompID;
  TargetCompID m_targetCompID;
  std::string m_sessionQualifier;
  bool m_isFIXT;
  std::string m_frozenString;
};
/*! @} */

inline bool operator<( const SessionID& lhs, const SessionID& rhs )
{
  return lhs.toStringFrozen() < rhs.toStringFrozen();
}

inline bool operator==( const SessionID& lhs, const SessionID& rhs )
{
  return lhs.toStringFrozen() == rhs.toStringFrozen();
}

inline bool operator!=( const SessionID& lhs, const SessionID& rhs )
{
  return !( lhs == rhs );
}

inline std::ostream& operator<<
( std::ostream& stream, const SessionID& sessionID )
{
  stream << sessionID.toStringFrozen();
  return stream;
}

inline std::istream& operator>>
( std::istream& stream, SessionID& sessionID )
{
  std::string str;
  stream >> str;
  sessionID.fromString( str );
  return stream;
}

}
#endif //FIX_SESSIONID_H

