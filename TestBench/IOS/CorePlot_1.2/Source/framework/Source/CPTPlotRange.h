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
#import "CPTDefinitions.h"

/// @file

/**
 *  @brief Enumeration of possible results of a plot range comparison.
 **/
typedef enum _CPTPlotRangeComparisonResult {
    CPTPlotRangeComparisonResultNumberBelowRange, ///< Number is below the range.
    CPTPlotRangeComparisonResultNumberInRange,    ///< Number is in the range.
    CPTPlotRangeComparisonResultNumberAboveRange  ///< Number is above the range.
}
CPTPlotRangeComparisonResult;

@interface CPTPlotRange : NSObject<NSCoding, NSCopying, NSMutableCopying> {
    @private
    NSDecimal location;
    NSDecimal length;
    double locationDouble;
    double lengthDouble;
}

/// @name Range Limits
/// @{
@property (nonatomic, readonly) NSDecimal location;
@property (nonatomic, readonly) NSDecimal length;
@property (nonatomic, readonly) NSDecimal end;
@property (nonatomic, readonly) double locationDouble;
@property (nonatomic, readonly) double lengthDouble;
@property (nonatomic, readonly) double endDouble;

@property (nonatomic, readonly) NSDecimal minLimit;
@property (nonatomic, readonly) NSDecimal midPoint;
@property (nonatomic, readonly) NSDecimal maxLimit;
@property (nonatomic, readonly) double minLimitDouble;
@property (nonatomic, readonly) double midPointDouble;
@property (nonatomic, readonly) double maxLimitDouble;
/// @}

/// @name Factory Methods
/// @{
+(id)plotRangeWithLocation:(NSDecimal)loc length:(NSDecimal)len;
/// @}

/// @name Initialization
/// @{
-(id)initWithLocation:(NSDecimal)loc length:(NSDecimal)len;
/// @}

/// @name Checking Ranges
/// @{
-(BOOL)contains:(NSDecimal)number;
-(BOOL)containsDouble:(double)number;
-(BOOL)isEqualToRange:(CPTPlotRange *)otherRange;
-(BOOL)containsRange:(CPTPlotRange *)otherRange;
-(BOOL)intersectsRange:(CPTPlotRange *)otherRange;
/// @}

/// @name Range Comparison
/// @{
-(CPTPlotRangeComparisonResult)compareToNumber:(NSNumber *)number;
-(CPTPlotRangeComparisonResult)compareToDecimal:(NSDecimal)number;
-(CPTPlotRangeComparisonResult)compareToDouble:(double)number;
/// @}

@end
