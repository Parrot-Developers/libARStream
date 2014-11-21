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
/// @file

/**
 *  @brief Enumeration of data formats for numeric data.
 **/
typedef enum _CPTDataTypeFormat {
    CPTUndefinedDataType = 0,        ///< Undefined
    CPTIntegerDataType,              ///< Integer
    CPTUnsignedIntegerDataType,      ///< Unsigned integer
    CPTFloatingPointDataType,        ///< Floating point
    CPTComplexFloatingPointDataType, ///< Complex floating point
    CPTDecimalDataType               ///< NSDecimal
}
CPTDataTypeFormat;

/**
 *  @brief Enumeration of memory arrangements for multi-dimensional data arrays.
 *  @see See <a href="http://en.wikipedia.org/wiki/Row-major_order">Wikipedia</a> for more information.
 **/
typedef enum _CPTDataOrder {
    CPTDataOrderRowsFirst,   ///< Numeric data is arranged in row-major order.
    CPTDataOrderColumnsFirst ///< Numeric data is arranged in column-major order.
}
CPTDataOrder;

/**
 *  @brief Structure that describes the encoding of numeric data samples.
 **/
typedef struct _CPTNumericDataType {
    CPTDataTypeFormat dataTypeFormat; ///< Data type format
    size_t sampleBytes;               ///< Number of bytes in each sample
    CFByteOrder byteOrder;            ///< Byte order
}
CPTNumericDataType;

#if __cplusplus
extern "C" {
#endif

/// @name Data Type Utilities
/// @{
CPTNumericDataType CPTDataType(CPTDataTypeFormat format, size_t sampleBytes, CFByteOrder byteOrder);
CPTNumericDataType CPTDataTypeWithDataTypeString(NSString *dataTypeString);
NSString *CPTDataTypeStringFromDataType(CPTNumericDataType dataType);
BOOL CPTDataTypeIsSupported(CPTNumericDataType format);
BOOL CPTDataTypeEqualToDataType(CPTNumericDataType dataType1, CPTNumericDataType dataType2);

/// @}

#if __cplusplus
}
#endif
