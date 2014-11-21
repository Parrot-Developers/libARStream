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
#import "_CPTMaskLayer.h"

/**
 *  @brief A utility layer used to mask the borders on other layers.
 **/
@implementation CPTMaskLayer

#pragma mark -
#pragma mark Init/Dealloc

/** @brief Initializes a newly allocated CPTMaskLayer object with the provided frame rectangle.
 *
 *	This is the designated initializer. The initialized layer will have the following properties:
 *	- @ref needsDisplayOnBoundsChange = @YES
 *
 *	@param newFrame The frame rectangle.
 *  @return The initialized CPTMaskLayer object.
 **/
-(id)initWithFrame:(CGRect)newFrame
{
    if ( (self = [super initWithFrame:newFrame]) ) {
        self.needsDisplayOnBoundsChange = YES;
    }
    return self;
}

#pragma mark -
#pragma mark Drawing

/// @cond

-(void)renderAsVectorInContext:(CGContextRef)context
{
    [super renderAsVectorInContext:context];

    CPTLayer *theMaskedLayer = (CPTLayer *)self.superlayer;

    if ( theMaskedLayer ) {
        CGContextSetRGBFillColor(context, 0.0, 0.0, 0.0, 1.0);

        if ( [theMaskedLayer isKindOfClass:[CPTLayer class]] ) {
            CGPathRef maskingPath = theMaskedLayer.sublayerMaskingPath;

            if ( maskingPath ) {
                CGContextAddPath(context, maskingPath);
                CGContextFillPath(context);
            }
        }
        else {
            CGContextFillRect(context, self.bounds);
        }
    }
}

/// @endcond

@end
