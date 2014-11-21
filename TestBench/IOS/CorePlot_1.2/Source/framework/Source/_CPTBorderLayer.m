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
#import "_CPTBorderLayer.h"

#import "CPTBorderedLayer.h"

/**
 *  @brief A utility layer used to draw the fill and border of a CPTBorderedLayer.
 *
 *  This layer is always the superlayer of a single CPTBorderedLayer. It draws the fill and
 *  border so that they are not clipped by the mask applied to the sublayer.
 **/
@implementation CPTBorderLayer

/** @property CPTBorderedLayer *maskedLayer
 *  @brief The CPTBorderedLayer masked being masked.
 *  Its fill and border are drawn into this layer so that they are outside the mask applied to the @par{maskedLayer}.
 **/
@synthesize maskedLayer;

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTBorderLayer object with the provided frame rectangle.
 *
 *  This is the designated initializer. The initialized layer will have the following properties:
 *  - @ref maskedLayer = @nil
 *  - @ref needsDisplayOnBoundsChange = @YES
 *
 *  @param newFrame The frame rectangle.
 *  @return The initialized CPTBorderLayer object.
 **/
-(id)initWithFrame:(CGRect)newFrame
{
    if ( (self = [super initWithFrame:newFrame]) ) {
        maskedLayer = nil;

        self.needsDisplayOnBoundsChange = YES;
    }
    return self;
}

/// @}

/// @cond

-(id)initWithLayer:(id)layer
{
    if ( (self = [super initWithLayer:layer]) ) {
        CPTBorderLayer *theLayer = (CPTBorderLayer *)layer;

        maskedLayer = [theLayer->maskedLayer retain];
    }
    return self;
}

-(void)dealloc
{
    [maskedLayer release];

    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [super encodeWithCoder:coder];

    [coder encodeObject:self.maskedLayer forKey:@"CPTBorderLayer.maskedLayer"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        maskedLayer = [[coder decodeObjectForKey:@"CPTBorderLayer.maskedLayer"] retain];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Drawing

/// @cond

-(void)renderAsVectorInContext:(CGContextRef)context
{
    if ( self.hidden ) {
        return;
    }

    CPTBorderedLayer *theMaskedLayer = self.maskedLayer;

    if ( theMaskedLayer ) {
        [super renderAsVectorInContext:context];
        [theMaskedLayer renderBorderedLayerAsVectorInContext:context];
    }
}

/// @endcond

#pragma mark -
#pragma mark Layout

/// @cond

-(void)layoutSublayers
{
    [super layoutSublayers];

    CPTBorderedLayer *theMaskedLayer = self.maskedLayer;

    if ( theMaskedLayer ) {
        CGRect newBounds = self.bounds;

        // undo the shadow margin so the masked layer is always the same size
        if ( self.shadow ) {
            CGSize sizeOffset = self.shadowMargin;

            newBounds.origin.x    -= sizeOffset.width;
            newBounds.origin.y    -= sizeOffset.height;
            newBounds.size.width  += sizeOffset.width * CPTFloat(2.0);
            newBounds.size.height += sizeOffset.height * CPTFloat(2.0);
        }

        theMaskedLayer.inLayout = YES;
        theMaskedLayer.frame    = newBounds;
        theMaskedLayer.inLayout = NO;
    }
}

-(NSSet *)sublayersExcludedFromAutomaticLayout
{
    CPTBorderedLayer *excludedLayer = self.maskedLayer;

    if ( excludedLayer ) {
        NSMutableSet *excludedSublayers = [[[super sublayersExcludedFromAutomaticLayout] mutableCopy] autorelease];
        if ( !excludedSublayers ) {
            excludedSublayers = [NSMutableSet set];
        }
        [excludedSublayers addObject:excludedLayer];
        return excludedSublayers;
    }
    else {
        return [super sublayersExcludedFromAutomaticLayout];
    }
}

/// @endcond

#pragma mark -
#pragma mark Accessors

/// @cond

-(void)setMaskedLayer:(CPTBorderedLayer *)newLayer
{
    if ( newLayer != maskedLayer ) {
        [maskedLayer release];
        maskedLayer = [newLayer retain];
        [self setNeedsDisplay];
    }
}

-(void)setBounds:(CGRect)newBounds
{
    if ( !CGRectEqualToRect(newBounds, self.bounds) ) {
        [super setBounds:newBounds];
        [self setNeedsLayout];
    }
}

/// @endcond

@end
