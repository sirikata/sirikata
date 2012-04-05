all:
	case "`uname`" in \
		*arwin*|*MINGW*|*CYGWIN*|*win32*) \
			cd build/cmake && \
			cmake . && \
			$(MAKE) $(*) \
			;; \
		*) \
			cd build/cmake && \
			./cmake_with_tools.sh . && \
			$(MAKE) $(*) \
			;; \
	esac ;

release:
	case "`uname`" in \
		*arwin*|*MINGW*|*CYGWIN*|*win32*) \
			cd build/cmake && \
			cmake . -DCMAKE_BUILD_TYPE=Release && \
			$(MAKE) $(*) \
			;; \
		*) \
			cd build/cmake && \
			./cmake_with_tools.sh . -DCMAKE_BUILD_TYPE=Release && \
			$(MAKE) $(*) \
			;; \
	esac ;

# Leaving out cmake_with_tools since this is probably running in a
# chroot where the caching is pointless
debian-release:
	cd build/cmake && \
	cmake . -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=Release && \
	$(MAKE) $(*)

debug:
	case "`uname`" in \
		*arwin*|*MINGW*|*CYGWIN*|*win32*) \
			cd build/cmake && \
			cmake . -DCMAKE_BUILD_TYPE=Debug && \
			$(MAKE) $(*) \
			;; \
		*) \
			cd build/cmake && \
			./cmake_with_tools.sh . -DCMAKE_BUILD_TYPE=Debug && \
			$(MAKE) $(*) \
			;; \
	esac ;

# These all assume you've already run one of the standard builds, so
# they don't need to use cmake_with_tools.
.PHONY: test
test:
	cd build/cmake && \
	cmake . && \
	$(MAKE) test $(*)

tests:
	cd build/cmake && \
	cmake . && \
	$(MAKE) tests $(*)

install:
	cd build/cmake && \
	$(MAKE) install $(*)

clean:
	cd build/cmake && \
	( test -e Makefile && $(MAKE) clean $(*) ) || true

DEPVC8REV=HEAD
DEPVC9REV=27
DEPOSXREV=99
DEPSOURCE=75
DEPARCHINDEP=7
#========== Dependencies ===========

distributions:
	case "`uname`" in \
		*arwin*) \
			svn co -r$(DEPOSXREV) http://sirikataosx.googlecode.com/svn/trunk dependencies \
			;; \
		*MINGW*|*CYGWIN*|*win32*) \
			if [ x = x"$(VCVER)" ]; then \
				echo To force a specific version of visual studio, set VCVER to 8 or 9 ; \
				svn co -r$(DEPVC9REV) http://sirikatawin32.googlecode.com/svn/trunk dependencies ; \
			else \
				if [ x"9" = x"$(VCVER)" ]; then \
					svn co -r$(DEPVC9REV) http://sirikatawin32.googlecode.com/svn/trunk dependencies ; \
				else \
					svn co -r$(DEPVC8REV) http://sirikata.googlecode.com/svn/trunk/win32vc$(VCVER) dependencies ; \
				fi \
			fi \
			;; \
		*) \
			svn co -r$(DEPSOURCE) http://sirikataunix.googlecode.com/svn/trunk dependencies \
			;; \
	esac ; \
	svn co -r$(DEPARCHINDEP) http://sirikatamachindep.googlecode.com/svn/trunk/ dependencies/machindependencies

update-dependencies: distributions
	git submodule update --init --recursive || true
	case "`uname`" in \
		*arwin*) \
			cd dependencies && svn update -r$(DEPOSXREV) \
			;; \
		*MINGW*|*CYGWIN*|*win32*) \
			if [ x = x"$(VCVER)" ]; then \
				echo To force a specific version of visual studio, set VCVER to 8 or 9 ; \
				cd dependencies && [ -e c:/Program\ Files/Microsoft\ Visual\ Studio\ 9*/VC/bin ] && \
					svn update -r$(DEPVC9REV)  || \
					svn update -r$(DEPVC8REV); \
			else \
				if [ x"9" = x"$(VCVER)" ]; then \
					cd dependencies && svn update -r$(DEPVC9REV) ; \
				else \
					cd dependencies && svn update -r$(DEPVC8REV); \
				fi \
			fi \
			;; \
		*) \
			cd dependencies && svn update -r$(DEPSOURCE) \
			;; \
	esac ; \
	svn co -r$(DEPARCHINDEP) http://sirikatamachindep.googlecode.com/svn/trunk/ machindependencies


minimal-depends: update-dependencies
	$(MAKE) -C dependencies minimal-depends $(*)

minimal-graphics-depends: update-dependencies
	$(MAKE) -C dependencies minimal-graphics-depends $(*)

minimal-depends-with-root: update-dependencies
	$(MAKE) -C dependencies minimalrootdepends minimaldepends $(*)

headless-depends: update-dependencies
	$(MAKE) -C dependencies headless-depends $(*)

depends: update-dependencies
	$(MAKE) -C dependencies depends $(*)

full-headless-depends: update-dependencies
	$(MAKE) -C dependencies full-headless-depends $(*)

full-depends: update-dependencies
	$(MAKE) -C dependencies full-depends $(*)


docs: Doxyfile
	doxygen
