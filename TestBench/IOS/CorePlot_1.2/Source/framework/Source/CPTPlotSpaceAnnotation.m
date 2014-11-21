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
#import "CPTPlotSpaceAnnotation.h"

#import "CPTPlotArea.h"
#import "CPTPlotAreaFrame.h"
#import "CPTPlotSpace.h"

/** @brief Positions a content layer relative to some anchor point in a plot space.
 *
 *  Plot space annotations are positioned relative to a plot space. This allows the
 *  annotation content layer to move with the plot when the plot space changes.
 *  This is useful for applications such as labels attached to specific data points on a plot.
 **/
@implementation CPTPlotSpaceAnnotation

/** @property NSArray *anchorPlotPoint
 *  @brief An array of NSDecimalNumber objects giving the anchor plot coordinates.
 **/
@synthesize anchorPlotPoint;

/** @property CPTPlotSpace *plotSpace
 *  @brief The plot space which the anchor is defined in.
 **/
@synthesize plotSpace;

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTPlotSpaceAnnotation object.
 *
 *  This is the designated initializer. The initialized layer will be anchored to
 *  a point in plot coordinates.
 *
 *  @param newPlotSpace The plot space which the anchor is defined in. Must be non-@nil.
 *  @param newPlotPoint An array of NSDecimalNumber objects giving the anchor plot coordinates.
 *  @return The initialized CPTPlotSpaceAnnotation object.
 **/
-(id)initWithPlotSpace:(CPTPlotSpace *)newPlotSpace anchorPlotPoint:(NSArray *)newPlotPoint
{
    NSParameterAssert(newPlotSpace);

    if ( (self = [super init]) ) {
        plotSpace       = [newPlotSpace retain];
        anchorPlotPoint = [newPlotPoint copy];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(positionContentLayer)
                                                     name:CPTPlotSpaceCoordinateMappingDidChangeNotification
                                                   object:plotSpace];
    }
    return self;
}

/// @}

/// @cond

// plotSpace is required; this will fail the assertion in -initWithPlotSpace:anchorPlotPoint:
-(id)init
{
    return [self initWithPlotSpace:nil anchorPlotPoint:nil];
}

-(void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [plotSpace release];
    [anchorPlotPoint release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [super encodeWithCoder:coder];

    [coder encodeObject:self.anchorPlotPoint forKey:@"CPTPlotSpaceAnnotation.anchorPlotPoint"];
    [coder encodeConditionalObject:self.plotSpace forKey:@"CPTPlotSpaceAnnotation.plotSpace"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        anchorPlotPoint = [[coder decodeObjectForKey:@"CPTPlotSpaceAnnotation.anchorPlotPoint"] copy];
        plotSpace       = [[coder decodeObjectForKey:@"CPTPlotSpaceAnnotation.plotSpace"] retain];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Layout

/// @cond

-(void)positionContentLayer
{
    CPTLayer *content = self.contentLayer;

    if ( content ) {
        CPTLayer *hostLayer = self.annotationHostLayer;
        if ( hostLayer ) {
            NSArray *plotAnchor = self.anchorPlotPoint;
            if ( plotAnchor ) {
                NSUInteger anchorCount = plotAnchor.count;

                // Get plot area point
                NSDecimal *decimalPoint = malloc(sizeof(NSDecimal) * anchorCount);
                for ( NSUInteger i = 0; i < anchorCount; i++ ) {
                    decimalPoint[i] = [[plotAnchor objectAtIndex:i] decimalValue];
                }
                CPTPlotSpace *thePlotSpace      = self.plotSpace;
                CGPoint plotAreaViewAnchorPoint = [thePlotSpace plotAreaViewPointForPlotPoint:decimalPoint];
                free(decimalPoint);

                CGPoint newPosition;
                CPTPlotArea *plotArea = thePlotSpace.graph.plotAreaFrame.plotArea;
                if ( plotArea ) {
                    newPosition = [plotArea convertPoint:plotAreaViewAnchorPoint toLayer:hostLayer];
                }
                else {
                    newPosition = CGPointZero;
                }
                CGPoint offset = self.displacement;
                newPosition.x += offset.x;
                newPosition.y += offset.y;

                content.anchorPoint = self.contentAnchorPoint;
                content.position    = newPosition;
                content.transform   = CATransform3DMakeRotation( self.rotation, CPTFloat(0.0), CPTFloat(0.0), CPTFloat(1.0) );
                [content pixelAlign];
            }
        }
    }
}

/// @endcond

#pragma mark -
#pragma mark Accessors

/// @cond

-(void)setAnchorPlotPoint:(NSArray *)newPlotPoint
{
    if ( anchorPlotPoint != newPlotPoint ) {
        [anchorPlotPoint release];
        anchorPlotPoint = [newPlotPoint copy];
        [self positionContentLayer];
    }
}

/// @endcond

@end
