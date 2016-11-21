AM_CPPFLAGS = -I$(srcdir)/../display $(XCBIMAGE_CFLAGS) $(XCBPRESENT_CFLAGS) $(XCB_CFLAGS) $(PANGOCAIRO_CFLAGS) $(CXX14_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = rgb-example
rgb_example_SOURCES = rgb-example.cc
rgb_example_LDADD = ../display/libdisplay.a $(XCBIMAGE_LIBS) $(XCBPRESENT_LIBS) $(XCB_LIBS) $(PANGOCAIRO_LIBS)
