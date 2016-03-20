#ifndef FIX41_QUOTEREQUEST_H
#define FIX41_QUOTEREQUEST_H

#include "Message.h"

namespace FIX41
{

  class QuoteRequest : public Message
  {
    static FIX::MsgType::Pack PackedType() { return FIX::MsgType::Pack("R"); }
  public:
    QuoteRequest() : Message(PackedType()) {}
    QuoteRequest(const FIX::Message& m) : Message(m) {}
    QuoteRequest(const Message& m) : Message(m) {}
    QuoteRequest(const QuoteRequest& m) : Message(m) {}
    static FIX::MsgType MsgType() { return FIX::MsgType("R"); }

    QuoteRequest(
      const FIX::QuoteReqID& aQuoteReqID,
      const FIX::Symbol& aSymbol )
    : Message(PackedType())
    {
      Sequence::push_back_to(*this, aSymbol);
      Sequence::push_back_to(*this, aQuoteReqID);
    }

    QuoteRequest(
      const FIX::QuoteReqID::Pack& aQuoteReqID,
      const FIX::Symbol::Pack& aSymbol )
    : Message(PackedType())
    {
      Sequence::push_back_to(*this, aSymbol);
      Sequence::push_back_to(*this, aQuoteReqID);
    }

    FIELD_SET(*this, FIX::QuoteReqID);
    FIELD_SET(*this, FIX::Symbol);
    FIELD_SET(*this, FIX::SymbolSfx);
    FIELD_SET(*this, FIX::SecurityID);
    FIELD_SET(*this, FIX::IDSource);
    FIELD_SET(*this, FIX::SecurityType);
    FIELD_SET(*this, FIX::MaturityMonthYear);
    FIELD_SET(*this, FIX::MaturityDay);
    FIELD_SET(*this, FIX::PutOrCall);
    FIELD_SET(*this, FIX::StrikePrice);
    FIELD_SET(*this, FIX::OptAttribute);
    FIELD_SET(*this, FIX::SecurityExchange);
    FIELD_SET(*this, FIX::Issuer);
    FIELD_SET(*this, FIX::SecurityDesc);
    FIELD_SET(*this, FIX::PrevClosePx);
    FIELD_SET(*this, FIX::Side);
    FIELD_SET(*this, FIX::OrderQty);
    FIELD_SET(*this, FIX::FutSettDate);
    FIELD_SET(*this, FIX::OrdType);
    FIELD_SET(*this, FIX::FutSettDate2);
    FIELD_SET(*this, FIX::OrderQty2);
  };

}

#endif
