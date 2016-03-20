noinst_PROGRAMS = codec

codec_SOURCES = codec.cpp

codec_LDADD = $(top_builddir)/src/C++/libquickfix.la $(RT_LIBS)

INCLUDES = -I$(top_builddir)/include -I..

all-local:
	rm -f ../../bin/codec
	ln -s ../examples/codec/codec ../../bin/codec
AM_CXXFLAGS = -DQUICKERFIX
