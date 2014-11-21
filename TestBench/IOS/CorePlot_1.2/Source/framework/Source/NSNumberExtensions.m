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
#import "NSNumberExtensions.h"

@implementation NSNumber(CPTExtensions)

/** @brief Creates and returns an NSNumber object containing a given value, treating it as a @ref CGFloat.
 *  @param number The value for the new number.
 *  @return An NSNumber object containing value, treating it as a @ref CGFloat.
 **/
+(NSNumber *)numberWithCGFloat:(CGFloat)number
{
#if CGFLOAT_IS_DOUBLE
    return [NSNumber numberWithDouble:number];

#else
    return [NSNumber numberWithFloat:number];
#endif
}

/** @brief Returns the value of the receiver as a @ref CGFloat.
 *  @return The value of the receiver as a @ref CGFloat.
 **/
-(CGFloat)cgFloatValue
{
#if CGFLOAT_IS_DOUBLE
    return [self doubleValue];

#else
    return [self floatValue];
#endif
}

/** @brief Returns an NSNumber object initialized to contain a given value, treated as a @ref CGFloat.
 *  @param number The value for the new number.
 *  @return An NSNumber object containing value, treating it as a @ref CGFloat.
 **/
-(id)initWithCGFloat:(CGFloat)number
{
#if CGFLOAT_IS_DOUBLE
    return [self initWithDouble:number];

#else
    return [self initWithFloat:number];
#endif
}

/** @brief Returns the value of the receiver as an NSDecimalNumber.
 *  @return The value of the receiver as an NSDecimalNumber.
 **/
-(NSDecimalNumber *)decimalNumber
{
    if ( [self isMemberOfClass:[NSDecimalNumber class]] ) {
        return (NSDecimalNumber *)self;
    }
    return [NSDecimalNumber decimalNumberWithDecimal:[self decimalValue]];
}

@end
