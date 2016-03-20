noinst_PROGRAMS = latency

latency_SOURCES = \
	latency.cpp \
	Application.h \
	Application.cpp

latency_LDADD = $(top_builddir)/src/C++/libquickfix.la $(RT_LIBS)

INCLUDES = -I$(top_builddir)/include -I..

all-local:
	rm -f ../../bin/latency
	ln -s ../examples/latency/latency ../../bin/latency
