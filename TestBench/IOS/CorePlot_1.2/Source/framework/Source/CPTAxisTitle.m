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
#import "CPTAxisTitle.h"

#import "CPTLayer.h"
#import "CPTUtilities.h"
#import <tgmath.h>

/** @brief An axis title.
 *
 *  The title can be text-based or can be the content of any CPTLayer provided by the user.
 **/
@implementation CPTAxisTitle

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

-(id)initWithContentLayer:(CPTLayer *)layer
{
    if ( layer ) {
        if ( (self = [super initWithContentLayer:layer]) ) {
            self.rotation = NAN;
        }
    }
    else {
        [self release];
        self = nil;
    }
    return self;
}

/// @}

#pragma mark -
#pragma mark Label comparison

/// @name Comparison
/// @{

/** @brief Returns a boolean value that indicates whether the received is equal to the given object.
 *  Axis titles are equal if they have the same @ref tickLocation, @ref rotation, and @ref contentLayer.
 *  @param object The object to be compared with the receiver.
 *  @return @YES if @par{object} is equal to the receiver, @NO otherwise.
 **/
-(BOOL)isEqual:(id)object
{
    if ( self == object ) {
        return YES;
    }
    else if ( [object isKindOfClass:[self class]] ) {
        CPTAxisTitle *otherTitle = object;

        if ( (self.rotation != otherTitle.rotation) || (self.offset != otherTitle.offset) ) {
            return NO;
        }
        if ( ![self.contentLayer isEqual:otherTitle] ) {
            return NO;
        }
        return CPTDecimalEquals(self.tickLocation, ( (CPTAxisLabel *)object ).tickLocation);
    }
    else {
        return NO;
    }
}

/// @}

/// @cond

-(NSUInteger)hash
{
    NSUInteger hashValue = 0;

    // Equal objects must hash the same.
    double tickLocationAsDouble = CPTDecimalDoubleValue(self.tickLocation);

    if ( !isnan(tickLocationAsDouble) ) {
        hashValue = (NSUInteger)fmod(ABS(tickLocationAsDouble), (double)NSUIntegerMax);
    }
    hashValue += (NSUInteger)fmod(ABS(self.rotation), (double)NSUIntegerMax);
    hashValue += (NSUInteger)fmod(ABS(self.offset), (double)NSUIntegerMax);

    return hashValue;
}

/// @endcond

@end
