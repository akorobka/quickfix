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

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 )
#include "stdafx.h"
#else
#include "config.h"
#endif

#include <memory>
#include "getopt-repl.h"
#include <iostream>
#include "Application.h"
#include "FieldConvertors.h"
#include "Values.h"
#include "FileStore.h"
#include "SessionID.h"
#include "Session.h"
#include "DataDictionary.h"
#include "Parser.h"
#include "Utility.h"
#include "SocketAcceptor.h"
#include "SocketInitiator.h"
#include "ThreadedSocketAcceptor.h"
#include "ThreadedSocketInitiator.h"
#include "fix42/Heartbeat.h"
#include "fix42/NewOrderSingle.h"
#include "fix42/QuoteRequest.h"

double testIntegerToString( int );
double testStringToInteger( int );
double testDoubleToString( int );
double testStringToDouble( int );
double testCreateHeartbeat( int );
double testIdentifyType( int );
double testSerializeToStringHeartbeat( int );
double testSerializeFromStringHeartbeat( int );
double testSerializeFromStringAndValidateHeartbeat( int );
double testCreateNewOrderSingle( int );
double testCreateNewOrderSinglePacked( int );
double testSerializeToStringNewOrderSingle( int );
double testSerializeFromStringNewOrderSingle( int );
double testSerializeFromStringAndValidateNewOrderSingle( int count );
double testCreateQuoteRequest( int );
double testCreateQuoteRequestPacked( int );
double testCreateQuoteRequestPackedInplace( int );
double testReadFromQuoteRequest( int );
double testSerializeToStringQuoteRequest( int );
double testSerializeFromStringQuoteRequest( int );
double testSerializeFromStringAndValidateQuoteRequest( int );
double testFileStoreNewOrderSingle( int );
double testValidateNewOrderSingle( int );
double testValidateDictNewOrderSingle( int );
double testValidateQuoteRequest( int );
double testValidateDictQuoteRequest( int );
double testSendOnSocket( int, short, bool );
double testSendOnThreadedSocket( int, short, bool );
void report( double, int );

std::auto_ptr<FIX::DataDictionary> s_dataDictionary;
const bool VALIDATE = true;
const bool DONT_VALIDATE = false;


int main( int argc, char** argv )
{
  int count = 0;
  short port = 0;

  int opt;
  while ( (opt = getopt( argc, argv, "+p:+c:" )) != -1 )
  {
    switch( opt )
    {
    case 'p':
      port = (short)atol( optarg );
      break;
    case 'c':
      count = atoi( optarg );
      break;
    default:
      std::cout << "usage: "
      << argv[ 0 ]
      << " -p port -c count" << std::endl;
      return 1;
    }
  }

  s_dataDictionary.reset( new FIX::DataDictionary( "../spec/FIX42.xml" ) );

  std::cout << "Converting integers to strings: ";
  report( testIntegerToString( count ), count );

  std::cout << "Converting strings to integers: ";
  report( testStringToInteger( count ), count );

  std::cout << "Converting doubles to strings: ";
  report( testDoubleToString( count ), count );

  std::cout << "Converting strings to doubles: ";
  report( testStringToDouble( count ), count );

  std::cout << "Creating Heartbeat messages: ";
  report( testCreateHeartbeat( count ), count );

  std::cout << "Identifying message types: ";
  report( testIdentifyType( count ), count );

  std::cout << "Serializing Heartbeat messages to strings: ";
  report( testSerializeToStringHeartbeat( count ), count );

  std::cout << "Serializing Heartbeat messages from strings: ";
  report( testSerializeFromStringHeartbeat( count ), count );

  std::cout << "Serializing Heartbeat messages from strings and validation: ";
  report( testSerializeFromStringAndValidateHeartbeat( count ), count );

  std::cout << "Creating NewOrderSingle messages: ";
  report( testCreateNewOrderSingle( count ), count );

  std::cout << "Creating NewOrderSingle messages (packed): ";
  report( testCreateNewOrderSinglePacked( count ), count );

  std::cout << "Serializing NewOrderSingle messages to strings: ";
  report( testSerializeToStringNewOrderSingle( count ), count );

  std::cout << "Serializing NewOrderSingle messages from strings: ";
  report( testSerializeFromStringNewOrderSingle( count ), count );

  std::cout << "Serializing NewOrderSingle messages from strings and validation: ";
  report( testSerializeFromStringAndValidateNewOrderSingle( count ), count );

  std::cout << "Creating QuoteRequest messages: ";
  report( testCreateQuoteRequest( count ), count );

  std::cout << "Creating QuoteRequest messages (packed): ";
  report( testCreateQuoteRequestPacked( count ), count );

  std::cout << "Creating QuoteRequest messages (packed, in place): ";
  report( testCreateQuoteRequestPackedInplace( count ), count );

  std::cout << "Serializing QuoteRequest messages to strings: ";
  report( testSerializeToStringQuoteRequest( count ), count );

  std::cout << "Serializing QuoteRequest messages from strings: ";
  report( testSerializeFromStringQuoteRequest( count ), count );

  std::cout << "Serializing QuoteRequest messages from strings and validation: ";
  report( testSerializeFromStringAndValidateQuoteRequest( count ), count );

  std::cout << "Reading fields from QuoteRequest message: ";
  report( testReadFromQuoteRequest( count ), count );

  std::cout << "Storing NewOrderSingle messages: ";
  report( testFileStoreNewOrderSingle( count ), count );
  std::cout << "Validating NewOrderSingle messages with no data dictionary: ";
  report( testValidateNewOrderSingle( count ), count );

  std::cout << "Validating NewOrderSingle messages with data dictionary: ";
  report( testValidateDictNewOrderSingle( count ), count );

  std::cout << "Validating QuoteRequest messages with no data dictionary: ";
  report( testValidateQuoteRequest( count ), count );

  std::cout << "Validating QuoteRequest messages with data dictionary: ";
  report( testValidateDictQuoteRequest( count ), count );

  std::cout << "Sending/Receiving NewOrderSingle/ExecutionReports on Socket";
  report( testSendOnSocket( count, port, false ), count );

  std::cout << "Sending/Receiving NewOrderSingle/ExecutionReports on ThreadedSocket";
  report( testSendOnThreadedSocket( count, port, false ), count );

  std::cout << "Sending/Receiving NewOrderSingle/ExecutionReports on Socket with dictionary";
  report( testSendOnSocket( count, port, true ), count );

  std::cout << "Sending/Receiving NewOrderSingle/ExecutionReports on ThreadedSocket with dictionary";
  report( testSendOnThreadedSocket( count, port, true ), count );

  return 0;
}

void report( double seconds, int count )
{
  double num_per_second = count / seconds;
  std::cout << std::endl << "    num: " << count
  << ", seconds: " << seconds
  << ", num_per_second: " << num_per_second << std::endl;
}

double testIntegerToString( int count )
{
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    FIX::IntConvertor::convert( 1234 );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testStringToInteger( int count )
{
  FIX::String::value_type value( "1234" );
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    FIX::IntConvertor::convert( value );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testDoubleToString( int count )
{
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    FIX::DoubleConvertor::convert( 123.45 );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testStringToDouble( int count )
{
  FIX::String::value_type value( "123.45" );
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    FIX::DoubleConvertor::convert( value );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testCreateHeartbeat( int count )
{
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    FIX42::Heartbeat();
  }

  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testIdentifyType( int count )
{
  FIX42::Heartbeat message;
  std::string messageString = message.toString();

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    FIX::identifyType( messageString );
  }

  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testSerializeToStringHeartbeat( int count )
{
  FIX42::Heartbeat message;
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    message.toString();
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testSerializeFromStringHeartbeat( int count )
{
  FIX42::Heartbeat message;
  std::string string = message.toString();
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    message.setString( string, VALIDATE, s_dataDictionary.get() );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testSerializeFromStringAndValidateHeartbeat( int count )
{
  FIX42::Heartbeat message;
  std::string string = message.toString();
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    message.setString( string, VALIDATE, s_dataDictionary.get() );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testCreateNewOrderSingle( int count )
{
  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    FIX::ClOrdID clOrdID( "ORDERID" );
    FIX::HandlInst handlInst( '1' );
    FIX::Symbol symbol( "LNUX" );
    FIX::Side side( FIX::Side_BUY );
    FIX::TransactTime transactTime;
    FIX::OrdType ordType( FIX::OrdType_MARKET );
    FIX42::NewOrderSingle( clOrdID, handlInst, symbol, side, transactTime, ordType );
  }

  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testCreateNewOrderSinglePacked( int count )
{
  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    FIX42::NewOrderSingle(
      FIX::ClOrdID::Pack( "ORDERID" ),
      FIX::HandlInst::Pack( '1' ),
      FIX::Symbol::Pack( "LNUX" ),
      FIX::Side::Pack( FIX::Side_BUY ),
      FIX::TransactTime::Pack( FIX::UtcTimeStamp() ),
      FIX::OrdType::Pack( FIX::OrdType_MARKET )
    );
  }

  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testSerializeToStringNewOrderSingle( int count )
{
  FIX::ClOrdID clOrdID( "ORDERID" );
  FIX::HandlInst handlInst( '1' );
  FIX::Symbol symbol( "LNUX" );
  FIX::Side side( FIX::Side_BUY );
  FIX::TransactTime transactTime;
  FIX::OrdType ordType( FIX::OrdType_MARKET );
  FIX42::NewOrderSingle message
  ( clOrdID, handlInst, symbol, side, transactTime, ordType );

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    message.toString();
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testSerializeFromStringAndValidateNewOrderSingle( int count )
{
  FIX::ClOrdID clOrdID( "ORDERID" );
  FIX::HandlInst handlInst( '1' );
  FIX::Symbol symbol( "LNUX" );
  FIX::Side side( FIX::Side_BUY );
  FIX::TransactTime transactTime;
  FIX::OrdType ordType( FIX::OrdType_MARKET );
  FIX42::NewOrderSingle message
    ( clOrdID, handlInst, symbol, side, transactTime, ordType );
  std::string string = message.toString();

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    message.setString( string, VALIDATE, s_dataDictionary.get() );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}


double testSerializeFromStringNewOrderSingle( int count )
{
  FIX::ClOrdID clOrdID( "ORDERID" );
  FIX::HandlInst handlInst( '1' );
  FIX::Symbol symbol( "LNUX" );
  FIX::Side side( FIX::Side_BUY );
  FIX::TransactTime transactTime;
  FIX::OrdType ordType( FIX::OrdType_MARKET );
  FIX42::NewOrderSingle message
  ( clOrdID, handlInst, symbol, side, transactTime, ordType );
  std::string string = message.toString();

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    message.setString( string, VALIDATE, s_dataDictionary.get() );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testCreateQuoteRequest( int count )
{
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  FIX::Symbol symbol;
  FIX::MaturityMonthYear maturityMonthYear;
  FIX::PutOrCall putOrCall;
  FIX::StrikePrice strikePrice;
  FIX::Side side;
  FIX::OrderQty orderQty;
  FIX::Currency currency;
  FIX::OrdType ordType;

  for ( int i = 0; i <= count; ++i )
  {
    FIX42::QuoteRequest massQuote( FIX::QuoteReqID("1") );
    FIX42::QuoteRequest::NoRelatedSym noRelatedSym;

    for( int j = 1; j <= 10; ++j )
    {
      symbol.setValue( "IBM" );
      maturityMonthYear.setValue( "022003" );
      putOrCall.setValue( FIX::PutOrCall_PUT );
      strikePrice.setValue( 120 );
      side.setValue( FIX::Side_BUY );
      orderQty.setValue( 100 );
      currency.setValue( "USD" );
      ordType.setValue( FIX::OrdType_MARKET );
      noRelatedSym.set( symbol );
      noRelatedSym.set( maturityMonthYear );
      noRelatedSym.set( putOrCall );
      noRelatedSym.set( strikePrice );
      noRelatedSym.set( side );
      noRelatedSym.set( orderQty );
      noRelatedSym.set( currency );
      noRelatedSym.set( ordType );
      massQuote.addGroup( noRelatedSym );
      noRelatedSym.clear();
    }
  }

  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testCreateQuoteRequestPacked( int count )
{
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();

  for ( int i = 0; i <= count; ++i )
  {
    FIX42::QuoteRequest massQuote( FIX::QuoteReqID::Pack("1") );
    FIX42::QuoteRequest::NoRelatedSym noRelatedSym;

    for( int j = 1; j <= 10; ++j )
    {
      noRelatedSym.set( FIX::Symbol::Pack( "IBM" ) );
      noRelatedSym.set( FIX::MaturityMonthYear::Pack( "022003" ) );
      noRelatedSym.set( FIX::PutOrCall::Pack( FIX::PutOrCall_PUT ) );
      noRelatedSym.set( FIX::StrikePrice::Pack( 120 ) );
      noRelatedSym.set( FIX::Side::Pack( FIX::Side_BUY ) );
      noRelatedSym.set( FIX::OrderQty::Pack( 100 ) );
      noRelatedSym.set( FIX::Currency::Pack( "USD" ) );
      noRelatedSym.set( FIX::OrdType::Pack( FIX::OrdType_MARKET ) );
      massQuote.addGroup( noRelatedSym );
      noRelatedSym.clear();
    }
  }

  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testCreateQuoteRequestPackedInplace( int count )
{
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();

  FIX42::QuoteRequest::NoRelatedSym noRelatedSym;

  for ( int i = 0; i <= count; ++i )
  {
    FIX42::QuoteRequest massQuote( FIX::QuoteReqID::Pack("1") );

    for( int j = 1; j <= 10; ++j )
    {
      FIX::FieldMap& g = massQuote.addGroup( noRelatedSym );
      g.addField( FIX::Symbol::Pack( "IBM" ) );
      g.addField( FIX::MaturityMonthYear::Pack( "022003" ) );
      g.addField( FIX::PutOrCall::Pack( FIX::PutOrCall_PUT ) );
      g.addField( FIX::StrikePrice::Pack( 120 ) );
      g.addField( FIX::Side::Pack( FIX::Side_BUY ) );
      g.addField( FIX::OrderQty::Pack( 100 ) );
      g.addField( FIX::Currency::Pack( "USD" ) );
      g.addField( FIX::OrdType::Pack( FIX::OrdType_MARKET ) );
    }
  }

  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testSerializeToStringQuoteRequest( int count )
{
  FIX42::QuoteRequest message( FIX::QuoteReqID("1") );
  FIX42::QuoteRequest::NoRelatedSym noRelatedSym;

  for( int i = 1; i <= 10; ++i )
  {
    noRelatedSym.set( FIX::Symbol("IBM") );
    noRelatedSym.set( FIX::MaturityMonthYear() );
    noRelatedSym.set( FIX::PutOrCall(FIX::PutOrCall_PUT) );
    noRelatedSym.set( FIX::StrikePrice(120) );
    noRelatedSym.set( FIX::Side(FIX::Side_BUY) );
    noRelatedSym.set( FIX::OrderQty(100) );
    noRelatedSym.set( FIX::Currency("USD") );
    noRelatedSym.set( FIX::OrdType(FIX::OrdType_MARKET) );
    message.addGroup( noRelatedSym );
  }

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int j = 0; j <= count; ++j )
  {
    message.toString();
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testSerializeFromStringQuoteRequest( int count )
{
  FIX42::QuoteRequest message( FIX::QuoteReqID("1") );
  FIX42::QuoteRequest::NoRelatedSym noRelatedSym;

  for( int i = 1; i <= 10; ++i )
  {
    noRelatedSym.set( FIX::Symbol("IBM") );
    noRelatedSym.set( FIX::MaturityMonthYear() );
    noRelatedSym.set( FIX::PutOrCall(FIX::PutOrCall_PUT) );
    noRelatedSym.set( FIX::StrikePrice(120) );
    noRelatedSym.set( FIX::Side(FIX::Side_BUY) );
    noRelatedSym.set( FIX::OrderQty(100) );
    noRelatedSym.set( FIX::Currency("USD") );
    noRelatedSym.set( FIX::OrdType(FIX::OrdType_MARKET) );
    message.addGroup( noRelatedSym );
  }
  std::string string = message.toString();

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int j = 0; j <= count; ++j )
  {
    message.setString( string, DONT_VALIDATE, s_dataDictionary.get() );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testSerializeFromStringAndValidateQuoteRequest( int count )
{
  FIX42::QuoteRequest message( FIX::QuoteReqID("1") );
  FIX42::QuoteRequest::NoRelatedSym noRelatedSym;

  for( int i = 1; i <= 10; ++i )
  {
    noRelatedSym.set( FIX::Symbol("IBM") );
    noRelatedSym.set( FIX::MaturityMonthYear() );
    noRelatedSym.set( FIX::PutOrCall(FIX::PutOrCall_PUT) );
    noRelatedSym.set( FIX::StrikePrice(120) );
    noRelatedSym.set( FIX::Side(FIX::Side_BUY) );
    noRelatedSym.set( FIX::OrderQty(100) );
    noRelatedSym.set( FIX::Currency("USD") );
    noRelatedSym.set( FIX::OrdType(FIX::OrdType_MARKET) );
    message.addGroup( noRelatedSym );
  }
  std::string string = message.toString();

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int j = 0; j <= count; ++j )
  {
    message.setString( string, VALIDATE, s_dataDictionary.get() );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testReadFromQuoteRequest( int count )
{
  count = count - 1;

  FIX42::QuoteRequest message( FIX::QuoteReqID("1") );
  FIX42::QuoteRequest::NoRelatedSym group;

  for( int i = 1; i <= 10; ++i )
  {
    group.set( FIX::Symbol("IBM") );
    group.set( FIX::MaturityMonthYear() );
    group.set( FIX::PutOrCall(FIX::PutOrCall_PUT) );
    group.set( FIX::StrikePrice(120) );
    group.set( FIX::Side(FIX::Side_BUY) );
    group.set( FIX::OrderQty(100) );
    group.set( FIX::Currency("USD") );
    group.set( FIX::OrdType(FIX::OrdType_MARKET) );
    message.addGroup( group );
  }
  group.clear();

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int j = 0; j <= count; ++j )
  {
    FIX::QuoteReqID quoteReqID;
    FIX::Symbol symbol;
    FIX::MaturityMonthYear maturityMonthYear;
    FIX::PutOrCall putOrCall;
    FIX::StrikePrice strikePrice;
    FIX::Side side;
    FIX::OrderQty orderQty;
    FIX::Currency currency;
    FIX::OrdType ordType;

    FIX::NoRelatedSym noRelatedSym;
    message.get( noRelatedSym );
    int end = noRelatedSym;
    for( int k = 1; k <= end; ++k )
    {
      message.getGroup( k, group );
      group.get( symbol );
      group.get( maturityMonthYear );
      group.get( putOrCall);
      group.get( strikePrice );
      group.get( side );
      group.get( orderQty );
      group.get( currency );
      group.get( ordType );
      maturityMonthYear.getValue();
      putOrCall.getValue();
      strikePrice.getValue();
      side.getValue();
      orderQty.getValue();
      currency.getValue();
      ordType.getValue();
    }
  }

  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testFileStoreNewOrderSingle( int count )
{
  FIX::BeginString beginString( FIX::BeginString_FIX42 );
  FIX::SenderCompID senderCompID( "SENDER" );
  FIX::TargetCompID targetCompID( "TARGET" );
  FIX::SessionID id( beginString, senderCompID, targetCompID );

  FIX::ClOrdID clOrdID( "ORDERID" );
  FIX::HandlInst handlInst( '1' );
  FIX::Symbol symbol( "LNUX" );
  FIX::Side side( FIX::Side_BUY );
  FIX::TransactTime transactTime;
  FIX::OrdType ordType( FIX::OrdType_MARKET );
  FIX42::NewOrderSingle message
  ( clOrdID, handlInst, symbol, side, transactTime, ordType );
  message.getHeader().set( FIX::MsgSeqNum( 1 ) );
  std::string messageString = message.toString();

  FIX::FileStore store( "store", id );
  store.reset();
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    store.set( ++i, messageString );
  }
  double ticks = (FIX::Util::Sys::TickCount::now() - start).seconds();
  store.reset();
  return ticks;
}

double testValidateNewOrderSingle( int count )
{
  FIX::ClOrdID clOrdID( "ORDERID" );
  FIX::HandlInst handlInst( '1' );
  FIX::Symbol symbol( "LNUX" );
  FIX::Side side( FIX::Side_BUY );
  FIX::TransactTime transactTime;
  FIX::OrdType ordType( FIX::OrdType_MARKET );
  FIX42::NewOrderSingle message
  ( clOrdID, handlInst, symbol, side, transactTime, ordType );
  message.getHeader().set( FIX::SenderCompID( "SENDER" ) );
  message.getHeader().set( FIX::TargetCompID( "TARGET" ) );
  message.getHeader().set( FIX::MsgSeqNum( 1 ) );

  FIX::DataDictionary dataDictionary;
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    dataDictionary.validate( message );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testValidateDictNewOrderSingle( int count )
{
  FIX::ClOrdID clOrdID( "ORDERID" );
  FIX::HandlInst handlInst( '1' );
  FIX::Symbol symbol( "LNUX" );
  FIX::Side side( FIX::Side_BUY );
  FIX::TransactTime transactTime;
  FIX::OrdType ordType( FIX::OrdType_MARKET );
  FIX42::NewOrderSingle message
  ( clOrdID, handlInst, symbol, side, transactTime, ordType );
  message.getHeader().set( FIX::SenderCompID( "SENDER" ) );
  message.getHeader().set( FIX::TargetCompID( "TARGET" ) );
  message.getHeader().set( FIX::MsgSeqNum( 1 ) );

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
  {
    s_dataDictionary->validate( message );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testValidateQuoteRequest( int count )
{
  FIX42::QuoteRequest message( FIX::QuoteReqID("1") );
  FIX42::QuoteRequest::NoRelatedSym noRelatedSym;

  for( int i = 1; i <= 10; ++i )
  {
    noRelatedSym.set( FIX::Symbol("IBM") );
    noRelatedSym.set( FIX::MaturityMonthYear() );
    noRelatedSym.set( FIX::PutOrCall(FIX::PutOrCall_PUT) );
    noRelatedSym.set( FIX::StrikePrice(120) );
    noRelatedSym.set( FIX::Side(FIX::Side_BUY) );
    noRelatedSym.set( FIX::OrderQty(100) );
    noRelatedSym.set( FIX::Currency("USD") );
    noRelatedSym.set( FIX::OrdType(FIX::OrdType_MARKET) );
    message.addGroup( noRelatedSym );
  }

  FIX::DataDictionary dataDictionary;
  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int j = 0; j <= count; ++j )
  {
    dataDictionary.validate( message );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

double testValidateDictQuoteRequest( int count )
{
  FIX42::QuoteRequest message( FIX::QuoteReqID("1") );
  FIX42::QuoteRequest::NoRelatedSym noRelatedSym;

  for( int i = 1; i <= 10; ++i )
  {
    noRelatedSym.set( FIX::Symbol("IBM") );
    noRelatedSym.set( FIX::MaturityMonthYear() );
    noRelatedSym.set( FIX::PutOrCall(FIX::PutOrCall_PUT) );
    noRelatedSym.set( FIX::StrikePrice(120) );
    noRelatedSym.set( FIX::Side(FIX::Side_BUY) );
    noRelatedSym.set( FIX::OrderQty(100) );
    noRelatedSym.set( FIX::Currency("USD") );
    noRelatedSym.set( FIX::OrdType(FIX::OrdType_MARKET) );
    message.addGroup( noRelatedSym );
  }

  count = count - 1;

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int j = 0; j <= count; ++j )
  {
    s_dataDictionary->validate( message );
  }
  return (FIX::Util::Sys::TickCount::now() - start).seconds();
}

class TestApplication : public FIX::NullApplication
{
public:
  TestApplication() : m_count(0) {}

  void fromApp( const FIX::Message& m, const FIX::SessionID& )
  throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType )
  {
    m_count++; 
  }

  int getCount() { return m_count; }

private:
  int m_count;
};

double testSendOnSocket( int count, short port, bool dictionary )
{
  std::stringstream stream;
  stream
    << "[DEFAULT]" << std::endl
    << "SocketConnectHost=localhost" << std::endl
    << "SocketConnectPort=" << (unsigned short)port << std::endl
    << "SocketAcceptPort=" << (unsigned short)port << std::endl
    << "SocketReuseAddress=Y" << std::endl
    << "StartTime=00:00:00" << std::endl
    << "EndTime=00:00:00" << std::endl;
  if ( dictionary )
    stream << "UseDataDictionary=Y" << std::endl
           << "DataDictionary=../spec/FIX42.xml" << std::endl;
  else
    stream << "UseDataDictionary=N" << std::endl;

  stream
    << "BeginString=FIX.4.2" << std::endl
    << "PersistMessages=N" << std::endl
    << "[SESSION]" << std::endl
    << "ConnectionType=acceptor" << std::endl
    << "SenderCompID=SERVER" << std::endl
    << "TargetCompID=CLIENT" << std::endl
    << "[SESSION]" << std::endl
    << "ConnectionType=initiator" << std::endl
    << "SenderCompID=CLIENT" << std::endl
    << "TargetCompID=SERVER" << std::endl
    << "HeartBtInt=30" << std::endl;

  FIX::ClOrdID clOrdID( "ORDERID" );
  FIX::HandlInst handlInst( '1' );
  FIX::Symbol symbol( "LNUX" );
  FIX::Side side( FIX::Side_BUY );
  FIX::TransactTime transactTime;
  FIX::OrdType ordType( FIX::OrdType_MARKET );
  FIX42::NewOrderSingle message( clOrdID, handlInst, symbol, side, transactTime, ordType );

  FIX::SessionID sessionID( "FIX.4.2", "CLIENT", "SERVER" );

  TestApplication application;
  FIX::MemoryStoreFactory factory;
  FIX::SessionSettings settings( stream );
  FIX::ScreenLogFactory logFactory( settings );

  FIX::SocketAcceptor acceptor( application, factory, settings );
  acceptor.start();

  FIX::SocketInitiator initiator( application, factory, settings );
  initiator.start();

  FIX::process_sleep( 1 );

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();

  for ( int i = 0; i <= count; ++i )
    FIX::Session::sendToTarget( message, sessionID );

  while( application.getCount() < count )
    FIX::process_sleep( 0.1 );

  double ticks = (FIX::Util::Sys::TickCount::now() - start).seconds();

  initiator.stop();
  acceptor.stop();

  return ticks;
}

double testSendOnThreadedSocket( int count, short port, bool dictionary )
{
  std::stringstream stream;
  stream
    << "[DEFAULT]" << std::endl
    << "SocketConnectHost=localhost" << std::endl
    << "SocketConnectPort=" << (unsigned short)port << std::endl
    << "SocketAcceptPort=" << (unsigned short)port << std::endl
    << "SocketReuseAddress=Y" << std::endl
    << "StartTime=00:00:00" << std::endl
    << "EndTime=00:00:00" << std::endl;
  if ( dictionary )
    stream << "UseDataDictionary=Y" << std::endl
           << "DataDictionary=../spec/FIX42.xml" << std::endl;
  else
    stream << "UseDataDictionary=N" << std::endl;

  stream
    << "BeginString=FIX.4.2" << std::endl
    << "PersistMessages=N" << std::endl
    << "[SESSION]" << std::endl
    << "ConnectionType=acceptor" << std::endl
    << "SenderCompID=SERVER" << std::endl
    << "TargetCompID=CLIENT" << std::endl
    << "[SESSION]" << std::endl
    << "ConnectionType=initiator" << std::endl
    << "SenderCompID=CLIENT" << std::endl
    << "TargetCompID=SERVER" << std::endl
    << "HeartBtInt=30" << std::endl;

  FIX::ClOrdID clOrdID( "ORDERID" );
  FIX::HandlInst handlInst( '1' );
  FIX::Symbol symbol( "LNUX" );
  FIX::Side side( FIX::Side_BUY );
  FIX::TransactTime transactTime;
  FIX::OrdType ordType( FIX::OrdType_MARKET );
  FIX42::NewOrderSingle message( clOrdID, handlInst, symbol, side, transactTime, ordType );

  FIX::SessionID sessionID( "FIX.4.2", "CLIENT", "SERVER" );

  TestApplication application;
  FIX::MemoryStoreFactory factory;
  FIX::SessionSettings settings( stream );

  FIX::ThreadedSocketAcceptor acceptor( application, factory, settings );
  acceptor.start();

  FIX::ThreadedSocketInitiator initiator( application, factory, settings );
  initiator.start();

  FIX::process_sleep( 1 );

  FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
  for ( int i = 0; i <= count; ++i )
    FIX::Session::sendToTarget( message, sessionID );

  while( application.getCount() < count )
    FIX::process_sleep( 0.1 );

  double ticks = (FIX::Util::Sys::TickCount::now() - start).seconds();

  initiator.stop();
  acceptor.stop();

  return ticks;
}
