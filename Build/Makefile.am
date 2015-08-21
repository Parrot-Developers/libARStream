###############################################################################
# THE LIBRARIES TO BUILD
###############################################################################
ACLOCAL_AMFLAGS                                             =   -I m4

# The list of header files that belong to the library (to be installed later)
HEADER_FILES                                                =   ../Includes/libARStream/ARSTREAM_Sender.h  \
                                                                ../Includes/libARStream/ARSTREAM_Sender2.h \
                                                                ../Includes/libARStream/ARSTREAM_Reader.h  \
                                                                ../Includes/libARStream/ARSTREAM_Reader2.h \
                                                                ../Includes/libARStream/ARSTREAM_Error.h   \
                                                                ../Includes/libARStream/ARStream.h

# The sources to add to the library and to add to the source distribution
SOURCE_FILES                                                =   $(HEADER_FILES)                          \
                                                                ../Sources/ARSTREAM_NetworkHeaders.h     \
                                                                ../Sources/ARSTREAM_Buffers.h            \
                                                                ../gen/Sources/ARSTREAM_Error.c              \
                                                                ../Sources/ARSTREAM_Sender.c             \
                                                                ../Sources/ARSTREAM_Sender2.c            \
                                                                ../Sources/ARSTREAM_Reader.c             \
                                                                ../Sources/ARSTREAM_Reader2.c            \
                                                                ../Sources/ARSTREAM_NetworkHeaders.c     \
                                                                ../Sources/ARSTREAM_Buffers.c


# The library names to build (note we are building static and shared libs)
if DEBUG_MODE
lib_LTLIBRARIES                                             =   libarstream_dbg.la
libarstream_dbg_ladir                                       =   $(includedir)/libARStream
libarstream_dbg_la_HEADERS                                  =   $(HEADER_FILES)
libarstream_dbg_la_SOURCES                                  =   $(SOURCE_FILES)
else
lib_LTLIBRARIES                                             =   libarstream.la
libarstream_ladir                                           =   $(includedir)/libARStream
libarstream_la_HEADERS                                      =   $(HEADER_FILES)
libarstream_la_SOURCES                                      =   $(SOURCE_FILES)
endif

check_PROGRAMS                                              =   ../TestBench/Linux/Sender/ARSTREAM_Sender_TestBench                      \
                                                                ../TestBench/Linux/Reader/ARSTREAM_Reader_TestBench                      \
                                                                ../TestBench/Linux/MP4Sender/ARSTREAM_MP4Sender_TestBench                \
                                                                ../TestBench/Linux/TCPSender/ARSTREAM_TCPSender_TestBench                \
                                                                ../TestBench/Linux/TCPReader/ARSTREAM_TCPReader_TestBench

___TestBench_Linux_Sender_ARSTREAM_Sender_TestBench_SOURCES          =   ../TestBench/Linux/Sender/ARSTREAM_Sender_LinuxTestBench.c       \
                                                                         ../TestBench/Common/Logger/ARSTREAM_Logger.c                     \
                                                                         ../TestBench/Common/Sender/ARSTREAM_Sender_TestBench.c
___TestBench_Linux_Reader_ARSTREAM_Reader_TestBench_SOURCES          =   ../TestBench/Linux/Reader/ARSTREAM_Reader_LinuxTestBench.c       \
                                                                         ../TestBench/Common/Logger/ARSTREAM_Logger.c                     \
                                                                         ../TestBench/Common/Reader/ARSTREAM_Reader_TestBench.c
___TestBench_Linux_MP4Sender_ARSTREAM_MP4Sender_TestBench_SOURCES    =   ../TestBench/Linux/MP4Sender/ARSTREAM_MP4Sender_LinuxTestBench.c \
                                                                         ../TestBench/Common/Logger/ARSTREAM_Logger.c                     \
                                                                         ../TestBench/Common/MP4Sender/ARSTREAM_MP4Sender_TestBench.c
___TestBench_Linux_TCPSender_ARSTREAM_TCPSender_TestBench_SOURCES    =   ../TestBench/Linux/TCPSender/ARSTREAM_TCPSender_LinuxTb.c        \
                                                                         ../TestBench/Common/TCPSender/ARSTREAM_TCPSender.c
___TestBench_Linux_TCPReader_ARSTREAM_TCPReader_TestBench_SOURCES    =   ../TestBench/Linux/TCPReader/ARSTREAM_TCPReader_LinuxTb.c        \
                                                                         ../TestBench/Common/Logger/ARSTREAM_Logger.c                     \
                                                                         ../TestBench/Common/TCPReader/ARSTREAM_TCPReader.c
if DEBUG_MODE
___TestBench_Linux_Sender_ARSTREAM_Sender_TestBench_LDADD            =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream_dbg.la
___TestBench_Linux_Reader_ARSTREAM_Reader_TestBench_LDADD            =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream_dbg.la
___TestBench_Linux_MP4Sender_ARSTREAM_MP4Sender_TestBench_LDADD      =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream_dbg.la
___TestBench_Linux_TCPSender_ARSTREAM_TCPSender_TestBench_LDADD      =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream_dbg.la
___TestBench_Linux_TCPReader_ARSTREAM_TCPReader_TestBench_LDADD      =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream_dbg.la
else
___TestBench_Linux_Sender_ARSTREAM_Sender_TestBench_LDADD            =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream.la
___TestBench_Linux_Reader_ARSTREAM_Reader_TestBench_LDADD            =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream.la
___TestBench_Linux_MP4Sender_ARSTREAM_MP4Sender_TestBench_LDADD      =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream.la
___TestBench_Linux_TCPSender_ARSTREAM_TCPSender_TestBench_LDADD      =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream.la
___TestBench_Linux_TCPReader_ARSTREAM_TCPReader_TestBench_LDADD      =   -larsal                         \
                                                                         -larnetworkal                   \
                                                                         -larnetwork                     \
                                                                         libarstream.la
endif

CLEAN_FILES                                                 =   libarstream.la                           \
                                                                libarstream_dbg.la

EXTRA_DIST                                                  =   bootstrap

# Ensure the distribution of the doxygen configuration file
# Doxygen support
if HAVE_DOXYGEN
include doxygen.am
EXTRA_DIST                                                  +=  doxygen.ac                              \
                                                                doxygen.am                              \
                                                                $(DX_CONFIG)
endif
