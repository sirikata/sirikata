/*  Meru
 *  ReplacingDataStream.cpp
 *
 *  Copyright (c) 2009, Stanford University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ReplacingDataStream.hpp"

#include "CDNArchive.hpp"
#include <boost/regex.hpp>
#include <vector>

namespace Sirikata {
namespace Graphics {

using namespace Sirikata::Transfer;

namespace MangleTextureName {
const static std::string UScor="_";
const static std::string DQuot="\"";
}

static String mangleTextureName(const String&resourceName, const String&materialName) {
    if (materialName.length()>2&&materialName[0]=='\"'&&materialName[materialName.length()-1]=='\"') {
        return mangleTextureName(resourceName,materialName.substr(1,materialName.length()-2));
    }
    using namespace MangleTextureName;
    String retval=resourceName;
    String::size_type where=0;
    while ((where=resourceName.find_first_of("\"",where))!=String::npos) {
        retval[where]='_';
    }
    return retval+=':'+materialName;
}

ReplacingDataStream::ReplacingDataStream(Ogre::DataStreamPtr &input, const Ogre::String &destination,const Ogre::NameValuePairList*textureAliases):DataStream(destination),file(input) {
    this->mTextureAliases=textureAliases;
    if (mTextureAliases) {
        Ogre::NameValuePairList::const_iterator where=mTextureAliases->find("");
        if (where!=mTextureAliases->end()) {
            this->mSourceURI = URI(where->second);
        }
    }
}
template <class MemoryBuffer> bool tnext_eol(const MemoryBuffer &input,
                                             typename MemoryBuffer::size_type &where_lexeme_start) {
    size_t size=input.size();
    typename MemoryBuffer::const_iterator iter = input.begin()+where_lexeme_start;

    if (*iter=='\"') {
        ++where_lexeme_start;//can't start on quote
        ++iter;
    }
    for (typename MemoryBuffer::size_type i=where_lexeme_start;
         i<size;
         ++i) {
        char cur=*iter;
        ++iter;
        char next=*iter;
        if (cur=='\r'||cur=='\n') {
            return true;
        }else if (!isblank(cur)) {
            if (cur=='/'&&where_lexeme_start+1<size&&next=='/')
                return true;
            return false;
        }
    }
    return true;
}
#ifndef STANDALONE
bool next_eol(const MemoryBuffer&input,MemoryBuffer::size_type&where_lexeme_start) {
    return tnext_eol(input,where_lexeme_start);
}
#endif
bool next_eol(const Ogre::String&input,Ogre::String::size_type&where_lexeme_start) {
    return tnext_eol(input,where_lexeme_start);
}

void ReplacingDataStream::find_lexeme (const Ogre::String&input,
                                       Ogre::String::size_type &where_lexeme_start,
                                       Ogre::String::size_type &return_lexeme_end) {
  return_lexeme_end=input.length();
  /*if (where_lexeme_start+1<return_lexeme_end) {
    if (!isspace(input[where_lexeme_start])) {
      if (input[where_lexeme_start]!='/') {
        if (input[where_lexeme_start+1]!='/') {
          return_lexeme_end=where_lexeme_start;//no space afterwards must be part of a bigger word
          return;
        }
      }
    }
    }*/
  while (where_lexeme_start!=return_lexeme_end) {
    char c =input[where_lexeme_start];
    if (!isspace(c)) {
      if (c=='/'&&where_lexeme_start+1!=return_lexeme_end&&input[where_lexeme_start+1]=='/') {
        //comment: find end of line
        while (where_lexeme_start++!=return_lexeme_end) {
          if (input[where_lexeme_start]=='\n')
            break;
        }
      }else{
        break;//beginning our quest for the end token
      }
    }
    ++where_lexeme_start;
  }
  return_lexeme_end=where_lexeme_start;
  bool comment_immunity=false;
  while (return_lexeme_end<input.length()) {
    char c =input[return_lexeme_end];
    if (c=='\"') {
        comment_immunity=!comment_immunity;
        //if (return_lexeme_end==where_lexeme_start)
        //    ++where_lexeme_start;
    }
    if (isspace(c)||c=='{'||(c=='/'&&comment_immunity==false&&return_lexeme_end+1<input.length()&&input[return_lexeme_end+1]=='/'&&(return_lexeme_end==where_lexeme_start||input[return_lexeme_end-1]!=':'))) {
      //if (return_lexeme_end>where_lexeme_start)
      //  --return_lexeme_end;
      break;
    }
    ++return_lexeme_end;
  }
  //if (return_lexeme_end>where_lexeme_start&&input[return_lexeme_end-1]=='\"') {
  //    --return_lexeme_end;
  //}
}
Ogre::String& unreplaceColonSlashSlash(Ogre::String &input) {
    return input;
    Ogre::String::size_type where=0;
    while ((where=input.find("___",where))!=Ogre::String::npos) {
        input[where]=':';
        input[where+1]='/';
        input[where+2]='/';
    }
    where=0;
    while ((where=input.find_first_of(1,where))!=Ogre::String::npos) {
        input[where]=':';
    }
    return input;
}
Ogre::String& replaceColonSlashSlash(Ogre::String& input) {
    return input;
    Ogre::String::size_type where=0;


    while ((where=input.find("://",where))!=Ogre::String::npos) {
        input[where]='_';
        input[where+1]='_';
        input[where+2]='_';
    }
    where=0;
    while ((where=input.find_first_of(":",where))!=Ogre::String::npos) {
        input[where]=1;
    }
    return input;
}
Ogre::String ReplacingDataStream::replace_lexeme(const Ogre::String &input,
                                                 Ogre::String::size_type where_lexeme_start,
                                                 Ogre::String::size_type &return_lexeme_end,
                                                 const Ogre::String &filename) {
  find_lexeme(input,where_lexeme_start,return_lexeme_end);
  if (where_lexeme_start<return_lexeme_end) {
      Ogre::String provided=mangleTextureName(filename,input.substr(where_lexeme_start,return_lexeme_end-where_lexeme_start));
      provides.push_back(provided);
      return "\""+(replaceColonSlashSlash(provided)+"\"");
  }else {
    return Ogre::String();
  }
}


///////////// FIXME: Hardcoded reference to "mhash:"

static const char* MERU_URI_HASH_SCHEME = ("mhash:");

static std::string possibleMhashCanonicalization(const std::string&input) {
    unsigned int MERU_URI_HASH_SCHEME_LENGTH=strlen(MERU_URI_HASH_SCHEME);
    if (input.length()>2&&input[0]=='\"'&&input[input.length()-1]=='\"') {
        return '\"'+possibleMhashCanonicalization(input.substr(1,input.length()-2))+'\"';
    }
    if (input.length()>=MERU_URI_HASH_SCHEME_LENGTH) {
        for (unsigned int i=0;i<MERU_URI_HASH_SCHEME_LENGTH;++i) {
            if (input[i]!=MERU_URI_HASH_SCHEME[i])
                return input;
        }
    }
    std::string::size_type wherecolon=input.rfind(':');
    if (wherecolon==std::string::npos||wherecolon<MERU_URI_HASH_SCHEME_LENGTH)
        return CDNArchive::canonicalizeHash(input);
    std::string retval(CDNArchive::canonicalizeHash(input.substr(0,wherecolon))+input.substr(wherecolon));
    return retval;
}

Ogre::String ReplacingDataStream::full_replace_lexeme(const Ogre::String &input,
                                                      Ogre::String::size_type where_lexeme_start,
                                                      Ogre::String::size_type &return_lexeme_end,
                                                      const Ogre::String &filename) {
  find_lexeme(input,where_lexeme_start,return_lexeme_end);
  if (where_lexeme_start<return_lexeme_end) {
      Ogre::String depended=input.substr(where_lexeme_start,return_lexeme_end-where_lexeme_start);
      if (depended.find_first_of('*')==Ogre::String::npos) {
          if (depended.find("://")==Ogre::String::npos) {
              depended=mangleTextureName(filename,depended);
              return "\""+possibleMhashCanonicalization(replaceColonSlashSlash(depended))+"\"";
          }else {
              return possibleMhashCanonicalization(replaceColonSlashSlash(depended));
          }
      }else {
          return possibleMhashCanonicalization(depended);
      }
  }else {
      return Ogre::String();
  }
}

static boost::regex mondo_regex("(?:^(?:\t|[[:space:]])*(material[[:space:]]+|vertex_program[[:space:]]+|fragment_program[[:space:]]+))|(?:^(?:\t|[[:space:]])*(vertex_program_ref[[:space:]]+|fragment_program_ref[[:space:]]+|shadow_caster_vertex_program_ref[[:space:]]+|shadow_receiver_vertex_program_ref[[:space:]]+|shadow_receiver_fragment_program_ref[[:space:]]+|delegate[[:space:]]+))|(?:(?:\t|[[:space:]])*(import[[:space:]].*?from[[:space:]]+))|(?:^(?:\t|[[:space:]])*(source[[:space:]]+))|(?:^(?:\t|[[:space:]])*(texture[[:space:]]+))|(?:^(?:\t|[[:space:]])*(anim_texture[[:space:]]+|cubic_texture[[:space:]]+))");
  //"([[:word:]])"
  //  "([[:space:]]|//)";
const static std::string myquote("\"");
void ReplacingDataStream::replace_reference(Ogre::String&retval, const Ogre::String&input, Ogre::String::size_type&pwhere,Ogre::String::const_iterator second_input, const Ogre::String&filename) {
      Ogre::String::size_type lexeme_start=second_input-input.begin(),return_lexeme_end;

      find_lexeme(input,lexeme_start,return_lexeme_end);
      if (lexeme_start<return_lexeme_end) {
          Ogre::String dep=input.substr(lexeme_start,return_lexeme_end-lexeme_start);
          retval+=input.substr(pwhere,lexeme_start-pwhere);
          String uri;
          if (dep.length() > 2 && dep[0]=='\"') {
            uri = dep.substr(1,dep.length()-2);
          }else {
            uri = dep;
          }
          if (mTextureAliases) {
            Ogre::String absURI(URI(mSourceURI.getContext(), uri).toString());
            Ogre::NameValuePairList::const_iterator where=mTextureAliases->find(absURI);
            if (where!=mTextureAliases->end()) {
              uri=where->second;
            }
          }
          if (dep.length() > 2 && dep[0]=='\"') {
              dep=myquote+CDN_REPLACING_MATERIAL_STREAM_HINT+uri+myquote;
          }else {
              dep=CDN_REPLACING_MATERIAL_STREAM_HINT+uri;
          }
          retval+=dep;
          pwhere=return_lexeme_end;
      }
}
void ReplacingDataStream::replace_texture_reference(Ogre::String&retval, const Ogre::String&input, Ogre::String::size_type&pwhere,Ogre::String::const_iterator second_input, bool texture_instead_of_source, const Ogre::String&filename) {
  if (mTextureAliases) {
    Ogre::String::size_type lexeme_start=second_input-input.begin(),return_lexeme_end;
    find_lexeme(input,lexeme_start,return_lexeme_end);
    if (lexeme_start<return_lexeme_end) {
      String dep=input.substr(lexeme_start,return_lexeme_end-lexeme_start);
      String uri;
      if (dep.length() > 2 && dep[0]=='\"') {
        uri = dep.substr(1,dep.length()-2);
      }else {
        uri = dep;
      }
      Ogre::String absURI(URI(mSourceURI.getContext(), uri).toString());
      Ogre::NameValuePairList::const_iterator where=mTextureAliases->find(absURI);
      if (where!=mTextureAliases->end()) {
        retval+=input.substr(pwhere,lexeme_start-pwhere);
        if (dep.length() > 2 && dep[0]=='\"') {
          retval+=myquote;
        }
        retval+=where->second;
        if (dep.length() > 2 && dep[0]=='\"') {
          retval+=myquote;
        }
        pwhere=return_lexeme_end;
      }
    }
  }
}
Ogre::String::size_type find_space_colon_space(const Ogre::String &input, Ogre::String::size_type start) {
    Ogre::String::size_type len=input.length();
    bool ok1=false,ok2=false,ok3=false;
    while (start<len&&isspace(input[start])) {
        start++;
        ok1=true;
    }
    if (start<input.length()&&input[start]==':') {
        ok2=true;
        start++;
    }
    while (start<len&&isspace(input[start])) {
        start++;
        ok3=true;
    }
    if (ok1&&ok2&&ok3) return start;
    return 0;
}


Ogre::String ReplacingDataStream::replaceData(Ogre::String input) {
  std::string::const_iterator start, end;
  start = input.begin();
  end = input.end();
  boost::match_results<std::string::const_iterator> what;
  boost::match_flag_type flags = boost::match_default;
  Ogre::String midval;
  Ogre::String::size_type pwhere=0,temp_size;


  while(boost::regex_search(start, end, what, mondo_regex, flags)) {
    start=what[0].second;
    if (what[1].matched) {//provides
      midval+=input.substr(pwhere,(what[0].second-input.begin())-pwhere);

      midval+=replace_lexeme(input,what[0].second-input.begin(),pwhere,mName);
      start=input.begin()+pwhere;

      if ((temp_size=find_space_colon_space(input,pwhere))) {//depends on material
          midval+=input.substr(pwhere,temp_size-pwhere);

          midval+=full_replace_lexeme(input,temp_size,pwhere,mName);
          start=input.begin()+pwhere;
      }
    }else if (what[3].matched) {//depends on imported file
        replace_reference(midval,input,pwhere,what[0].second,mName);
    }else if (what[4].matched) {//depends on source file
        replace_texture_reference(midval,input,pwhere,what[0].second,false,mName);
    }else if (what[5].matched) {//depends on texture file
        replace_texture_reference(midval,input,pwhere,what[0].second,true,mName);
    }else if (what[6].matched) {//depends on anim_ or cubic_ files
        Ogre::String::size_type whereend=what[0].second-input.begin();
        //kwhere keeps track of location in stream without modifying the pwhere
        //it just seeks through the line until it finds the end....and if the replace_texture_reference modifies pwhere kwhere is updated to match
        Ogre::String::size_type kwhere=whereend;
        while(whereend=kwhere,find_lexeme(input,kwhere,whereend),!next_eol(input,whereend)) {
            Ogre::String::size_type wherestart=kwhere;
            Ogre::String::size_type oldpwhere=pwhere;
            replace_texture_reference(midval,input,pwhere,input.begin()+kwhere,true,mName);
            if (oldpwhere==pwhere) {
                find_lexeme(input,wherestart,kwhere);
            }else {
                kwhere=pwhere;
            }
        }
    }else if (what[2].matched) {//depends on material
      midval+=input.substr(pwhere,(what[0].second-input.begin())-pwhere);

      midval+=full_replace_lexeme(input,what[0].second-input.begin(),pwhere,mName);
      start=input.begin()+pwhere;
    }
  }
  midval+=input.substr(pwhere);

  start=midval.begin();
  end=midval.end();
  std::string retval;
  std::string cur_expr;
  size_t num_texture_aliases=mTextureAliases?mTextureAliases->size():0;
  if (num_texture_aliases!=0) {
      for (Ogre::NameValuePairList::const_iterator iter=mTextureAliases->begin(),iterend=mTextureAliases->end();iter!=iterend;++iter) {
          if (iter->first.empty()) {
            continue;
          }
          if (cur_expr.length())
              cur_expr+='|';
          cur_expr+='(';
          cur_expr+='\\';
          cur_expr+='Q';
          cur_expr+=iter->first;
          cur_expr+='\\';
          cur_expr+='E';
          cur_expr+=')';
      }

      boost::regex expression(cur_expr);
      pwhere=0;
      while(boost::regex_search(start, end, what, expression, flags)) {
          start=what[0].second;

          for (size_t i=1;i<=num_texture_aliases;++i) {
              if (what[i].matched) {
                  Ogre::String absURI(URI(mSourceURI.getContext(), String(what[i].first,what[i].second)).toString());
                  Ogre::NameValuePairList::const_iterator where=mTextureAliases->find(absURI);
                  if (where!=mTextureAliases->end()) {
                      retval+=midval.substr(pwhere,(what[0].first-midval.begin())-pwhere);
                      retval+=where->second;
                      start=what[0].second;
                      pwhere=start-midval.begin();
                  }else {
                      SILOG(resource,error, "Unable to find string "<<Ogre::String(what[i].first,what[i].second)<<" in NameValuePairList");
                  }
                  break;
              }
          }
      }
      retval+=midval.substr(pwhere);
  }else {
      return midval;
  }


  return retval;
}
void ReplacingDataStream::verifyData() const{
  if (helper.isNull()) {
    ReplacingDataStream*thus=const_cast<ReplacingDataStream*>(this);//initialize cache in a way that violates constness
    thus->dataAsString=thus->replaceData(thus->file->getAsString());
    thus->mSize=dataAsString.length();
/*
    char filename[1024];
    static int blah=0;
    sprintf(filename,"d:\\vw\\win32meru\\%d.txt",blah++);
    FILE * fp=fopen (filename,"wb");
    fwrite(dataAsString.data(),thus->mSize,1,fp);
    fclose(fp);
*/
    Ogre::MemoryDataStreamPtr newhelper(new Ogre::MemoryDataStream((void*)dataAsString.data(),mSize));
    thus->helper=newhelper;
  }
}
void ReplacingDataStream::preprocessData(std::vector<Ogre::String>&provides,std::vector<Ogre::String>&depends_on) {
  verifyData();
  provides=this->provides;
  depends_on=this->depends_on;
}
Ogre::String ReplacingDataStream::getAsString() {
  verifyData();
  return dataAsString;
}
size_t ReplacingDataStream::read(void* buf, size_t count){
  verifyData();
  return helper->read(buf,count);
}
void ReplacingDataStream::skip(long count) {
  verifyData();
  helper->skip(count);
}

void ReplacingDataStream::seek( size_t pos ) {
  verifyData();
  helper->seek(pos);
}
size_t ReplacingDataStream::tell(void) const{
  verifyData();
  return helper->tell();
}
bool ReplacingDataStream::eof(void) const {
  verifyData();
  return helper->eof();
}
void ReplacingDataStream::close(void) {
  helper->close();
  file->close();
  dataAsString=Ogre::String();
}

} // namespace Graphics
} // namespace Sirikata
