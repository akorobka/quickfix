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

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 )
#else
#include "config.h"
#endif

#include "quickfix/FileStore.h"
#include "quickfix/NullStore.h"
#include "quickfix/ThreadedSocketInitiator.h"
#include "quickfix/ThreadedSocketAcceptor.h"
#include "quickfix/SessionSettings.h"
#include "Application.h"
#include <string>
#include <iostream>
#include <fstream>

#include "../../src/getopt-repl.h"

#ifdef _DEBUG
      const size_t NumberOfMessages = 1000;
#else
      const size_t NumberOfMessages = 50000U;
#endif

int main( int argc, char** argv )
{
  const std::string rawMessage = "8=FIX.4.2\0019=3\00135=D\00149=Quick\00156=CME\00134=01\00152=20130809-14:33:01\00111=90001008\00121=1\00155=ABC\00154=1\00138=100\00140=1\00160=20130809-14:33:01\00110=000\001";

  if ( argc != 2 )
  {
    std::cout << "usage: " << argv[ 0 ]
    << " FILE." << std::endl;
    return 0;
  }
  std::string file = argv[ 1 ];

  try
  {
    FIX::SessionSettings settings( file );

    Application application(NumberOfMessages);
    // FIX::FileStoreFactory storeFactory( settings );
    FIX::NullStoreFactory storeFactoryI, storeFactoryA;
    FIX::ScreenLogFactory logFactory( settings );
    FIX::ThreadedSocketAcceptor acceptor( application, storeFactoryA, settings );

    FIX::Message msg(rawMessage, false);

    acceptor.start();
    {
      FIX::ThreadedSocketInitiator initiator( application, storeFactoryI, settings );// */, logFactory );
      FIX::SessionID sid( "FIX.4.2", "Quick", "CME");
      FIX::Session* ps = FIX::Session::lookupSession(sid);

      initiator.start();
  
      while( !application.initialized()) {
          FIX::Util::Sys::SchedYield();
      }
  
      for (size_t i = 0; i < NumberOfMessages; ++i)
      {
        application.acquire();
        ps->send(msg);
      }
      application.acquire();
  
      std::cout << std::endl << std::endl << "received " << application.received() << ", in "
		<< application.elapsed() << " sec, stopping..." << std::endl;
      double* spans = application.spans();
      std::cout << "min: " << spans[0] * 1000000 << " usec";
      std::cout << ", max: " << spans[NumberOfMessages - 1] * 1000000 << " usec";
      std::cout << ", median: " << spans[NumberOfMessages / 2] * 1000000 << " usec" << std::endl;
  
      initiator.stop();
    }
    acceptor.stop();

    return 0;
  }
  catch ( std::exception & e )
  {
    std::cout << e.what();
    return 1;
  }
}
