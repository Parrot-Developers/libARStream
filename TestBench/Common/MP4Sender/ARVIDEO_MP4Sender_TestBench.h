/**
 * @file ARVIDEO_MP4Sender_TestBench.h
 * @brief Header file for the platform independant MP4TestBench
 * @date 06/14/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_MP4SENDER_TESTBENCH_H_
#define _ARVIDEO_MP4SENDER_TESTBENCH_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARVIDEO_MP4Sender_TestBenchMain (int argc, char *argv[]);

/**
 * @brief Stops the testbench (if its running)
 */
void ARVIDEO_MP4Sender_TestBenchStop ();

/**
 * Percentage of frames which were correclty sent
 */
extern float ARVIDEO_MP4Sender_PercentOk;

#endif /* _ARVIDEO_MP4SENDER_TESTBENCH_H_ */
