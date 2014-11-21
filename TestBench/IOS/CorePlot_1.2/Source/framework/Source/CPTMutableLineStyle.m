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
#import "CPTMutableLineStyle.h"

/** @brief Mutable wrapper for various line drawing properties.
 *
 *  If you need to customize properties of a line style, you should use this class rather than the
 *  immutable super class.
 *
 **/

@implementation CPTMutableLineStyle

/** @property CGLineCap lineCap
 *  @brief The style for the endpoints of lines drawn in a graphics context. Default is @ref kCGLineCapButt.
 **/
@dynamic lineCap;

/** @property CGLineJoin lineJoin
 *  @brief The style for the joins of connected lines in a graphics context. Default is @ref kCGLineJoinMiter.
 **/
@dynamic lineJoin;

/** @property CGFloat miterLimit
 *  @brief The miter limit for the joins of connected lines in a graphics context. Default is @num{10.0}.
 **/
@dynamic miterLimit;

/** @property CGFloat lineWidth
 *  @brief The line width for a graphics context. Default is @num{1.0}.
 **/
@dynamic lineWidth;

/** @property NSArray *dashPattern
 *  @brief The dash-and-space pattern for the line. Default is @nil.
 **/
@dynamic dashPattern;

/** @property CGFloat patternPhase
 *  @brief The starting phase of the line dash pattern. Default is @num{0.0}.
 **/
@dynamic patternPhase;

/** @property CPTColor *lineColor
 *  @brief The current stroke color in a context. Default is solid black.
 **/
@dynamic lineColor;

/** @property CPTFill *lineFill;
 *  @brief The current line fill. Default is @nil.
 *
 *  If @nil, the line is drawn using the
 *  @ref lineColor.
 **/
@dynamic lineFill;

@end
