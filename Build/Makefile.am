###############################################################################
# THE LIBRARIES TO BUILD
###############################################################################
ACLOCAL_AMFLAGS                                             =   -I m4

# The list of header files that belong to the library (to be installed later)
HEADER_FILES                                                =   ../Includes/libARVideo/ARVIDEO_Sender.h \
                                                                ../Includes/libARVideo/ARVIDEO_Reader.h

# The sources to add to the library and to add to the source distribution
SOURCE_FILES                                                =   $(HEADER_FILES)                         \
                                                                ../Sources/ARVIDEO_NetworkHeaders.h     \
                                                                ../Sources/ARVIDEO_Buffers.h            \
                                                                ../Sources/ARVIDEO_Sender.c             \
                                                                ../Sources/ARVIDEO_Reader.c             \
                                                                ../Sources/ARVIDEO_NetworkHeaders.c     \
                                                                ../Sources/ARVIDEO_Buffers.c


# The library names to build (note we are building static and shared libs)
if DEBUG_MODE
lib_LTLIBRARIES                                             =   libarvideo_dbg.la
libarvideo_dbg_ladir                                        =   $(includedir)/libARVideo
libarvideo_dbg_la_HEADERS                                   =   $(HEADER_FILES)
libarvideo_dbg_la_SOURCES                                   =   $(SOURCE_FILES)
else
lib_LTLIBRARIES                                             =   libarvideo.la
libarvideo_ladir                                            =   $(includedir)/libARVideo
libarvideo_la_HEADERS                                       =   $(HEADER_FILES)
libarvideo_la_SOURCES                                       =   $(SOURCE_FILES)
endif

check_PROGRAMS                                              =   ../TestBench/Sender/ARVIDEO_Sender_TestBench    \
                                                                ../TestBench/Reader/ARVIDEO_Reader_TestBench    \
                                                                ../TestBench/MP4Sender/ARVIDEO_MP4Sender_TestBench

___TestBench_Sender_ARVIDEO_Sender_TestBench_SOURCES        =   ../TestBench/Sender/ARVIDEO_Sender_TestBench.c
___TestBench_Reader_ARVIDEO_Reader_TestBench_SOURCES        =   ../TestBench/Reader/ARVIDEO_Reader_TestBench.c
___TestBench_MP4Sender_ARVIDEO_MP4Sender_TestBench_SOURCES  =   ../TestBench/MP4Sender/ARVIDEO_MP4Sender_TestBench.c
if DEBUG_MODE
___TestBench_Sender_ARVIDEO_Sender_TestBench_LDADD          =   -larsal                                 \
                                                                -larnetworkal                           \
                                                                -larnetwork                             \
                                                                libarvideo_dbg.la
___TestBench_Reader_ARVIDEO_Reader_TestBench_LDADD          =   -larsal                                 \
                                                                -larnetworkal                           \
                                                                -larnetwork                             \
                                                                libarvideo_dbg.la
___TestBench_MP4Sender_ARVIDEO_MP4Sender_TestBench_LDADD    =   -larsal                                 \
                                                                -larnetworkal                           \
                                                                -larnetwork                             \
                                                                libarvideo_dbg.la
else
___TestBench_Sender_ARVIDEO_Sender_TestBench_LDADD          =   -larsal                                 \
                                                                -larnetworkal                           \
                                                                -larnetwork                             \
                                                                libarvideo.la
___TestBench_Reader_ARVIDEO_Reader_TestBench_LDADD          =   -larsal                                 \
                                                                -larnetworkal                           \
                                                                -larnetwork                             \
                                                                libarvideo.la
___TestBench_MP4Sender_ARVIDEO_MP4Sender_TestBench_LDADD    =   -larsal                                 \
                                                                -larnetworkal                           \
                                                                -larnetwork                             \
                                                                libarvideo.la
endif

CLEAN_FILES                                                 =   libarvideo.la                           \
                                                                libarvideo_dbg.la

EXTRA_DIST                                                  =   bootstrap

# Ensure the distribution of the doxygen configuration file
# Doxygen support
if HAVE_DOXYGEN
include doxygen.am
EXTRA_DIST                                                  +=  doxygen.ac                              \
                                                                doxygen.am                              \
                                                                $(DX_CONFIG)
endif
