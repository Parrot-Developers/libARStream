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
/// @file

typedef CGFloat (*CPTAnimationTimingFunction)(CGFloat, CGFloat);

#if __cplusplus
extern "C" {
#endif

/// @name Linear
/// @{
CGFloat CPTAnimationTimingFunctionLinear(CGFloat time, CGFloat duration);

/// @}

/// @name Backing
/// @{
CGFloat CPTAnimationTimingFunctionBackIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionBackOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionBackInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Bounce
/// @{
CGFloat CPTAnimationTimingFunctionBounceIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionBounceOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionBounceInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Circular
/// @{
CGFloat CPTAnimationTimingFunctionCircularIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionCircularOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionCircularInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Elastic
/// @{
CGFloat CPTAnimationTimingFunctionElasticIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionElasticOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionElasticInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Exponential
/// @{
CGFloat CPTAnimationTimingFunctionExponentialIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionExponentialOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionExponentialInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Sinusoidal
/// @{
CGFloat CPTAnimationTimingFunctionSinusoidalIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionSinusoidalOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionSinusoidalInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Cubic
/// @{
CGFloat CPTAnimationTimingFunctionCubicIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionCubicOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionCubicInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Quadratic
/// @{
CGFloat CPTAnimationTimingFunctionQuadraticIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionQuadraticOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionQuadraticInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Quartic
/// @{
CGFloat CPTAnimationTimingFunctionQuarticIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionQuarticOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionQuarticInOut(CGFloat time, CGFloat duration);

/// @}

/// @name Quintic
/// @{
CGFloat CPTAnimationTimingFunctionQuinticIn(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionQuinticOut(CGFloat time, CGFloat duration);
CGFloat CPTAnimationTimingFunctionQuinticInOut(CGFloat time, CGFloat duration);

/// @}

#if __cplusplus
}
#endif
