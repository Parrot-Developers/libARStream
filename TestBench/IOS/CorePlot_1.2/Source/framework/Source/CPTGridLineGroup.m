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
#import "CPTGridLineGroup.h"

#import "CPTAxis.h"
#import "CPTAxisSet.h"
#import "CPTPlotArea.h"

/** @brief A group of grid line layers.
 *
 *  When using separate axis layers, this layer serves as the common superlayer for the grid line layers.
 *  Otherwise, this layer handles the drawing for the grid lines. It supports mixing the two modes;
 *  some axes can use separate grid line layers while others are handled by the grid line group.
 **/
@implementation CPTGridLineGroup

/** @property __cpt_weak CPTPlotArea *plotArea
 *  @brief The plot area that this grid line group belongs to.
 **/
@synthesize plotArea;

/** @property BOOL major
 *  @brief If @YES, draw the major grid lines, else draw the minor grid lines.
 **/
@synthesize major;

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTGridLineGroup object with the provided frame rectangle.
 *
 *  This is the designated initializer. The initialized layer will have the following properties:
 *  - @ref plotArea = @nil
 *  - @ref major = @NO
 *  - @ref needsDisplayOnBoundsChange = @YES
 *
 *  @param newFrame The frame rectangle.
 *  @return The initialized CPTGridLineGroup object.
 **/
-(id)initWithFrame:(CGRect)newFrame
{
    if ( (self = [super initWithFrame:newFrame]) ) {
        plotArea = nil;
        major    = NO;

        self.needsDisplayOnBoundsChange = YES;
    }
    return self;
}

/// @}

/// @cond

-(id)initWithLayer:(id)layer
{
    if ( (self = [super initWithLayer:layer]) ) {
        CPTGridLineGroup *theLayer = (CPTGridLineGroup *)layer;

        plotArea = theLayer->plotArea;
        major    = theLayer->major;
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [super encodeWithCoder:coder];

    [coder encodeConditionalObject:self.plotArea forKey:@"CPTGridLineGroup.plotArea"];
    [coder encodeBool:self.major forKey:@"CPTGridLineGroup.major"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        plotArea = [coder decodeObjectForKey:@"CPTGridLineGroup.plotArea"];
        major    = [coder decodeBoolForKey:@"CPTGridLineGroup.major"];
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

    for ( CPTAxis *axis in self.plotArea.axisSet.axes ) {
        if ( !axis.separateLayers ) {
            [axis drawGridLinesInContext:context isMajor:self.major];
        }
    }
}

/// @endcond

#pragma mark -
#pragma mark Accessors

/// @cond

-(void)setPlotArea:(CPTPlotArea *)newPlotArea
{
    if ( newPlotArea != plotArea ) {
        plotArea = newPlotArea;

        if ( plotArea ) {
            [self setNeedsDisplay];
        }
    }
}

/// @endcond

@end
