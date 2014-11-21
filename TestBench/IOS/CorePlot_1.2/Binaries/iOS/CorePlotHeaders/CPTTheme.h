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
/// @ingroup themeNames
/// @{
extern NSString *const kCPTDarkGradientTheme; ///< A graph theme with dark gray gradient backgrounds and light gray lines.
extern NSString *const kCPTPlainBlackTheme;   ///< A graph theme with black backgrounds and white lines.
extern NSString *const kCPTPlainWhiteTheme;   ///< A graph theme with white backgrounds and black lines.
extern NSString *const kCPTSlateTheme;        ///< A graph theme with colors that match the default iPhone navigation bar, toolbar buttons, and table views.
extern NSString *const kCPTStocksTheme;       ///< A graph theme with a gradient background and white lines.
/// @}

@class CPTGraph;
@class CPTPlotAreaFrame;
@class CPTAxisSet;
@class CPTMutableTextStyle;

@interface CPTTheme : NSObject<NSCoding> {
    @private
    Class graphClass;
}

@property (nonatomic, readwrite, retain) Class graphClass;

/// @name Theme Management
/// @{
+(void)registerTheme:(Class)themeClass;
+(NSArray *)themeClasses;
+(CPTTheme *)themeNamed:(NSString *)theme;
+(NSString *)name;
/// @}

/// @name Theme Usage
/// @{
-(void)applyThemeToGraph:(CPTGraph *)graph;
/// @}

@end

/** @category CPTTheme(AbstractMethods)
 *  @brief CPTTheme abstract methods—must be overridden by subclasses
 **/
@interface CPTTheme(AbstractMethods)

/// @name Theme Usage
/// @{
-(id)newGraph;

-(void)applyThemeToBackground:(CPTGraph *)graph;
-(void)applyThemeToPlotArea:(CPTPlotAreaFrame *)plotAreaFrame;
-(void)applyThemeToAxisSet:(CPTAxisSet *)axisSet;
/// @}

@end
