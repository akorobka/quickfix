#if defined(QUICKERFIX)

#include "quickfix/Field.h"
#include "quickfix/Values.h"
#include "quickfix/Message.h"


//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    FIX::Message msg;
    msg.getHeader().setField( FIX::BeginString( "FIX.4.4" ) );
    msg.getHeader().setField( FIX::MsgType( FIX::MsgType_NewOrderSingle ) );
    msg.getHeader().setField( FIX::MsgSeqNum(78) );
    msg.getHeader().setField( FIX::SenderCompID("A12345B") );
    msg.getHeader().setField( FIX::SenderSubID("2DEFGH4") );
    msg.getHeader().setField( FIX::SendingTime(FIX::UtcTimeStamp()) );
    msg.getHeader().setField( FIX::TargetCompID("COMPARO") );
    msg.getHeader().setField( FIX::TargetSubID("G") );
    msg.getHeader().setField( FIX::SenderLocationID("AU,SY") );

    msg.setField( FIX::Account( "01234567") );
    msg.setField( FIX::ClOrdID( "4" ) );
    msg.setField( FIX::OrderQty( 50 ) );
    msg.setField( FIX::OrdType( FIX::OrdType_LIMIT) );
    msg.setField( FIX::Price( 400.5) );
    msg.setField( FIX::HandlInst( '1' ) );
    msg.setField( FIX::Symbol( "OC") );
    msg.setField( FIX::Text( "NIGEL") );
    msg.setField( FIX::Side( FIX::Side_BUY ) );
    msg.setField( FIX::SecurityDesc( "AOZ3 C02000") );
    msg.setField( FIX::TimeInForce( FIX::TimeInForce_DAY ) );
    msg.setField( FIX::TransactTime() );
    msg.setField( FIX::SecurityType( FIX::SecurityType_OPTION ) );

    std::string output;
    double tt = 0;
    for(int i = 0 ; i < 1000000; ++i)
    {
        FIX::Message msg(FIX::Message::SerializedOnce);
        msg.getHeader().setField( FIX::BeginString::Pack( "FIX.4.4" ) );
        msg.getHeader().setField( FIX::MsgType::Pack( FIX::MsgType_NewOrderSingle ) );
        msg.getHeader().setField( FIX::MsgSeqNum::Pack(78));
        msg.getHeader().setField( FIX::SenderCompID::Pack("A12345B"));
        msg.getHeader().setField( FIX::SenderSubID::Pack("2DEFGH4"));
        msg.getHeader().setField( FIX::SendingTime::Pack(FIX::UtcTimeStamp()));
        msg.getHeader().setField( FIX::TargetCompID::Pack("COMPARO"));
        msg.getHeader().setField( FIX::TargetSubID::Pack("G"));
        msg.getHeader().setField( FIX::SenderLocationID::Pack("AU,SY"));

        msg.setField( FIX::Account::Pack( "01234567") );
        msg.setField( FIX::ClOrdID::Pack( "4" ) );
        msg.setField( FIX::OrderQty::Pack( 50 ) );
        msg.setField( FIX::OrdType::Pack( FIX::OrdType_LIMIT) );
        msg.setField( FIX::Price::Pack( 400.5) );
        msg.setField( FIX::HandlInst::Pack( '1' ) );
        msg.setField( FIX::Symbol::Pack( "OC") );
        msg.setField( FIX::Text::Pack( "NIGEL") );
        msg.setField( FIX::Side::Pack( FIX::Side_BUY ) );
        msg.setField( FIX::SecurityDesc::Pack( "AOZ3 C02000") );
        msg.setField( FIX::TimeInForce::Pack( FIX::TimeInForce_DAY ) );
        msg.setField( FIX::TransactTime() );
        msg.setField( FIX::SecurityType::Pack( FIX::SecurityType_OPTION ) );

        FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
        msg.toString(output);
        FIX::Util::Sys::TickCount end = FIX::Util::Sys::TickCount::now();

        tt += (end-start).seconds();
    }

    std::cout << "to string - " << tt << std::endl;

    if( argc > 1 )
    {
      if( FIX::file_exists(argv[1]) )
      {
        FIX::DataDictionary dict(argv[1]);

        FIX::Util::Sys::TickCount start = FIX::Util::Sys::TickCount::now();
        for(int i = 0 ; i < 1000000; ++i)
        {
          FIX::Message * tmp = new FIX::Message(output, dict);
          delete tmp;
        }
        FIX::Util::Sys::TickCount end = FIX::Util::Sys::TickCount::now();

        std::cout << "from string - " << (end-start).seconds() << std::endl;
      }
      else
        std::cout << "Unable to open dictionary file " << argv[1] << std::endl;
    }
  return 0;
}

#elif defined(QUICKFIX)

#include "quickfix/Field.h"
#include "quickfix/Values.h"
#include "quickfix/Message.h"
#include <tbb/tick_count.h>

typedef tbb::tick_count TickCount;

//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    FIX::Message msg;
    msg.getHeader().setField( FIX::BeginString( "FIX.4.4" ) );
    msg.getHeader().setField( FIX::MsgType( FIX::MsgType_NewOrderSingle ) );
    msg.getHeader().setField( FIX::MsgSeqNum(78) );
    msg.getHeader().setField( FIX::SenderCompID("A12345B") );
    msg.getHeader().setField( FIX::SenderSubID("2DEFGH4") );
    msg.getHeader().setField( FIX::SendingTime(FIX::UtcTimeStamp()) );
    msg.getHeader().setField( FIX::TargetCompID("COMPARO") );
    msg.getHeader().setField( FIX::TargetSubID("G") );
    msg.getHeader().setField( FIX::SenderLocationID("AU,SY") );

    msg.setField( FIX::Account( "01234567") );
    msg.setField( FIX::ClOrdID( "4" ) );
    msg.setField( FIX::OrderQty( 50 ) );
    msg.setField( FIX::OrdType( FIX::OrdType_LIMIT) );
    msg.setField( FIX::Price( 400.5) );
    msg.setField( FIX::HandlInst( '1' ) );
    msg.setField( FIX::Symbol( "OC") );
    msg.setField( FIX::Text( "NIGEL") );
    msg.setField( FIX::Side( FIX::Side_BUY ) );
    msg.setField( FIX::SecurityDesc( "AOZ3 C02000") );
    msg.setField( FIX::TimeInForce( FIX::TimeInForce_DAY ) );
    msg.setField( FIX::TransactTime() );
    msg.setField( FIX::SecurityType( FIX::SecurityType_OPTION ) );

    std::string output;
    double tt = 0;
    for(int i = 0 ; i < 1000000; ++i)
    {
        FIX::Message msg;
        msg.getHeader().setField( FIX::BeginString( "FIX.4.4" ) );
        msg.getHeader().setField( FIX::MsgType( FIX::MsgType_NewOrderSingle ) );
        msg.getHeader().setField( FIX::MsgSeqNum(78));
        msg.getHeader().setField( FIX::SenderCompID("A12345B"));
        msg.getHeader().setField( FIX::SenderSubID("2DEFGH4"));
        msg.getHeader().setField( FIX::SendingTime(FIX::UtcTimeStamp()));
        msg.getHeader().setField( FIX::TargetCompID("COMPARO"));
        msg.getHeader().setField( FIX::TargetSubID("G"));
        msg.getHeader().setField( FIX::SenderLocationID("AU,SY"));

        msg.setField( FIX::Account( "01234567") );
        msg.setField( FIX::ClOrdID( "4" ) );
        msg.setField( FIX::OrderQty( 50 ) );
        msg.setField( FIX::OrdType( FIX::OrdType_LIMIT) );
        msg.setField( FIX::Price( 400.5) );
        msg.setField( FIX::HandlInst( '1' ) );
        msg.setField( FIX::Symbol( "OC") );
        msg.setField( FIX::Text( "NIGEL") );
        msg.setField( FIX::Side( FIX::Side_BUY ) );
        msg.setField( FIX::SecurityDesc( "AOZ3 C02000") );
        msg.setField( FIX::TimeInForce( FIX::TimeInForce_DAY ) );
        msg.setField( FIX::TransactTime() );
        msg.setField( FIX::SecurityType( FIX::SecurityType_OPTION ) );

        TickCount start = TickCount::now();
        msg.toString(output);
        TickCount end = TickCount::now();

        tt += (end-start).seconds();
    }

    std::cout << "to string - " << tt << std::endl;

    if( argc > 1 )
    {
      if( FIX::file_exists(argv[1]) )
      {
        FIX::DataDictionary dict(argv[1]);

	TickCount start = TickCount::now();
        for(int i = 0 ; i < 1000000; ++i)
        {
          FIX::Message * tmp = new FIX::Message(output, dict);
          delete tmp;
        }
	TickCount end = TickCount::now();

        std::cout << "from string - " << (end-start).seconds() << std::endl;
      }
      else
        std::cout << "Unable to open dictionary file " << argv[1] << std::endl;
    }
  return 0;
}

#elif defined(FIX8)

// f8 headers
#include "fix8/f8includes.hpp"
#include "fix8/message.hpp"
#include "tbb/tick_count.h"
#include "COMPAROFIX_types.hpp"
#include "COMPAROFIX_router.hpp"
#include "COMPAROFIX_classes.hpp"

using namespace FIX8::COMPAROFIX;
using namespace FIX8;

//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    NewOrderSingle *nos(new NewOrderSingle);
    *nos->Header() << new msg_seq_num(78)
                   << new sender_comp_id("A12345B")
                   << new SenderSubID("2DEFGH4")
                   << new sending_time
                   << new target_comp_id("COMPARO")
                   << new TargetSubID("G")
                   << new SenderLocationID("AU,SY");

    *nos << new TransactTime
         << new Account("01234567")
         << new OrderQty(50)
         << new Price(400.5)
         << new ClOrdID("4")
         << new HandlInst(HandlInst_AUTOEXECPRIV)
         << new OrdType(OrdType_LIMIT)
         << new Side(Side_BUY)
         << new Symbol("OC")
         << new Text("NIGEL")
         << new TimeInForce(TimeInForce_DAY)
         << new SecurityDesc("AOZ3 C02000")
         << new SecurityType(SecurityType_OPTION);

    std::string output, tmp;
    nos->encode(output);
    double tt = 0;

    for(int i = 0 ; i < 1000000; ++i)
    {
        NewOrderSingle *nos(new NewOrderSingle);
        *nos->Header() << new msg_seq_num(78)
                       << new sender_comp_id("A12345B")
                       << new SenderSubID("2DEFGH4")
                       << new sending_time
                       << new target_comp_id("COMPARO")
                       << new TargetSubID("G")
                       << new SenderLocationID("AU,SY");

        *nos  << new TransactTime
              << new Account("01234567")
              << new OrderQty(50)
              << new Price(400.5)
              << new ClOrdID("4")
              << new HandlInst(HandlInst_AUTOEXECPRIV)
              << new OrdType(OrdType_LIMIT)
              << new Side(Side_BUY)
              << new Symbol("OC")
              << new Text("NIGEL")
              << new TimeInForce(TimeInForce_DAY)
              << new SecurityDesc("AOZ3 C02000")
              << new SecurityType(SecurityType_OPTION);

        tbb::tick_count start = tbb::tick_count::now();
        nos->encode(tmp);
        tbb::tick_count end = tbb::tick_count::now();
        tt += (end-start).seconds();

        delete nos;
    }
    std::cout  << "to string- " << tt << std::endl;

    FIX8::Message * msg = NULL;
    tbb::tick_count start = tbb::tick_count::now();
    for(int i = 0 ; i < 1000000; ++i)
    {
        msg = FIX8::Message::factory(FIX8::COMPAROFIX::ctx(), output);
        delete msg;
    }

    tbb::tick_count end = tbb::tick_count::now();
    std::cout << "from string - " <<  (end-start).seconds() << std::endl;
    return 0;
}

#else
#error Please define QUICKERFIX, QUICKFIX or FIX8
#endif
