//
//  ARVDisplayRGB.h
//  ARVideoLive
//
//  Created by Nicolas BRULEZ on 17/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ARVFrameBuffer.h"

@interface ARVDisplayRGB : NSObject

- (id)initWithFrame:(CGRect)frame andResolution:(CGSize)res;

- (UIImageView *)getView;

- (void)displayFrame:(ARVFrame *)frame;

- (void)incrementNbDisplayed;
- (int)getAndResetNbDisplayed;

@end
