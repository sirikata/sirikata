all:
	cd build/cmake;cmake .; $(MAKE) $(*)

depends:
	uname | grep arwin && svn co http://sirikata.googlecode.com/svn/trunk/osx10.4 dependencies || echo "Linux system"
	uname | grep arwin || ( svn co http://sirikata.googlecode.com/svn/trunk/source dependencies && cd dependencies && sudo ./ubuntu-install `whoami` $(*) )
