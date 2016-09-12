/**
 * @author Eric Joanis
 * @file socket_utils.h
 * @brief Utility functions for TCP communications using sockets directly.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#ifndef _SOCKET_UTILS_H_
#define _SOCKET_UTILS_H_

#include <string>
#include <netdb.h>

namespace Portage {

class SocketUtils {

   std::string hostname;             ///< remote host
   std::string portno;               ///< remote port
   std::string remote_description;   ///< remote description for error reporting
   std::string self_description;     ///< self description for client use in messages

   struct addrinfo hints;            ///< structure for resolving remote host
   struct addrinfo *address;         ///< pointer to resolved remote host

public:

   /**
    * Create a SocketUtils object from a HOST:PORT description.
    *
    * @param socket_spec  String giving HOST:PORT of the remote connection
    */
   explicit SocketUtils(const std::string& socket_spec);

   /**
    * Get a short string describing the local end of the connecion.
    * @return  local host name and PBS_JOBID, if known.
    */
   const std::string& selfDescription() const { return self_description; }

   /**
    * Get a short string describing the remote end of the connecion.
    * @return  host:port as given as socket_spec to the constructor
    */
   const std::string& remoteDescription() const { return remote_description; }

   /**
    * Send a short message to this SocketUtils's host and port, and get a short response back.
    *
    * The message and the response should be no longer than 255 characters.
    * This function will accept longer messages, but has not been tested with
    * and is not designed for long messages.
    *
    * @param message        message to send; a newline will automatically be added at the end.
    * @param[out] response  will hold response from server, if any, including newlines.
    *
    * @return true iff full communication cycle is successful
    */
   bool sendRecv(std::string message, std::string& response) const;

};


} // Portage

#endif // _SOCKET_UTILS_H_
