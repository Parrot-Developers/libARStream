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
#import "CPTColorSpace.h"

#import "NSCoderExtensions.h"

/// @cond
@interface CPTColorSpace()

@property (nonatomic, readwrite, assign) CGColorSpaceRef cgColorSpace;

@end

/// @endcond

#pragma mark -

/** @brief An immutable color space.
 *
 *  An immutable object wrapper class around @ref CGColorSpaceRef.
 **/

@implementation CPTColorSpace

/** @property CGColorSpaceRef cgColorSpace
 *  @brief The @ref CGColorSpaceRef to wrap around.
 **/
@synthesize cgColorSpace;

#pragma mark -
#pragma mark Class methods

/** @brief Returns a shared instance of CPTColorSpace initialized with the standard RGB space.
 *
 *  The standard RGB space is created by the
 *  @if MacOnly @ref CGColorSpaceCreateWithName ( @ref kCGColorSpaceGenericRGB ) function. @endif
 *  @if iOSOnly @ref CGColorSpaceCreateDeviceRGB() function. @endif
 *
 *  @return A shared CPTColorSpace object initialized with the standard RGB colorspace.
 **/
+(CPTColorSpace *)genericRGBSpace
{
    static CPTColorSpace *space = nil;

    if ( nil == space ) {
        CGColorSpaceRef cgSpace = NULL;
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        cgSpace = CGColorSpaceCreateDeviceRGB();
#else
        cgSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
#endif
        space = [[CPTColorSpace alloc] initWithCGColorSpace:cgSpace];
        CGColorSpaceRelease(cgSpace);
    }
    return space;
}

#pragma mark -
#pragma mark Init/Dealloc

/** @brief Initializes a newly allocated colorspace object with the specified color space.
 *  This is the designated initializer.
 *
 *  @param colorSpace The color space.
 *  @return The initialized CPTColorSpace object.
 **/
-(id)initWithCGColorSpace:(CGColorSpaceRef)colorSpace
{
    if ( (self = [super init]) ) {
        CGColorSpaceRetain(colorSpace);
        cgColorSpace = colorSpace;
    }
    return self;
}

/// @cond

-(void)dealloc
{
    CGColorSpaceRelease(cgColorSpace);
    [super dealloc];
}

-(void)finalize
{
    CGColorSpaceRelease(cgColorSpace);
    [super finalize];
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeCGColorSpace:self.cgColorSpace forKey:@"CPTColorSpace.cgColorSpace"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super init]) ) {
        cgColorSpace = [coder newCGColorSpaceDecodeForKey:@"CPTColorSpace.cgColorSpace"];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Accessors

/// @cond

-(void)setCGColorSpace:(CGColorSpaceRef)newSpace
{
    if ( newSpace != cgColorSpace ) {
        CGColorSpaceRelease(cgColorSpace);
        CGColorSpaceRetain(newSpace);
        cgColorSpace = newSpace;
    }
}

/// @endcond

@end
