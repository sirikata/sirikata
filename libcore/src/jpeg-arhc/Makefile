all:
#	g++ -fprofile-generate -I. -I ../../include  *.cpp -I../../../dependencies/installed-xz/include ../../../dependencies/installed-xz/lib/liblzma.a -g -std=c++0x -pg -O3
#	g++ -fprofile-arcs -ftest-coverage -I. -I ../../include  *.cpp -I../../../dependencies/installed-xz/include -L../../../dependencies/installed-xz/lib -llzma -g -std=c++0x -pg -O3
	g++ -DHAS_LZHAM -I. -I ../../include  *.cpp -I../../../dependencies/installed-xz/include -I../../../dependencies/installed-lzham/include ../../../dependencies/installed-xz/lib/liblzma.a -g -std=c++0x  -lpthread ../../../dependencies/installed-lzham/lib/liblzhamdecomp.so ../../../dependencies/installed-lzham/lib/liblzhamcomp.so ../../../dependencies/installed-lzham/lib/liblzhamdll.so -Wl,-rpath=../../../dependencies/installed-lzham/lib
# ../../../dependencies/installed-lzham/lib/liblzhamdecomp.a ../../../dependencies/installed-lzham/lib/liblzhamcomp.a
#	g++ -I. -I ../../include  *.cpp -I../../../dependencies/installed-xz/include -L../../../dependencies/installed-xz/lib -llzma -g -std=c++0x
#	g++ -I. -I ../../include  *.cpp -I../../../dependencies/installed-xz/include -L../../../dependencies/installed-xz/lib -llzma -g -std=c++0x -O3 -pg
#	g++ -fprofile-use -I. -I ../../include  *.cpp -I../../../dependencies/installed-xz/include ../../../dependencies/installed-xz/lib/liblzma.a -g -std=c++0x -pg -O3