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
#import "CPTResponder.h"

@class CPTLayer;
@class CPTPlotRange;
@class CPTGraph;
@class CPTPlotSpace;

/// @name Plot Space
/// @{

/** @brief Plot space coordinate change notification.
 *
 *  This notification is posted to the default notification center whenever the mapping between
 *  the plot space coordinate system and drawing coordinates changes.
 *  @ingroup notification
 **/
extern NSString *const CPTPlotSpaceCoordinateMappingDidChangeNotification;

/// @}

/**
 *  @brief Plot space delegate.
 **/
@protocol CPTPlotSpaceDelegate<NSObject>

@optional

/// @name Scaling
/// @{

/** @brief @optional Informs the receiver that it should uniformly scale (e.g., in response to a pinch gesture).
 *  @param space The plot space.
 *  @param interactionScale The scaling factor.
 *  @param interactionPoint The coordinates of the scaling centroid.
 *  @return @YES if the gesture should be handled by the plot space, and @NO if not.
 *  In either case, the delegate may choose to take extra actions, or handle the scaling itself.
 **/
-(BOOL)plotSpace:(CPTPlotSpace *)space shouldScaleBy:(CGFloat)interactionScale aboutPoint:(CGPoint)interactionPoint;

/// @}

/// @name Scrolling
/// @{

/** @brief @optional Notifies that plot space is going to scroll.
 *  @param space The plot space.
 *  @param proposedDisplacementVector The proposed amount by which the plot space will shift.
 *  @return The displacement actually applied.
 **/
-(CGPoint)plotSpace:(CPTPlotSpace *)space willDisplaceBy:(CGPoint)proposedDisplacementVector;

/// @}

/// @name Plot Range Changes
/// @{

/** @brief @optional Notifies that plot space is going to change a plot range.
 *  @param space The plot space.
 *  @param newRange The proposed new plot range.
 *  @param coordinate The coordinate of the range.
 *  @return The new plot range to be used.
 **/
-(CPTPlotRange *)plotSpace:(CPTPlotSpace *)space willChangePlotRangeTo:(CPTPlotRange *)newRange forCoordinate:(CPTCoordinate)coordinate;

/** @brief @optional Notifies that plot space has changed a plot range.
 *  @param space The plot space.
 *  @param coordinate The coordinate of the range.
 **/
-(void)plotSpace:(CPTPlotSpace *)space didChangePlotRangeForCoordinate:(CPTCoordinate)coordinate;

/// @}

/// @name User Interaction
/// @{

/** @brief @optional Notifies that plot space intercepted a device down event.
 *  @param space The plot space.
 *  @param event The native event.
 *  @param point The point in the host view.
 *  @return Whether the plot space should handle the event or not.
 *  In either case, the delegate may choose to take extra actions, or handle the scaling itself.
 **/
-(BOOL)plotSpace:(CPTPlotSpace *)space shouldHandlePointingDeviceDownEvent:(CPTNativeEvent *)event atPoint:(CGPoint)point;

/** @brief @optional Notifies that plot space intercepted a device dragged event.
 *  @param space The plot space.
 *  @param event The native event.
 *  @param point The point in the host view.
 *  @return Whether the plot space should handle the event or not.
 *  In either case, the delegate may choose to take extra actions, or handle the scaling itself.
 **/
-(BOOL)plotSpace:(CPTPlotSpace *)space shouldHandlePointingDeviceDraggedEvent:(CPTNativeEvent *)event atPoint:(CGPoint)point;

/** @brief @optional Notifies that plot space intercepted a device cancelled event.
 *  @param space The plot space.
 *  @param event The native event.
 *  @return Whether the plot space should handle the event or not.
 *  In either case, the delegate may choose to take extra actions, or handle the scaling itself.
 **/
-(BOOL)plotSpace:(CPTPlotSpace *)space shouldHandlePointingDeviceCancelledEvent:(CPTNativeEvent *)event;

/** @brief @optional Notifies that plot space intercepted a device up event.
 *  @param space The plot space.
 *  @param event The native event.
 *  @param point The point in the host view.
 *  @return Whether the plot space should handle the event or not.
 *  In either case, the delegate may choose to take extra actions, or handle the scaling itself.
 **/
-(BOOL)plotSpace:(CPTPlotSpace *)space shouldHandlePointingDeviceUpEvent:(CPTNativeEvent *)event atPoint:(CGPoint)point;

/// @}

@end

#pragma mark -

@interface CPTPlotSpace : NSObject<CPTResponder, NSCoding> {
    @private
    __cpt_weak CPTGraph *graph;
    id<NSCopying, NSCoding, NSObject> identifier;
    __cpt_weak id<CPTPlotSpaceDelegate> delegate;
    BOOL allowsUserInteraction;
}

@property (nonatomic, readwrite, copy) id<NSCopying, NSCoding, NSObject> identifier;
@property (nonatomic, readwrite, assign) BOOL allowsUserInteraction;
@property (nonatomic, readwrite, cpt_weak_property) __cpt_weak CPTGraph *graph;
@property (nonatomic, readwrite, cpt_weak_property) __cpt_weak id<CPTPlotSpaceDelegate> delegate;

@end

#pragma mark -

/** @category CPTPlotSpace(AbstractMethods)
 *  @brief CPTPlotSpace abstract methods—must be overridden by subclasses
 **/
@interface CPTPlotSpace(AbstractMethods)

/// @name Coordinate Space Conversions
/// @{
-(CGPoint)plotAreaViewPointForPlotPoint:(NSDecimal *)plotPoint;
-(CGPoint)plotAreaViewPointForDoublePrecisionPlotPoint:(double *)plotPoint;
-(void)plotPoint:(NSDecimal *)plotPoint forPlotAreaViewPoint:(CGPoint)point;
-(void)doublePrecisionPlotPoint:(double *)plotPoint forPlotAreaViewPoint:(CGPoint)point;

-(CGPoint)plotAreaViewPointForEvent:(CPTNativeEvent *)event;
-(void)plotPoint:(NSDecimal *)plotPoint forEvent:(CPTNativeEvent *)event;
-(void)doublePrecisionPlotPoint:(double *)plotPoint forEvent:(CPTNativeEvent *)event;
/// @}

/// @name Coordinate Range
/// @{
-(void)setPlotRange:(CPTPlotRange *)newRange forCoordinate:(CPTCoordinate)coordinate;
-(CPTPlotRange *)plotRangeForCoordinate:(CPTCoordinate)coordinate;
/// @}

/// @name Scale Types
/// @{
-(void)setScaleType:(CPTScaleType)newType forCoordinate:(CPTCoordinate)coordinate;
-(CPTScaleType)scaleTypeForCoordinate:(CPTCoordinate)coordinate;
/// @}

/// @name Adjusting Ranges
/// @{
-(void)scaleToFitPlots:(NSArray *)plots;
-(void)scaleBy:(CGFloat)interactionScale aboutPoint:(CGPoint)interactionPoint;
/// @}

@end
