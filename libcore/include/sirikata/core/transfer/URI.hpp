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

#include <sirikata/core/util/Sha256.hpp>
#include <sirikata/core/transfer/Range.hpp> // defines cache_usize_type, cache_ssize_type
#ifdef _WIN32
#include <locale>
#endif
namespace Sirikata {
/// URI.hpp: Fingerprint and URI class
namespace Transfer {

/// simple file ID class--should make no assumptions about which hash.
typedef SHA256 Fingerprint;

/** Contains the current directory, hostname, protocol and user.
 * The idea is that you can pass this along with a string into the
 * URI constructor, and it will compute a new URI based on relative
 * or absolute paths.
 *
 * Does not handle query strings (?param1&param2...) or anchors (\#someanchor)
 * although, those would also be forms of relative URIs.
 */
class URIContext {
	friend class URI;

	std::string mProto;
	std::string mHost;
	std::string mUser;
	std::string mDirectory; ///< DOES NOT include initial or ending slash.
//	AuthenticationCreds mAuth;

	/** Should handle resolving ".." and "." inside of a path. */
	static inline void resolveParentDirectories(std::string &str) {
		// Do nothing for now.
		/*
		std::string::size_type slashpos = 0;
		while (true) {
			slashpos = str.find('/', slashpos);
			if (slashpos != std::string::npos) {
				std::string dir = str.substr(slashpos, slashpos)

			}
			slashpos++;
		}
		*/
		/*
		while (str[0]=='.'&&str[1]=='.'&&str[2]=='/') {
			str = str.substr(3);
		}
		std::string::size_type nextdotdot = 0;
		while ((nextdotdot = str.find("../", nextdotdot+1)) != std::string::npos) {

		}
		*/
	}

	struct IsSpace {
		inline bool operator()(const unsigned char c) {
			int kspace=(char)c;
			return std::isspace(kspace
#ifdef _WIN32
				,std::locale()
#endif
				)!=false;
		}
	};

	void cleanup(std::string &s) {
		// hostnames and protocols are case-insensitive.
		for (std::string::size_type i = 0; i < s.length(); ++i) {
			s[i] = std::tolower(s[i]
#ifdef _WIN32
				,std::locale()
#endif

			);
		}
		// remove any illegal characters such as spaces.
		s.erase(std::remove_if(s.begin(), s.end(), IsSpace()), s.end());
	}

public:
	/// Default constructor (://@/) -- use this along with an absolute URI.
	URIContext() {
	}

	/// Absolute URI constructor.
	URIContext(const std::string &newProto,
			const std::string &newHost,
			const std::string &newUser,
			const std::string &newDirectory)
		: mProto(newProto),
		  mHost(newHost),
		  mUser(newUser),
		  mDirectory(newDirectory){

		cleanup(mProto);
		cleanup(mHost);
	}

	/** Acts like the string-parsing constructor, NULL strings mean
	 * inherit from the old context, however it's tricky to pass in
	 * string pointers if you constructed them on the fly. */
	URIContext(const URIContext &parent,
			const std::string *newProto,
			const std::string *newHost,
			const std::string *newUser,
			const std::string *newDirectory)
		: mProto(newProto?*newProto:parent.proto()),
		  mHost(newHost?*newHost:parent.host()),
		  mUser(newUser?*newUser:parent.username()),
		  mDirectory(newDirectory?*newDirectory:parent.basepath()){
		cleanup(mProto);
		cleanup(mHost);
	}

	/** The most useful constructor -- parses a URI string, and
	 * tries to follow rules regarding relative paths and hostnames.
	 */
	URIContext(const URIContext &parent, const std::string &identifier)
			: mProto(parent.proto()),
			  mHost(parent.host()),
			  mUser(parent.username()),
			  mDirectory(parent.basepath()) {
		parse(identifier);
	}

	/** The most useful constructor -- parses a URI string, and
	 * tries to follow rules regarding relative paths and hostnames.
	 *
	 * This constructor is provided for convenience, and in many cases,
	 * URIs are only allowed to be absolute.
	 */
	URIContext(const std::string &identifier) {
		parse(identifier);
	}
private:
	void parse(std::string identifier) {
		if (!identifier.empty() && identifier[identifier.length()-1] != '/') {
			identifier += '/';
		}
		std::string::size_type colonpos = identifier.find(':');
		std::string::size_type firstslashpos = identifier.find('/');
		std::string::size_type startpos = 0;
		if (colonpos != std::string::npos &&
				(firstslashpos == std::string::npos || colonpos < firstslashpos)) {
			/* FIXME: Only accept [a-z0-9] as part of the protocol. We don't want an IPv6 address or
			  long filename with a colon in it being misinterpreted as a protocol.
			  Also, port numbers will be preceded by a colon */

			// global path
			mProto = identifier.substr(0, colonpos);
			mUser = std::string();
			startpos = colonpos+1;
		}

		if (identifier.length() > startpos+2 && identifier[startpos]=='/' && identifier[startpos+1]=='/') {
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
			if (atpos != std::string::npos && atpos >= beginpos && atpos < slashpos) {
				mUser = identifier.substr(beginpos, atpos-beginpos);
				beginpos = atpos+1;
			}
			if (slashpos != beginpos) {
				if (slashpos == std::string::npos) {
					mHost = identifier.substr(beginpos);
				} else {
					mHost = identifier.substr(beginpos, slashpos-beginpos);
				}
			} else {
				mHost = std::string();
			}
			startpos = slashpos;
		}
		if (identifier.length() > startpos && identifier[startpos]=='/') {
			// server-relative path
			std::string::size_type lastdir = identifier.rfind('/');
			if (lastdir > startpos) {
				mDirectory = identifier.substr(startpos+1, lastdir-startpos-1);
			} else {
				mDirectory = std::string();
			}
		} else {
			// directory-relative path -- not implemented here
			std::string::size_type lastdir = identifier.rfind('/');
			if (lastdir != std::string::npos && lastdir > startpos) {
				if (mDirectory.empty()) {
					mDirectory = identifier.substr(startpos, lastdir-startpos);
				} else {
					mDirectory += '/' + identifier.substr(startpos, lastdir-startpos);
				}
			}
		}

		resolveParentDirectories(mDirectory);

		cleanup(mProto);
		cleanup(mHost);
	}

public:
	/// protocol getter (without a ':'), like "http".
	inline const std::string &proto() const {
		return mProto;
	}

	inline void setProto(const std::string &proto) {
		mProto = proto;
	}

	/** username getter--may be empty for no username.
	 * Try not to store passwords in here, but if there is a password,
	 * it will be of the form "username:password".
	 */
	inline const std::string &username() const {
		return mUser;
	}

	inline void setUsername(const std::string &user) {
		mUser = user;
	}

	/** hostname getter--includes port, if any, as well
	 * as has brackets around IPv6 addresses. Examples:
	 * www.example.com
	 * www.example.com:80
	 * [fe80::202:b3ff:fe00:0102]:8080
	 */
	inline const std::string &host() const {
		return mHost;
	}


	inline void setHost(const std::string &host) {
		mHost = host;
	}

	/* The directory, excluding both initial slash and ending slash.
	 * Often, this may just be the empty string, but depends on protocol. */
	inline const std::string &basepath() const {
		return mDirectory;
	}

	inline void setBasepath(const std::string &basepath) {
		mDirectory = basepath;
	}

	void toParentPath(std::string *pathString) {
		std::string::size_type slash = mDirectory.rfind('/');
		if (slash == std::string::npos) {
			if (pathString) {
				if (!mDirectory.empty()) {
					*pathString = mDirectory + "/" + *pathString;
				}
			}
			mDirectory = std::string();
		} else {
			if (pathString) {
				if (slash > 0) {
					*pathString = mDirectory.substr(slash+1) + "/" + *pathString;
				}
			}
			mDirectory = mDirectory.substr(0, slash);
		}
	}

	void toParentContext(std::string *pathString) {
		if (mDirectory.empty()) {
			if (mHost.empty()) {
				if (pathString) {
					*pathString = mProto + ":" + *pathString;
				}
				mProto = std::string();
			} else {
				if (pathString) {
					*pathString = "//" + (mUser.empty()?"":mUser+"@") + mHost + "/" + *pathString;
				}
				mHost = std::string();
			}
			mUser = std::string();
		} else {
			toParentPath(pathString);
		}
	}
/*
	void relocate(const URIContext &source, const URIContext &dest) {
		std::string
	}
*/

	/// Constructs a URI... will exclude an empty username.
	inline std::string toString(bool trailingSlash=true) const {
		std::string ret (mProto + "://" + (mUser.empty() ? std::string() : (mUser + "@")) + mHost);
		if (!mDirectory.empty()) {
			ret += ("/" + mDirectory);
		}
		if (trailingSlash) {
			return ret + '/';
		}
		return ret;
	}

	/// ordering comparison -- more accurate than ordering based on toString().
	inline bool operator< (const URIContext &other) const {
		/* Note: I am testing user before hostname to keep this ordering
		 * the same as if you used a string version of the URI.
		 */
		if (mProto == other.mProto) {
			if (mUser == other.mUser) {
				if (mHost == other.mHost) {
					return mDirectory < other.mDirectory;
				}
				return mHost < other.mHost;
			}
			return mUser < other.mUser;
		}
		return mProto < other.mProto;
	}

	/// equality comparison
	inline bool operator==(const URIContext &other) const {
		return mDirectory == other.mDirectory &&
			mUser == other.mUser &&
			mHost == other.mHost &&
			mProto == other.mProto;
	}
	inline bool operator!=(const URIContext &other) const {
		// We can ignore the hash if it references the same URL.
		return !((*this) == other);
	}
};

/// Display both the URI string and the corresponding Fingerprint.
inline std::ostream &operator<<(std::ostream &str, const URIContext &urictx) {
	return str << urictx.toString();
}

/// URI stores both a uri string as well as a Fingerprint to verify it.
class URI {
	URIContext mContext;
	std::string mPath; // should have no slashes.

	void findSlash(const std::string &url) {
		std::string::size_type slash = url.rfind('/');
		if (slash != std::string::npos) {
			// FIXME: handle incomplete URIs correctly
			if (slash > 0 && url[slash-1] == '/' && !(slash > 1 && url[slash-2] == '/')) {
				// this is actually a hostname section... don't copy it into the filename.
				// unless there were three slashes in a row.
				mPath = std::string();
				mContext.parse(url);
			} else {
				mPath = url.substr(slash+1);
				mContext.parse(url.substr(0, slash+1));
			}
		} else {
			std::string::size_type colon = url.find(':');
			if (colon != std::string::npos) {
				mPath = url.substr(colon+1);
				mContext.parse(url.substr(0, colon+1));
			} else {
				mPath = url;
			}
		}
	}
public:
	/// Default constructor--calls default constructor for URIContext as well.
	explicit URI() {
	}

	/** Constructs a new URI based on an old context.
	 *
	 * @param parentContext  A URIContext to base relative paths from--
	 *           this may be the default constructor.
	 * @param url   A relative or absolute URI.
	 */
	URI(const URIContext &parentContext, const std::string &url)
			: mContext(parentContext) {
		findSlash(url);
	}

	/** Constructs an absolute URI. To be used when the security
	 * implications of relative URIs are not clear.
	 *
	 * @param url   An absolute URI.
	 */
	explicit URI(const char *url) {
		findSlash(url);
	}

	/** Constructs an absolute URI. To be used when the security
	 * implications of relative URIs are not clear.
	 *
	 * @param url   An absolute URI.
	 */
	explicit URI(const std::string &url) {
		findSlash(url);
	}

	/** Gets the corresponding context, from which you can construct
	 * another relative URI.
	 */
	inline const URIContext &context() const {
		return mContext;
	}

	/// Gives a writable reference to a URIContext.
	inline URIContext &getContext() {
		return mContext;
	}

	/// Returns the protocol used (== context().proto())
	inline const std::string &proto() const {
		return mContext.proto();
	}

	/// Returns the hostname (or empty if there is none, same as context)
	inline const std::string &host() const {
		return mContext.host();
	}

	/// Returns the username (or empty if there is none, same as context)
	inline const std::string &username() const {
		return mContext.username();
	}

	/// Returns the path without slashes on either end.
	inline const std::string &basepath() const {
		return mContext.basepath();
	}

	/// Returns just the filename withot slashes.
	inline const std::string &filename() const {
		return mPath;
	}

	inline void setFilename(const std::string &file) {
		mPath = file;
	}

	/** Returns an absolute path that represents this file, including
	 * a slash at the beginning, and will include a slash at the end
	 * only if filename() returns the empty string.
	 */
	inline std::string fullpath() const {
		if (mContext.basepath().empty()) {
			return '/' + mPath;
		} else {
			return '/' + mContext.basepath() + '/' + mPath;
		}
	}
/*
	void relocate(const URIContext &source, const URIContext &dest) {
		getContext().relocate(source, dest);
	}
*/
	/** const accessor for the full string URI
	 * Note that URIContext::toString does include the ending slash.
	 */
	inline std::string toString () const {
		return mContext.toString() + mPath;
	}

	/// Compare the URI
	inline bool operator<(const URI &other) const {
		// We can ignore the hash if it references the same URL.
		if (mContext == other.mContext) {
			return mPath < other.mPath;
		}
		return mContext < other.mContext;
	}

	/// Check the URI for equality
	inline bool operator==(const URI &other) const {
		// We can ignore the hash if it references the same URL.
		return mPath == other.mPath && mContext == other.mContext;
	}
	inline bool operator!=(const URI &other) const {
		// We can ignore the hash if it references the same URL.
		return !((*this) == other);
	}

};

/// Display both the URI string (including its context).
inline std::ostream &operator<<(std::ostream &str, const URI &uri) {
	return str << uri.toString();
}

/** Container for both a fingerprint and a download URI, as is most
 * commonly be used in CacheLayer for file downloads.
 */
class RemoteFileId {
	Fingerprint mHash;
	URI mURI;

public:
	RemoteFileId() {
	}

	RemoteFileId(const Fingerprint &fingerprint, const URI &uri)
			: mHash(fingerprint), mURI(uri) {
	}

	RemoteFileId(const Fingerprint &fingerprint, const URIContext &context)
			: mHash(fingerprint), mURI(context, fingerprint.convertToHexString()) {
	}

	explicit RemoteFileId(const URI &fingerprinturi)
			: mURI(fingerprinturi) {
		const std::string &path = fingerprinturi.filename();
		mHash = Fingerprint::convertFromHex(path);
	}

	inline std::string toString() const {
                return mURI.toString();
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
	inline bool operator== (const RemoteFileId &other) const {
	       return uri() == other.uri();
	}
        inline bool operator!= (const RemoteFileId &other) const {
               return uri() != other.uri();
        }
        inline bool operator< (const RemoteFileId &other) const {
               return uri() < other.uri();
        }
};

}
}

#endif /* SIRIKATA_URI_HPP__ */
