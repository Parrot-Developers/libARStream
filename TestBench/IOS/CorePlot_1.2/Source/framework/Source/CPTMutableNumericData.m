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
#import "CPTMutableNumericData.h"

/** @brief An annotated NSMutableData type.
 *
 *  CPTNumericData combines a mutable data buffer with information
 *  about the data (shape, data type, size, etc.).
 *  The data is assumed to be an array of one or more dimensions
 *  of a single type of numeric data. Each numeric value in the array,
 *  which can be more than one byte in size, is referred to as a @quote{sample}.
 *  The structure of this object is similar to the NumPy <code>ndarray</code>
 *  object.
 **/
@implementation CPTMutableNumericData

/** @property void *mutableBytes
 *  @brief Returns a pointer to the data buffer’s contents.
 **/
@dynamic mutableBytes;

/** @property NSArray *shape
 *  @brief The shape of the data buffer array. Set a new shape to change the size of the data buffer.
 *
 *  The shape describes the dimensions of the sample array stored in
 *  the data buffer. Each entry in the shape array represents the
 *  size of the corresponding array dimension and should be an unsigned
 *  integer encoded in an instance of NSNumber.
 **/
@dynamic shape;

#pragma mark -
#pragma mark Accessors

/// @cond

-(void *)mutableBytes
{
    return [(NSMutableData *)self.data mutableBytes];
}

-(void)setShape:(NSArray *)newShape
{
    if ( newShape != shape ) {
        [shape release];
        shape = [newShape copy];

        NSUInteger sampleCount = 1;
        for ( NSNumber *num in shape ) {
            sampleCount *= [num unsignedIntegerValue];
        }

        ( (NSMutableData *)self.data ).length = sampleCount * self.sampleBytes;
    }
}

/// @endcond

@end
