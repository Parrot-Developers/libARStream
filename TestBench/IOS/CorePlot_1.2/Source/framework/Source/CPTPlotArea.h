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
#import "CPTAnnotationHostLayer.h"
#import "CPTGraph.h"
#import "CPTLayer.h"

@class CPTAxis;
@class CPTAxisLabelGroup;
@class CPTAxisSet;
@class CPTGridLineGroup;
@class CPTPlotGroup;
@class CPTLineStyle;
@class CPTFill;

@interface CPTPlotArea : CPTAnnotationHostLayer {
    @private
    CPTGridLineGroup *minorGridLineGroup;
    CPTGridLineGroup *majorGridLineGroup;
    CPTAxisSet *axisSet;
    CPTPlotGroup *plotGroup;
    CPTAxisLabelGroup *axisLabelGroup;
    CPTAxisLabelGroup *axisTitleGroup;
    CPTFill *fill;
    NSArray *topDownLayerOrder;
    CPTGraphLayerType *bottomUpLayerOrder;
    BOOL updatingLayers;
}

/// @name Layers
/// @{
@property (nonatomic, readwrite, retain) CPTGridLineGroup *minorGridLineGroup;
@property (nonatomic, readwrite, retain) CPTGridLineGroup *majorGridLineGroup;
@property (nonatomic, readwrite, retain) CPTAxisSet *axisSet;
@property (nonatomic, readwrite, retain) CPTPlotGroup *plotGroup;
@property (nonatomic, readwrite, retain) CPTAxisLabelGroup *axisLabelGroup;
@property (nonatomic, readwrite, retain) CPTAxisLabelGroup *axisTitleGroup;
/// @}

/// @name Layer Ordering
/// @{
@property (nonatomic, readwrite, retain) NSArray *topDownLayerOrder;
/// @}

/// @name Decorations
/// @{
@property (nonatomic, readwrite, copy) CPTLineStyle *borderLineStyle;
@property (nonatomic, readwrite, copy) CPTFill *fill;
/// @}

/// @name Axis Set Layer Management
/// @{
-(void)updateAxisSetLayersForType:(CPTGraphLayerType)layerType;
-(void)setAxisSetLayersForType:(CPTGraphLayerType)layerType;
-(unsigned)sublayerIndexForAxis:(CPTAxis *)axis layerType:(CPTGraphLayerType)layerType;
/// @}

@end
