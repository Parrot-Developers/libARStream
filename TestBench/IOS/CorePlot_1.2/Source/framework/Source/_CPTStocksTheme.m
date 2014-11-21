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
#import "_CPTStocksTheme.h"

#import "CPTBorderedLayer.h"
#import "CPTColor.h"
#import "CPTFill.h"
#import "CPTGradient.h"
#import "CPTMutableLineStyle.h"
#import "CPTMutableTextStyle.h"
#import "CPTPlotAreaFrame.h"
#import "CPTUtilities.h"
#import "CPTXYAxis.h"
#import "CPTXYAxisSet.h"
#import "CPTXYGraph.h"

NSString *const kCPTStocksTheme = @"Stocks";

/**
 *  @brief Creates a CPTXYGraph instance formatted with a gradient background and white lines.
 **/
@implementation _CPTStocksTheme

+(void)load
{
    [self registerTheme:self];
}

+(NSString *)name
{
    return kCPTStocksTheme;
}

#pragma mark -

-(void)applyThemeToBackground:(CPTGraph *)graph
{
    graph.fill = [CPTFill fillWithColor:[CPTColor blackColor]];
}

-(void)applyThemeToPlotArea:(CPTPlotAreaFrame *)plotAreaFrame
{
    CPTGradient *stocksBackgroundGradient = [[[CPTGradient alloc] init] autorelease];

    stocksBackgroundGradient = [stocksBackgroundGradient addColorStop:[CPTColor colorWithComponentRed:CPTFloat(0.21569) green:CPTFloat(0.28627) blue:CPTFloat(0.44706) alpha:CPTFloat(1.0)]
                                                           atPosition:CPTFloat(0.0)];
    stocksBackgroundGradient = [stocksBackgroundGradient addColorStop:[CPTColor colorWithComponentRed:CPTFloat(0.09412) green:CPTFloat(0.17255) blue:CPTFloat(0.36078) alpha:CPTFloat(1.0)]
                                                           atPosition:CPTFloat(0.5)];
    stocksBackgroundGradient = [stocksBackgroundGradient addColorStop:[CPTColor colorWithComponentRed:CPTFloat(0.05882) green:CPTFloat(0.13333) blue:CPTFloat(0.33333) alpha:CPTFloat(1.0)]
                                                           atPosition:CPTFloat(0.5)];
    stocksBackgroundGradient = [stocksBackgroundGradient addColorStop:[CPTColor colorWithComponentRed:CPTFloat(0.05882) green:CPTFloat(0.13333) blue:CPTFloat(0.33333) alpha:CPTFloat(1.0)]
                                                           atPosition:CPTFloat(1.0)];

    stocksBackgroundGradient.angle = CPTFloat(270.0);
    plotAreaFrame.fill             = [CPTFill fillWithGradient:stocksBackgroundGradient];

    CPTMutableLineStyle *borderLineStyle = [CPTMutableLineStyle lineStyle];
    borderLineStyle.lineColor = [CPTColor colorWithGenericGray:CPTFloat(0.2)];
    borderLineStyle.lineWidth = CPTFloat(0.0);

    plotAreaFrame.borderLineStyle = borderLineStyle;
    plotAreaFrame.cornerRadius    = CPTFloat(14.0);
}

-(void)applyThemeToAxisSet:(CPTAxisSet *)axisSet
{
    CPTXYAxisSet *xyAxisSet             = (CPTXYAxisSet *)axisSet;
    CPTMutableLineStyle *majorLineStyle = [CPTMutableLineStyle lineStyle];

    majorLineStyle.lineCap   = kCGLineCapRound;
    majorLineStyle.lineColor = [CPTColor whiteColor];
    majorLineStyle.lineWidth = CPTFloat(3.0);

    CPTMutableLineStyle *minorLineStyle = [CPTMutableLineStyle lineStyle];
    minorLineStyle.lineColor = [CPTColor whiteColor];
    minorLineStyle.lineWidth = CPTFloat(3.0);

    CPTXYAxis *x                        = xyAxisSet.xAxis;
    CPTMutableTextStyle *whiteTextStyle = [[[CPTMutableTextStyle alloc] init] autorelease];
    whiteTextStyle.color    = [CPTColor whiteColor];
    whiteTextStyle.fontSize = CPTFloat(14.0);
    CPTMutableTextStyle *minorTickWhiteTextStyle = [[[CPTMutableTextStyle alloc] init] autorelease];
    minorTickWhiteTextStyle.color    = [CPTColor whiteColor];
    minorTickWhiteTextStyle.fontSize = CPTFloat(12.0);
    x.labelingPolicy                 = CPTAxisLabelingPolicyFixedInterval;
    x.majorIntervalLength            = CPTDecimalFromDouble(0.5);
    x.orthogonalCoordinateDecimal    = CPTDecimalFromDouble(0.0);
    x.tickDirection                  = CPTSignNone;
    x.minorTicksPerInterval          = 4;
    x.majorTickLineStyle             = majorLineStyle;
    x.minorTickLineStyle             = minorLineStyle;
    x.axisLineStyle                  = majorLineStyle;
    x.majorTickLength                = CPTFloat(7.0);
    x.minorTickLength                = CPTFloat(5.0);
    x.labelTextStyle                 = whiteTextStyle;
    x.minorTickLabelTextStyle        = minorTickWhiteTextStyle;
    x.titleTextStyle                 = whiteTextStyle;

    CPTXYAxis *y = xyAxisSet.yAxis;
    y.labelingPolicy              = CPTAxisLabelingPolicyFixedInterval;
    y.majorIntervalLength         = CPTDecimalFromDouble(0.5);
    y.minorTicksPerInterval       = 4;
    y.orthogonalCoordinateDecimal = CPTDecimalFromDouble(0.0);
    y.tickDirection               = CPTSignNone;
    y.majorTickLineStyle          = majorLineStyle;
    y.minorTickLineStyle          = minorLineStyle;
    y.axisLineStyle               = majorLineStyle;
    y.majorTickLength             = CPTFloat(7.0);
    y.minorTickLength             = CPTFloat(5.0);
    y.labelTextStyle              = whiteTextStyle;
    y.minorTickLabelTextStyle     = minorTickWhiteTextStyle;
    y.titleTextStyle              = whiteTextStyle;
}

#pragma mark -
#pragma mark NSCoding Methods

-(Class)classForCoder
{
    return [CPTTheme class];
}

@end
