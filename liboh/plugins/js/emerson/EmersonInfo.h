#ifndef _EMERSON_INFO_
#define _EMERSON_INFO_

#include <antlr3.h>
#include <string>
#include <stack>
#include "EmersonPlatform.hpp"

class FileInfo
{
  std::string _fileName;
  Emerson::uint32 _line;
  Emerson::uint32 _charPos;

  public:
  FileInfo(){}
  FileInfo(std::string fileName_, Emerson::uint32 line_, Emerson::uint32 charPos_):_fileName(fileName_), _line(line_), _charPos(charPos_){}
  inline std::string fileName() { return _fileName;}
  inline Emerson::uint32 line() { return _line;}
  inline Emerson::uint32 charPos() { return _charPos; }
  inline void lineIs(Emerson::uint32 line_) { _line = line_;}
  inline void charPosIs(Emerson::uint32 charPos_) { _charPos = charPos_ ;}
};

class EmersonInfo
{
  std::stack<FileInfo> _fileStack;
  //void (*errorFunction_)(struct ANTLR3_BASE_RECOGNIZER_struct*, pANTLR3_UINT8*);
  void* errorFunction_;
  void* mismatchTokenFunction_;
  void* mismatchSetFunction_;

  public:
  inline FileInfo& currentFileInfo() { return _fileStack.top();}
  inline FileInfo& fileInfo() {return _fileStack.top();}

  inline void* errorFunction() { return errorFunction_; }
  inline void errorFunctionIs(void(*_errorFunction)(struct ANTLR3_BASE_RECOGNIZER_struct*, pANTLR3_UINT8*) ) { errorFunction_ = (void*)_errorFunction;}

  inline void* mismatchTokenFunction() { return mismatchTokenFunction_;}
  inline void mismatchTokenFunctionIs(void*(*_mismatchTokenFunction)(struct ANTLR3_BASE_RECOGNIZER_struct*, ANTLR3_UINT32, pANTLR3_BITSET_LIST) ) { mismatchTokenFunction_ = (void*)_mismatchTokenFunction;}



  void push(std::string fileName_){
    _fileStack.push(FileInfo(fileName_, 1, 0));
  }

};


#endif
