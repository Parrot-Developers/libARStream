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
#import "CPTPlotAreaFrame.h"

#import "CPTAxisSet.h"
#import "CPTPlotArea.h"
#import "CPTPlotGroup.h"

/// @cond
@interface CPTPlotAreaFrame()

@property (nonatomic, readwrite, retain) CPTPlotArea *plotArea;

@end

/// @endcond

#pragma mark -

/**
 *  @brief A layer drawn on top of the graph layer and behind all plot elements.
 *
 *  All graph elements, except for titles, legends, and other annotations
 *  attached directly to the graph itself are clipped to the plot area frame.
 **/
@implementation CPTPlotAreaFrame

/** @property CPTPlotArea *plotArea
 *  @brief The plot area.
 **/
@synthesize plotArea;

/** @property CPTAxisSet *axisSet
 *  @brief The axis set.
 **/
@dynamic axisSet;

/** @property CPTPlotGroup *plotGroup
 *  @brief The plot group.
 **/
@dynamic plotGroup;

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTPlotAreaFrame object with the provided frame rectangle.
 *
 *  This is the designated initializer. The initialized layer will have the following properties:
 *  - @ref plotArea = a new CPTPlotArea with the same frame rectangle
 *  - @ref masksToBorder = @YES
 *  - @ref needsDisplayOnBoundsChange = @YES
 *
 *  @param newFrame The frame rectangle.
 *  @return The initialized CPTPlotAreaFrame object.
 **/
-(id)initWithFrame:(CGRect)newFrame
{
    if ( (self = [super initWithFrame:newFrame]) ) {
        plotArea = nil;

        CPTPlotArea *newPlotArea = [(CPTPlotArea *)[CPTPlotArea alloc] initWithFrame:newFrame];
        self.plotArea = newPlotArea;
        [newPlotArea release];

        self.masksToBorder              = YES;
        self.needsDisplayOnBoundsChange = YES;
    }
    return self;
}

/// @}

/// @cond

-(id)initWithLayer:(id)layer
{
    if ( (self = [super initWithLayer:layer]) ) {
        CPTPlotAreaFrame *theLayer = (CPTPlotAreaFrame *)layer;

        plotArea = [theLayer->plotArea retain];
    }
    return self;
}

-(void)dealloc
{
    [plotArea release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [super encodeWithCoder:coder];

    [coder encodeObject:self.plotArea forKey:@"CPTPlotAreaFrame.plotArea"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        plotArea = [[coder decodeObjectForKey:@"CPTPlotAreaFrame.plotArea"] retain];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Accessors

/// @cond

-(void)setPlotArea:(CPTPlotArea *)newPlotArea
{
    if ( newPlotArea != plotArea ) {
        [plotArea removeFromSuperlayer];
        [plotArea release];
        plotArea = [newPlotArea retain];
        if ( plotArea ) {
            [self insertSublayer:plotArea atIndex:0];
        }
        [self setNeedsLayout];
    }
}

-(CPTAxisSet *)axisSet
{
    return self.plotArea.axisSet;
}

-(void)setAxisSet:(CPTAxisSet *)newAxisSet
{
    self.plotArea.axisSet = newAxisSet;
}

-(CPTPlotGroup *)plotGroup
{
    return self.plotArea.plotGroup;
}

-(void)setPlotGroup:(CPTPlotGroup *)newPlotGroup
{
    self.plotArea.plotGroup = newPlotGroup;
}

/// @endcond

@end
