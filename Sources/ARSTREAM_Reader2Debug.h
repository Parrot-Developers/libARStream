/*
    Copyright (C) 2015 Parrot SA

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
 * @file ARSTREAM_Reader2Debug.h
 * @brief Stream reader on network (v2) debug addon
 * @date 05/28/2015
 * @author aurelien.barre@parrot.com
 */

#ifndef _ARSTREAM_READER2DEBUG_H_
#define _ARSTREAM_READER2DEBUG_H_

/*
 * System Headers
 */
#include <inttypes.h>

/*
 * Types
 */

/**
 * @brief An ARSTREAM_Reader2Debug_t instance allow analyzing streamed frames from a network
 */
typedef struct ARSTREAM_Reader2Debug_t ARSTREAM_Reader2Debug_t;

/*
 * Functions declarations
 */

/**
 * @brief Creates a new ARSTREAM_Reader2Debug_t
 * @warning This function allocates memory. An ARSTREAM_Reader2Debug_t muse be deleted by a call to ARSTREAM_Reader2Debug_Delete
 *
 * @param outputRtpStatFile if not null, output RTP stats to a stat file
 * @param outputFrameStatFile if not null, output video stats to a stat file
 * @param outputVideoFile if not null, output the video stream to a stat file
 * @return A pointer to the new ARSTREAM_Reader2Debug_t, or NULL if an error occured
 */
ARSTREAM_Reader2Debug_t* ARSTREAM_Reader2Debug_New (int outputRtpStatFile, int outputFrameStatFile, int outputVideoFile);

/**
 * @brief Deletes an ARSTREAM_Reader2Debug_t
 *
 * @param rdbg Pointer to the ARSTREAM_Reader2Debug_t * to delete
 *
 * @note The library uses a double pointer, so it can set *rdbg to NULL after freeing it
 */
void ARSTREAM_Reader2Debug_Delete (ARSTREAM_Reader2Debug_t **rdbg);

/**
 * @brief Process an Access Unit
 *
 * @param rdbg Pointer to the ARSTREAM_Reader2Debug_t
 * @param pAu pointer to the access unit bitstream
 * @param auSize size of the access unit
 * @param timestamp access unit timestamp
 * @param receptionTs reception timestamp
 * @param missingPackets number of missing packets in the access unit
 * @param totalPackets total number of packets in the access unit including missingPackets
 *
 */
void ARSTREAM_Reader2Debug_ProcessAu (ARSTREAM_Reader2Debug_t *rdbg, uint8_t* pAu, uint32_t auSize, uint64_t timestamp, uint64_t receptionTs, int missingPackets, int totalPackets);

/**
 * @brief Process a NAL Unit
 *
 * @param rdbg Pointer to the ARSTREAM_Reader2Debug_t
 * @param pNalu pointer to the NAL unit bitstream
 * @param naluSize size of the NAL unit
 * @param timestamp access unit timestamp
 * @param receptionTs reception timestamp
 * @param missingPacketsBefore number of missing packets before the NAL unit
 * @param totalPackets total number of packets in the NAL unit including missingPacketsBefore
 * @param isFirstNaluInAu the NAL unit is the first in the access unit
 * @param isLastNaluInAu the NAL unit is the last in the access unit
 *
 */
void ARSTREAM_Reader2Debug_ProcessNalu (ARSTREAM_Reader2Debug_t *rdbg, uint8_t* pNalu, uint32_t naluSize, uint64_t timestamp, uint64_t receptionTs, int missingPacketsBefore, int totalPackets, int isFirstNaluInAu, int isLastNaluInAu);

/**
 * @brief Process a packet
 *
 * @param rdbg Pointer to the ARSTREAM_Reader2Debug_t
 * @param recvTimestamp packet reception timestamp
 * @param timestamp RTP timestamp
 * @param seqNum RTP sequence number
 * @param markerBit RTP M marker bit
 * @param bytes packet size
 *
 */
void ARSTREAM_Reader2Debug_ProcessPacket(ARSTREAM_Reader2Debug_t *rdbg, uint64_t recvTimestamp, uint32_t timestamp, uint16_t seqNum, uint16_t markerBit, uint32_t bytes);

#endif /* _ARSTREAM_READER2DEBUG_H_ */

