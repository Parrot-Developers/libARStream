#ifndef H264P_PARROT_H
#define H264P_PARROT_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define H264P_PARROT_DRAGON_SERIAL_NUMBER_PART_LENGTH 9                  /**< Serial number part length */


/**
 * @brief User data SEI types.
 */
typedef enum
{
    H264P_UNKNOWN_USER_DATA_SEI = 0,                    /**< Unknown user data SEI */
    H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V1,         /**< "Parrot Dragon Basic" v1 user data SEI */
    H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V1,      /**< "Parrot Dragon Extended" v1 user data SEI */
    H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V2,         /**< "Parrot Dragon Basic" v2 user data SEI */
    H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V2,      /**< "Parrot Dragon Extended" v2 user data SEI */

} H264P_ParrotUserDataSeiTypes_t;


/**
 * "Parrot Dragon Basic" v1 user data SEI.
 * UUID: 8818b6d5-4aff-45ad-ba04-bc0cbae6a5fd
 */
#define H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V1_UUID_0 0x8818b6d5      /**< "Parrot Dragon Basic" v1 user data SEI UUID part 1 (bit 0..31) */
#define H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V1_UUID_1 0x4aff45ad      /**< "Parrot Dragon Basic" v1 user data SEI UUID part 2 (bit 32..63) */
#define H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V1_UUID_2 0xba04bc0c      /**< "Parrot Dragon Basic" v1 user data SEI UUID part 3 (bit 64..95) */
#define H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V1_UUID_3 0xbae6a5fd      /**< "Parrot Dragon Basic" v1 user data SEI UUID part 4 (bit 96..127) */


/**
 * "Parrot Dragon Basic" v2 user data SEI.
 * UUID: f1433a75-e491-4bf5-aadf-455ddf6ac0a8
 */
#define H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V2_UUID_0 0xf1433a75      /**< "Parrot Dragon Basic" v2 user data SEI UUID part 1 (bit 0..31) */
#define H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V2_UUID_1 0xe4914bf5      /**< "Parrot Dragon Basic" v2 user data SEI UUID part 2 (bit 32..63) */
#define H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V2_UUID_2 0xaadf455d      /**< "Parrot Dragon Basic" v2 user data SEI UUID part 3 (bit 64..95) */
#define H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V2_UUID_3 0xdf6ac0a8      /**< "Parrot Dragon Basic" v2 user data SEI UUID part 4 (bit 96..127) */


/**
 * "Parrot Dragon Extended" v1 user data SEI.
 * UUID: 5aace927-933f-41ff-b863-af7e617532cf
 */
#define H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V1_UUID_0 0x5aace927     /**< "Parrot Dragon Extended" v1 user data SEI UUID part 1 (bit 0..31) */
#define H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V1_UUID_1 0x933f41ff     /**< "Parrot Dragon Extended" v1 user data SEI UUID part 2 (bit 32..63) */
#define H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V1_UUID_2 0xb863af7e     /**< "Parrot Dragon Extended" v1 user data SEI UUID part 3 (bit 64..95) */
#define H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V1_UUID_3 0x617532cf     /**< "Parrot Dragon Extended" v1 user data SEI UUID part 4 (bit 96..127) */

/**
 * "Parrot Dragon Extended" v2 user data SEI.
 * UUID: 937a509b-2f23-4df6-8be3-330569d3b5bb
 */
#define H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V2_UUID_0 0x937a509b     /**< "Parrot Dragon Extended" v2 user data SEI UUID part 1 (bit 0..31) */
#define H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V2_UUID_1 0x2f234df6     /**< "Parrot Dragon Extended" v2 user data SEI UUID part 2 (bit 32..63) */
#define H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V2_UUID_2 0x8be33305     /**< "Parrot Dragon Extended" v2 user data SEI UUID part 3 (bit 64..95) */
#define H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V2_UUID_3 0x69d3b5bb     /**< "Parrot Dragon Extended" v2 user data SEI UUID part 4 (bit 96..127) */


/**
 * @brief "Parrot Dragon Basic" v1 user data SEI.
 */
typedef struct
{
    uint32_t uuid[4];               /**< User data SEI UUID */
    uint32_t frameIndex;            /**< Frame index */
    uint32_t acquisitionTsH;        /**< Frame acquisition timestamp (high 32 bits) */
    uint32_t acquisitionTsL;        /**< Frame acquisition timestamp (low 32 bits) */
    uint32_t prevMse_fp8;           /**< Previous frame MSE, 8 bits fixed point value */

} H264P_ParrotDragonBasicUserDataSeiV1_t;


/**
 * @brief "Parrot Dragon Basic" v2 user data SEI.
 */
typedef struct
{
    uint32_t uuid[4];               /**< User data SEI UUID */
    uint32_t frameIndex;            /**< Frame index */
    uint32_t acquisitionTsH;        /**< Frame acquisition timestamp (high 32 bits) */
    uint32_t acquisitionTsL;        /**< Frame acquisition timestamp (low 32 bits) */

} H264P_ParrotDragonBasicUserDataSeiV2_t;


/**
 * @brief "Parrot Dragon Extended" v1 user data SEI.
 */
typedef struct
{
    uint32_t uuid[4];                                                       /**< User data SEI UUID */
    uint32_t frameIndex;                                                    /**< Frame index */
    uint32_t acquisitionTsH;                                                /**< Frame acquisition timestamp (high 32 bits) */
    uint32_t acquisitionTsL;                                                /**< Frame acquisition timestamp (low 32 bits) */
    uint32_t prevMse_fp8;                                                   /**< Previous frame MSE, 8 bits fixed point value */
    uint32_t batteryPercentage;                                             /**< Battery charge percentage */
    int32_t latitude_fp20;                                                  /**< GPS latitude (deg), 20 bits fixed point value */
    int32_t longitude_fp20;                                                 /**< GPS longitude (deg), 20 bits fixed point value */
    int32_t altitude_fp16;                                                  /**< GPS altitude (m), 16 bits fixed point value */
    int32_t absoluteHeight_fp16;                                            /**< Absolute height (m), 16 bits fixed point value */
    int32_t relativeHeight_fp16;                                            /**< Relative height (m), 16 bits fixed point value */
    int32_t xSpeed_fp16;                                                    /**< X speed (m/s), 16 bits fixed point value */
    int32_t ySpeed_fp16;                                                    /**< Y speed (m/s), 16 bits fixed point value */
    int32_t zSpeed_fp16;                                                    /**< Z speed (m/s), 16 bits fixed point value */
    uint32_t distance_fp16;                                                 /**< Distance from home (m), 16 bits fixed point value */
    int32_t heading_fp16;                                                   /**< Heading (rad), 16 bits fixed point value */
    int32_t yaw_fp16;                                                       /**< Yaw/psi (rad), 16 bits fixed point value */
    int32_t pitch_fp16;                                                     /**< Pitch/theta (rad), 16 bits fixed point value */
    int32_t roll_fp16;                                                      /**< Roll/phi (rad), 16 bits fixed point value */
    int32_t cameraPan_fp16;                                                 /**< Camera pan (rad), 16 bits fixed point value */
    int32_t cameraTilt_fp16;                                                /**< Camera tilt (rad), 16 bits fixed point value */
    uint32_t videoStreamingTargetBitrate;                                   /**< Video srteaming encoder target bitrate (bit/s) */
    int32_t  wifiRssi;                                                      /**< Wifi RSSI (dBm) */
    uint32_t wifiMcsRate;                                                   /**< Wifi MCS rate (bit/s) */
    uint32_t wifiTxRate;                                                    /**< Wifi Tx rate (bit/s) */
    uint32_t wifiRxRate;                                                    /**< Wifi Rx rate (bit/s) */
    uint32_t wifiTxFailRate;                                                /**< Wifi Tx fail rate (#/s) */
    uint32_t wifiTxErrorRate;                                               /**< Wifi Tx error rate (#/s) */
    uint32_t postReprojTimestampDelta;                                      /**< Post-reprojection time */
    uint32_t postEeTimestampDelta;                                          /**< Post-EE time */
    uint32_t postScalingTimestampDelta;                                     /**< Post-scaling time */
    uint32_t postStreamingEncodingTimestampDelta;                           /**< Post-streaming-encoding time */
    uint32_t postNetworkInputTimestampDelta;                                /**< Post-network-input time */
    uint32_t systemTsH;                                                     /**< Frame acquisition timestamp (high 32 bits) */
    uint32_t systemTsL;                                                     /**< Frame acquisition timestamp (low 32 bits) */
    char serialNumberH[H264P_PARROT_DRAGON_SERIAL_NUMBER_PART_LENGTH + 1];  /**< Serial number high part */
    char serialNumberL[H264P_PARROT_DRAGON_SERIAL_NUMBER_PART_LENGTH + 1];  /**< Serial number low part */

} H264P_ParrotDragonExtendedUserDataSeiV1_t;


/**
 * @brief "Parrot Dragon Extended" v2 user data SEI.
 */
typedef struct
{
    uint32_t uuid[4];                                                       /**< User data SEI UUID */
    uint32_t frameIndex;                                                    /**< Frame index */
    uint32_t acquisitionTsH;                                                /**< Frame acquisition timestamp (high 32 bits) */
    uint32_t acquisitionTsL;                                                /**< Frame acquisition timestamp (low 32 bits) */
    uint32_t batteryPercentage;                                             /**< Battery charge percentage */
    int32_t latitude_fp20;                                                  /**< GPS latitude (deg), 20 bits fixed point value */
    int32_t longitude_fp20;                                                 /**< GPS longitude (deg), 20 bits fixed point value */
    int32_t altitude_fp16;                                                  /**< GPS altitude (m), 16 bits fixed point value */
    int32_t absoluteHeight_fp16;                                            /**< Absolute height (m), 16 bits fixed point value */
    int32_t relativeHeight_fp16;                                            /**< Relative height (m), 16 bits fixed point value */
    int32_t xSpeed_fp16;                                                    /**< X speed (m/s), 16 bits fixed point value */
    int32_t ySpeed_fp16;                                                    /**< Y speed (m/s), 16 bits fixed point value */
    int32_t zSpeed_fp16;                                                    /**< Z speed (m/s), 16 bits fixed point value */
    uint32_t distance_fp16;                                                 /**< Distance from home (m), 16 bits fixed point value */
    int32_t yaw_fp16;                                                       /**< Yaw/psi (rad), 16 bits fixed point value */
    int32_t pitch_fp16;                                                     /**< Pitch/theta (rad), 16 bits fixed point value */
    int32_t roll_fp16;                                                      /**< Roll/phi (rad), 16 bits fixed point value */
    int32_t cameraPan_fp16;                                                 /**< Camera pan (rad), 16 bits fixed point value */
    int32_t cameraTilt_fp16;                                                /**< Camera tilt (rad), 16 bits fixed point value */
    uint32_t videoStreamingTargetBitrate;                                   /**< Video streaming encoder target bitrate (bit/s) */
    uint32_t videoStreamingDecimation;                                      /**< Video streaming encoder decimation factor */
    uint32_t videoStreamingGopLength;                                       /**< Video streaming encoder GOP length */
    int32_t videoStreamingPrevFrameType;                                    /**< Video streaming previous frame type */
    uint32_t videoStreamingPrevFrameSize;                                   /**< Video streaming previous frame size (bytes) */
    uint32_t videoStreamingPrevFrameMseY_fp8;                               /**< Video streaming previous frame MSE, 8 bits fixed point value */
    int32_t videoRecordingPrevFrameType;                                    /**< Video recording previous frame type */
    uint32_t videoRecordingPrevFrameSize;                                   /**< Video recording previous frame size (bytes) */
    uint32_t videoRecordingPrevFrameMseY_fp8;                               /**< Video recording previous frame MSE, 8 bits fixed point value */
    int32_t  wifiRssi;                                                      /**< Wifi RSSI (dBm) */
    uint32_t wifiMcsRate;                                                   /**< Wifi MCS rate (bit/s) */
    uint32_t wifiTxRate;                                                    /**< Wifi Tx rate (bit/s) */
    uint32_t wifiRxRate;                                                    /**< Wifi Rx rate (bit/s) */
    uint32_t wifiTxFailRate;                                                /**< Wifi Tx fail rate (#/s) */
    uint32_t wifiTxErrorRate;                                               /**< Wifi Tx error rate (#/s) */
    uint32_t preReprojTimestampDelta;                                       /**< Pre-reprojection time */
    uint32_t postReprojTimestampDelta;                                      /**< Post-reprojection time */
    uint32_t postEeTimestampDelta;                                          /**< Post-EE time */
    uint32_t postScalingTimestampDelta;                                     /**< Post-scaling time */
    uint32_t postStreamingEncodingTimestampDelta;                           /**< Post-streaming-encoding time */
    uint32_t postRecordingEncodingTimestampDelta;                           /**< Post-recording-encoding time */
    uint32_t postNetworkInputTimestampDelta;                                /**< Post-network-input time */
    uint32_t systemTsH;                                                     /**< Frame acquisition timestamp (high 32 bits) */
    uint32_t systemTsL;                                                     /**< Frame acquisition timestamp (low 32 bits) */
    uint32_t streamingMonitorTimeInterval;                                  /**< Streaming monitoring time interval in microseconds */
    uint32_t streamingMeanAcqToNetworkTime;                                 /**< Streaming mean acquisition to network time */
    uint32_t streamingAcqToNetworkJitter;                                   /**< Streaming acquisition to network time jitter */
    uint32_t streamingMeanNetworkTime;                                      /**< Streaming mean acquisition to network time */
    uint32_t streamingNetworkJitter;                                        /**< Streaming acquisition to network time jitter */
    uint32_t streamingBytesSent;                                            /**< Streaming bytes sent during the interval */
    uint32_t streamingMeanPacketSize;                                       /**< Streaming mean packet size */
    uint32_t streamingPacketSizeStdDev;                                     /**< Streaming packet size standard deviation */
    uint32_t streamingPacketsSent;                                          /**< Streaming packets sent during the interval */
    uint32_t streamingBytesDropped;                                         /**< Streaming bytes dropped during the interval */
    uint32_t streamingNaluDropped;                                          /**< Streaming NAL units dropped during the interval */
    char serialNumberH[H264P_PARROT_DRAGON_SERIAL_NUMBER_PART_LENGTH + 1];  /**< Serial number high part */
    char serialNumberL[H264P_PARROT_DRAGON_SERIAL_NUMBER_PART_LENGTH + 1];  /**< Serial number low part */

} H264P_ParrotDragonExtendedUserDataSeiV2_t;


/**
 * @brief Get the user data SEI type.
 *
 * The function returns the user data SEI type if it is recognized.
 *
 * @param pBuf Pointer to the user data SEI buffer.
 * @param bufSize Size of the user data SEI.
 *
 * @return The user data SEI type if it is recognized.
 * @return H264P_UNKNOWN_USER_DATA_SEI if the user data SEI type is not recognized.
 * @return -1 if an error occurred.
 */
H264P_ParrotUserDataSeiTypes_t H264P_getParrotUserDataSeiType(const void* pBuf, unsigned int bufSize);


/**
 * @brief Parse a "Parrot Dragon Basic" v1 user data SEI.
 *
 * The function parses a "Parrot Dragon Basic" v1 user data SEI with UUID 8818b6d5-4aff-45ad-ba04-bc0cbae6a5fd and fills the userDataSei structure.
 *
 * @param pBuf Pointer to the user data SEI buffer.
 * @param bufSize Size of the user data SEI.
 * @param userDataSei Pointer to the structure to fill.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_parseParrotDragonBasicUserDataSeiV1(const void* pBuf, unsigned int bufSize, H264P_ParrotDragonBasicUserDataSeiV1_t *userDataSei);


/**
 * @brief Parse a "Parrot Dragon Basic" v2 user data SEI.
 *
 * The function parses a "Parrot Dragon Basic" v2 user data SEI with UUID f1433a75-e491-4bf5-aadf-455ddf6ac0a8 and fills the userDataSei structure.
 *
 * @param pBuf Pointer to the user data SEI buffer.
 * @param bufSize Size of the user data SEI.
 * @param userDataSei Pointer to the structure to fill.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_parseParrotDragonBasicUserDataSeiV2(const void* pBuf, unsigned int bufSize, H264P_ParrotDragonBasicUserDataSeiV2_t *userDataSei);


/**
 * @brief Parse a "Parrot Dragon Extended" v1 user data SEI.
 *
 * The function parses a "Parrot Dragon Extended" v1 user data SEI with UUID 5aace927-933f-41ff-b863-af7e617532cf and fills the userDataSei structure.
 *
 * @param pBuf Pointer to the user data SEI buffer.
 * @param bufSize Size of the user data SEI.
 * @param userDataSei Pointer to the structure to fill.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_parseParrotDragonExtendedUserDataSeiV1(const void* pBuf, unsigned int bufSize, H264P_ParrotDragonExtendedUserDataSeiV1_t *userDataSei);


/**
 * @brief Parse a "Parrot Dragon Extended" v2 user data SEI.
 *
 * The function parses a "Parrot Dragon Extended" v2 user data SEI with UUID 937a509b-2f23-4df6-8be3-330569d3b5bb and fills the userDataSei structure.
 *
 * @param pBuf Pointer to the user data SEI buffer.
 * @param bufSize Size of the user data SEI.
 * @param userDataSei Pointer to the structure to fill.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_parseParrotDragonExtendedUserDataSeiV2(const void* pBuf, unsigned int bufSize, H264P_ParrotDragonExtendedUserDataSeiV2_t *userDataSei);


#ifdef __cplusplus
}
#endif

#endif //#ifndef H264P_PARROT_H

