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
/** @category NSCoder(CPTExtensions)
 *  @brief Core Plot extensions to NSCoder.
 **/
@interface NSCoder(CPTExtensions)

/// @name Encoding Data
/// @{
-(void)encodeCGFloat:(CGFloat)number forKey:(NSString *)key;
-(void)encodeCPTPoint:(CGPoint)point forKey:(NSString *)key;
-(void)encodeCPTSize:(CGSize)size forKey:(NSString *)key;
-(void)encodeCPTRect:(CGRect)rect forKey:(NSString *)key;

-(void)encodeCGColorSpace:(CGColorSpaceRef)colorSpace forKey:(NSString *)key;
-(void)encodeCGPath:(CGPathRef)path forKey:(NSString *)key;
-(void)encodeCGImage:(CGImageRef)image forKey:(NSString *)key;

-(void)encodeDecimal:(NSDecimal)number forKey:(NSString *)key;
/// @}

/// @name Decoding Data
/// @{
-(CGFloat)decodeCGFloatForKey:(NSString *)key;
-(CGPoint)decodeCPTPointForKey:(NSString *)key;
-(CGSize)decodeCPTSizeForKey:(NSString *)key;
-(CGRect)decodeCPTRectForKey:(NSString *)key;

-(CGColorSpaceRef)newCGColorSpaceDecodeForKey:(NSString *)key;
-(CGPathRef)newCGPathDecodeForKey:(NSString *)key;
-(CGImageRef)newCGImageDecodeForKey:(NSString *)key;

-(NSDecimal)decodeDecimalForKey:(NSString *)key;
/// @}

@end
