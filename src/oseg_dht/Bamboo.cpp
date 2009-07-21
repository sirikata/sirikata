


/*
 * Copyright (c) 2001-2003 Regents of the University of California.
 * All rights reserved.
 *
 * See the file LICENSE included in this distribution for details.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "gateway_prot.h"
#include "Bamboo.hpp"



Bamboo::Bamboo(char* host, int port)
{
  lookup_server (host, port, &addr); //populates the addr variable
  clnt = connect_to_server3 (&addr); //connects client to addr  
  
}

Bamboo::~Bamboo()
{
  //kill the initialized client.
  clnt_destroy (clnt);
}



/*
  Just a simple null call to bamboo server.
*/
void Bamboo::nullCall()
{
  do_null_call3(clnt);
}


/*
Simple get
*/
bamboo_get_result* Bamboo::get(Bamboo_key keyVal) const
{
  bamboo_bftm_get_arguments         get_args;
  bamboo_get_result*              get_result;

  //clears memory for get_args
  memset (&get_args,  0, sizeof (get_args));

  get_args.key = keyVal;

  get_result   = do_bftm_get3(clnt, &get_args);

  return get_result;
}



//note: may also want to make a function that removes values up until the final value.
void Bamboo::remove(Bamboo_key keyVal, Bamboo_val valVal) const
{
  bamboo_bftm_rm_arguments         rm_args;
  
  //clearing rms
  memset (&rm_args,  0, sizeof (rm_args));

  rm_args.key   = keyVal;
  rm_args.value = valVal;


  //put the data into the hash.
  do_bftm_rm3(clnt,&rm_args);

}

void Bamboo::put(char* keyVal, char* valVal) const
{
  bamboo_bftm_put_arguments     put_args;
  memset(&put_args, 0, sizeof(put_args));

  put_args.key    =  keyVal;
  put_args.value  =  valVal;

  do_bftm_put3(clnt, &put_args);
}



/*
  Changed to version 3.
 */
CLIENT* Bamboo::connect_to_server3 (struct sockaddr_in *addr)
{
    CLIENT *clnt;
    int sockp = RPC_ANYSOCK;
    clnt = clnttcp_create (addr, BAMBOO_DHT_GATEWAY_PROGRAM, 
            BAMBOO_DHT_GATEWAY_VERSION_3, &sockp, 0, 0);
    if (clnt == NULL)
    {
        clnt_pcreateerror ("create error");
        exit (1);
    }
    return clnt;
}



void Bamboo::lookup_server (char *host, int port, struct sockaddr_in *addr)
{
    struct hostent *h;
    h = gethostbyname (host); 
    if (h == NULL)
    {
        perror ("gethostbyname");
        exit (1);
    }

    bzero (addr, sizeof (struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons (port);
    addr->sin_addr = *((struct in_addr *) h->h_addr);
}


/*

 */
void Bamboo::do_null_call3 (CLIENT *clnt)
{
    char *null_args = NULL; 
    void *null_result; 

    null_result = bamboo_dht_proc_null_3((void*)&null_args, clnt);
    if (null_result == (void *) NULL) {
        clnt_perror (clnt, "null call failed.");
        exit (1);
    }
}



void Bamboo::do_bftm_rm3(CLIENT *clnt, bamboo_bftm_rm_arguments *rm_args) const
{
  bamboo_stat     *rm_result;

  rm_args->application    =        BAMBOO_APP_STRING;
  rm_args->client_library =       BAMBOO_CLIB_STRING;
  rm_args->ttl_sec        =    BAMBOO_DEFAULT_RM_TTL;
  rm_args->secret         =            BAMBOO_SECRET;
  
  
  rm_result = bamboo_bftm_dht_proc_rm_3 (rm_args, clnt);
  if (rm_result == (bamboo_stat *) NULL)
  {
    clnt_perror (clnt, "bftm_rm3 failed");
    exit (1);
  }

}


void Bamboo::do_bftm_put3(CLIENT *clnt, bamboo_bftm_put_arguments *put_args) const
{
  bamboo_stat     *put_result;

  put_args->application    =         BAMBOO_APP_STRING;
  put_args->client_library =        BAMBOO_CLIB_STRING;
  put_args->ttl_sec        =    BAMBOO_DEFAULT_PUT_TTL;
  put_args->secret         =             BAMBOO_SECRET;

  
  put_result = bamboo_bftm_dht_proc_put_3 (put_args, clnt);
  if (put_result == (bamboo_stat *) NULL)
  {
    clnt_perror (clnt, "bftm_put3 failed");
    exit (1);
  }

}


bamboo_get_result* Bamboo::do_bftm_get3 (CLIENT *clnt, bamboo_bftm_get_arguments *get_args) const
{
  bamboo_get_result  *get_result;

  get_args->application       =                 BAMBOO_APP_STRING;
  get_args->client_library    =                BAMBOO_CLIB_STRING;
  get_args->maxvals           =   BAMBOO_DEFAULT_BFTM_GET_MAXVALS;

  get_result = bamboo_bftm_dht_proc_get_3 (get_args, clnt);
  if (get_result == (bamboo_get_result *) NULL)
  {
    clnt_perror (clnt, "bftm_get3 failed");
    exit (1);
  }
  else
  {
    return get_result;
  }
}
