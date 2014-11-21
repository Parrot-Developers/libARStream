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
#import "CPTPlatformSpecificDefines.h"

/**
 *  @brief The basis of all event processing in Core Plot.
 **/
@protocol CPTResponder<NSObject>

/// @name User Interaction
/// @{

/**
 *  @brief @required Informs the receiver that the user has
 *  @if MacOnly pressed the mouse button. @endif
 *  @if iOSOnly touched the screen. @endif
 *  @param event The OS event.
 *  @param interactionPoint The coordinates of the interaction.
 *  @return Whether the event was handled or not.
 **/
-(BOOL)pointingDeviceDownEvent:(CPTNativeEvent *)event atPoint:(CGPoint)interactionPoint;

/**
 *  @brief @required Informs the receiver that the user has
 *  @if MacOnly released the mouse button. @endif
 *  @if iOSOnly lifted their finger off the screen. @endif
 *  @param event The OS event.
 *  @param interactionPoint The coordinates of the interaction.
 *  @return Whether the event was handled or not.
 **/
-(BOOL)pointingDeviceUpEvent:(CPTNativeEvent *)event atPoint:(CGPoint)interactionPoint;

/**
 *  @brief @required Informs the receiver that the user has moved
 *  @if MacOnly the mouse with the button pressed. @endif
 *  @if iOSOnly their finger while touching the screen. @endif
 *  @param event The OS event.
 *  @param interactionPoint The coordinates of the interaction.
 *  @return Whether the event was handled or not.
 **/
-(BOOL)pointingDeviceDraggedEvent:(CPTNativeEvent *)event atPoint:(CGPoint)interactionPoint;

/**
 *  @brief @required Informs the receiver that tracking of
 *  @if MacOnly mouse moves @endif
 *  @if iOSOnly touches @endif
 *  has been cancelled for any reason.
 *  @param event The OS event.
 *  @return Whether the event was handled or not.
 **/
-(BOOL)pointingDeviceCancelledEvent:(CPTNativeEvent *)event;
/// @}

@end
