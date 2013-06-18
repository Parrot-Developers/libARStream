/**
 * @file ARVIDEO_Sender_TestBench.h
 * @brief Header file for the platform independant Sender TestBench
 * @date 06/14/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_SENDER_TESTBENCH_H_
#define _ARVIDEO_SENDER_TESTBENCH_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARVIDEO_Sender_TestBenchMain (int argc, char *argv[]);

/**
 * @brief Stop the testbench if it was running
 */
void ARVIDEO_Sender_TestBenchStop ();

/**
 * Percentage of frames which were correclty sent
 */
extern float ARVIDEO_Sender_PercentOk;

#endif /* _ARVIDEO_SENDER_TESTBENCH_H_ */
