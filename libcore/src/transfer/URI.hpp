/*  Sirikata Transfer -- Content Transfer management system
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
/*  Created on: Jan 5, 2009 */

#ifndef SIRIKATA_URI_HPP__
#define SIRIKATA_URI_HPP__

#include "util/Sha256.hpp"
#include "Range.hpp" // defines cache_usize_type, cache_ssize_type

namespace Sirikata {
/// URI.hpp: Fingerprint and URI class
namespace Transfer {

/// simple file ID class--should make no assumptions about which hash.
typedef SHA256 Fingerprint;

class URIContext {
	std::string mProto;
	std::string mHost;
	std::string mUser;
	std::string mDirectory; ///< Does not include initial slash, but includes ending slash.
//	AuthenticationCreds mAuth;

	static inline void resolveParentDirectories(std::string &str) {
		std::string::size_type slashpos = 0;
		while (true) {
			slashpos = str.find('/', slashpos);
			if (slashpos != std::string::npos) {
				std::string dir = str.substr(slashpos, slashpos)

			}
			slashpos++;
		}
		/*
		while (str[0]=='.'&&str[1]=='.'&&str[2]=='/') {
			str = str.substr(3);
		}
		std::string::size_type nextdotdot = 0;
		while ((nextdotdot = str.find("../", nextdotdot+1)) != std::string::npos) {

		}
		*/
	}

public:
	URIContext() {
	}

	URIContext(const URIContext &parent,
			const std::string *newProto,
			const std::string *newHost,
			const std::string *newUser,
			const std::string *newDirectory)
		: mProto(newProto?*newProto:parent.proto()),
		  mHost(newHost?*newHost:parent.host()),
		  mUser(newUser?*newUser:parent.user()),
		  mDirectory(newDirectory?*newDirectory:parent.basepath()){
	}
	URIContext(const URIContext &parent, const std::string &identifier)
			: mProto(parent.proto()),
			  mHost(parent.host()),
			  mUser(parent.user()),
			  mDirectory(parent.basepath()) {
		std::string::size_type colonpos = identifier.find(':');
		std::string::size_type startpos = 0;
		if (colonpos != std::string::npos) {
			/* FIXME: Only accept [a-z0-9] as part of the protocol. We don't want an IPv6 address or
			  long filename with a colon in it being misinterpreted as a protocol.
			  Also, port numbers will be preceded by a colon */

			// global path
			mProto = mIdentifier.substr(0, colonpos);
			startpos = colonpos+1;
		}

		if (identifier.length() < startpos+2) {
			return identifier.substr(startpos);
		}

		if (identifier[startpos]=='/' && identifier[startpos+1]=='/') {
			/* FIXME: IPv6 addresses have colons and are surrounded by braces.
			 * Example: "http://someuser@[2001:5c0:1101:4300::1]:8080/somedir/somefile*/

			// protocol-relative path
			mUser = std::string(); // clear out user (set to blank if unspecified)
			//mHost = std::string(); // we actually keep this the same.

			std::string::size_type atpos, slashpos;
			std::string::size_type beginpos = startpos+2;
			slashpos = identifier.find('/',beginpos);
			atpos = identifier.rfind('@', slashpos);
			// Authenticated URI
			if (atpos != std::string::npos && atpos > beginpos && atpos < slashpos) {
				beginpos = atpos+1;
				mUser = identifier.substr(beginpos, atpos-beginpos);
			}
			if (slashpos != beginpos) {
				if (slashpos == std::string::npos) {
					mHost = mIdentifier.substr(beginpos);
				} else {
					mHost = mIdentifier.substr(beginpos, slashpos-beginpos);
				}
			}
			startpos = slashpos;
		}
		if (identifier[startpos]=='/') {
			// server-relative path
			std::string::size_type lastdir = identifier.rfind('/');
			if (lastdir > startpos) {
				mDirectory = identifier.substr(startpos+1, lastdir-startpos);
			} else {
				mDirectory = std::string();
			}
		} else {
			// directory-relative path -- not implemented here
			std::string::size_type lastdir = identifier.rfind('/');
			if (lastdir != std::string::npos && lastdir > startpos) {
				mDirectory += identifier.substr(startpos, lastdir-startpos-1);
			}
		}

		resolveParentDirectories(mDirectory);

	}
};

/// URI stores both a uri string as well as a Fingerprint to verify it.
class URI {
	URIContext mContext;
	std::string mPath;
public:
	URI(const URIContext &parentContext, const std::string &url) {
	}

	/// Returns the protocol used.
	const std::string &proto() const {
		return mContext.proto();
	}

	/// Returns the hostname (or empty if there is none).
	std::string host() const {
	}

	/// const accessor for the full string URI
	inline operator const std::string &() const {
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

class RemoteFileId {
	Fingerprint mHash;
	URI mURI;

public:
	RemoteFileId(const URI &fingerprinturi)
			: mURI(fingerprinturi) {
		const std::string &path = fingerprinturi.path();
		std::string::size_type lastslash = path.rfind('/');
		if (lastslash != std::string::npos) {
			mHash = Fingerprint::fromHex(path.substr(lastslash+1));
		} else {
			mHash = Fingerprint::fromHex(path);
		}
	}

	/// accessor for the hashed value
	inline const Fingerprint &fingerprint() const {
		return mHash;
	}

	/// const accessor for the full string URI
	inline const URI &uri() const {
		return mURI;
	}
	/// accessor for the full URI
	inline URI &uri() {
		return mURI;
	}
};

}
}

#endif /* SIRIKATA_URI_HPP__ */
