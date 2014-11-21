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
#import "CPTMutableNumericData+TypeConversion.h"

#import "CPTNumericData+TypeConversion.h"

@implementation CPTMutableNumericData(TypeConversion)

/** @property CPTNumericDataType dataType
 *  @brief The type of data stored in the data buffer.
 **/
@dynamic dataType;

/** @property CPTDataTypeFormat dataTypeFormat
 *  @brief The format of the data stored in the data buffer.
 **/
@dynamic dataTypeFormat;

/** @property size_t sampleBytes
 *  @brief The number of bytes in a single sample of data.
 **/
@dynamic sampleBytes;

/** @property CFByteOrder byteOrder
 *  @brief The byte order used to store each sample in the data buffer.
 **/
@dynamic byteOrder;

/** @brief Converts the current numeric data to a new data type.
 *  @param newDataType The new data type format.
 *  @param newSampleBytes The number of bytes used to store each sample.
 *  @param newByteOrder The new byte order.
 *  @return A copy of the current numeric data converted to the new data type.
 **/
-(void)convertToType:(CPTDataTypeFormat)newDataType
         sampleBytes:(size_t)newSampleBytes
           byteOrder:(CFByteOrder)newByteOrder
{
    self.dataType = CPTDataType(newDataType, newSampleBytes, newByteOrder);
}

#pragma mark -
#pragma mark Accessors

/// @cond

-(void)setDataTypeFormat:(CPTDataTypeFormat)newDataTypeFormat
{
    CPTNumericDataType myDataType = self.dataType;

    if ( newDataTypeFormat != myDataType.dataTypeFormat ) {
        self.dataType = CPTDataType(newDataTypeFormat, myDataType.sampleBytes, myDataType.byteOrder);
    }
}

-(void)setSampleBytes:(size_t)newSampleBytes
{
    CPTNumericDataType myDataType = self.dataType;

    if ( newSampleBytes != myDataType.sampleBytes ) {
        self.dataType = CPTDataType(myDataType.dataTypeFormat, newSampleBytes, myDataType.byteOrder);
    }
}

-(void)setByteOrder:(CFByteOrder)newByteOrder
{
    CPTNumericDataType myDataType = self.dataType;

    if ( newByteOrder != myDataType.byteOrder ) {
        self.dataType = CPTDataType(myDataType.dataTypeFormat, myDataType.sampleBytes, newByteOrder);
    }
}

-(void)setDataType:(CPTNumericDataType)newDataType
{
    CPTNumericDataType myDataType = self.dataType;

    if ( (myDataType.dataTypeFormat == newDataType.dataTypeFormat) &&
         (myDataType.sampleBytes == newDataType.sampleBytes) &&
         (myDataType.byteOrder == newDataType.byteOrder) ) {
        return;
    }

    NSParameterAssert(myDataType.dataTypeFormat != CPTUndefinedDataType);
    NSParameterAssert(myDataType.byteOrder != CFByteOrderUnknown);

    NSParameterAssert( CPTDataTypeIsSupported(newDataType) );
    NSParameterAssert(newDataType.dataTypeFormat != CPTUndefinedDataType);
    NSParameterAssert(newDataType.byteOrder != CFByteOrderUnknown);

    dataType = newDataType;

    if ( ( myDataType.sampleBytes == sizeof(int8_t) ) && ( newDataType.sampleBytes == sizeof(int8_t) ) ) {
        return;
    }

    NSMutableData *myData     = (NSMutableData *)self.data;
    CFByteOrder hostByteOrder = CFByteOrderGetCurrent();

    NSUInteger sampleCount = myData.length / myDataType.sampleBytes;

    if ( myDataType.byteOrder != hostByteOrder ) {
        [self swapByteOrderForData:myData sampleSize:myDataType.sampleBytes];
    }

    if ( newDataType.sampleBytes > myDataType.sampleBytes ) {
        NSMutableData *newData = [[NSMutableData alloc] initWithLength:(sampleCount * newDataType.sampleBytes)];
        [self convertData:myData dataType:&myDataType toData:newData dataType:&newDataType];
        [data release];
        data   = newData;
        myData = newData;
    }
    else {
        [self convertData:myData dataType:&myDataType toData:myData dataType:&newDataType];
        myData.length = sampleCount * newDataType.sampleBytes;
    }

    if ( newDataType.byteOrder != hostByteOrder ) {
        [self swapByteOrderForData:myData sampleSize:newDataType.sampleBytes];
    }
}

/// @endcond

@end
