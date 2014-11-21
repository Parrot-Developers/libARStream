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
#import "CPTFill.h"

#import "CPTColor.h"
#import "CPTImage.h"
#import "_CPTFillColor.h"
#import "_CPTFillGradient.h"
#import "_CPTFillImage.h"

/** @brief Draws area fills.
 *
 *  CPTFill instances can be used to fill drawing areas with colors (including patterns),
 *  gradients, and images. Drawing methods are provided to fill rectangular areas and
 *  arbitrary drawing paths.
 **/

@implementation CPTFill

#pragma mark -
#pragma mark Init/Dealloc

/** @brief Creates and returns a new CPTFill instance initialized with a given color.
 *  @param aColor The color.
 *  @return A new CPTFill instance initialized with the given color.
 **/
+(CPTFill *)fillWithColor:(CPTColor *)aColor
{
    return [[(_CPTFillColor *)[_CPTFillColor alloc] initWithColor:aColor] autorelease];
}

/** @brief Creates and returns a new CPTFill instance initialized with a given gradient.
 *  @param aGradient The gradient.
 *  @return A new CPTFill instance initialized with the given gradient.
 **/
+(CPTFill *)fillWithGradient:(CPTGradient *)aGradient
{
    return [[[_CPTFillGradient alloc] initWithGradient:aGradient] autorelease];
}

/** @brief Creates and returns a new CPTFill instance initialized with a given image.
 *  @param anImage The image.
 *  @return A new CPTFill instance initialized with the given image.
 **/
+(CPTFill *)fillWithImage:(CPTImage *)anImage
{
    return [[(_CPTFillImage *)[_CPTFillImage alloc] initWithImage:anImage] autorelease];
}

/** @brief Initializes a newly allocated CPTFill object with the provided color.
 *  @param aColor The color.
 *  @return The initialized CPTFill object.
 **/
-(id)initWithColor:(CPTColor *)aColor
{
    [self release];

    self = [(_CPTFillColor *)[_CPTFillColor alloc] initWithColor:aColor];

    return self;
}

/** @brief Initializes a newly allocated CPTFill object with the provided gradient.
 *  @param aGradient The gradient.
 *  @return The initialized CPTFill object.
 **/
-(id)initWithGradient:(CPTGradient *)aGradient
{
    [self release];

    self = [[_CPTFillGradient alloc] initWithGradient:aGradient];

    return self;
}

/** @brief Initializes a newly allocated CPTFill object with the provided image.
 *  @param anImage The image.
 *  @return The initialized CPTFill object.
 **/
-(id)initWithImage:(CPTImage *)anImage
{
    [self release];

    self = [(_CPTFillImage *)[_CPTFillImage alloc] initWithImage:anImage];

    return self;
}

#pragma mark -
#pragma mark NSCopying Methods

/// @cond

-(id)copyWithZone:(NSZone *)zone
{
    // do nothing--implemented in subclasses
    return nil;
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    // do nothing--implemented in subclasses
}

-(id)initWithCoder:(NSCoder *)coder
{
    id fill = [coder decodeObjectForKey:@"_CPTFillColor.fillColor"];

    if ( fill ) {
        return [self initWithColor:fill];
    }

    fill = [coder decodeObjectForKey:@"_CPTFillGradient.fillGradient"];
    if ( fill ) {
        return [self initWithGradient:fill];
    }

    fill = [coder decodeObjectForKey:@"_CPTFillImage.fillImage"];
    if ( fill ) {
        return [self initWithImage:fill];
    }

    return self;
}

/// @endcond

@end

#pragma mark -

@implementation CPTFill(AbstractMethods)

#pragma mark -
#pragma mark Drawing

/** @brief Draws the gradient into the given graphics context inside the provided rectangle.
 *  @param rect The rectangle to draw into.
 *  @param context The graphics context to draw into.
 **/
-(void)fillRect:(CGRect)rect inContext:(CGContextRef)context
{
    // do nothing--subclasses override to do drawing here
}

/** @brief Draws the gradient into the given graphics context clipped to the current drawing path.
 *  @param context The graphics context to draw into.
 **/
-(void)fillPathInContext:(CGContextRef)context
{
    // do nothing--subclasses override to do drawing here
}

@end
