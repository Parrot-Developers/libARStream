/**
 * @file ARSTREAMING_TCPReader.h
 * @brief Header file for the Drone2-like streaming test
 * @date 07/10/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAMING_TCPREADER_H_
#define _ARSTREAMING_TCPREADER_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARSTREAMING_TCPReader_Main (int argc, char *argv[]);

/**
 * @brief Stop the testbench if it was running
 */
void ARSTREAMING_TCPReader_Stop ();

/**
 * @brief Get mean time between frames, averaged over 15 last frames
 * @return Mean time between frames (ms)
 */
int ARSTREAMING_TCPReader_GetMeanTimeBetweenFrames ();

#endif /* _ARSTREAMING_TCPREADER_H_ */
