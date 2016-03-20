#ifndef FIX40_EXECUTIONREPORT_H
#define FIX40_EXECUTIONREPORT_H

#include "Message.h"

namespace FIX40
{

  class ExecutionReport : public Message
  {
    static FIX::MsgType::Pack PackedType() { return FIX::MsgType::Pack("8"); }
  public:
    ExecutionReport() : Message(PackedType()) {}
    ExecutionReport(const FIX::Message& m) : Message(m) {}
    ExecutionReport(const Message& m) : Message(m) {}
    ExecutionReport(const ExecutionReport& m) : Message(m) {}
    static FIX::MsgType MsgType() { return FIX::MsgType("8"); }

    ExecutionReport(
      const FIX::OrderID& aOrderID,
      const FIX::ExecID& aExecID,
      const FIX::ExecTransType& aExecTransType,
      const FIX::OrdStatus& aOrdStatus,
      const FIX::Symbol& aSymbol,
      const FIX::Side& aSide,
      const FIX::OrderQty& aOrderQty,
      const FIX::LastShares& aLastShares,
      const FIX::LastPx& aLastPx,
      const FIX::CumQty& aCumQty,
      const FIX::AvgPx& aAvgPx )
    : Message(PackedType())
    {
      Sequence::push_back_to(*this, aAvgPx);
      Sequence::push_back_to(*this, aCumQty);
      Sequence::push_back_to(*this, aExecID);
      Sequence::push_back_to(*this, aExecTransType);
      Sequence::push_back_to(*this, aLastPx);
      Sequence::push_back_to(*this, aLastShares);
      Sequence::push_back_to(*this, aOrderID);
      Sequence::push_back_to(*this, aOrderQty);
      Sequence::push_back_to(*this, aOrdStatus);
      Sequence::push_back_to(*this, aSide);
      Sequence::push_back_to(*this, aSymbol);
    }

    ExecutionReport(
      const FIX::OrderID::Pack& aOrderID,
      const FIX::ExecID::Pack& aExecID,
      const FIX::ExecTransType::Pack& aExecTransType,
      const FIX::OrdStatus::Pack& aOrdStatus,
      const FIX::Symbol::Pack& aSymbol,
      const FIX::Side::Pack& aSide,
      const FIX::OrderQty::Pack& aOrderQty,
      const FIX::LastShares::Pack& aLastShares,
      const FIX::LastPx::Pack& aLastPx,
      const FIX::CumQty::Pack& aCumQty,
      const FIX::AvgPx::Pack& aAvgPx )
    : Message(PackedType())
    {
      Sequence::push_back_to(*this, aAvgPx);
      Sequence::push_back_to(*this, aCumQty);
      Sequence::push_back_to(*this, aExecID);
      Sequence::push_back_to(*this, aExecTransType);
      Sequence::push_back_to(*this, aLastPx);
      Sequence::push_back_to(*this, aLastShares);
      Sequence::push_back_to(*this, aOrderID);
      Sequence::push_back_to(*this, aOrderQty);
      Sequence::push_back_to(*this, aOrdStatus);
      Sequence::push_back_to(*this, aSide);
      Sequence::push_back_to(*this, aSymbol);
    }

    FIELD_SET(*this, FIX::OrderID);
    FIELD_SET(*this, FIX::ClOrdID);
    FIELD_SET(*this, FIX::ClientID);
    FIELD_SET(*this, FIX::ExecBroker);
    FIELD_SET(*this, FIX::ListID);
    FIELD_SET(*this, FIX::ExecID);
    FIELD_SET(*this, FIX::ExecTransType);
    FIELD_SET(*this, FIX::ExecRefID);
    FIELD_SET(*this, FIX::OrdStatus);
    FIELD_SET(*this, FIX::OrdRejReason);
    FIELD_SET(*this, FIX::Account);
    FIELD_SET(*this, FIX::SettlmntTyp);
    FIELD_SET(*this, FIX::FutSettDate);
    FIELD_SET(*this, FIX::Symbol);
    FIELD_SET(*this, FIX::SymbolSfx);
    FIELD_SET(*this, FIX::SecurityID);
    FIELD_SET(*this, FIX::IDSource);
    FIELD_SET(*this, FIX::Issuer);
    FIELD_SET(*this, FIX::SecurityDesc);
    FIELD_SET(*this, FIX::Side);
    FIELD_SET(*this, FIX::OrderQty);
    FIELD_SET(*this, FIX::OrdType);
    FIELD_SET(*this, FIX::Price);
    FIELD_SET(*this, FIX::StopPx);
    FIELD_SET(*this, FIX::Currency);
    FIELD_SET(*this, FIX::TimeInForce);
    FIELD_SET(*this, FIX::ExpireTime);
    FIELD_SET(*this, FIX::ExecInst);
    FIELD_SET(*this, FIX::Rule80A);
    FIELD_SET(*this, FIX::LastShares);
    FIELD_SET(*this, FIX::LastPx);
    FIELD_SET(*this, FIX::LastMkt);
    FIELD_SET(*this, FIX::LastCapacity);
    FIELD_SET(*this, FIX::CumQty);
    FIELD_SET(*this, FIX::AvgPx);
    FIELD_SET(*this, FIX::TradeDate);
    FIELD_SET(*this, FIX::TransactTime);
    FIELD_SET(*this, FIX::ReportToExch);
    FIELD_SET(*this, FIX::Commission);
    FIELD_SET(*this, FIX::CommType);
    FIELD_SET(*this, FIX::NetMoney);
    FIELD_SET(*this, FIX::SettlCurrAmt);
    FIELD_SET(*this, FIX::SettlCurrency);
    FIELD_SET(*this, FIX::Text);
    FIELD_SET(*this, FIX::NoMiscFees);
    class NoMiscFees: public FIX::Group
    {
    public:
    NoMiscFees() : FIX::Group(136,137,FIX::message_order(137,138,139,0)) {}
      FIELD_SET(*this, FIX::MiscFeeAmt);
      FIELD_SET(*this, FIX::MiscFeeCurr);
      FIELD_SET(*this, FIX::MiscFeeType);
    };
  };

}

#endif
