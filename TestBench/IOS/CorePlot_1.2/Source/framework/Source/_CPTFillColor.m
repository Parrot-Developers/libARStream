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
#import "_CPTFillColor.h"

#import "CPTColor.h"

/// @cond
@interface _CPTFillColor()

@property (nonatomic, readwrite, copy) CPTColor *fillColor;

@end

/// @endcond

/** @brief Draws CPTColor area fills.
 *
 *  Drawing methods are provided to fill rectangular areas and arbitrary drawing paths.
 **/

@implementation _CPTFillColor

/** @property fillColor
 *  @brief The fill color.
 **/
@synthesize fillColor;

#pragma mark -
#pragma mark Init/Dealloc

/** @brief Initializes a newly allocated _CPTFillColor object with the provided color.
 *  @param aColor The color.
 *  @return The initialized _CPTFillColor object.
 **/
-(id)initWithColor:(CPTColor *)aColor
{
    if ( (self = [super init]) ) {
        fillColor = [aColor retain];
    }
    return self;
}

/// @cond

-(void)dealloc
{
    [fillColor release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark Drawing

/** @brief Draws the color into the given graphics context inside the provided rectangle.
 *  @param rect The rectangle to draw into.
 *  @param context The graphics context to draw into.
 **/
-(void)fillRect:(CGRect)rect inContext:(CGContextRef)context
{
    CGContextSaveGState(context);
    CGContextSetFillColorWithColor(context, self.fillColor.cgColor);
    CGContextFillRect(context, rect);
    CGContextRestoreGState(context);
}

/** @brief Draws the color into the given graphics context clipped to the current drawing path.
 *  @param context The graphics context to draw into.
 **/
-(void)fillPathInContext:(CGContextRef)context
{
    CGContextSaveGState(context);
    CGContextSetFillColorWithColor(context, self.fillColor.cgColor);
    CGContextFillPath(context);
    CGContextRestoreGState(context);
}

#pragma mark -
#pragma mark NSCopying Methods

/// @cond

-(id)copyWithZone:(NSZone *)zone
{
    _CPTFillColor *copy = [[[self class] allocWithZone:zone] init];

    copy->fillColor = [self->fillColor copyWithZone:zone];

    return copy;
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(Class)classForCoder
{
    return [CPTFill class];
}

-(void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeObject:self.fillColor forKey:@"_CPTFillColor.fillColor"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super init]) ) {
        fillColor = [[coder decodeObjectForKey:@"_CPTFillColor.fillColor"] retain];
    }
    return self;
}

/// @endcond

@end
