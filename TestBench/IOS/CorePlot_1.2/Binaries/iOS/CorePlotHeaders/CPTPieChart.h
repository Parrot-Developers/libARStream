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

@class CPTColor;
@class CPTFill;
@class CPTMutableNumericData;
@class CPTNumericData;
@class CPTPieChart;
@class CPTTextLayer;
@class CPTLineStyle;

/// @ingroup plotBindingsPieChart
/// @{
extern NSString *const CPTPieChartBindingPieSliceWidthValues;
extern NSString *const CPTPieChartBindingPieSliceFills;
extern NSString *const CPTPieChartBindingPieSliceRadialOffsets;
/// @}

/**
 *  @brief Enumeration of pie chart data source field types.
 **/
typedef enum _CPTPieChartField {
    CPTPieChartFieldSliceWidth,           ///< Pie slice width.
    CPTPieChartFieldSliceWidthNormalized, ///< Pie slice width normalized [0, 1].
    CPTPieChartFieldSliceWidthSum         ///< Cumulative sum of pie slice widths.
}
CPTPieChartField;

/**
 *  @brief Enumeration of pie slice drawing directions.
 **/
typedef enum _CPTPieDirection {
    CPTPieDirectionClockwise,       ///< Pie slices are drawn in a clockwise direction.
    CPTPieDirectionCounterClockwise ///< Pie slices are drawn in a counter-clockwise direction.
}
CPTPieDirection;

#pragma mark -

/**
 *  @brief A pie chart data source.
 **/
@protocol CPTPieChartDataSource<CPTPlotDataSource>
@optional

/// @name Slice Style
/// @{

/** @brief @optional Gets a range of slice fills for the given pie chart.
 *  @param pieChart The pie chart.
 *  @param indexRange The range of the data indexes of interest.
 *  @return The pie slice fill for the slice with the given index.
 **/
-(NSArray *)sliceFillsForPieChart:(CPTPieChart *)pieChart recordIndexRange:(NSRange)indexRange;

/** @brief @optional Gets a fill for the given pie chart slice.
 *  This method will not be called if
 *  @link CPTPieChartDataSource::sliceFillsForPieChart:recordIndexRange: -sliceFillsForPieChart:recordIndexRange: @endlink
 *  is also implemented in the datasource.
 *  @param pieChart The pie chart.
 *  @param idx The data index of interest.
 *  @return The pie slice fill for the slice with the given index.
 **/
-(CPTFill *)sliceFillForPieChart:(CPTPieChart *)pieChart recordIndex:(NSUInteger)idx;

/// @}

/// @name Slice Layout
/// @{

/** @brief @optional Gets a range of slice offsets for the given pie chart.
 *  @param pieChart The pie chart.
 *  @param indexRange The range of the data indexes of interest.
 *  @return An array of radial offsets.
 **/
-(NSArray *)radialOffsetsForPieChart:(CPTPieChart *)pieChart recordIndexRange:(NSRange)indexRange;

/** @brief @optional Offsets the slice radially from the center point. Can be used to @quote{explode} the chart.
 *  This method will not be called if
 *  @link CPTPieChartDataSource::radialOffsetsForPieChart:recordIndexRange: -radialOffsetsForPieChart:recordIndexRange: @endlink
 *  is also implemented in the datasource.
 *  @param pieChart The pie chart.
 *  @param idx The data index of interest.
 *  @return The radial offset in view coordinates. Zero is no offset.
 **/
-(CGFloat)radialOffsetForPieChart:(CPTPieChart *)pieChart recordIndex:(NSUInteger)idx;

/// @}

/// @name Legends
/// @{

/** @brief @optional Gets the legend title for the given pie chart slice.
 *  @param pieChart The pie chart.
 *  @param idx The data index of interest.
 *  @return The title text for the legend entry for the point with the given index.
 **/
-(NSString *)legendTitleForPieChart:(CPTPieChart *)pieChart recordIndex:(NSUInteger)idx;

/// @}
@end

#pragma mark -

/**
 *  @brief Pie chart delegate.
 **/
@protocol CPTPieChartDelegate<CPTPlotDelegate>

@optional

/// @name Slice Selection
/// @{

/** @brief @optional Informs the delegate that a pie slice was
 *  @if MacOnly clicked. @endif
 *  @if iOSOnly touched. @endif
 *  @param plot The pie chart.
 *  @param idx The index of the
 *  @if MacOnly clicked pie slice. @endif
 *  @if iOSOnly touched pie slice. @endif
 **/
-(void)pieChart:(CPTPieChart *)plot sliceWasSelectedAtRecordIndex:(NSUInteger)idx;

/** @brief @optional Informs the delegate that a pie slice was
 *  @if MacOnly clicked. @endif
 *  @if iOSOnly touched. @endif
 *  @param plot The pie chart.
 *  @param idx The index of the
 *  @if MacOnly clicked pie slice. @endif
 *  @if iOSOnly touched pie slice. @endif
 *  @param event The event that triggered the selection.
 **/
-(void)pieChart:(CPTPieChart *)plot sliceWasSelectedAtRecordIndex:(NSUInteger)idx withEvent:(CPTNativeEvent *)event;

/// @}

@end

#pragma mark -

@interface CPTPieChart : CPTPlot {
    @private
    CGFloat pieRadius;
    CGFloat pieInnerRadius;
    CGFloat startAngle;
    CGFloat endAngle;
    CPTPieDirection sliceDirection;
    CGPoint centerAnchor;
    CPTLineStyle *borderLineStyle;
    CPTFill *overlayFill;
    BOOL labelRotationRelativeToRadius;
}

/// @name Appearance
/// @{
@property (nonatomic, readwrite) CGFloat pieRadius;
@property (nonatomic, readwrite) CGFloat pieInnerRadius;
@property (nonatomic, readwrite) CGFloat startAngle;
@property (nonatomic, readwrite) CGFloat endAngle;
@property (nonatomic, readwrite) CPTPieDirection sliceDirection;
@property (nonatomic, readwrite) CGPoint centerAnchor;
/// @}

/// @name Drawing
/// @{
@property (nonatomic, readwrite, copy) CPTLineStyle *borderLineStyle;
@property (nonatomic, readwrite, copy) CPTFill *overlayFill;
/// @}

/// @name Data Labels
/// @{
@property (nonatomic, readwrite, assign) BOOL labelRotationRelativeToRadius;
/// @}

/// @name Information
/// @{
-(NSUInteger)pieSliceIndexAtAngle:(CGFloat)angle;
-(CGFloat)medianAngleForPieSliceIndex:(NSUInteger)index;
/// @}

/// @name Factory Methods
/// @{
+(CPTColor *)defaultPieSliceColorForIndex:(NSUInteger)pieSliceIndex;
/// @}

@end
