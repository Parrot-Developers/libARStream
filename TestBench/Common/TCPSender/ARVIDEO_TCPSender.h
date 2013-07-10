/**
 * @file ARVIDEO_TCPSender.h
 * @brief Header file for the Drone2-like video test
 * @date 07/10/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_TCPSENDER_H_
#define _ARVIDEO_TCPSENDER_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARVIDEO_TCPSender_Main (int argc, char *argv[]);

/**
 * @brief Stop the testbench if it was running
 */
void ARVIDEO_TCPSender_Stop ();

/**
 * Check the connexion status
 */
int ARVIDEO_TCPSender_IsConnected ();

#endif /* _ARVIDEO_TCPSENDER_H_ */
