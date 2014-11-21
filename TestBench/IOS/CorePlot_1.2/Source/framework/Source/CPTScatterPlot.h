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
#import "CPTDefinitions.h"
#import "CPTPlot.h"

/// @file

@class CPTLineStyle;
@class CPTMutableNumericData;
@class CPTNumericData;
@class CPTPlotSymbol;
@class CPTScatterPlot;
@class CPTFill;

/// @ingroup plotBindingsScatterPlot
/// @{
extern NSString *const CPTScatterPlotBindingXValues;
extern NSString *const CPTScatterPlotBindingYValues;
extern NSString *const CPTScatterPlotBindingPlotSymbols;
/// @}

/**
 *  @brief Enumeration of scatter plot data source field types
 **/
typedef enum _CPTScatterPlotField {
    CPTScatterPlotFieldX, ///< X values.
    CPTScatterPlotFieldY  ///< Y values.
}
CPTScatterPlotField;

/**
 *  @brief Enumeration of scatter plot interpolation algorithms
 **/
typedef enum _CPTScatterPlotInterpolation {
    CPTScatterPlotInterpolationLinear,    ///< Linear interpolation.
    CPTScatterPlotInterpolationStepped,   ///< Steps beginning at data point.
    CPTScatterPlotInterpolationHistogram, ///< Steps centered at data point.
    CPTScatterPlotInterpolationCurved     ///< Bezier curve interpolation.
}
CPTScatterPlotInterpolation;

#pragma mark -

/**
 *  @brief A scatter plot data source.
 **/
@protocol CPTScatterPlotDataSource<CPTPlotDataSource>

@optional

/// @name Plot Symbols
/// @{

/** @brief @optional Gets a range of plot symbols for the given scatter plot.
 *  @param plot The scatter plot.
 *  @param indexRange The range of the data indexes of interest.
 *  @return An array of plot symbols.
 **/
-(NSArray *)symbolsForScatterPlot:(CPTScatterPlot *)plot recordIndexRange:(NSRange)indexRange;

/** @brief @optional Gets a single plot symbol for the given scatter plot.
 *  This method will not be called if
 *  @link CPTScatterPlotDataSource::symbolsForScatterPlot:recordIndexRange: -symbolsForScatterPlot:recordIndexRange: @endlink
 *  is also implemented in the datasource.
 *  @param plot The scatter plot.
 *  @param idx The data index of interest.
 *  @return The plot symbol to show for the point with the given index.
 **/
-(CPTPlotSymbol *)symbolForScatterPlot:(CPTScatterPlot *)plot recordIndex:(NSUInteger)idx;

/// @}

@end

#pragma mark -

/**
 *  @brief Scatter plot delegate.
 **/
@protocol CPTScatterPlotDelegate<CPTPlotDelegate>

@optional

/// @name Point Selection
/// @{

/** @brief @optional Informs the delegate that a data point was
 *  @if MacOnly clicked. @endif
 *  @if iOSOnly touched. @endif
 *  @param plot The scatter plot.
 *  @param idx The index of the
 *  @if MacOnly clicked data point. @endif
 *  @if iOSOnly touched data point. @endif
 **/
-(void)scatterPlot:(CPTScatterPlot *)plot plotSymbolWasSelectedAtRecordIndex:(NSUInteger)idx;

/** @brief @optional Informs the delegate that a data point was
 *  @if MacOnly clicked. @endif
 *  @if iOSOnly touched. @endif
 *  @param plot The scatter plot.
 *  @param idx The index of the
 *  @if MacOnly clicked data point. @endif
 *  @if iOSOnly touched data point. @endif
 *  @param event The event that triggered the selection.
 **/
-(void)scatterPlot:(CPTScatterPlot *)plot plotSymbolWasSelectedAtRecordIndex:(NSUInteger)idx withEvent:(CPTNativeEvent *)event;

/// @}

@end

#pragma mark -

@interface CPTScatterPlot : CPTPlot {
    @private
    CPTScatterPlotInterpolation interpolation;
    CPTLineStyle *dataLineStyle;
    CPTPlotSymbol *plotSymbol;
    CPTFill *areaFill;
    CPTFill *areaFill2;
    NSDecimal areaBaseValue;
    NSDecimal areaBaseValue2;
    CGFloat plotSymbolMarginForHitDetection;
}

/// @name Appearance
/// @{
@property (nonatomic, readwrite) NSDecimal areaBaseValue;
@property (nonatomic, readwrite) NSDecimal areaBaseValue2;
@property (nonatomic, readwrite, assign) CPTScatterPlotInterpolation interpolation;
/// @}

/// @name Drawing
/// @{
@property (nonatomic, readwrite, copy) CPTLineStyle *dataLineStyle;
@property (nonatomic, readwrite, copy) CPTPlotSymbol *plotSymbol;
@property (nonatomic, readwrite, copy) CPTFill *areaFill;
@property (nonatomic, readwrite, copy) CPTFill *areaFill2;
/// @}

/// @name User Interaction
/// @{
@property (nonatomic, readwrite, assign) CGFloat plotSymbolMarginForHitDetection;
/// @}

/// @name Visible Points
/// @{
-(NSUInteger)indexOfVisiblePointClosestToPlotAreaPoint:(CGPoint)viewPoint;
-(CGPoint)plotAreaPointOfVisiblePointAtIndex:(NSUInteger)idx;
/// @}

/// @name Plot Symbols
/// @{
-(CPTPlotSymbol *)plotSymbolForRecordIndex:(NSUInteger)idx;
/// @}

@end
