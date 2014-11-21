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
#import "CPTMutableNumericDataTypeConversionTests.h"

#import "CPTMutableNumericData+TypeConversion.h"
#import "CPTUtilities.h"

static const NSUInteger numberOfSamples = 5;
static const double precision           = 1.0e-6;

@implementation CPTMutableNumericDataTypeConversionTests

-(void)testFloatToDoubleInPlaceConversion
{
    NSMutableData *data = [NSMutableData dataWithLength:numberOfSamples * sizeof(float)];
    float *samples      = (float *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = sinf(i);
    }

    CPTMutableNumericData *numericData = [[CPTMutableNumericData alloc] initWithData:data
                                                                            dataType:CPTDataType( CPTFloatingPointDataType, sizeof(float), NSHostByteOrder() )
                                                                               shape:nil];

    numericData.sampleBytes = sizeof(double);

    const double *doubleSamples = (const double *)[numericData.data bytes];
    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        STAssertEqualsWithAccuracy( (double)samples[i], doubleSamples[i], precision, @"(float)%g != (double)%g", samples[i], doubleSamples[i] );
    }
    [numericData release];
}

-(void)testDoubleToFloatInPlaceConversion
{
    NSMutableData *data = [NSMutableData dataWithLength:numberOfSamples * sizeof(double)];
    double *samples     = (double *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = sin(i);
    }

    CPTMutableNumericData *numericData = [[CPTMutableNumericData alloc] initWithData:data
                                                                            dataType:CPTDataType( CPTFloatingPointDataType, sizeof(double), NSHostByteOrder() )
                                                                               shape:nil];

    numericData.sampleBytes = sizeof(float);

    const float *floatSamples = (const float *)[numericData.data bytes];
    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        STAssertEqualsWithAccuracy( (double)floatSamples[i], samples[i], precision, @"(float)%g != (double)%g", floatSamples[i], samples[i] );
    }
    [numericData release];
}

-(void)testFloatToIntegerInPlaceConversion
{
    NSMutableData *data = [NSMutableData dataWithLength:numberOfSamples * sizeof(float)];
    float *samples      = (float *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = sinf(i) * 1000.0f;
    }

    CPTMutableNumericData *numericData = [[CPTMutableNumericData alloc] initWithData:data
                                                                            dataType:CPTDataType( CPTFloatingPointDataType, sizeof(float), NSHostByteOrder() )
                                                                               shape:nil];

    numericData.dataType = CPTDataType( CPTIntegerDataType, sizeof(NSInteger), NSHostByteOrder() );

    const NSInteger *intSamples = (const NSInteger *)[numericData.data bytes];
    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        STAssertEqualsWithAccuracy( (NSInteger)samples[i], intSamples[i], precision, @"(float)%g != (NSInteger)%ld", samples[i], (long)intSamples[i] );
    }
    [numericData release];
}

-(void)testIntegerToFloatInPlaceConversion
{
    NSMutableData *data = [NSMutableData dataWithLength:numberOfSamples * sizeof(NSInteger)];
    NSInteger *samples  = (NSInteger *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = (NSInteger)(sin(i) * 1000.0);
    }

    CPTMutableNumericData *numericData = [[CPTMutableNumericData alloc] initWithData:data
                                                                            dataType:CPTDataType( CPTIntegerDataType, sizeof(NSInteger), NSHostByteOrder() )
                                                                               shape:nil];

    numericData.dataType = CPTDataType( CPTFloatingPointDataType, sizeof(float), NSHostByteOrder() );

    const float *floatSamples = (const float *)[numericData.data bytes];
    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        STAssertEqualsWithAccuracy(floatSamples[i], (float)samples[i], precision, @"(float)%g != (NSInteger)%ld", floatSamples[i], (long)samples[i]);
    }
    [numericData release];
}

-(void)testDecimalToDoubleInPlaceConversion
{
    NSMutableData *data = [NSMutableData dataWithLength:numberOfSamples * sizeof(NSDecimal)];
    NSDecimal *samples  = (NSDecimal *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = CPTDecimalFromDouble( sin(i) );
    }

    CPTMutableNumericData *numericData = [[CPTMutableNumericData alloc] initWithData:data
                                                                            dataType:CPTDataType( CPTDecimalDataType, sizeof(NSDecimal), NSHostByteOrder() )
                                                                               shape:nil];

    numericData.dataType = CPTDataType( CPTFloatingPointDataType, sizeof(double), NSHostByteOrder() );

    const double *doubleSamples = (const double *)[numericData.data bytes];
    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        STAssertEquals(CPTDecimalDoubleValue(samples[i]), doubleSamples[i], @"(NSDecimal)%@ != (double)%g", CPTDecimalStringValue(samples[i]), doubleSamples[i]);
    }
    [numericData release];
}

-(void)testDoubleToDecimalInPlaceConversion
{
    NSMutableData *data = [NSMutableData dataWithLength:numberOfSamples * sizeof(double)];
    double *samples     = (double *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = sin(i);
    }

    CPTMutableNumericData *numericData = [[CPTMutableNumericData alloc] initWithData:data
                                                                            dataType:CPTDataType( CPTFloatingPointDataType, sizeof(double), NSHostByteOrder() )
                                                                               shape:nil];

    numericData.dataType = CPTDataType( CPTDecimalDataType, sizeof(NSDecimal), NSHostByteOrder() );

    const NSDecimal *decimalSamples = (const NSDecimal *)[numericData.data bytes];
    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        STAssertTrue(CPTDecimalEquals( decimalSamples[i], CPTDecimalFromDouble(samples[i]) ), @"(NSDecimal)%@ != (double)%g", CPTDecimalStringValue(decimalSamples[i]), samples[i]);
    }
    [numericData release];
}

-(void)testTypeConversionSwapsByteOrderIntegerInPlace
{
    CFByteOrder hostByteOrder    = CFByteOrderGetCurrent();
    CFByteOrder swappedByteOrder = (hostByteOrder == CFByteOrderBigEndian) ? CFByteOrderLittleEndian : CFByteOrderBigEndian;

    uint32_t start    = 1000;
    NSData *startData = [NSData dataWithBytesNoCopy:&start
                                             length:sizeof(uint32_t)
                                       freeWhenDone:NO];

    CPTMutableNumericData *numericData = [[CPTMutableNumericData alloc] initWithData:startData
                                                                            dataType:CPTDataType(CPTUnsignedIntegerDataType, sizeof(uint32_t), hostByteOrder)
                                                                               shape:nil];

    numericData.byteOrder = swappedByteOrder;

    uint32_t end = *(const uint32_t *)numericData.bytes;
    STAssertEquals(CFSwapInt32(start), end, @"Bytes swapped");

    numericData.byteOrder = hostByteOrder;

    uint32_t startRoundTrip = *(const uint32_t *)numericData.bytes;
    STAssertEquals(start, startRoundTrip, @"Round trip");
    [numericData release];
}

-(void)testTypeConversionSwapsByteOrderDoubleInPlace
{
    CFByteOrder hostByteOrder    = CFByteOrderGetCurrent();
    CFByteOrder swappedByteOrder = (hostByteOrder == CFByteOrderBigEndian) ? CFByteOrderLittleEndian : CFByteOrderBigEndian;

    double start      = 1000.0;
    NSData *startData = [NSData dataWithBytesNoCopy:&start
                                             length:sizeof(double)
                                       freeWhenDone:NO];

    CPTMutableNumericData *numericData = [[CPTMutableNumericData alloc] initWithData:startData
                                                                            dataType:CPTDataType(CPTFloatingPointDataType, sizeof(double), hostByteOrder)
                                                                               shape:nil];

    numericData.byteOrder = swappedByteOrder;

    uint64_t end = *(const uint64_t *)numericData.bytes;
    union swap {
        double v;
        CFSwappedFloat64 sv;
    }
    result;
    result.v = start;
    STAssertEquals(CFSwapInt64(result.sv.v), end, @"Bytes swapped");

    numericData.byteOrder = hostByteOrder;

    double startRoundTrip = *(const double *)numericData.bytes;
    STAssertEquals(start, startRoundTrip, @"Round trip");
    [numericData release];
}

@end
