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
#import "CPTAxisSet.h"

#import "CPTAxis.h"
#import "CPTGraph.h"
#import "CPTLineStyle.h"
#import "CPTPlotArea.h"

/**
 *  @brief A container layer for the set of axes for a graph.
 **/
@implementation CPTAxisSet

/** @property NSArray *axes
 *  @brief The axes in the axis set.
 **/
@synthesize axes;

/** @property CPTLineStyle *borderLineStyle
 *  @brief The line style for the layer border.
 *  If @nil, the border is not drawn.
 **/
@synthesize borderLineStyle;

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTAxisSet object with the provided frame rectangle.
 *
 *  This is the designated initializer. The initialized layer will have the following properties:
 *  - @ref axes = empty array
 *  - @ref borderLineStyle = @nil
 *  - @ref needsDisplayOnBoundsChange = @YES
 *
 *  @param newFrame The frame rectangle.
 *  @return The initialized CPTAxisSet object.
 **/
-(id)initWithFrame:(CGRect)newFrame
{
    if ( (self = [super initWithFrame:newFrame]) ) {
        axes            = [[NSArray array] retain];
        borderLineStyle = nil;

        self.needsDisplayOnBoundsChange = YES;
    }
    return self;
}

/// @}

/// @cond

-(id)initWithLayer:(id)layer
{
    if ( (self = [super initWithLayer:layer]) ) {
        CPTAxisSet *theLayer = (CPTAxisSet *)layer;

        axes            = [theLayer->axes retain];
        borderLineStyle = [theLayer->borderLineStyle retain];
    }
    return self;
}

-(void)dealloc
{
    [axes release];
    [borderLineStyle release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [super encodeWithCoder:coder];

    [coder encodeObject:self.axes forKey:@"CPTAxisSet.axes"];
    [coder encodeObject:self.borderLineStyle forKey:@"CPTAxisSet.borderLineStyle"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        axes            = [[coder decodeObjectForKey:@"CPTAxisSet.axes"] copy];
        borderLineStyle = [[coder decodeObjectForKey:@"CPTAxisSet.borderLineStyle"] copy];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Labeling

/**
 *  @brief Updates the axis labels for each axis in the axis set.
 **/
-(void)relabelAxes
{
    NSArray *theAxes = self.axes;

    [theAxes makeObjectsPerformSelector:@selector(setNeedsLayout)];
    [theAxes makeObjectsPerformSelector:@selector(setNeedsRelabel)];
}

#pragma mark -
#pragma mark Axes

/**
 *  @brief Returns the first, second, third, etc. axis with the given coordinate value.
 *
 *  For example, to find the second x-axis, use a @par{coordinate} of #CPTCoordinateX
 *  and @par{idx} of @num{1}.
 *
 *  @param coordinate The axis coordinate.
 *  @param idx The zero-based index.
 *  @return The axis matching the given coordinate and index, or @nil if no match is found.
 **/
-(CPTAxis *)axisForCoordinate:(CPTCoordinate)coordinate atIndex:(NSUInteger)idx
{
    CPTAxis *foundAxis = nil;
    NSUInteger count   = 0;

    for ( CPTAxis *axis in self.axes ) {
        if ( axis.coordinate == coordinate ) {
            if ( count == idx ) {
                foundAxis = axis;
                break;
            }
            else {
                count++;
            }
        }
    }

    return foundAxis;
}

#pragma mark -
#pragma mark Accessors

/// @cond

-(void)setAxes:(NSArray *)newAxes
{
    if ( newAxes != axes ) {
        for ( CPTAxis *axis in axes ) {
            [axis removeFromSuperlayer];
            axis.plotArea = nil;
        }
        [newAxes retain];
        [axes release];
        axes = newAxes;
        CPTPlotArea *plotArea = (CPTPlotArea *)self.superlayer;
        for ( CPTAxis *axis in axes ) {
            [self addSublayer:axis];
            axis.plotArea = plotArea;
        }
        [self setNeedsLayout];
        [self setNeedsDisplay];
    }
}

-(void)setBorderLineStyle:(CPTLineStyle *)newLineStyle
{
    if ( newLineStyle != borderLineStyle ) {
        [borderLineStyle release];
        borderLineStyle = [newLineStyle copy];
        [self setNeedsDisplay];
    }
}

/// @endcond

@end
