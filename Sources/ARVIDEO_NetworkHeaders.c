/**
 * @file ARVIDEO_NetworkHeaders.c
 * @brief Video headers on network
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#include <config.h>

/*
 * System Headers
 */

/*
 * Private Headers
 */
#include "ARVIDEO_NetworkHeaders.h"

/*
 * ARSDK Headers
 */
#include <libARSAL/ARSAL_Print.h>

/*
 * Macros
 */
#define ARVIDEO_NETWORK_HEADERS_TAG "ARVIDEO_NetworkHeaders"

/*
 * Types
 */

/*
 * Internal functions declarations
 */

/*
 * Internal functions implementation
 */

/*
 * Implementation
 */

int ARVIDEO_NetworkHeaders_AckPacketAllFlagsSet (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int maxFlag)
{
    int res = 1;
    if (0 <= maxFlag && maxFlag < 64)
    {
        // Check only in first packet
        uint64_t lo_mask = (1ll << maxFlag) - 1ll;
        res = ((packet->lowPacketsAck & lo_mask) == lo_mask) ? 1 : 0;
    }
    else if (64 <= maxFlag && maxFlag < 128)
    {
        // We need to check for the second packet also
        uint64_t lo_mask = UINT64_MAX;
        uint32_t hi_mask = (1ll << (maxFlag-32)) - 1ll;
        int hi_res = ((packet->highPacketsAck & hi_mask) == hi_mask) ? 1 : 0;
        int lo_res = ((packet->lowPacketsAck & lo_mask) == lo_mask) ? 1 : 0;
        res = (hi_res == 1 && lo_res == 1) ? 1 : 0;
    }
    else
    {
        // We ask for more bits that we have, return 'false'
        res = 0;
    }
    return res;
}

int ARVIDEO_NetworkHeaders_AckPacketFlagIsSet (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int flag)
{
    int retVal = 0;
    if (0 <= flag && flag < 64)
    {
        retVal = ((packet->lowPacketsAck & (1ll << flag)) != 0) ? 1 : 0;
    }
    else if (64 <= flag && flag < 128)
    {
        retVal = ((packet->highPacketsAck & (1ll << (flag - 64))) != 0) ? 1 : 0;
    }
    return retVal;
}

void ARVIDEO_NetworkHeaders_AckPacketReset (ARVIDEO_NetworkHeaders_AckPacket_t *packet)
{
    packet->highPacketsAck = 0ll;
    packet->lowPacketsAck = 0ll;
}

void ARVIDEO_NetworkHeaders_AckPacketSetFlag (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int flagToSet)
{
    if (0 <= flagToSet && flagToSet < 64)
    {
        packet->lowPacketsAck |= (1ll << flagToSet);
    }
    else if (64 <= flagToSet && flagToSet < 128)
    {
        packet->highPacketsAck |= (1ll << (flagToSet-64));
    }
}

void ARVIDEO_NetworkHeaders_AckPacketSetFlags (ARVIDEO_NetworkHeaders_AckPacket_t *dst, ARVIDEO_NetworkHeaders_AckPacket_t *src)
{
    dst->highPacketsAck |= src->highPacketsAck;
    dst->lowPacketsAck  |= src->lowPacketsAck;
}

int ARVIDEO_NetworkHeaders_AckPacketUnsetFlag (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int flagToRemove)
{
    int retVal = 0;
    if (0 <= flagToRemove && flagToRemove < 64)
    {
        packet->lowPacketsAck &= ~(1ll << flagToRemove);
    }
    else if (64 <= flagToRemove && flagToRemove < 128)
    {
        packet->highPacketsAck &= ~(1ll << flagToRemove);
    }

    if (0ll == packet->lowPacketsAck &&
        0ll == packet->highPacketsAck)
    {
        retVal = 1;
    }
    return retVal;
}

int ARVIDEO_NetworkHeaders_AckPacketUnsetFlags (ARVIDEO_NetworkHeaders_AckPacket_t *dst, ARVIDEO_NetworkHeaders_AckPacket_t *src)
{
    int retVal = 0;
    dst->highPacketsAck &= ~(src->highPacketsAck);
    dst->lowPacketsAck  &= ~(src->lowPacketsAck);
    if (0ll == dst->lowPacketsAck &&
        0ll == dst->highPacketsAck)
    {
        retVal = 1;
    }
    return retVal;
}

void ARVIDEO_NetworkHeaders_AckPacketDump (const char *prefix, ARVIDEO_NetworkHeaders_AckPacket_t *packet)
{
    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_NETWORK_HEADERS_TAG, "Packet dump: %s", prefix);
    if (packet == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_NETWORK_HEADERS_TAG, "Packet is NULL");
    }
    else
    {
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_NETWORK_HEADERS_TAG, " - Frame number : %d", packet->frameNumber);
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_NETWORK_HEADERS_TAG, " - HI 64 bits : %016llX", packet->highPacketsAck);
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_NETWORK_HEADERS_TAG, " - LO 64 bits : %016llX", packet->lowPacketsAck);
    }
}
