/**
 * @author Patrick Paul
 * @file net_srv.cc  Implementation of Net_srv
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#include "net_srv.h"

#include "ace/SOCK_Stream.h"
#include "ace/Message_Block.h"
#include <sstream>

using namespace Portage;

Net_srv::Net_srv(Translator_if * translator_instance, unsigned int port) throw():
  pmTr_engine(translator_instance),
  mPort(port),
  mAcceptor(),
  mHandle_set()
{
  // Raise the socket handle limit to the maximum.
  ACE::set_handle_limit ();

  // Create the server addresses.
  ACE_INET_Addr server_addr (mPort);

  // Create acceptors, reuse the address.
  if (mAcceptor.open (server_addr, 1) == -1){
    ACE_ERROR ((LM_ERROR,"%p\n","open"));
    ACE_ASSERT (false);
  }

  // Check to see what addresses we actually got bound to!
  else if (mAcceptor.get_local_addr (server_addr) == -1){
    ACE_ERROR ((LM_ERROR,"%p\n","get_local_addr"));
    ACE_ASSERT (false);
  }

  ACE_DEBUG ((LM_DEBUG,"(%P|%t) Starting server at port %d \n", server_addr.get_port_number ()));

  mHandle_set.set_bit (mAcceptor.get_handle ());
}


Net_srv::~Net_srv() throw(){
  pmTr_engine=NULL;
}      


void Net_srv::run() throw(){
  // Keep these objects out here to prevent excessive constructor
  // calls within the loop.
  ACE_SOCK_Stream new_stream;

  while(true){
    if (mHandle_set.is_set (mAcceptor.get_handle ())){
      if (mAcceptor.accept (new_stream) == -1){
	ACE_ERROR ((LM_ERROR,"%p\n","accept"));
	continue;
      }
      else{
	ACE_DEBUG ((LM_DEBUG,"(%P|%t) Connection received\n"));
      }
      
      // Run le server. traite la connection
      HandleRequest (new_stream.get_handle ());
    }
  }
}


void Net_srv::HandleRequest (ACE_HANDLE handle) throw(){
  ACE_INET_Addr cli_addr;
  ACE_SOCK_Stream new_stream;
  new_stream.set_handle (handle);
  string src;
  string tgt;

  if (new_stream.get_remote_addr (cli_addr) == -1)
    ACE_ERROR ((LM_ERROR,"%p\n","get_remote_addr"));

  ACE_DEBUG ((LM_DEBUG,"(%P|%t) Client %s connected on port %d\n",
              cli_addr.get_host_name (),
              cli_addr.get_port_number ()));

  char buf[BUFSIZ];

  //hardcode pour nettoyer le buffer buf....
  memset(buf, 0, sizeof(buf));

  long total_len_rcvd = 0;
  ssize_t r_bytes = 0;  
  long total_len_expected = 0;
  //  stringstream len_str;
  string header_str;

  //bool header_found = false;
  //char* loc_in_buf = &buf[0];
  
  GetHeaderFromStream(new_stream, header_str);
  total_len_expected = GetTotalLengthValue(header_str);

  for (;;){
    r_bytes = new_stream.recv(buf, sizeof(buf)-1);
      
    if (r_bytes == -1){
      ACE_ERROR ((LM_ERROR,"%p\n","recv"));
      break;
    }
    else if (r_bytes == 0){
      ACE_DEBUG ((LM_DEBUG,"(%P|%t) EOT - Received a total of %d bytes\n", total_len_rcvd));
      r_bytes = 0;
      break;
    }
    else{
      //Réception partielle.
      //ACE_DEBUG ((LM_DEBUG,"(%P|%t) RCVD:%s", buf));
      src += buf;
      total_len_rcvd += r_bytes;
      r_bytes = 0;
      //clear buf to make sure we are not concatenating old data back into the src string
      memset(buf, 0, sizeof(buf));
      if(total_len_rcvd == total_len_expected){
	total_len_expected = 0;
	break;
      }
    }
  }  

  pmTr_engine->translate(src, tgt);

  long current_len_sent = 0;

  stringstream my_stream;
  my_stream << tgt.size();

  string header = "<length>" + my_stream.str() + "</length>";

  iovec iov[2];
  iov[0].iov_base = (char *)header.c_str();
  iov[0].iov_len = header.size();
  iov[1].iov_base = (char *)tgt.c_str();
  iov[1].iov_len = tgt.size();
  
  current_len_sent = new_stream.sendv_n(iov, 2);
  
  //return translated string
  //  if (new_stream.send (tgt.c_str(), tgt.size()+1) == -1){
  //    ACE_ERROR ((LM_ERROR,"%p\n","send_n"));
  //  }
  
  ACE_DEBUG ((LM_DEBUG,"\n(%P|%t) Request handled, closing connection to client\n"));
  new_stream.close ();
}

void Net_srv::GetHeaderFromStream(ACE_SOCK_Stream &stream, string& output) throw(){
  string s;
  ssize_t r_bytes = 0;
  char buf[BUFSIZ];
  char * loc_in_buf = &buf[0];

  //first clear buffer
  memset(buf, 0, sizeof(buf));
  
  //Read minimum size of header
  r_bytes = stream.recv(loc_in_buf, NET_SRV_MIN_HEADER_LEN);
  assert (r_bytes == NET_SRV_MIN_HEADER_LEN);
  r_bytes = 0;
  loc_in_buf += NET_SRV_MIN_HEADER_LEN;

  //complete until we find '>'
  do{
    r_bytes = stream.recv(loc_in_buf, 1);
    r_bytes = 0;
    ++loc_in_buf;
  }
  while(*(loc_in_buf-1) != '>');

  //Our internal pointer now points at the end of the relevant string
  s = buf;

  //Clear memory before returning
  memset(buf, 0, sizeof(buf));
  loc_in_buf = NULL;
  output = s;
}

long Net_srv::GetTotalLengthValue(const string& header) throw(){
  long retv = 0;
  string::size_type beg_num;
  string::size_type end_num;
  string starting_markup = "<length>";

  beg_num = starting_markup.size();
  end_num = header.find("</length>",0);

  string temp = header.substr(beg_num, end_num-beg_num);

  //  ACE_DEBUG ((LM_DEBUG,"\n(%P|%t) HEADER: %s\n", header.c_str()));
  //  ACE_DEBUG ((LM_DEBUG,"\n(%P|%t) beg=%d end=%d\n", beg_num, end_num));
  //  ACE_DEBUG ((LM_DEBUG,"\n(%P|%t) SUBSTR: %s\n", temp.c_str()));

  retv = strtol(temp.c_str(), NULL, 10);

  //  ACE_DEBUG ((LM_DEBUG,"\n(%P|%t) Converted value = %d\n", retv));
  return retv;
}
