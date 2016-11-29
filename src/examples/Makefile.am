AM_CPPFLAGS = -I$(srcdir)/../../third_party/decklink -I$(srcdir)/../display $(XCBPRESENT_CFLAGS) $(XCB_CFLAGS) $(PANGOCAIRO_CFLAGS) $(CXX14_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = rgb-example
rgb_example_SOURCES = rgb-example.cc
rgb_example_LDADD = ../../third_party/decklink/libdecklink.a ../display/libdisplay.a $(XCBPRESENT_LIBS) $(XCB_LIBS) $(PANGOCAIRO_LIBS)
