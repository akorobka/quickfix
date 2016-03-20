#ifndef FIX40_QUOTEREQUEST_H
#define FIX40_QUOTEREQUEST_H

#include "Message.h"

namespace FIX40
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
    FIELD_SET(*this, FIX::Issuer);
    FIELD_SET(*this, FIX::SecurityDesc);
    FIELD_SET(*this, FIX::PrevClosePx);
    FIELD_SET(*this, FIX::Side);
    FIELD_SET(*this, FIX::OrderQty);
  };

}

#endif
