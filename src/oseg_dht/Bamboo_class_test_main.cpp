

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "gateway_prot.h"

#include "Bamboo.hpp"
#include <iostream>
#include <iomanip>


int main (int argc, char** argv)
{
  struct sockaddr_in addr;

  //  Bamboo bambi           ("127.0.0.1", 3632);
  Bamboo bambi           ("bmistree.stanford.edu", 3632);
  bamboo_get_result*              get_result;

  
  bambi.put("aaa","rosebud");
  bambi.put("aaa","redrum");


  get_result = bambi.get("aaa");
  
  std::cout<<"\n \tNumber of responses gotten:  "<<get_result->values.values_len<<"\n";
  for (int s= 0; s < get_result->values.values_len; ++s)
  {
    std::cout<<"\n\t\tResponse gotten: "<<get_result->values.values_val[s].value.bamboo_value_val ;
  }
  std::cout<<"\n\n\n";

  
  bambi.remove("aaa","rosebud");
  bambi.remove("aaa","redrum");
  

  return 0;
}






