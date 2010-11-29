/*  Meru
 *  ReplacingDataStream.hpp
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
#ifndef _REPLACING_DATA_STREAM_HPP_
#define _REPLACING_DATA_STREAM_HPP_

#include "../OgreHeaders.hpp"
#include <OgreDataStream.h>
#ifndef STANDALONE
#include <OgreCommon.h>
#endif

#include <sirikata/core/transfer/URI.hpp>

namespace Sirikata {
namespace Graphics {

/**
 * A class that replaces text out of an Ogre DataStream as it is read
 * Currently does the simple thing by reading the entire document as
 * a string whenever data is requested. This is what the parser down
 * the line does anyway in current ogre so it's not an efficiency issue
 * This class may be inherited and replaceData overwritten
 * but the default implementation of replaceData works appropriately
 * for the meru file naming conventions for ogre materials.
 */
class ReplacingDataStream : public Ogre::DataStream{
protected:
  static void find_lexeme (const Ogre::String&input,
                                                Ogre::String::size_type &where_lexeme_start,
                                                Ogre::String::size_type &return_lexeme_end);
  Ogre::String dataAsString;
  Ogre::DataStreamPtr file;
  Ogre::MemoryDataStreamPtr helper;
  std::vector<Ogre::String> provides;//materials and programs this provides
  std::vector<Ogre::String> depends_on;//materials and programs this depends on
  const Ogre::NameValuePairList*mTextureAliases;
    Transfer::URI mSourceURI;
  ///loads in and replaces the data
  void verifyData()const;
    /**
     * This function gets a callback from the ReplacingDataStream whenever a script file name is encountered
     * it is defined as a noop but is used in replace_material tools
     * \param retval will return the name of the material as it is modified by firstOrThirdLevelName
     * \param input is passed from ReplacingDataStream as the entire data
     * \param pwhere is the location where the material was discovered and may be shifted to where the next input should be read if something is returned in retval
     * \param second_input is passed from ReplacingDataStream as where the current material file name was discovered
     * \param filename is the name of the current script which is not used in this context
     */
  virtual void replace_reference(Ogre::String&retval, const Ogre::String&input, Ogre::String::size_type&pwhere,Ogre::String::const_iterator second_input, const Ogre::String&filename);
    /**
     * This function gets a callback from the ReplacingDataStream whenever a source (Vertex Program) or texture file name is encountered
     * it is defined as a noop but is used in replace_material tools
     * \param retval will return the name of the material as it is modified by firstOrThirdLevelName
     * \param input is passed from ReplacingDataStream as the entire data
     * \param pwhere is the location where the material was discovered and may be shifted to where the next input should be read if something is returned in retval
     * \param second_input is passed from ReplacingDataStream as where the current source or texture file name was discovered
     * \param texture_instead_of_source currently unused
     * \param filename is the name of the current script which is not used in this context
     */
  virtual void replace_texture_reference(Ogre::String&retval, const Ogre::String&input, Ogre::String::size_type&pwhere,Ogre::String::const_iterator second_input, bool texture_instead_of_source,const Ogre::String&filename);
    /**
     * This function gets a callback from the ReplacingDataStream whenever a material name depended-on is encountered
     * it uses this callback to replace internal material references with the now-known corresponding hash or third level name
     * Thusly a if a material meru://foo\@bar/baz defined program Foo and used it here then it would transform Foo into meru://foo\@bar/baz:Foo
     * \param input is passed from ReplacingDataStream as the entire data
     * \param where_lexeme_start is passed from ReplacingDataStream as where the current material name was discovered
     * \param return_lexeme_end is returned from this function after find_lexeme is run to tear out the material name
     * \param filename is the name of the current material which is not used in this context
     */
  virtual Ogre::String full_replace_lexeme(const Ogre::String &input,
                                   Ogre::String::size_type where_lexeme_start,
                                   Ogre::String::size_type &return_lexeme_end,
                                   const Ogre::String &filename);
    /**
     * This function gets a callback from the ReplacingDataStream whenever a material name that the material provides is encountered
     * it uses this callback to replace the provided material with a mangling such that
     * if a material meru://foo\@bar/baz defined program Foo and used it later then it would transform Foo into meru://foo\@bar/baz:Foo
     * \param input is passed from ReplacingDataStream as the entire data
     * \param where_lexeme_start is passed from ReplacingDataStream as where the current material name was discovered
     * \param return_lexeme_end is returned from this function after find_lexeme is run to tear out the material when replacing it name
     * \param filename is the name of the current script which is not used in this context
     */

  virtual Ogre::String replace_lexeme(const Ogre::String &input,
                              Ogre::String::size_type where_lexeme_start,
                              Ogre::String::size_type &return_lexeme_end,
                              const Ogre::String &filename);


protected:
/**
 * Replaces items after
 * Naming-type tokens with the DataStream filename and __
 * and it replaces references to those likewise.
 * file 0xbadf00d with material hair is transformed as follows:
 * vertex_program swirl cg { }
 * material hair {
 *   vertex_program_ref swirl;
 *   fragment_program_ref file://0xbeefd00d/hair
 * }
 * becomes
 * vertex_program badf00d__swirl cg{ }
 * material bad00d__hair {
 *   vertex_program_ref badf00d__swirl
 *   fragment_program_ref beefd00d__hair
 * }
 */
  virtual Ogre::String replaceData(Ogre::String);
public:
    ///This function accelerates the precomputation of material mangling and returns depends and provides lists to boot
  void preprocessData(std::vector<Ogre::String>&provides,std::vector<Ogre::String>&depends_on);
    /**
     * The ReplacingDataStream constructs a file that will transparently modify a material as it reads it
     * \param input is another datastream this one piggy-backs on
     * \param destination is the filename that the material will come to represent
     * \param textureAliases is a list of textures thirdname->first name list so that efficient materials can refer to the first names directly
     */
  ReplacingDataStream(Ogre::DataStreamPtr&input,const Ogre::String &destination, const Ogre::NameValuePairList*textureAliases);
  virtual size_t read(void* buf, size_t count);
  virtual Ogre::String getAsString(void);
  virtual void skip(long count);

  virtual void seek( size_t pos );
  virtual size_t tell(void) const;
  virtual bool eof(void) const;
  virtual void close(void);
};

} // namespace Graphics
} // namespace Sirikata

#endif //_REPLACING_DATA_STREAM_HPP_
