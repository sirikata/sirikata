



#ifndef _SIRIKATA_BAMBOO_HPP
#define _SIRIKATA_BAMBOO_HPP


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "gateway_prot.h"


#define BAMBOO_APP_STRING "meru oseg"
#define BAMBOO_CLIB_STRING "meru oseg"
#define BAMBOO_DEFAULT_BFTM_GET_MAXVALS 10

//puts are good for a year
#define BAMBOO_DEFAULT_PUT_TTL 31536000
//rms are instant
#define BAMBOO_DEFAULT_RM_TTL  0

#define BAMBOO_SECRET          "meru_secret"

#define BAMBOO_KEY_SIZE        36
#define BAMBOO_VAL_SIZE        20



class Bamboo
{
  
public:

  typedef char Bamboo_key [BAMBOO_KEY_SIZE];
  typedef char Bamboo_val [BAMBOO_VAL_SIZE];

  
  Bamboo(char* host, int port);
  ~Bamboo();

  void nullCall();
  void remove(Bamboo_key keyVal, Bamboo_val valVal) const;
  void put(Bamboo_key keyVal, Bamboo_val valVal) const;
  bamboo_get_result* get(Bamboo_key keyVal) const;

private:
  struct sockaddr_in addr;
  CLIENT *clnt;
    
  CLIENT* connect_to_server3 (struct sockaddr_in *addr);
  void lookup_server (char *host, int port, struct sockaddr_in *addr);
  void do_null_call3(CLIENT *clnt);  
  void do_bftm_rm3(CLIENT *clnt, bamboo_bftm_rm_arguments *rm_args)                     const;  
  void do_bftm_put3(CLIENT *clnt, bamboo_bftm_put_arguments *put_args)                  const;
  bamboo_get_result* do_bftm_get3 (CLIENT *clnt, bamboo_bftm_get_arguments *get_args)   const;

};

#endif


