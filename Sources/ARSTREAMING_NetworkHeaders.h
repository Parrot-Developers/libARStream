/**
 * @file ARSTREAMING_NetworkHeaders.h
 * @brief Streaming headers on network
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAMING_NETWORK_HEADERS_PRIVATE_H_
#define _ARSTREAMING_NETWORK_HEADERS_PRIVATE_H_

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

#define ARSTREAMING_NETWORK_HEADERS_MAX_FRAGMENTS_PER_FRAME (128)
#define ARSTREAMING_NETWORK_HEADERS_FRAGMENT_SIZE (1000)
#define ARSTREAMING_NETWORK_HEADERS_MAX_FRAME_SIZE (ARSTREAMING_NETWORK_HEADERS_FRAGMENT_SIZE * ARSTREAMING_NETWORK_HEADERS_MAX_FRAGMENTS_PER_FRAME)

#define ARSTREAMING_NETWORK_HEADERS_FLAG_FLUSH_FRAME (1)

/*
 * Types
 */

/**
 * @brief Header for streaming data frames
 */
typedef struct {
    uint16_t frameNumber; /**< id of the current frame */
    uint8_t frameFlags; /**< Infos on the current frame */
    uint8_t fragmentNumber; /**< Index of the current fragment in current frame */
    uint8_t fragmentsPerFrame; /**< Number of fragments in current frame */
} __attribute__ ((packed)) ARSTREAMING_NetworkHeaders_DataHeader_t;

/* frameFlags structure :
 *  x x x x x x x x
 *  | | | | | | | \-> FLUSH FRAME
 *  | | | | | | \-> UNUSED
 *  | | | | | \-> UNUSED
 *  | | | | \-> UNUSED
 *  | | | \-> UNUSED
 *  | | \-> UNUSED
 *  | \-> UNUSED
 *  \-> UNUSED
 */

/**
 * @brief Content of streaming ack frames
 *
 * This struct is a 128bits bitfield
 *
 * On network, a 1 bit denotes that this packet is ACK
 *
 * This stucture is also used internally by the library to track packets that must be sent.
 * In this case, a 1 bit denotes that the packet must be sent
 */
typedef struct {
    uint16_t frameNumber; /**< id of the current frame */
    uint64_t highPacketsAck; /**< Upper 64 packets bitfield */
    uint64_t lowPacketsAck; /**< Lower 64 packets bitfield */
} __attribute__ ((packed)) ARSTREAMING_NetworkHeaders_AckPacket_t;

/*
 * Functions declarations
 */

/**
 * @brief Tests if all flags between 0 and maxFlag are set
 * @param packet The packet to test
 * @param maxFlag The maximum index (inclusive) to test
 * @return 1 if all flags with index [0;maxFlag] are set, 0 otherwise
 */
int ARSTREAMING_NetworkHeaders_AckPacketAllFlagsSet (ARSTREAMING_NetworkHeaders_AckPacket_t *packet, int maxFlag);

/**
 * @brief Tests if a flag is set in the packet
 * @param packet The packet to test
 * @param flag The index of the flag to test
 * @return 1 if the flag is set, 0 otherwise
 */
int ARSTREAMING_NetworkHeaders_AckPacketFlagIsSet (ARSTREAMING_NetworkHeaders_AckPacket_t *packet, int flag);

/**
 * @brief Resets all flags in a packet to zero
 * @param packet The packet to reset
 */
void ARSTREAMING_NetworkHeaders_AckPacketReset (ARSTREAMING_NetworkHeaders_AckPacket_t *packet);

/**
 * @brief Sets a flag in a packet
 * This function has no effect if the flag was already set
 * @param packet The packet to modify
 * @param flag The index of the flag to set
 */
void ARSTREAMING_NetworkHeaders_AckPacketSetFlag (ARSTREAMING_NetworkHeaders_AckPacket_t *packet, int flag);

/**
 * @brief Sets all flags from packet src into packet dst
 * @param dst the packet to modify
 * @param src the packet which contains the flags to set
 */
void ARSTREAMING_NetworkHeaders_AckPacketSetFlags (ARSTREAMING_NetworkHeaders_AckPacket_t *dst, ARSTREAMING_NetworkHeaders_AckPacket_t *src);

/**
 * @brief Unsets a flag in a packet
 * @param packet The packet to modify
 * @param flag The index of the flag to unset
 * @return 1 if the packet is empty after removal (or if it was already empty before)
 * @return 0 otherwise
 */
int ARSTREAMING_NetworkHeaders_AckPacketUnsetFlag (ARSTREAMING_NetworkHeaders_AckPacket_t *packet, int flag);

/**
 * @brief Unsets all flags from packet src into packet dst
 * @param dst the packet to modify
 * @param src the packet which contains the flags to unset
 * @return 1 if the packet is empty after removal (or if it was already empty before)
 * @return 0 otherwise
 */
int ARSTREAMING_NetworkHeaders_AckPacketUnsetFlags (ARSTREAMING_NetworkHeaders_AckPacket_t *dst, ARSTREAMING_NetworkHeaders_AckPacket_t *src);

/**
 * @brief Count the number of flags set in range [0;nb[
 * @param packet The packet to test
 * @param nb the number of flags to test
 * @return The number of flags set (=1) in the packet for the range [0:nb[
 */
uint32_t ARSTREAMING_NetworkHeaders_AckPacketCountSet (ARSTREAMING_NetworkHeaders_AckPacket_t *packet, int nb);

/**
 * @brief Count the number of flags unset in range [0;nb[
 * @param packet The packet to test
 * @param nb the number of flags to test
 * @return The number of flags unset (=0) in the packet for the range [0:nb[
 */
uint32_t ARSTREAMING_NetworkHeaders_AckPacketCountNotSet (ARSTREAMING_NetworkHeaders_AckPacket_t *packet, int nb);


/**
 * @brief Dump an ack packet
 * @param prefix prefix of the dump
 * @param packet The packet to dump
 */
void ARSTREAMING_NetworkHeaders_AckPacketDump (const char *prefix, ARSTREAMING_NetworkHeaders_AckPacket_t *packet);

#endif /* _ARSTREAMING_NETWORK_HEADERS_PRIVATE_H_ */
