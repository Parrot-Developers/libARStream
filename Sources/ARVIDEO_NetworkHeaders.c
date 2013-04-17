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
    int cnt = 0;
    for (cnt = 0; cnt < maxFlag && res == 1; cnt++)
    {
        res = ARVIDEO_NetworkHeaders_AckPacketFlagIsSet (packet, cnt);
    }
    return res;
}

int ARVIDEO_NetworkHeaders_AckPacketFlagIsSet (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int flag)
{
    int retVal = 0;
    if (0 <= flag && flag <= 63)
    {
        retVal = ((packet->lowPacketsAck & (1ll << flag)) != 0) ? 1 : 0;
    }
    else if (64 <= flag && flag <= 127)
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
    if (0 <= flagToSet && flagToSet <= 63)
    {
        packet->lowPacketsAck |= (1ll << flagToSet);
    }
    else if (64 <= flagToSet && flagToSet <= 127)
    {
        packet->highPacketsAck |= (1ll << (flagToSet-64));
    }
}

int ARVIDEO_NetworkHeaders_AckPacketUnsetFlag (ARVIDEO_NetworkHeaders_AckPacket_t *packet, int flagToRemove)
{
    int retVal = 0;
    if (0 <= flagToRemove && flagToRemove <= 63)
    {
        packet->lowPacketsAck &= ~ (1ll << flagToRemove);
    }
    else if (64 <= flagToRemove && flagToRemove <= 127)
    {
        packet->highPacketsAck &= ~ (1ll << flagToRemove);
    }

    if (0ll == packet->lowPacketsAck &&
        0ll == packet->highPacketsAck)
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
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_NETWORK_HEADERS_TAG, " - Frame number : %d", packet->numFrame);
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_NETWORK_HEADERS_TAG, " - HI 64 bits : %016llX", packet->highPacketsAck);
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_NETWORK_HEADERS_TAG, " - LO 64 bits : %016llX", packet->lowPacketsAck);
    }
}
