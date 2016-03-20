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

#ifndef LATENCY_APPLICATION_H
#define LATENCY_APPLICATION_H

#include <queue>
#include <iostream>

#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Values.h"
#include "quickfix/Utility.h"
#include "quickfix/Mutex.h"

#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "quickfix/fix42/MarketDataRequest.h"
#include "quickfix/fix43/MarketDataRequest.h"

class Application
      : public FIX::Application
{
  FIX::Util::Sys::TickCount tick_;
  std::size_t  num_spans_;
  double*	   spans_;
  double	   totalSec_;
  FIX::Util::Sys::Semaphore ready_;
  FIX::AtomicCount logonCount_;
  FIX::AtomicCount receiveCount_;

public:
    Application(std::size_t NumSpans);
  virtual ~Application() { delete [] spans_; }

  // Application overloads
  void onCreate( const FIX::SessionID& ) {}
  void onLogon( const FIX::SessionID& sessionID );
  void onLogout( const FIX::SessionID& sessionID );
  void toAdmin( FIX::Message&, const FIX::SessionID& ) {}
  void toApp( FIX::Message&, const FIX::SessionID& )
  throw( FIX::DoNotSend ) {}
  void fromAdmin( const FIX::Message&, const FIX::SessionID& )
  throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon ) {}
  void fromApp( const FIX::Message& message, const FIX::SessionID& sessionID )
  throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType );

  bool initialized() const { return logonCount_ == 2; }
  void acquire() {
    ready_.wait();
    tick_ = FIX::Util::Sys::TickCount::now();
  }
  void release(int seq) {
    FIX::Util::Sys::TickCount tock = FIX::Util::Sys::TickCount::now();
    double span = (tock - tick_).seconds();
    spans_[seq] = span;
    totalSec_ += span;
    ready_.post();
  } 
  FIX::AtomicCount::value_type received() const { return receiveCount_; }
  double elapsed() const { return totalSec_; }

  double* spans() {
    std::sort(spans_, spans_ + num_spans_);
    return spans_;
  }
};

#endif
