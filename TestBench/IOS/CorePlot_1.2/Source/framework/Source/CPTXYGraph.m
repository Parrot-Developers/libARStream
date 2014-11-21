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
#import "CPTXYGraph.h"

#import "CPTXYAxis.h"
#import "CPTXYAxisSet.h"
#import "CPTXYPlotSpace.h"

/// @cond
@interface CPTXYGraph()

@property (nonatomic, readwrite, assign) CPTScaleType xScaleType;
@property (nonatomic, readwrite, assign) CPTScaleType yScaleType;

@end

/// @endcond

#pragma mark -

/**
 *  @brief A graph using a cartesian (X-Y) plot space.
 **/
@implementation CPTXYGraph

/** @property CPTScaleType xScaleType
 *  @brief The scale type for the x-axis.
 **/
@synthesize xScaleType;

/** @property CPTScaleType yScaleType
 *  @brief The scale type for the y-axis.
 **/
@synthesize yScaleType;

#pragma mark -
#pragma mark Init/Dealloc

/** @brief Initializes a newly allocated CPTXYGraph object with the provided frame rectangle and scale types.
 *
 *  This is the designated initializer.
 *
 *  @param newFrame The frame rectangle.
 *  @param newXScaleType The scale type for the x-axis.
 *  @param newYScaleType The scale type for the y-axis.
 *  @return The initialized CPTXYGraph object.
 **/
-(id)initWithFrame:(CGRect)newFrame xScaleType:(CPTScaleType)newXScaleType yScaleType:(CPTScaleType)newYScaleType
{
    if ( (self = [super initWithFrame:newFrame]) ) {
        xScaleType = newXScaleType;
        yScaleType = newYScaleType;
    }
    return self;
}

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTXYGraph object with the provided frame rectangle.
 *
 *  The initialized layer will have the following properties:
 *  - @link CPTXYPlotSpace::xScaleType xScaleType @endlink = #CPTScaleTypeLinear
 *  - @link CPTXYPlotSpace::yScaleType yScaleType @endlink = #CPTScaleTypeLinear
 *
 *  @param newFrame The frame rectangle.
 *  @return The initialized CPTXYGraph object.
 *  @see @link CPTXYGraph::initWithFrame:xScaleType:yScaleType: -initWithFrame:xScaleType:yScaleType: @endlink
 **/
-(id)initWithFrame:(CGRect)newFrame
{
    return [self initWithFrame:newFrame xScaleType:CPTScaleTypeLinear yScaleType:CPTScaleTypeLinear];
}

/// @}

/// @cond

-(id)initWithLayer:(id)layer
{
    if ( (self = [super initWithLayer:layer]) ) {
        CPTXYGraph *theLayer = (CPTXYGraph *)layer;

        xScaleType = theLayer->xScaleType;
        yScaleType = theLayer->yScaleType;
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

    [coder encodeInt:self.xScaleType forKey:@"CPTXYGraph.xScaleType"];
    [coder encodeInt:self.yScaleType forKey:@"CPTXYGraph.yScaleType"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        xScaleType = (CPTScaleType)[coder decodeIntForKey : @"CPTXYGraph.xScaleType"];
        yScaleType = (CPTScaleType)[coder decodeIntForKey : @"CPTXYGraph.yScaleType"];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Factory Methods

/// @cond

-(CPTPlotSpace *)newPlotSpace
{
    CPTXYPlotSpace *space = [[CPTXYPlotSpace alloc] init];

    space.xScaleType = self.xScaleType;
    space.yScaleType = self.yScaleType;
    return space;
}

-(CPTAxisSet *)newAxisSet
{
    CPTXYAxisSet *newAxisSet = [(CPTXYAxisSet *)[CPTXYAxisSet alloc] initWithFrame:self.bounds];

    newAxisSet.xAxis.plotSpace = self.defaultPlotSpace;
    newAxisSet.yAxis.plotSpace = self.defaultPlotSpace;
    return newAxisSet;
}

/// @endcond

@end
