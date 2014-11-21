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
#import "CPTXYAxisSet.h"

#import "CPTLineStyle.h"
#import "CPTPathExtensions.h"
#import "CPTUtilities.h"
#import "CPTXYAxis.h"

/**
 *  @brief A set of cartesian (X-Y) axes.
 **/
@implementation CPTXYAxisSet

/** @property CPTXYAxis *xAxis
 *  @brief The x-axis.
 **/
@dynamic xAxis;

/** @property CPTXYAxis *yAxis
 *  @brief The y-axis.
 **/
@dynamic yAxis;

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTXYAxisSet object with the provided frame rectangle.
 *
 *  This is the designated initializer. The @ref axes array
 *  will contain two new axes with the following properties:
 *
 *  <table>
 *  <tr><td>@bold{Axis}</td><td>@link CPTAxis::coordinate coordinate @endlink</td><td>@link CPTAxis::tickDirection tickDirection @endlink</td></tr>
 *  <tr><td>@ref xAxis</td><td>#CPTCoordinateX</td><td>#CPTSignNegative</td></tr>
 *  <tr><td>@ref yAxis</td><td>#CPTCoordinateY</td><td>#CPTSignNegative</td></tr>
 *  </table>
 *
 *  @param newFrame The frame rectangle.
 *  @return The initialized CPTXYAxisSet object.
 **/
-(id)initWithFrame:(CGRect)newFrame
{
    if ( (self = [super initWithFrame:newFrame]) ) {
        CPTXYAxis *xAxis = [(CPTXYAxis *)[CPTXYAxis alloc] initWithFrame:newFrame];
        xAxis.coordinate    = CPTCoordinateX;
        xAxis.tickDirection = CPTSignNegative;

        CPTXYAxis *yAxis = [(CPTXYAxis *)[CPTXYAxis alloc] initWithFrame:newFrame];
        yAxis.coordinate    = CPTCoordinateY;
        yAxis.tickDirection = CPTSignNegative;

        self.axes = [NSArray arrayWithObjects:xAxis, yAxis, nil];
        [xAxis release];
        [yAxis release];
    }
    return self;
}

/// @}

#pragma mark -
#pragma mark Drawing

/// @cond

-(void)renderAsVectorInContext:(CGContextRef)context
{
    if ( self.hidden ) {
        return;
    }

    CPTLineStyle *theLineStyle = self.borderLineStyle;
    if ( theLineStyle ) {
        [super renderAsVectorInContext:context];

        CALayer *superlayer = self.superlayer;
        CGRect borderRect   = CPTAlignRectToUserSpace(context, [self convertRect:superlayer.bounds fromLayer:superlayer]);

        [theLineStyle setLineStyleInContext:context];

        CGFloat radius = superlayer.cornerRadius;

        if ( radius > 0.0 ) {
            radius = MIN( MIN( radius, borderRect.size.width * CPTFloat(0.5) ), borderRect.size.height * CPTFloat(0.5) );

            CGContextBeginPath(context);
            AddRoundedRectPath(context, borderRect, radius);

            [theLineStyle strokePathInContext:context];
        }
        else {
            [theLineStyle strokeRect:borderRect inContext:context];
        }
    }
}

/// @endcond

#pragma mark -
#pragma mark Accessors

/// @cond

-(CPTXYAxis *)xAxis
{
    return (CPTXYAxis *)[self axisForCoordinate:CPTCoordinateX atIndex:0];
}

-(CPTXYAxis *)yAxis
{
    return (CPTXYAxis *)[self axisForCoordinate:CPTCoordinateY atIndex:0];
}

/// @endcond

@end
