/**
 * @file ARSTREAM_TCPSender.h
 * @brief Header file for the Drone2-like stream test
 * @date 07/10/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAM_TCPSENDER_H_
#define _ARSTREAM_TCPSENDER_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARSTREAM_TCPSender_Main (int argc, char *argv[]);

/**
 * @brief Stop the testbench if it was running
 */
void ARSTREAM_TCPSender_Stop ();

/**
 * Check the connexion status
 */
int ARSTREAM_TCPSender_IsConnected ();

#endif /* _ARSTREAM_TCPSENDER_H_ */
