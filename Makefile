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


#========== Dependencies ===========

dependencies:
	case "`uname`" in \
		*arwin*) \
			svn co http://sirikata.googlecode.com/svn/trunk/osx10.4 dependencies \
			;; \
		*CYGWIN*|*win32*) \
			if [ x = x"$(VCVER)" ]; then \
				echo To force a specific version of visual studio, set VCVER to 8 or 9 ; \
				[ -e c:/Program\ Files/Microsoft\ Visual\ Studio\ 9/VC/bin ] && \
					svn co http://sirikata.googlecode.com/svn/trunk/win32vc9 dependencies || \
					svn co http://sirikata.googlecode.com/svn/trunk/win32vc8 dependencies \
			else \
				svn co http://sirikata.googlecode.com/svn/trunk/win32vc$(VCVER) dependencies ; \
			fi \
			;; \
		*) \
			svn co http://sirikata.googlecode.com/svn/trunk/source dependencies \
			;; \
	esac ; \
	[ -d dependencies ]

update-dependencies: dependencies
	cd dependencies && svn update

minimaldepends: update-dependencies
	$(MAKE) -C dependencies minimaldepends $(*)

depends: update-dependencies
	$(MAKE) -C dependencies depends $(*)

fulldepends: update-dependencies
	$(MAKE) -C dependencies fulldepends $(*)

