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
#import "_CPTXYTheme.h"

#import "CPTPlotRange.h"
#import "CPTUtilities.h"
#import "CPTXYGraph.h"
#import "CPTXYPlotSpace.h"

/**
 *  @brief Creates a CPTXYGraph instance formatted with padding of 60 on each side and X and Y plot ranges of +/- 1.
 **/
@implementation _CPTXYTheme

/// @name Initialization
/// @{

-(id)init
{
    if ( (self = [super init]) ) {
        self.graphClass = [CPTXYGraph class];
    }
    return self;
}

/// @}

-(id)newGraph
{
    CPTXYGraph *graph;

    if ( self.graphClass ) {
        graph = [(CPTXYGraph *)[self.graphClass alloc] initWithFrame:CPTRectMake(0.0, 0.0, 200.0, 200.0)];
    }
    else {
        graph = [(CPTXYGraph *)[CPTXYGraph alloc] initWithFrame:CPTRectMake(0.0, 0.0, 200.0, 200.0)];
    }
    graph.paddingLeft   = CPTFloat(60.0);
    graph.paddingTop    = CPTFloat(60.0);
    graph.paddingRight  = CPTFloat(60.0);
    graph.paddingBottom = CPTFloat(60.0);

    CPTXYPlotSpace *plotSpace = (CPTXYPlotSpace *)graph.defaultPlotSpace;
    plotSpace.xRange = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromDouble(-1.0) length:CPTDecimalFromDouble(1.0)];
    plotSpace.yRange = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromDouble(-1.0) length:CPTDecimalFromDouble(1.0)];

    [self applyThemeToGraph:graph];

    return graph;
}

#pragma mark -
#pragma mark NSCoding Methods

-(Class)classForCoder
{
    return [CPTTheme class];
}

@end
