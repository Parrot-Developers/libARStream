/**
 * @file ARSTREAMING_TCPSender.h
 * @brief Header file for the Drone2-like streaming test
 * @date 07/10/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAMING_TCPSENDER_H_
#define _ARSTREAMING_TCPSENDER_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARSTREAMING_TCPSender_Main (int argc, char *argv[]);

/**
 * @brief Stop the testbench if it was running
 */
void ARSTREAMING_TCPSender_Stop ();

/**
 * Check the connexion status
 */
int ARSTREAMING_TCPSender_IsConnected ();

#endif /* _ARSTREAMING_TCPSENDER_H_ */
