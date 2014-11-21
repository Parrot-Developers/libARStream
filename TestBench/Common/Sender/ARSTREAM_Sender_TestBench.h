/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/**
 * @file ARSTREAM_Sender_TestBench.h
 * @brief Header file for the platform independant Sender TestBench
 * @date 06/14/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAM_SENDER_TESTBENCH_H_
#define _ARSTREAM_SENDER_TESTBENCH_H_

/**
 * @brief Testbench entry point
 * @param argc Argument count of the main function
 * @param argv Arguments values of the main function
 * @return The "main" return value
 */
int ARSTREAM_Sender_TestBenchMain (int argc, char *argv[]);

/**
 * @brief Stop the testbench if it was running
 */
void ARSTREAM_Sender_TestBenchStop ();

/**
 * Percentage of frames which were correclty sent
 */
extern float ARSTREAM_Sender_PercentOk;

/**
 * Gets the mean time between frame (averaged over 15 frames)
 */
int ARSTREAM_SenderTb_GetMeanTimeBetweenFrames ();

/**
 * Gets libARNetwork estimated Latency
 * @return estimated network latency in ms
 */
int ARSTREAM_SenderTb_GetLatency ();

/**
 * @brief Gets the number of missed frames since last call
 * @return Number of frames missed since last call
 */
int ARSTREAM_SenderTb_GetMissedFrames ();

/**
 * @brief Gets the estimated efficiency of the sender
 * @return Estimated efficiency (0.0 is bad, 1.0 is perfect)
 */
float ARSTREAM_SenderTb_GetEfficiency ();

/**
 * @brief Gets the estimated paket misses on ack stream
 * @return Estimated ack packet loss [0-100]
 */
int ARSTREAM_SenderTb_GetEstimatedLoss ();

#endif /* _ARSTREAM_SENDER_TESTBENCH_H_ */
