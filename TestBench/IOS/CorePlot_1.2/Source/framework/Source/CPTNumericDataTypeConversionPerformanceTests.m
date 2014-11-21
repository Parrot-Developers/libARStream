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
#import "CPTNumericDataTypeConversionPerformanceTests.h"

#import "CPTNumericData+TypeConversion.h"
#import <mach/mach_time.h>

static const size_t numberOfSamples  = 10000000;
static const NSUInteger numberOfReps = 5;

@implementation CPTNumericDataTypeConversionPerformanceTests

-(void)testFloatToDoubleConversion
{
    NSMutableData *data = [[NSMutableData alloc] initWithLength:numberOfSamples * sizeof(float)];
    float *samples      = (float *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = sinf(i);
    }

    CPTNumericData *floatNumericData = [[CPTNumericData alloc] initWithData:data
                                                                   dataType:CPTDataType( CPTFloatingPointDataType, sizeof(float), CFByteOrderGetCurrent() )
                                                                      shape:nil];

    [data release];

    mach_timebase_info_data_t time_base_info;
    mach_timebase_info(&time_base_info);

    NSUInteger iterations = 0;
    uint64_t elapsed      = 0;

    for ( NSUInteger i = 0; i < numberOfReps; i++ ) {
        uint64_t start = mach_absolute_time();

        CPTNumericData *doubleNumericData = [floatNumericData dataByConvertingToType:CPTFloatingPointDataType sampleBytes:sizeof(double) byteOrder:CFByteOrderGetCurrent()];

        uint64_t now = mach_absolute_time();

        elapsed += now - start;

        [[doubleNumericData retain] release];
        iterations++;
    }

    double avgTime = 1.0e-6 * (double)(elapsed * time_base_info.numer / time_base_info.denom) / iterations;
    STFail(@"Avg. time = %g ms for %lu points.", avgTime, (unsigned long)numberOfSamples);

    [floatNumericData release];
}

-(void)testDoubleToFloatConversion
{
    NSMutableData *data = [[NSMutableData alloc] initWithLength:numberOfSamples * sizeof(double)];
    double *samples     = (double *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = sin(i);
    }

    CPTNumericData *doubleNumericData = [[CPTNumericData alloc] initWithData:data
                                                                    dataType:CPTDataType( CPTFloatingPointDataType, sizeof(double), CFByteOrderGetCurrent() )
                                                                       shape:nil];

    [data release];

    mach_timebase_info_data_t time_base_info;
    mach_timebase_info(&time_base_info);

    NSUInteger iterations = 0;
    uint64_t elapsed      = 0;

    for ( NSUInteger i = 0; i < numberOfReps; i++ ) {
        uint64_t start = mach_absolute_time();

        CPTNumericData *floatNumericData = [doubleNumericData dataByConvertingToType:CPTFloatingPointDataType sampleBytes:sizeof(float) byteOrder:CFByteOrderGetCurrent()];

        uint64_t now = mach_absolute_time();

        elapsed += now - start;

        [[floatNumericData retain] release];
        iterations++;
    }

    double avgTime = 1.0e-6 * (double)(elapsed * time_base_info.numer / time_base_info.denom) / iterations;
    STFail(@"Avg. time = %g ms for %lu points.", avgTime, (unsigned long)numberOfSamples);

    [doubleNumericData release];
}

-(void)testIntegerToDoubleConversion
{
    NSMutableData *data = [[NSMutableData alloc] initWithLength:numberOfSamples * sizeof(NSInteger)];
    NSInteger *samples  = (NSInteger *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = (NSInteger)(sin(i) * 1000.0);
    }

    CPTNumericData *integerNumericData = [[CPTNumericData alloc] initWithData:data
                                                                     dataType:CPTDataType( CPTIntegerDataType, sizeof(NSInteger), CFByteOrderGetCurrent() )
                                                                        shape:nil];

    [data release];

    mach_timebase_info_data_t time_base_info;
    mach_timebase_info(&time_base_info);

    NSUInteger iterations = 0;
    uint64_t elapsed      = 0;

    for ( NSUInteger i = 0; i < numberOfReps; i++ ) {
        uint64_t start = mach_absolute_time();

        CPTNumericData *doubleNumericData = [integerNumericData dataByConvertingToType:CPTFloatingPointDataType sampleBytes:sizeof(double) byteOrder:CFByteOrderGetCurrent()];

        uint64_t now = mach_absolute_time();

        elapsed += now - start;

        [[doubleNumericData retain] release];
        iterations++;
    }

    double avgTime = 1.0e-6 * (double)(elapsed * time_base_info.numer / time_base_info.denom) / iterations;
    STFail(@"Avg. time = %g ms for %lu points.", avgTime, (unsigned long)numberOfSamples);

    [integerNumericData release];
}

-(void)testDoubleToIntegerConversion
{
    NSMutableData *data = [[NSMutableData alloc] initWithLength:numberOfSamples * sizeof(double)];
    double *samples     = (double *)[data mutableBytes];

    for ( NSUInteger i = 0; i < numberOfSamples; i++ ) {
        samples[i] = sin(i) * 1000.0;
    }

    CPTNumericData *doubleNumericData = [[CPTNumericData alloc] initWithData:data
                                                                    dataType:CPTDataType( CPTFloatingPointDataType, sizeof(double), CFByteOrderGetCurrent() )
                                                                       shape:nil];

    [data release];

    mach_timebase_info_data_t time_base_info;
    mach_timebase_info(&time_base_info);

    NSUInteger iterations = 0;
    uint64_t elapsed      = 0;

    for ( NSUInteger i = 0; i < numberOfReps; i++ ) {
        uint64_t start = mach_absolute_time();

        CPTNumericData *integerNumericData = [doubleNumericData dataByConvertingToType:CPTIntegerDataType sampleBytes:sizeof(NSInteger) byteOrder:CFByteOrderGetCurrent()];

        uint64_t now = mach_absolute_time();

        elapsed += now - start;

        [[integerNumericData retain] release];
        iterations++;
    }

    double avgTime = 1.0e-6 * (double)(elapsed * time_base_info.numer / time_base_info.denom) / iterations;
    STFail(@"Avg. time = %g ms for %lu points.", avgTime, (unsigned long)numberOfSamples);

    [doubleNumericData release];
}

@end
