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
#import "CPTNumericDataType.h"

@interface CPTNumericData : NSObject<NSCopying, NSMutableCopying, NSCoding> {
    @protected
    NSData *data;
    CPTNumericDataType dataType;
    NSArray *shape; // array of dimension shapes (NSNumber<unsigned>)
    CPTDataOrder dataOrder;
}

/// @name Data Buffer
/// @{
@property (readonly, copy) NSData *data;
@property (readonly, assign) const void *bytes;
@property (readonly, assign) NSUInteger length;
/// @}

/// @name Data Format
/// @{
@property (readonly, assign) CPTNumericDataType dataType;
@property (readonly, assign) CPTDataTypeFormat dataTypeFormat;
@property (readonly, assign) size_t sampleBytes;
@property (readonly, assign) CFByteOrder byteOrder;
/// @}

/// @name Dimensions
/// @{
@property (readonly, copy) NSArray *shape;
@property (readonly, assign) NSUInteger numberOfDimensions;
@property (readonly, assign) NSUInteger numberOfSamples;
@property (readonly, assign) CPTDataOrder dataOrder;
/// @}

/// @name Factory Methods
/// @{
+(id)numericDataWithData:(NSData *)newData dataType:(CPTNumericDataType)newDataType shape:(NSArray *)shapeArray;
+(id)numericDataWithData:(NSData *)newData dataTypeString:(NSString *)newDataTypeString shape:(NSArray *)shapeArray;
+(id)numericDataWithArray:(NSArray *)newData dataType:(CPTNumericDataType)newDataType shape:(NSArray *)shapeArray;
+(id)numericDataWithArray:(NSArray *)newData dataTypeString:(NSString *)newDataTypeString shape:(NSArray *)shapeArray;

+(id)numericDataWithData:(NSData *)newData dataType:(CPTNumericDataType)newDataType shape:(NSArray *)shapeArray dataOrder:(CPTDataOrder)order;
+(id)numericDataWithData:(NSData *)newData dataTypeString:(NSString *)newDataTypeString shape:(NSArray *)shapeArray dataOrder:(CPTDataOrder)order;
+(id)numericDataWithArray:(NSArray *)newData dataType:(CPTNumericDataType)newDataType shape:(NSArray *)shapeArray dataOrder:(CPTDataOrder)order;
+(id)numericDataWithArray:(NSArray *)newData dataTypeString:(NSString *)newDataTypeString shape:(NSArray *)shapeArray dataOrder:(CPTDataOrder)order;
/// @}

/// @name Initialization
/// @{
-(id)initWithData:(NSData *)newData dataType:(CPTNumericDataType)newDataType shape:(NSArray *)shapeArray;
-(id)initWithData:(NSData *)newData dataTypeString:(NSString *)newDataTypeString shape:(NSArray *)shapeArray;
-(id)initWithArray:(NSArray *)newData dataType:(CPTNumericDataType)newDataType shape:(NSArray *)shapeArray;
-(id)initWithArray:(NSArray *)newData dataTypeString:(NSString *)newDataTypeString shape:(NSArray *)shapeArray;

-(id)initWithData:(NSData *)newData dataType:(CPTNumericDataType)newDataType shape:(NSArray *)shapeArray dataOrder:(CPTDataOrder)order;
-(id)initWithData:(NSData *)newData dataTypeString:(NSString *)newDataTypeString shape:(NSArray *)shapeArray dataOrder:(CPTDataOrder)order;
-(id)initWithArray:(NSArray *)newData dataType:(CPTNumericDataType)newDataType shape:(NSArray *)shapeArray dataOrder:(CPTDataOrder)order;
-(id)initWithArray:(NSArray *)newData dataTypeString:(NSString *)newDataTypeString shape:(NSArray *)shapeArray dataOrder:(CPTDataOrder)order;
/// @}

/// @name Samples
/// @{
-(NSUInteger)sampleIndex:(NSUInteger)idx, ...;
-(void *)samplePointer:(NSUInteger)sample;
-(void *)samplePointerAtIndex:(NSUInteger)idx, ...;
-(NSNumber *)sampleValue:(NSUInteger)sample;
-(NSNumber *)sampleValueAtIndex:(NSUInteger)idx, ...;
-(NSArray *)sampleArray;
/// @}

@end
