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

#if __cplusplus
extern "C" {
#endif

/// @name Convert NSDecimal to Primitive Types
/// @{
int8_t CPTDecimalCharValue(NSDecimal decimalNumber);
int16_t CPTDecimalShortValue(NSDecimal decimalNumber);
int32_t CPTDecimalLongValue(NSDecimal decimalNumber);
int64_t CPTDecimalLongLongValue(NSDecimal decimalNumber);
int CPTDecimalIntValue(NSDecimal decimalNumber);
NSInteger CPTDecimalIntegerValue(NSDecimal decimalNumber);

uint8_t CPTDecimalUnsignedCharValue(NSDecimal decimalNumber);
uint16_t CPTDecimalUnsignedShortValue(NSDecimal decimalNumber);
uint32_t CPTDecimalUnsignedLongValue(NSDecimal decimalNumber);
uint64_t CPTDecimalUnsignedLongLongValue(NSDecimal decimalNumber);
unsigned int CPTDecimalUnsignedIntValue(NSDecimal decimalNumber);
NSUInteger CPTDecimalUnsignedIntegerValue(NSDecimal decimalNumber);

float CPTDecimalFloatValue(NSDecimal decimalNumber);
double CPTDecimalDoubleValue(NSDecimal decimalNumber);
CGFloat CPTDecimalCGFloatValue(NSDecimal decimalNumber);

NSString *CPTDecimalStringValue(NSDecimal decimalNumber);

/// @}

/// @name Convert Primitive Types to NSDecimal
/// @{
NSDecimal CPTDecimalFromChar(int8_t anInt);
NSDecimal CPTDecimalFromShort(int16_t anInt);
NSDecimal CPTDecimalFromLong(int32_t anInt);
NSDecimal CPTDecimalFromLongLong(int64_t anInt);
NSDecimal CPTDecimalFromInt(int i);
NSDecimal CPTDecimalFromInteger(NSInteger i);

NSDecimal CPTDecimalFromUnsignedChar(uint8_t i);
NSDecimal CPTDecimalFromUnsignedShort(uint16_t i);
NSDecimal CPTDecimalFromUnsignedLong(uint32_t i);
NSDecimal CPTDecimalFromUnsignedLongLong(uint64_t i);
NSDecimal CPTDecimalFromUnsignedInt(unsigned int i);
NSDecimal CPTDecimalFromUnsignedInteger(NSUInteger i);

NSDecimal CPTDecimalFromFloat(float aFloat);
NSDecimal CPTDecimalFromDouble(double aDouble);
NSDecimal CPTDecimalFromCGFloat(CGFloat aCGFloat);

NSDecimal CPTDecimalFromString(NSString *stringRepresentation);

/// @}

/// @name NSDecimal Arithmetic
/// @{
NSDecimal CPTDecimalAdd(NSDecimal leftOperand, NSDecimal rightOperand);
NSDecimal CPTDecimalSubtract(NSDecimal leftOperand, NSDecimal rightOperand);
NSDecimal CPTDecimalMultiply(NSDecimal leftOperand, NSDecimal rightOperand);
NSDecimal CPTDecimalDivide(NSDecimal numerator, NSDecimal denominator);

/// @}

/// @name NSDecimal Comparison
/// @{
BOOL CPTDecimalGreaterThan(NSDecimal leftOperand, NSDecimal rightOperand);
BOOL CPTDecimalGreaterThanOrEqualTo(NSDecimal leftOperand, NSDecimal rightOperand);
BOOL CPTDecimalLessThan(NSDecimal leftOperand, NSDecimal rightOperand);
BOOL CPTDecimalLessThanOrEqualTo(NSDecimal leftOperand, NSDecimal rightOperand);
BOOL CPTDecimalEquals(NSDecimal leftOperand, NSDecimal rightOperand);

/// @}

/// @name NSDecimal Utilities
/// @{
NSDecimal CPTDecimalNaN(void);

/// @}

/// @name Ranges
/// @{
NSRange CPTExpandedRange(NSRange range, NSInteger expandBy);

/// @}

/// @name Coordinates
/// @{
CPTCoordinate CPTOrthogonalCoordinate(CPTCoordinate coord);

/// @}

/// @name Gradient Colors
/// @{
CPTRGBAColor CPTRGBAColorFromCGColor(CGColorRef color);

/// @}

/// @name Quartz Pixel-Alignment Functions
/// @{
CGPoint CPTAlignPointToUserSpace(CGContextRef context, CGPoint point);
CGSize CPTAlignSizeToUserSpace(CGContextRef context, CGSize size);
CGRect CPTAlignRectToUserSpace(CGContextRef context, CGRect rect);

CGPoint CPTAlignIntegralPointToUserSpace(CGContextRef context, CGPoint point);
CGRect CPTAlignIntegralRectToUserSpace(CGContextRef context, CGRect rect);

/// @}

/// @name String Formatting for Core Graphics Structs
/// @{
NSString *CPTStringFromPoint(CGPoint point);
NSString *CPTStringFromSize(CGSize size);
NSString *CPTStringFromRect(CGRect rect);

/// @}

/// @name CGPoint Utilities
/// @{
CGFloat squareOfDistanceBetweenPoints(CGPoint point1, CGPoint point2);

/// @}

#if __cplusplus
}
#endif
