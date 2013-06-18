/**
 * @file ARVIDEO_NetworkHeaders.h
 * @brief Video headers on network
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_NETWORK_HEADERS_PRIVATE_H_
#define _ARVIDEO_NETWORK_HEADERS_PRIVATE_H_

/*
 * System Headers
 */

#include <inttypes.h>

/*
 * Private Headers
 */

/*
 * ARSDK Headers
 */

/*
 * Macros
 */

#define ARVIDEO_NETWORK_HEADERS_MAX_FRAGMENTS_PER_FRAME (128)
#define ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE (1000)
#define ARVIDEO_NETWORK_HEADERS_MAX_FRAME_SIZE (ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE * ARVIDEO_NETWORK_HEADERS_MAX_FRAGMENTS_PER_FRAME)

/*
 * Types
 */

/**
 * @brief Header for video data frames
 */
typedef struct {
    uint16_t frameNumber; /**< Current frame number */
    uint8_t fragmentNumber; /**< Index of the current fragment in current frame */
    uint8_t fragmentsPerFrame; /**< Number of fragments un current frame */
} __attribute__ ((packed)) ARVIDEO_NetworkHeaders_DataHeader_t;

/**
 * @brief Content of video ack frames
 *
 * This struct is a 128bits bitfield
 *
 * On network, a 1 bit denotes that this packet is ACK
 *
 * This stucture is also used internally by the library to track packets that must be sent.
 * In this case, a 1 bit denotes that the packet must be sent
 */
typedef struct {
    uint16_t numFrame; /**< Frame number on which the ack packet refers */
    uint64_t highPacketsAck; /**< Upper 64 packets bitfield */
    uint64_t lowPacketsAck; /**< Lower 64 packets bitfield */
} __attribute__ ((packed)) ARVIDEO_NetworkHeaders_AckPacket_t;

/*
 * Functions declarations
 */

/**
 * @brief Tests if all flags between 0 and maxFlag are set
 * @param packet The packet to test
 * @param maxFlag The maximum index (inclusive) to test
 * @return 1 if all flags with index [0;maxFlag] are set, 0 otherwise
 */
int ARVIDEO_NetworkHeaders_AckPacketAllFlagsSet (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int maxFlag);

/**
 * @brief Tests if a flag is set in the packet
 * @param packet The packet to test
 * @param flag The index of the flag to test
 * @return 1 if the flag is set, 0 otherwise
 */
int ARVIDEO_NetworkHeaders_AckPacketFlagIsSet (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int flag);

/**
 * @brief Resets all flags in a packet to zero
 * @param packet The packet to reset
 */
void ARVIDEO_NetworkHeaders_AckPacketReset (ARVIDEO_NetworkHeaders_AckPacket_t *packet);

/**
 * @brief Sets a flag in a packet
 * This function has no effect if the flag was already set
 * @param packet The packet to modify
 * @param flag The index of the flag to set
 */
void ARVIDEO_NetworkHeaders_AckPacketSetFlag (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int flag);

/**
 * @brief Sets all flags from packet src into packet dst
 * @param dst the packet to modify
 * @param src the packet which contains the flags to set
 */
void ARVIDEO_NetworkHeaders_AckPacketSetFlags (ARVIDEO_NetworkHeaders_AckPacket_t *dst, ARVIDEO_NetworkHeaders_AckPacket_t *src);

/**
 * @brief Unsets a flag in a packet
 * @param packet The packet to modify
 * @param flag The index of the flag to unset
 * @return 1 if the packet is empty after removal (or if it was already empty before)
 * @return 0 otherwise
 */
int ARVIDEO_NetworkHeaders_AckPacketUnsetFlag (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int flag);

/**
 * @brief Unsets all flags from packet src into packet dst
 * @param dst the packet to modify
 * @param src the packet which contains the flags to unset
 * @return 1 if the packet is empty after removal (or if it was already empty before)
 * @return 0 otherwise
 */
int ARVIDEO_NetworkHeaders_AckPacketUnsetFlags (ARVIDEO_NetworkHeaders_AckPacket_t *dst, ARVIDEO_NetworkHeaders_AckPacket_t *src);

/**
 * @brief Dump an ack packet
 * @param prefix prefix of the dump
 * @param packet The packet to dump
 */
void ARVIDEO_NetworkHeaders_AckPacketDump (const char *prefix, ARVIDEO_NetworkHeaders_AckPacket_t *packet);

#endif /* _ARVIDEO_NETWORK_HEADERS_PRIVATE_H_ */
