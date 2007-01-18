/**
 * @author Patrick Paul
 * @file paddle_client.cc  Program paddle_client
 *
 *
 * COMMENTS:  Utilise les fonctionalites des stream recoit comme argument un fichier, le # de port et l'addresse du serveur.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/INET_Addr.h"
#include "ace/Log_Msg.h"
#include "ace/Mem_Map.h"
#include <iostream>
#include <sstream>
#include <assert.h>

using namespace std;

void GetHeaderFromStream(ACE_SOCK_Stream &stream, string& output) throw();
long GetTotalLengthValue(string header) throw();

/// Minimum server header lenght
const long NET_SRV_MIN_HEADER_LEN = 17;//<length></length>

int main(int argc, char * argv[]){
  if (argc != 4) {
    cerr << "Usage: paddle_client mapped_file port host" << endl;
    return 0;
  }

  char * host = argv[3];
  static const char * port = argv[2];
  
  ACE_INET_Addr remote_addr(port, host);
  ACE_SOCK_Connector con;
  ACE_SOCK_Stream cli_stream;

  if (con.connect (cli_stream,remote_addr) == -1){
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%P|%t) %p\n",
                       "connection failed"),
                      0);
  }
  else{
    /*
    ACE_DEBUG ((LM_DEBUG,
                "(%P|%t) connecte a %s au port %d\n",
                remote_addr.get_host_name (),
                remote_addr.get_port_number ()));
    */
  }

  //send data
  /*
    ACE_DEBUG ((LM_DEBUG,
              "(%P|%t) Debut de la transmission\n"));
  */

  //mem map file
  ACE_Mem_Map mapped_file (argv[1]);

  long current_len_sent = 0;
  long total_len_sent = 0;
  
  stringstream my_stream;
  my_stream << mapped_file.size();

  string header = "<length>" + my_stream.str() + "</length>";

  iovec iov[2];
  iov[0].iov_base = (char *)header.c_str();
  iov[0].iov_len = header.size();
  iov[1].iov_base = (char *)mapped_file.addr();
  iov[1].iov_len = mapped_file.size();

  current_len_sent = cli_stream.sendv_n(iov, 2);

//  current_len_sent = cli_stream.send_n((char *)mapped_file.addr(), mapped_file.size());
  
  if (current_len_sent == -1){    
    ACE_ERROR ((LM_ERROR,"(%P|%t) send_n returned -1\n"));
    return 1;
  }

  total_len_sent += current_len_sent;
  

  //receive data
  char buf[BUFSIZ];

  //hardcode pour nettoyer le buffer buf....
  memset(buf, 0, sizeof(buf));

  long total_len_rcvd = 0;
  ssize_t r_bytes = 0;
  long total_len_expected = 0;
  string header_str;

  GetHeaderFromStream(cli_stream, header_str);
  total_len_expected = GetTotalLengthValue(header_str);

  string src;


  for (;;){
    r_bytes = cli_stream.recv(buf, sizeof(buf)-1);
      
    if (r_bytes == -1){
      ACE_ERROR ((LM_ERROR,"%p ***ERROR***\n","recv"));
      break;
    }
    else if (r_bytes == 0){
      //ACE_DEBUG ((LM_DEBUG,"(%P|%t) EOT - Received a total of %d bytes\n", total_len_rcvd));
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
  
  //Print value returned
  cout << src;
  
  //ACE_DEBUG ((LM_DEBUG, "(%P|%t) total_len_sent == %d\n", total_len_sent));
  
  //fermer le stream
  cli_stream.close ();

  return 0;
}


void GetHeaderFromStream(ACE_SOCK_Stream &stream, string& output) throw(){
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

long GetTotalLengthValue(string header) throw(){
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
