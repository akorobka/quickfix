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

#ifndef FIX_PARSER_H
#define FIX_PARSER_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#include "Exceptions.h"
#include "FixString.h"
#include <iostream>
#include <string>

#ifdef _MSC_VER
#pragma warning( disable : 4125 ) // DISABLE warning C4125: decimal digit terminates octal escape sequence
#endif

namespace FIX
{
/// Parses %FIX messages off an input stream.
class Parser
{
  enum State
  {
    RTR,
    HAVE_BEGIN_STRING,
    HAVE_BODY_LENGTH,
    HAVE_BODY,
    HAVE_CHECKSUM,
    HAVE_MESSAGE
  };

  static const int DefaultBufferLength = 65536;
  static const int MaxBufferLength = 1048576;
  static const int LowBufferWatermark = 1024;

public:
  Parser()
  : m_state( RTR ),
    m_bufferSize( DefaultBufferLength ),
    m_cursor( 0 ), m_pos( 0 ), m_begin( 0 ),
    m_bodyLength( 0 )
  {
    m_buffer = (char*)::malloc(m_bufferSize);
  }
  ~Parser()
  {
    ::free(m_buffer);
  }

  void addToStream( const char* p, std::size_t sz )
  {
    while( true )
    {
      Sg::sg_buf_t buf = buffer();
      if ( sz > (std::size_t)IOV_LEN(buf) )
      {
        ::memcpy( IOV_BUF(buf), p, IOV_LEN(buf) ); 
        sz -= IOV_LEN(buf);
        p += IOV_LEN(buf);
        advance( IOV_LEN(buf) );
      }
      else
      {
        ::memcpy( IOV_BUF(buf), p, sz );
        advance( sz );
        break;
      }
    }
  }
  void addToStream( const std::string& s )
  {
    addToStream( String::c_str(s), String::length(s) );
  }
  std::size_t extractLength( const char* msg, std::size_t size )
  throw ( MessageParseError );
  bool readFixMessage( std::string& str )
  throw ( MessageParseError )
  {
    if( parse() )
    {
      Sg::sg_buf_t buf;
      retrieve( buf );
      str = Sg::toString(buf);
      return true;
    }
    return false;
  }

  void reset()
  {
    m_cursor = m_pos = 0;
    m_state = RTR;
  }

  bool HEAVYUSE parse()
  {
    const char* p;
    int cursor = m_cursor;
    switch( m_state )
    {
      case RTR:
        m_bodyLength = 0;
        if( LIKELY(m_pos - cursor > 2) )
        {
          Util::CharBuffer::Fixed<2> beginStringTag = { { '8', '=' } };
          p = Util::CharBuffer::find( beginStringTag,
                                      m_buffer + cursor, m_pos - cursor );
          if( LIKELY(NULL != p) )
          {
            m_begin = p - m_buffer;
            cursor = m_begin + 2;
            m_state = HAVE_BEGIN_STRING;
          }
          else // skip garbage
          {
            m_buffer[0] = m_buffer[m_pos - 1];
            m_cursor = 0;
            m_pos = 1;
            return false;
          }
        }
        else
          return false;

      case HAVE_BEGIN_STRING:
        if( LIKELY(m_pos - cursor > 3) )
        {
          p = Util::CharBuffer::memmem(m_buffer + cursor,
                                       m_pos - cursor, "\0019=", 3);
          if( LIKELY(NULL != p ) )
          {
            cursor = p - m_buffer + 3;
            m_state = HAVE_BODY_LENGTH;
          }
          else
          {
            cursor = m_pos - 2;
            break;
          }
        }
        else
        {
          break;
        }
        // fall through

      case HAVE_BODY_LENGTH:
        if( LIKELY(cursor < m_pos) )
        {
          do
          {
            int v = m_buffer[cursor++];
            unsigned a = v - '0';
            if( a < 10 )
            {
              m_bodyLength = m_bodyLength * 10 + a;
            }
            else if (v == '\001' && m_bodyLength < MaxBufferLength)
            {
              m_state = HAVE_BODY;
              break;
            }
            else
            {
              m_cursor = cursor;
              m_state = RTR;
              throw MessageParseError( "BodyLength error" );
            }
          } while( cursor < m_pos );
        }
        else
          break;
        // fall through

      case HAVE_BODY:
        if( LIKELY(m_pos >= cursor + m_bodyLength + 3) )
        {
          Util::CharBuffer::Fixed<4> checkSumTag = { { '\001','1','0','=' } };
          cursor += m_bodyLength - 1;
          p = Util::CharBuffer::find( checkSumTag,
                                      m_buffer + cursor, m_pos - cursor);
          if( LIKELY(NULL != p) && LIKELY((p - m_buffer) == cursor) )
          {
            cursor = p - m_buffer + 4;
            m_state = HAVE_CHECKSUM;
          }
          else
          {
            m_cursor = p ? p - m_buffer + 4 : cursor;
            m_state = RTR;
            throw MessageParseError( "BodyLength mismatch" );
          }
        }
        else
          break;

        // fall through
      case HAVE_CHECKSUM:
        if( LIKELY(m_pos - cursor > 0) &&
            LIKELY(NULL != (p = (const char*)::memchr( m_buffer + cursor,
                                               '\001', m_pos - cursor )) ) )
        {
          cursor = p - m_buffer + 1;
          m_state = HAVE_MESSAGE;
        }
        else
        {
          cursor = m_pos;      
          break;
        }
      case HAVE_MESSAGE:
        break;
    }

    if( LIKELY(m_pos - m_begin < MaxBufferLength) )
    {
      m_cursor = cursor;
      return HAVE_MESSAGE == m_state;
    }

    reset();
    throw MessageParseError( "Message is too long" );
  }

  void advance( int size )
  {
    m_pos += size;
  }

  void retrieve( Sg::sg_buf_t& buf )
  {
    IOV_BUF(buf) = m_buffer + m_begin;
    IOV_LEN(buf) = m_cursor - m_begin;
    m_state = RTR;
  }

  Sg::sg_buf_t buffer()
  {
    if( LIKELY(m_state == RTR && m_pos == m_cursor) )
    {
      m_pos = m_cursor = 0;
    }
    else
    {
      if( m_bufferSize - m_pos < LowBufferWatermark )
      {
        if( (int)m_state > RTR && m_begin > (m_bufferSize >> 1) )
        {
          ::memcpy( m_buffer, m_buffer + m_begin, m_pos - m_begin );
          m_cursor -= m_begin;
          m_pos -= m_begin;
          m_begin = 0;
        }
        else
        {
          m_bufferSize <<= 1;
          m_buffer = (char*)::realloc( m_buffer, m_bufferSize );
        }
      }
    }
    Sg::sg_buf_t buf = IOV_BUF_INITIALIZER( m_buffer + m_pos, 
                                            m_bufferSize - m_pos );
    return buf;
  }

  template <std::size_t S>
  static Sg::sg_buf_t findField( Sg::sg_buf_t msg, const Util::CharBuffer::Fixed<S>& f )
  {
    const char* b =
        Util::CharBuffer::find( f, (char*)IOV_BUF(msg), IOV_LEN(msg) );
    if( LIKELY(NULL != b) )
    {
      std::size_t offset = ( b += S ) - (char*)IOV_BUF(msg);
      const char* e = (const char*)::memchr( b, '\001', IOV_LEN(msg) - offset );
      if ( LIKELY(NULL != e) )
      {
        IOV_BUF(msg) = (char*)IOV_BUF(msg) + offset;
        IOV_LEN(msg) = e - b;
        return msg;
      }
    }
    IOV_BUF(msg) = NULL;
    return msg;
  }

  static inline Sg::sg_buf_t findBeginString( Sg::sg_buf_t msg )
  {
    Util::CharBuffer::Fixed<2> beginStringTag = { { '8', '=' } };
    return findField( msg, beginStringTag );
  }

  static inline Sg::sg_buf_t findSenderCompID( Sg::sg_buf_t msg )
  {
    Util::CharBuffer::Fixed<4> senderCompIDTag = { { '\001', '4', '9', '=' } };
    return findField( msg, senderCompIDTag );
  }

  static inline Sg::sg_buf_t findTargetCompID( Sg::sg_buf_t msg )
  {
    Util::CharBuffer::Fixed<4> targetCompIDTag = { { '\001', '5', '6', '=' } };
    return findField( msg, targetCompIDTag );
  }

  static inline Sg::sg_buf_t findMsgType( Sg::sg_buf_t msg )
  {
    Util::CharBuffer::Fixed<4> msgTypeTag = { { '\001', '3', '5', '=' } };
    return findField( msg, msgTypeTag );
  }

private:
  char* m_buffer;
  State m_state;
  int m_bufferSize;
  int m_cursor;
  int m_pos;
  int m_begin;
  int m_bodyLength;
};
}

#ifdef _MSC_VER
#pragma warning( default: 4125 ) // RE-ENABLE warning C4125: decimal digit terminates octal escape sequence
#endif

#endif //FIX_PARSER_H
