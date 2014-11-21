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
#import "CPTMutablePlotRange.h"

#import "CPTUtilities.h"

/** @brief Defines a mutable range of plot data.
 *
 *  If you need to change the plot range, you should use this class rather than the
 *  immutable super class.
 *
 **/
@implementation CPTMutablePlotRange

/** @property location
 *  @brief The starting value of the range.
 **/
@dynamic location;

/** @property length
 *  @brief The length of the range.
 **/
@dynamic length;

#pragma mark -
#pragma mark Combining ranges

/** @brief Extends the range to include another range. The sign of @ref length is unchanged.
 *  @param other The other plot range.
 **/
-(void)unionPlotRange:(CPTPlotRange *)other
{
    if ( !other ) {
        return;
    }

    NSDecimal min1    = self.minLimit;
    NSDecimal min2    = other.minLimit;
    NSDecimal minimum = CPTDecimalLessThan(min1, min2) ? min1 : min2;

    NSDecimal max1    = self.maxLimit;
    NSDecimal max2    = other.maxLimit;
    NSDecimal maximum = CPTDecimalGreaterThan(max1, max2) ? max1 : max2;

    NSDecimal newLocation, newLength;
    if ( CPTDecimalGreaterThanOrEqualTo( self.length, CPTDecimalFromInteger(0) ) ) {
        newLocation = minimum;
        newLength   = CPTDecimalSubtract(maximum, minimum);
    }
    else {
        newLocation = maximum;
        newLength   = CPTDecimalSubtract(minimum, maximum);
    }

    self.location = newLocation;
    self.length   = newLength;
}

/** @brief Sets the messaged object to the intersection with another range. The sign of @ref length is unchanged.
 *  @param other The other plot range.
 **/
-(void)intersectionPlotRange:(CPTPlotRange *)other
{
    if ( !other ) {
        return;
    }

    NSDecimal min1    = self.minLimit;
    NSDecimal min2    = other.minLimit;
    NSDecimal minimum = CPTDecimalGreaterThan(min1, min2) ? min1 : min2;

    NSDecimal max1    = self.maxLimit;
    NSDecimal max2    = other.maxLimit;
    NSDecimal maximum = CPTDecimalLessThan(max1, max2) ? max1 : max2;

    if ( CPTDecimalGreaterThanOrEqualTo(maximum, minimum) ) {
        NSDecimal newLocation, newLength;
        if ( CPTDecimalGreaterThanOrEqualTo( self.length, CPTDecimalFromInteger(0) ) ) {
            newLocation = minimum;
            newLength   = CPTDecimalSubtract(maximum, minimum);
        }
        else {
            newLocation = maximum;
            newLength   = CPTDecimalSubtract(minimum, maximum);
        }

        self.location = newLocation;
        self.length   = newLength;
    }
    else {
        self.length = CPTDecimalFromInteger(0);
    }
}

#pragma mark -
#pragma mark Expanding/Contracting ranges

/** @brief Extends/contracts the range by a given factor.
 *  @param factor Factor used. A value of @num{1.0} gives no change.
 *  Less than @num{1.0} is a contraction, and greater than @num{1.0} is expansion.
 **/
-(void)expandRangeByFactor:(NSDecimal)factor
{
    NSDecimal oldLength      = self.length;
    NSDecimal newLength      = CPTDecimalMultiply(oldLength, factor);
    NSDecimal locationOffset = CPTDecimalDivide( CPTDecimalSubtract(oldLength, newLength), CPTDecimalFromInteger(2) );
    NSDecimal newLocation    = CPTDecimalAdd(self.location, locationOffset);

    self.location = newLocation;
    self.length   = newLength;
}

#pragma mark -
#pragma mark Shifting Range

/** @brief Moves the whole range so that the @ref location fits in other range.
 *  @param otherRange Other range.
 *  The minimum possible shift is made. The range @ref length is unchanged.
 **/
-(void)shiftLocationToFitInRange:(CPTPlotRange *)otherRange
{
    NSParameterAssert(otherRange);

    switch ( [otherRange compareToNumber:[NSDecimalNumber decimalNumberWithDecimal:self.location]] ) {
        case CPTPlotRangeComparisonResultNumberBelowRange:
            self.location = otherRange.minLimit;
            break;

        case CPTPlotRangeComparisonResultNumberAboveRange:
            self.location = otherRange.maxLimit;
            break;

        default:
            // in range--do nothing
            break;
    }
}

/** @brief Moves the whole range so that the @ref end point fits in other range.
 *  @param otherRange Other range.
 *  The minimum possible shift is made. The range @ref length is unchanged.
 **/
-(void)shiftEndToFitInRange:(CPTPlotRange *)otherRange
{
    NSParameterAssert(otherRange);

    switch ( [otherRange compareToNumber:[NSDecimalNumber decimalNumberWithDecimal:self.end]] ) {
        case CPTPlotRangeComparisonResultNumberBelowRange:
            self.location = CPTDecimalSubtract(otherRange.minLimit, self.length);
            break;

        case CPTPlotRangeComparisonResultNumberAboveRange:
            self.location = CPTDecimalSubtract(otherRange.maxLimit, self.length);
            break;

        default:
            // in range--do nothing
            break;
    }
}

@end
