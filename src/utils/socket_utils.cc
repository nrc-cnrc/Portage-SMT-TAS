/**
 * @author Eric Joanis
 * @file socket_utils.cc
 * @brief Utility functions for TCP communications using sockets directly.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#include "socket_utils.h"
#include "portage_defs.h"
#include "errors.h"
#include "str_utils.h"
#include "file_utils.h"
#include <sys/types.h> // for socket
#include <sys/socket.h> // socket()
#include <netdb.h> // getaddrinfo()
#include <errno.h> // errno
#include <cstring> // strerror()

using namespace Portage;
using namespace std;

/// Convenient errno->string conversion
static string strerr() {
   string result;
   if (errno != 0) {
      result = ": ";
      result += strerror(errno);
   }
   return result;
}

SocketUtils::SocketUtils(const string& socket_spec) : remote_description(socket_spec)
{
   vector<string> tokens;
   if (split(socket_spec, tokens, ":") != 2)
      error(ETFatal, "Invalid socket specification: %s; must be HOST:PORT", socket_spec.c_str());

   hostname = tokens[0];
   portno = tokens[1];

   iMagicStream pipe("( uname -n; echo :$PBS_JOBID ) | sed -e 's/.iit.nrc.ca//g' |");
   if (pipe) {
      string line;
      getline(pipe, line);
      trim(line);
      self_description = line;
      getline(pipe, line);
      trim(line);
      self_description += line;
      if (!self_description.empty() && *(self_description.rbegin()) == ':')
         self_description.erase(self_description.end()-1);
      if (! self_description.empty())
         self_description = " (" + self_description + ")";
   }

   // resolve the hostname and port and put it into an appropriate structure
   bzero(&hints, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   if (0 != getaddrinfo(hostname.c_str(), portno.c_str(), &hints, &address))
      error(ETFatal, "Can't resolve %s:%s", hostname.c_str(), portno.c_str());
}

bool SocketUtils::sendRecv(string message, string& response) const
{
   errno = 0;

   // create a socket
   int sockfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
   if (sockfd < 0) {
      error(ETWarn, "Can't create socket" + strerr());
      return false;
   }

   // connect
   if (connect(sockfd, address->ai_addr, address->ai_addrlen) < 0) {
      error(ETWarn, "Error connecting to %s%s",
            remote_description.c_str(), strerr().c_str());
      return false;
   }

   // send the message
   message += "\n";
   const Uint bufsize=max(255, message.length()+1);
   char buffer[bufsize];
   bzero(buffer, bufsize);
   strncpy(buffer, message.c_str(), bufsize);
   int n = send(sockfd, buffer, message.length(), 0);
   if (n < 0) {
      error(ETWarn, "Error writing to socket to %s%s",
            remote_description.c_str(), strerr().c_str());
      return false;
   }
   if (n < int(message.length())) {
      error(ETWarn, "Incomplete write to socket: %u < %u", n, message.length());
      return false;
   }

   // get the answer
   bzero(buffer, bufsize);
   n = read(sockfd, buffer, bufsize-1);
   if (n < 0) {
      error(ETWarn, "Error reading from socket to %s%s",
            remote_description.c_str(), strerr().c_str());
      return false;
   }

   // close the socket - warn on error but don't return false
   if (close(sockfd) != 0)
      error(ETWarn, "Error closing socket to %s%s",
            remote_description.c_str(), strerr().c_str());

   // and pass it on
   response = buffer;
   return true;
}
