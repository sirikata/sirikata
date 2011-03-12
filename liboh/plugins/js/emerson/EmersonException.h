#ifndef _EMERSON_EXCEPTION_H_
#define _EMERSON_EXCEPTION_H_

#include <string>


using namespace std;
class EmersonException
{
  private:
  std::string msg_;

  public:
  EmersonException(std::string _msg)
  {
    msg_ = _msg;
  }

  inline std::string msg() { return msg_; }
};


#endif
