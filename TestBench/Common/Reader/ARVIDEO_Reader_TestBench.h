/**
 * @file ARVIDEO_Reader_TestBench.h
 * @brief Header file for the platform independant Reader TestBench
 * @date 06/14/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_READER_TESTBENCH_H_
#define _ARVIDEO_READER_TESTBENCH_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARVIDEO_Reader_TestBenchMain (int argc, char *argv[]);

/**
 * Percentage of frames which were correclty received
 */
extern float ARVIDEO_Reader_PercentOk;

/**
 * Gets libARNetwork estimated Latency
 * @return estimated network latency in ms
 */
int ARVIDEO_ReaderTb_GetLatency ();

#endif /* _ARVIDEO_READER_TESTBENCH_H_ */
