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
#import "CPTPlotGroup.h"

#import "CPTPlot.h"

/**
 *  @brief Defines the coordinate system of a plot.
 **/
@implementation CPTPlotGroup

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        // support old archives
        if ( [coder containsValueForKey:@"CPTPlotGroup.identifier"] ) {
            self.identifier = [coder decodeObjectForKey:@"CPTPlotGroup.identifier"];
        }
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Organizing Plots

/** @brief Add a plot to this plot group.
 *  @param plot The plot.
 **/
-(void)addPlot:(CPTPlot *)plot
{
    NSParameterAssert(plot);

    [self addSublayer:plot];
}

/** @brief Add a plot to this plot group at the given index.
 *  @param plot The plot.
 *  @param idx The index at which to insert the plot. This value must not be greater than the count of elements in the sublayer array.
 **/
-(void)insertPlot:(CPTPlot *)plot atIndex:(NSUInteger)idx
{
    NSParameterAssert(plot);
    NSParameterAssert(idx <= [[self sublayers] count]);

    [self insertSublayer:plot atIndex:(unsigned)idx];
}

/** @brief Remove a plot from this plot group.
 *  @param plot The plot to remove.
 **/
-(void)removePlot:(CPTPlot *)plot
{
    if ( self == [plot superlayer] ) {
        [plot removeFromSuperlayer];
    }
}

#pragma mark -
#pragma mark Drawing

/// @cond

-(void)renderAsVectorInContext:(CGContextRef)context
{
    // nothing to draw
}

/// @endcond

@end
