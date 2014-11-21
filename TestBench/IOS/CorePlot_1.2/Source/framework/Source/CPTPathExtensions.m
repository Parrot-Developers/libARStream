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
#import "CPTPathExtensions.h"

/** @brief Creates a rectangular path with rounded corners.
 *
 *  @param rect The bounding rectangle for the path.
 *  @param cornerRadius The radius of the rounded corners.
 *  @return The new path. Caller is responsible for releasing this.
 **/
CGPathRef CreateRoundedRectPath(CGRect rect, CGFloat cornerRadius)
{
    // In order to draw a rounded rectangle, we will take advantage of the fact that
    // CGPathAddArcToPoint will draw straight lines past the start and end of the arc
    // in order to create the path from the current position and the destination position.

    CGFloat minX = CGRectGetMinX(rect), midX = CGRectGetMidX(rect), maxX = CGRectGetMaxX(rect);
    CGFloat minY = CGRectGetMinY(rect), midY = CGRectGetMidY(rect), maxY = CGRectGetMaxY(rect);

    CGMutablePathRef path = CGPathCreateMutable();

    CGPathMoveToPoint(path, NULL, minX, midY);
    CGPathAddArcToPoint(path, NULL, minX, minY, midX, minY, cornerRadius);
    CGPathAddArcToPoint(path, NULL, maxX, minY, maxX, midY, cornerRadius);
    CGPathAddArcToPoint(path, NULL, maxX, maxY, midX, maxY, cornerRadius);
    CGPathAddArcToPoint(path, NULL, minX, maxY, minX, midY, cornerRadius);
    CGPathCloseSubpath(path);

    return path;
}

/** @brief Adds a rectangular path with rounded corners to a graphics context.
 *
 *  @param context The graphics context.
 *  @param rect The bounding rectangle for the path.
 *  @param cornerRadius The radius of the rounded corners.
 **/
void AddRoundedRectPath(CGContextRef context, CGRect rect, CGFloat cornerRadius)
{
    CGPathRef path = CreateRoundedRectPath(rect, cornerRadius);

    CGContextAddPath(context, path);
    CGPathRelease(path);
}
