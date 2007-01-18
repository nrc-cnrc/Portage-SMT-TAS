/**
 * @author Patrick Paul
 * @file net_srv.h  Translation server.
 *
 * $Id$
 * 
 * Class that implements Portage basic interface
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada 
 * 
 * Contains the definition for the network server functionality.
 */
 
#ifndef __NET_SRV__
#define __NET_SRV__

#include "translator_if.h"

// This variables in ace conflict with variables named the same in log4cxx, but
// we don't use them, so we just undef them here.
#undef PACKAGE_VERSION
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME

#include "ace/Svc_Handler.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Handle_Set.h"

// Undef them again, so net_srv.h can be included before or after logging.h.
#undef PACKAGE_VERSION
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME

namespace Portage {

/// Minimal server header length
const long NET_SRV_MIN_HEADER_LEN = 17;//<length></length>

/// Server side implementation that processes translation requests.
class Net_srv
{
  public:
    /**
     * Construct.
     * @param translator_instance reference to an object that implements the translator_if interface
     * @param port to listen on
     */   
    Net_srv(Translator_if * translator_instance, unsigned int port) throw();
    /// Destructor.
    ~Net_srv() throw();      
  
    /// Start running the server
    void run() throw();
  
  private:
    /**
     * Translates the received request and sends back its translation.
     * @param handle handle identifying the request 
     */
    void HandleRequest(ACE_HANDLE handle) throw();
    /**
     * Retrieves the header from a request
     * @param stream  stream containing a request with an header
     * @param output  returned retrieved header
     */
    void GetHeaderFromStream(ACE_SOCK_Stream &stream, string& output) throw();
    /**
     * Extracts the body's length from the header
     * @param header  request's header
     * @return Returns the body's length
     */
    long GetTotalLengthValue(const string& header) throw();
  
    Translator_if *    pmTr_engine;  ///< Translator instance
    unsigned int       mPort;        ///< Listening port number
    ACE_SOCK_Acceptor  mAcceptor;    ///< Connections acceptor
    ACE_Handle_Set     mHandle_set;  ///< Keeps track of the active connections
};// END CLASS DEFINITION Net_srv
}; // ends namespace Portage
#endif // __NET_SRV__
