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
#import "_CPTConstraintsFixed.h"

#import "NSCoderExtensions.h"

/// @cond
@interface _CPTConstraintsFixed()

@property (nonatomic, readwrite) CGFloat offset;
@property (nonatomic, readwrite) BOOL isFixedToLower;

@end

/// @endcond

#pragma mark -

/** @brief Implements a one-dimensional constrained position within a given numeric range.
 *
 *  Supports fixed distance from either end of the range and a proportional fraction of the range.
 **/
@implementation _CPTConstraintsFixed

@synthesize offset;
@synthesize isFixedToLower;

#pragma mark -
#pragma mark Init/Dealloc

/** @brief Initializes a newly allocated CPTConstraints instance initialized with a fixed offset from the lower bound.
 *  @param newOffset The offset.
 *  @return The initialized CPTConstraints object.
 **/
-(id)initWithLowerOffset:(CGFloat)newOffset
{
    if ( (self = [super init]) ) {
        offset         = newOffset;
        isFixedToLower = YES;
    }

    return self;
}

/** @brief Initializes a newly allocated CPTConstraints instance initialized with a fixed offset from the upper bound.
 *  @param newOffset The offset.
 *  @return The initialized CPTConstraints object.
 **/
-(id)initWithUpperOffset:(CGFloat)newOffset
{
    if ( (self = [super init]) ) {
        offset         = newOffset;
        isFixedToLower = NO;
    }

    return self;
}

#pragma mark -
#pragma mark Comparison

-(BOOL)isEqualToConstraint:(CPTConstraints *)otherConstraint
{
    if ( [self class] != [otherConstraint class] ) {
        return NO;
    }
    return (self.offset == ( (_CPTConstraintsFixed *)otherConstraint ).offset) &&
           (self.isFixedToLower == ( (_CPTConstraintsFixed *)otherConstraint ).isFixedToLower);
}

#pragma mark -
#pragma mark Positioning

/** @brief Compute the position given a range of values.
 *  @param lowerBound The lower bound; must be less than or equal to the upperBound.
 *  @param upperBound The upper bound; must be greater than or equal to the lowerBound.
 *  @return The calculated position.
 **/
-(CGFloat)positionForLowerBound:(CGFloat)lowerBound upperBound:(CGFloat)upperBound
{
    NSAssert(lowerBound <= upperBound, @"lowerBound must be less than or equal to upperBound");

    CGFloat position;

    if ( self.isFixedToLower ) {
        position = lowerBound + self.offset;
    }
    else {
        position = upperBound - self.offset;
    }

    return position;
}

#pragma mark -
#pragma mark NSCopying Methods

/// @cond

-(id)copyWithZone:(NSZone *)zone
{
    _CPTConstraintsFixed *copy = [[[self class] allocWithZone:zone] init];

    copy->offset         = self->offset;
    copy->isFixedToLower = self->isFixedToLower;

    return copy;
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(Class)classForCoder
{
    return [CPTConstraints class];
}

-(void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeCGFloat:self.offset forKey:@"_CPTConstraintsFixed.offset"];
    [coder encodeBool:self.isFixedToLower forKey:@"_CPTConstraintsFixed.isFixedToLower"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super init]) ) {
        offset         = [coder decodeCGFloatForKey:@"_CPTConstraintsFixed.offset"];
        isFixedToLower = [coder decodeBoolForKey:@"_CPTConstraintsFixed.isFixedToLower"];
    }
    return self;
}

/// @endcond

@end
