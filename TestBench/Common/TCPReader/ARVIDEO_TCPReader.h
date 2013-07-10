/**
 * @file ARVIDEO_TCPReader.h
 * @brief Header file for the Drone2-like video test
 * @date 07/10/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_TCPREADER_H_
#define _ARVIDEO_TCPREADER_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARVIDEO_TCPReader_Main (int argc, char *argv[]);

/**
 * @brief Stop the testbench if it was running
 */
void ARVIDEO_TCPReader_Stop ();

/**
 * @brief Get mean time between frames, averaged over 15 last frames
 * @return Mean time between frames (ms)
 */
int ARVIDEO_TCPReader_GetMeanTimeBetweenFrames ();

#endif /* _ARVIDEO_TCPREADER_H_ */
