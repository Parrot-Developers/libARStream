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
#import <Cocoa/Cocoa.h>

#import <CorePlot/CPTAnimation.h>
#import <CorePlot/CPTAnimationOperation.h>
#import <CorePlot/CPTAnimationPeriod.h>
#import <CorePlot/CPTAnnotation.h>
#import <CorePlot/CPTAnnotationHostLayer.h>
#import <CorePlot/CPTAxis.h>
#import <CorePlot/CPTAxisLabel.h>
#import <CorePlot/CPTAxisSet.h>
#import <CorePlot/CPTAxisTitle.h>
#import <CorePlot/CPTBarPlot.h>
#import <CorePlot/CPTBorderedLayer.h>
#import <CorePlot/CPTCalendarFormatter.h>
#import <CorePlot/CPTColor.h>
#import <CorePlot/CPTColorSpace.h>
#import <CorePlot/CPTConstraints.h>
#import <CorePlot/CPTDecimalNumberValueTransformer.h>
#import <CorePlot/CPTDefinitions.h>
#import <CorePlot/CPTExceptions.h>
#import <CorePlot/CPTFill.h>
#import <CorePlot/CPTGradient.h>
#import <CorePlot/CPTGraph.h>
#import <CorePlot/CPTImage.h>
#import <CorePlot/CPTLayer.h>
#import <CorePlot/CPTLayerAnnotation.h>
#import <CorePlot/CPTLegend.h>
#import <CorePlot/CPTLegendEntry.h>
#import <CorePlot/CPTLimitBand.h>
#import <CorePlot/CPTLineCap.h>
#import <CorePlot/CPTLineStyle.h>
#import <CorePlot/CPTMutableLineStyle.h>
#import <CorePlot/CPTMutableNumericData.h>
#import <CorePlot/CPTMutableNumericData+TypeConversion.h>
#import <CorePlot/CPTMutablePlotRange.h>
#import <CorePlot/CPTMutableShadow.h>
#import <CorePlot/CPTMutableTextStyle.h>
#import <CorePlot/CPTNumericDataType.h>
#import <CorePlot/CPTNumericData.h>
#import <CorePlot/CPTNumericData+TypeConversion.h>
#import <CorePlot/CPTPieChart.h>
#import <CorePlot/CPTPlatformSpecificDefines.h>
#import <CorePlot/CPTPlatformSpecificFunctions.h>
#import <CorePlot/CPTPlatformSpecificCategories.h>
#import <CorePlot/CPTPathExtensions.h>
#import <CorePlot/CPTPlot.h>
#import <CorePlot/CPTPlotArea.h>
#import <CorePlot/CPTPlotAreaFrame.h>
#import <CorePlot/CPTPlotRange.h>
#import <CorePlot/CPTPlotSpace.h>
#import <CorePlot/CPTPlotSpaceAnnotation.h>
#import <CorePlot/CPTPlotSymbol.h>
#import <CorePlot/CPTRangePlot.h>
#import <CorePlot/CPTResponder.h>
#import <CorePlot/CPTScatterPlot.h>
#import <CorePlot/CPTShadow.h>
#import <CorePlot/CPTTextLayer.h>
#import <CorePlot/CPTTextStyle.h>
#import <CorePlot/CPTTradingRangePlot.h>
#import <CorePlot/CPTTheme.h>
#import <CorePlot/CPTTimeFormatter.h>
#import <CorePlot/CPTUtilities.h>
#import <CorePlot/CPTXYAxis.h>
#import <CorePlot/CPTXYAxisSet.h>
#import <CorePlot/CPTXYGraph.h>
#import <CorePlot/CPTXYPlotSpace.h>
#import <CorePlot/CPTGraphHostingView.h>
