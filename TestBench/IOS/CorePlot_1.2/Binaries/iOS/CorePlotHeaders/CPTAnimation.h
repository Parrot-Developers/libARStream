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
@class CPTAnimationOperation;
@class CPTAnimationPeriod;

/**
 *  @brief Enumeration of animation curves.
 **/
typedef enum _CPTAnimationCurve {
    CPTAnimationCurveDefault,          ///< Use the default animation curve.
    CPTAnimationCurveLinear,           ///< Linear animation curve.
    CPTAnimationCurveBackIn,           ///< Backing in animation curve.
    CPTAnimationCurveBackOut,          ///< Backing out animation curve.
    CPTAnimationCurveBackInOut,        ///< Backing in and out animation curve.
    CPTAnimationCurveBounceIn,         ///< Bounce in animation curve.
    CPTAnimationCurveBounceOut,        ///< Bounce out animation curve.
    CPTAnimationCurveBounceInOut,      ///< Bounce in and out animation curve.
    CPTAnimationCurveCircularIn,       ///< Circular in animation curve.
    CPTAnimationCurveCircularOut,      ///< Circular out animation curve.
    CPTAnimationCurveCircularInOut,    ///< Circular in and out animation curve.
    CPTAnimationCurveElasticIn,        ///< Elastic in animation curve.
    CPTAnimationCurveElasticOut,       ///< Elastic out animation curve.
    CPTAnimationCurveElasticInOut,     ///< Elastic in and out animation curve.
    CPTAnimationCurveExponentialIn,    ///< Exponential in animation curve.
    CPTAnimationCurveExponentialOut,   ///< Exponential out animation curve.
    CPTAnimationCurveExponentialInOut, ///< Exponential in and out animation curve.
    CPTAnimationCurveSinusoidalIn,     ///< Sinusoidal in animation curve.
    CPTAnimationCurveSinusoidalOut,    ///< Sinusoidal out animation curve.
    CPTAnimationCurveSinusoidalInOut,  ///< Sinusoidal in and out animation curve.
    CPTAnimationCurveCubicIn,          ///< Cubic in animation curve.
    CPTAnimationCurveCubicOut,         ///< Cubic out animation curve.
    CPTAnimationCurveCubicInOut,       ///< Cubic in and out animation curve.
    CPTAnimationCurveQuadraticIn,      ///< Quadratic in animation curve.
    CPTAnimationCurveQuadraticOut,     ///< Quadratic out animation curve.
    CPTAnimationCurveQuadraticInOut,   ///< Quadratic in and out animation curve.
    CPTAnimationCurveQuarticIn,        ///< Quartic in animation curve.
    CPTAnimationCurveQuarticOut,       ///< Quartic out animation curve.
    CPTAnimationCurveQuarticInOut,     ///< Quartic in and out animation curve.
    CPTAnimationCurveQuinticIn,        ///< Quintic in animation curve.
    CPTAnimationCurveQuinticOut,       ///< Quintic out animation curve.
    CPTAnimationCurveQuinticInOut      ///< Quintic in and out animation curve.
}
CPTAnimationCurve;

/**
 *  @brief Animation delegate.
 **/
@protocol CPTAnimationDelegate<NSObject>

@optional

/// @name Animation
/// @{

/** @brief @optional Informs the delegate that an animation operation started animating.
 *  @param operation The animation operation.
 **/
-(void)animationDidStart:(CPTAnimationOperation *)operation;

/** @brief @optional Informs the delegate that an animation operation stopped after reaching its full duration.
 *  @param operation The animation operation.
 **/
-(void)animationDidFinish:(CPTAnimationOperation *)operation;

/** @brief @optional Informs the delegate that an animation operation was stopped before reaching its full duration.
 *  @param operation The animation operation.
 **/
-(void)animationCancelled:(CPTAnimationOperation *)operation;

/** @brief @optional Informs the delegate that the animated property is about to update.
 *  @param operation The animation operation.
 **/
-(void)animationWillUpdate:(CPTAnimationOperation *)operation;

/** @brief @optional Informs the delegate that the animated property has been updated.
 *  @param operation The animation operation.
 **/
-(void)animationDidUpdate:(CPTAnimationOperation *)operation;

/// @}

@end

#pragma mark -

@interface CPTAnimation : NSObject
{
    @private
    NSMutableArray *animationOperations;
    NSMutableArray *runningAnimationOperations;
    NSMutableArray *expiredAnimationOperations;
    NSTimer *timer;

    CGFloat timeOffset;
    CPTAnimationCurve defaultAnimationCurve;
}

/// @name Time
/// @{
@property (nonatomic, readonly, assign) CGFloat timeOffset;
/// @}

/// @name Animation Curve
/// @{
@property (nonatomic, assign) CPTAnimationCurve defaultAnimationCurve;
/// @}

/// @name Animation Controller Instance
/// @{
+(CPTAnimation *)sharedInstance;
/// @}

/// @name Property Animation
/// @{
+(CPTAnimationOperation *)animate:(id)object property:(NSString *)property period:(CPTAnimationPeriod *)period animationCurve:(CPTAnimationCurve)animationCurve delegate:(NSObject<CPTAnimationDelegate> *)delegate;
/// @}

/// @name Animation Management
/// @{
-(CPTAnimationOperation *)addAnimationOperation:(CPTAnimationOperation *)animationOperation;
-(void)removeAnimationOperation:(CPTAnimationOperation *)animationOperation;
-(void)removeAllAnimationOperations;
/// @}

@end
