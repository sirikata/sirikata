/*     Iridium Transfer -- Content Transfer management system
 *  URI.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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
 *  * Neither the name of Iridium nor the names of its contributors may
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
/*  Created on: Jan 5, 2009 */

#ifndef IRIDIUM_URI_HPP__
#define IRIDIUM_URI_HPP__

#include <string>
#include "util/Sha256.hpp"

namespace Iridium {
/// URI.hpp: Fingerprint and URI class
namespace Transfer {

/// simple file ID class--should make no assumptions about which hash.
typedef SHA256 Fingerprint;

/// URI stores both a uri string as well as a Fingerprint to verify it.
class URI {
	Fingerprint mHash;
	std::string mIdentifier;
public:
	URI(const Fingerprint &hash, const std::string &url)
			: mHash(hash), mIdentifier(url) {
	}

	/// accessor for the hashed value
	inline const Fingerprint &fingerprint() const {
		return mHash;
	}

	/// Returns the protocol used.
	std::string proto() const {
		std::string::size_type pos = mIdentifier.find(':');
		if (pos == std::string::npos) {
			return std::string();
		} else {
			return mIdentifier.substr(0, pos);
		}
	}

	/// Returns the hostname (or empty if there is none).
	std::string host() const {
		std::string::size_type pos, atpos, slashpos, beginpos;
		pos = mIdentifier.find("://");
		if (pos == std::string::npos) {
			return std::string();
		} else {
			slashpos = mIdentifier.find('/', pos+3);
			atpos = mIdentifier.rfind('@', slashpos);
			// Authenticated URI
			if (atpos > pos && atpos < slashpos) {
				beginpos = atpos+1;
			} else {
				beginpos = pos+3;
			}
			return mIdentifier.substr(pos, slashpos-beginpos);
		}
	}

	/// const accessor for the full string URI
	inline const std::string &uri() const {
		return mIdentifier;
	}
	/// const accessor for the full str
	inline std::string &uri() {
		return mIdentifier;
	}

	/// Compare the URI only (to compare hash, use .fingerprint())
	inline bool operator<(const URI &other) const {
		// We can ignore the hash if it references the same URL.
		return (mIdentifier < other.mIdentifier);
	}

	/// Check the URI for equality, (to compare hash, use .fingerprint())
	inline bool operator==(const URI &other) const {
		// We can ignore the hash if it references the same URL.
		return (mIdentifier == other.mIdentifier);
	}

};

/// Display both the URI string and the corresponding Fingerprint.
inline std::ostream &operator<<(std::ostream &str, const URI &uri) {
	//" (hash " << uri.fingerprint() << ")" <<
	return str << uri.uri();
}

}
}

#endif /* IRIDIUM_URI_HPP__ */
