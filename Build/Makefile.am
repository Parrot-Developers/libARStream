###############################################################################
# THE LIBRARIES TO BUILD
###############################################################################
ACLOCAL_AMFLAGS                                             =   -I m4

# The list of header files that belong to the library (to be installed later)
HEADER_FILES                                                =   ../Includes/libARStreaming/ARSTREAMING_Sender.h \
                                                                ../Includes/libARStreaming/ARSTREAMING_Reader.h \
                                                                ../Includes/libARStreaming/ARSTREAMING_Error.h

# The sources to add to the library and to add to the source distribution
SOURCE_FILES                                                =   $(HEADER_FILES)                         \
                                                                ../Sources/ARSTREAMING_NetworkHeaders.h     \
                                                                ../Sources/ARSTREAMING_Buffers.h            \
                                                                ../Sources/ARSTREAMING_Error.c              \
                                                                ../Sources/ARSTREAMING_Sender.c             \
                                                                ../Sources/ARSTREAMING_Reader.c             \
                                                                ../Sources/ARSTREAMING_NetworkHeaders.c     \
                                                                ../Sources/ARSTREAMING_Buffers.c


# The library names to build (note we are building static and shared libs)
if DEBUG_MODE
lib_LTLIBRARIES                                             =   libarstreaming_dbg.la
libarstreaming_dbg_ladir                                    =   $(includedir)/libARStreaming
libarstreaming_dbg_la_HEADERS                               =   $(HEADER_FILES)
libarstreaming_dbg_la_SOURCES                               =   $(SOURCE_FILES)
else
lib_LTLIBRARIES                                             =   libarstreaming.la
libarstreaming_ladir                                        =   $(includedir)/libARStreaming
libarstreaming_la_HEADERS                                   =   $(HEADER_FILES)
libarstreaming_la_SOURCES                                   =   $(SOURCE_FILES)
endif

check_PROGRAMS                                              =   ../TestBench/Linux/Sender/ARSTREAMING_Sender_TestBench                      \
                                                                ../TestBench/Linux/Reader/ARSTREAMING_Reader_TestBench                      \
                                                                ../TestBench/Linux/MP4Sender/ARSTREAMING_MP4Sender_TestBench                \
                                                                ../TestBench/Linux/TCPSender/ARSTREAMING_TCPSender_TestBench                \
                                                                ../TestBench/Linux/TCPReader/ARSTREAMING_TCPReader_TestBench

___TestBench_Linux_Sender_ARSTREAMING_Sender_TestBench_SOURCES          =   ../TestBench/Linux/Sender/ARSTREAMING_Sender_LinuxTestBench.c       \
                                                                            ../TestBench/Common/Logger/ARSTREAMING_Logger.c                     \
                                                                            ../TestBench/Common/Sender/ARSTREAMING_Sender_TestBench.c
___TestBench_Linux_Reader_ARSTREAMING_Reader_TestBench_SOURCES          =   ../TestBench/Linux/Reader/ARSTREAMING_Reader_LinuxTestBench.c       \
                                                                            ../TestBench/Common/Logger/ARSTREAMING_Logger.c                     \
                                                                            ../TestBench/Common/Reader/ARSTREAMING_Reader_TestBench.c
___TestBench_Linux_MP4Sender_ARSTREAMING_MP4Sender_TestBench_SOURCES    =   ../TestBench/Linux/MP4Sender/ARSTREAMING_MP4Sender_LinuxTestBench.c \
                                                                            ../TestBench/Common/Logger/ARSTREAMING_Logger.c                     \
                                                                            ../TestBench/Common/MP4Sender/ARSTREAMING_MP4Sender_TestBench.c
___TestBench_Linux_TCPSender_ARSTREAMING_TCPSender_TestBench_SOURCES    =   ../TestBench/Linux/TCPSender/ARSTREAMING_TCPSender_LinuxTb.c        \
                                                                            ../TestBench/Common/TCPSender/ARSTREAMING_TCPSender.c
___TestBench_Linux_TCPReader_ARSTREAMING_TCPReader_TestBench_SOURCES    =   ../TestBench/Linux/TCPReader/ARSTREAMING_TCPReader_LinuxTb.c        \
                                                                            ../TestBench/Common/Logger/ARSTREAMING_Logger.c                     \
                                                                            ../TestBench/Common/TCPReader/ARSTREAMING_TCPReader.c
if DEBUG_MODE
___TestBench_Linux_Sender_ARSTREAMING_Sender_TestBench_LDADD            =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                        libarstreaming_dbg.la
___TestBench_Linux_Reader_ARSTREAMING_Reader_TestBench_LDADD            =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming_dbg.la
___TestBench_Linux_MP4Sender_ARSTREAMING_MP4Sender_TestBench_LDADD      =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming_dbg.la
___TestBench_Linux_TCPSender_ARSTREAMING_TCPSender_TestBench_LDADD      =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming_dbg.la
___TestBench_Linux_TCPReader_ARSTREAMING_TCPReader_TestBench_LDADD      =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming_dbg.la
else
___TestBench_Linux_Sender_ARSTREAMING_Sender_TestBench_LDADD            =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming.la
___TestBench_Linux_Reader_ARSTREAMING_Reader_TestBench_LDADD            =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming.la
___TestBench_Linux_MP4Sender_ARSTREAMING_MP4Sender_TestBench_LDADD      =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming.la
___TestBench_Linux_TCPSender_ARSTREAMING_TCPSender_TestBench_LDADD      =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming.la
___TestBench_Linux_TCPReader_ARSTREAMING_TCPReader_TestBench_LDADD      =   -larsal                         \
                                                                            -larnetworkal                   \
                                                                            -larnetwork                     \
                                                                            libarstreaming.la
endif

CLEAN_FILES                                                 =   libarstreaming.la                           \
                                                                libarstreaming_dbg.la

EXTRA_DIST                                                  =   bootstrap

# Ensure the distribution of the doxygen configuration file
# Doxygen support
if HAVE_DOXYGEN
include doxygen.am
EXTRA_DIST                                                  +=  doxygen.ac                              \
                                                                doxygen.am                              \
                                                                $(DX_CONFIG)
endif
