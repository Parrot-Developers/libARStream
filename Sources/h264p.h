#ifndef H264P_H
#define H264P_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif //#ifdef __cplusplus


/**
 * @brief H264P instance handle.
 */
typedef void* H264P_Handle;


/**
 * @brief H264P configuration for initialization.
 */
typedef struct {
    int extractUserDataSei;                 /**< enable user data SEI extraction, see H264P_GetUserDataSei() */
    int printLogs;                          /**< output parsing logs to stdout */

} H264P_Config_t;


/**
 * @brief Output slice information.
 */
typedef struct {
    int idrPicFlag;                         /**< idrPicFlag syntax element */
    int nal_ref_idc;                        /**< nal_ref_idc syntax element */
    int nal_unit_type;                      /**< nal_unit_type syntax element */
    int first_mb_in_slice;                  /**< first_mb_in_slice syntax element */
    int slice_type;                         /**< slice_type syntax element */
    int sliceTypeMod5;                      /**< slice_type syntax element modulo 5 */
    int frame_num;                          /**< frame_num syntax element */
    int idr_pic_id;                         /**< idr_pic_id syntax element */
    int slice_qp_delta;                     /**< slice_qp_delta syntax element */
    int disable_deblocking_filter_idc;      /**< disable_deblocking_filter_idc syntax element */

} H264P_SliceInfo_t;


/**
 * @brief Initialize an H264P instance.
 *
 * The library allocates the required resources. The user must call H264P_Free() to free the resources.
 *
 * @param parserHandle Pointer to the handle used in future calls to the library.
 * @param config The instance configuration.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_Init(H264P_Handle* parserHandle, H264P_Config_t* config);


/**
 * @brief Free an H264P instance.
 *
 * The library frees the allocated resources.
 *
 * @param parserHandle Instance handle.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_Free(H264P_Handle parserHandle);


/**
 * @brief Read the next NAL unit from a file.
 *
 * The function finds the next NALU start and end in the file. The NALU shall then be parsed using the H264P_ParseNalu() function.
 *
 * @param parserHandle Instance handle.
 * @param fp Opened file to parse.
 * @param fileSize Total file size.
 *
 * @return 0 if no error occurred.
 * @return -2 if no start code has been found.
 * @return -1 if an error occurred.
 */
int H264P_ReadNextNalu_file(H264P_Handle parserHandle, FILE* fp, unsigned int fileSize);


/**
 * @brief Read the next NAL unit from a buffer.
 *
 * The function finds the next NALU start and end in the buffer. The NALU shall then be parsed using the H264P_ParseNalu() function.
 *
 * @param parserHandle Instance handle.
 * @param buf Buffer to parse.
 * @param bufSize Buffer size.
 * @param nextStartCodePos Optional pointer to the following NALU start code filled if one has been found (i.e. if more NALUs are present 
 * in the buffer).
 *
 * @return 0 if no error occurred.
 * @return -2 if no start code has been found.
 * @return -1 if an error occurred.
 */
int H264P_ReadNextNalu_buffer(H264P_Handle parserHandle, void* pBuf, unsigned int bufSize, unsigned int* nextStartCodePos);


/**
 * @brief Parse the NAL unit.
 *
 * The function parses the current NAL unit. A call either to H264P_ReadNextNalu_file() or H264P_ReadNextNalu_buffer() must have been made 
 * prior to calling this function.
 *
 * @param parserHandle Instance handle.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_ParseNalu(H264P_Handle parserHandle);


/**
 * @brief Get the NAL unit type.
 *
 * The function returns the NALU type of the last parsed NALU. A call to H264P_ParseNalu() must have been made prior to calling this function.
 *
 * @param parserHandle Instance handle.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_GetLastNaluType(H264P_Handle parserHandle);


/**
 * @brief Get the slice info.
 *
 * The function returns the slice info of the last parsed slice. A call to H264P_ParseNalu() must have been made prior to calling this function. 
 * This function should only be called if the last NALU type is either 1 or 5 (coded slice).
 *
 * @param parserHandle Instance handle.
 * @param sliceInfo Pointer to the slice info structure to fill.
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_GetSliceInfo(H264P_Handle parserHandle, H264P_SliceInfo_t* sliceInfo);


/**
 * @brief Get the user data SEI.
 *
 * The function gets a pointer to the last user data SEI parsed. A call to H264P_ParseNalu() must have been made prior to calling this function. 
 * This function should only be called if the last NALU type is 6 and the library instance has been initialized with extractUserDataSei = 1 in the config.
 * If no user data SEI has been found or if extractUserDataSei == 0 the function fills pBuf with a NULL pointer.
 * 
 * @warning The returned pointer is managed by the library and must not be freed.
 *
 * @param parserHandle Instance handle.
 * @param pBuf Pointer to the user data SEI buffer pointer (filled with NULL if no user data SEI is available).
 * @param bufSize Pointer to the user data SEI buffer size (filled with 0 if no user data SEI is available).
 *
 * @return 0 if no error occurred.
 * @return -1 if an error occurred.
 */
int H264P_GetUserDataSei(H264P_Handle parserHandle, void** pBuf, unsigned int* bufSize);


#ifdef __cplusplus
}
#endif //#ifdef __cplusplus

#endif //#ifndef H264P_H

