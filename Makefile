all:
	cd build/cmake;cmake .; $(MAKE) $(*)

release:
	cd build/cmake;cmake . -DCMAKE_BUILD_TYPE=Release; $(MAKE) $(*)

debug:
	cd build/cmake;cmake . -DCMAKE_BUILD_TYPE=Debug; $(MAKE) $(*)

test:
	cd build/cmake;cmake .; $(MAKE) test $(*)

tests:
	cd build/cmake;cmake .; $(MAKE) tests $(*)

clean:
	cd build/cmake; $(MAKE) clean $(*)

minimaldepends:
	uname | grep arwin && svn co http://sirikata.googlecode.com/svn/trunk/osx10.4 dependencies && tar -jx --directory dependencies --file dependencies/dotnet-protobufs*.bz2 && tar -jx --directory dependencies --file dependencies/FreeImage-*.bz2 && tar -jx --directory dependencies --file dependencies/curl*.bz2 && tar -jx --directory dependencies --file dependencies/boost*.bz2 && tar -jx --directory dependencies --file dependencies/ois*.bz2 && unzip -d dependencies dependencies/SDL-1.3*.zip && tar -jx --directory dependencies --file dependencies/ogre*.bz2 && tar -jx --directory dependencies --file dependencies/antlr-*.bz2 || echo "Linux system"
	uname | grep arwin || uname | grep CYGWIN || ( svn co http://sirikata.googlecode.com/svn/trunk/source dependencies && cd dependencies && ./install --without-mono )
	uname | grep arwin || ( uname | grep CYGWIN && ( (ls c:/Program\ Files/Microsoft\ Visual\ Studio\ 9.0/VC/bin && svn co http://sirikata.googlecode.com/svn/trunk/win32vc9 dependencies) ||  svn co http://sirikata.googlecode.com/svn/trunk/win32vc8 dependencies ) && unzip dependencies/dotnet-protobufs*.zip -d dependencies && unzip dependencies/FreeImage-*.zip -d dependencies && unzip dependencies/curl*.zip -d dependencies && unzip dependencies/boost*.zip -d dependencies  && unzip dependencies/ogre*.zip -d dependencies && unzip dependencies/SDL*.zip -d dependencies && unzip dependencies/ois*.zip -d dependencies ) || uname | grep Linux

depends:
	uname | grep arwin && svn co http://sirikata.googlecode.com/svn/trunk/osx10.4 dependencies && tar -jx --directory dependencies --file dependencies/dotnet-protobufs*.bz2 && tar -jx --directory dependencies --file dependencies/FreeImage-*.bz2 && tar -jx --directory dependencies --file dependencies/curl*.bz2 && tar -jx --directory dependencies --file dependencies/boost*.bz2 && tar -jx --directory dependencies --file dependencies/ois*.bz2 && unzip -d dependencies dependencies/SDL-1.3*.zip && tar -jx --directory dependencies --file dependencies/ogre*.bz2 && tar -jx --directory dependencies --file dependencies/antlr-*.bz2 || echo "Linux system"
	uname | grep arwin || uname | grep CYGWIN || ( svn co http://sirikata.googlecode.com/svn/trunk/source dependencies && cd dependencies && ./install )
	uname | grep arwin || ( uname | grep CYGWIN && ( (ls c:/Program\ Files/Microsoft\ Visual\ Studio\ 9.0/VC/bin && svn co http://sirikata.googlecode.com/svn/trunk/win32vc9 dependencies) ||  svn co http://sirikata.googlecode.com/svn/trunk/win32vc8 dependencies ) && unzip dependencies/dotnet-protobufs*.zip -d dependencies && unzip dependencies/FreeImage-*.zip -d dependencies && unzip dependencies/curl*.zip -d dependencies && unzip dependencies/boost*.zip -d dependencies  && unzip dependencies/ogre*.zip -d dependencies && unzip dependencies/SDL*.zip -d dependencies && unzip dependencies/ois*.zip -d dependencies ) || uname | grep Linux

fulldepends:
	uname | grep arwin && svn co http://sirikata.googlecode.com/svn/trunk/osx10.4 dependencies && tar -jx --directory dependencies --file dependencies/dotnet-protobufs*.bz2 && tar -jx --directory dependencies --file dependencies/FreeImage-*.bz2 && tar -jx --directory dependencies --file dependencies/curl*.bz2 && tar -jx --directory dependencies --file dependencies/boost*.bz2 && tar -jx --directory dependencies --file dependencies/ois*.bz2 && unzip -d dependencies dependencies/SDL-1.3*.zip && tar -jx --directory dependencies --file dependencies/ogre*.bz2 && tar -jx --directory dependencies --file dependencies/antlr-*.bz2 || echo "Linux system"
	uname | grep arwin || uname | grep CYGWIN || ( svn co http://sirikata.googlecode.com/svn/trunk/source dependencies && cd dependencies && sudo ./install --use-root --user `whoami` )
	uname | grep arwin || ( uname | grep CYGWIN && ( (ls c:/Program\ Files/Microsoft\ Visual\ Studio\ 9.0/VC/bin && svn co http://sirikata.googlecode.com/svn/trunk/win32vc9 dependencies) ||  svn co http://sirikata.googlecode.com/svn/trunk/win32vc8 dependencies ) && unzip dependencies/dotnet-protobufs*.zip -d dependencies && unzip dependencies/FreeImage-*.zip -d dependencies && unzip dependencies/curl*.zip -d dependencies && unzip dependencies/boost*.zip -d dependencies  && unzip dependencies/ogre*.zip -d dependencies && unzip dependencies/ois*.zip -d dependencies ) || uname | grep Linux
