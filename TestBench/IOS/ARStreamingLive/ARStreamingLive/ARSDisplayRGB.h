//
//  ARSDisplayRGB.h
//  ARStreamingLive
//
//  Created by Nicolas BRULEZ on 17/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ARSFrameBuffer.h"

@interface ARSDisplayRGB : NSObject

- (id)initWithFrame:(CGRect)frame andResolution:(CGSize)res;

- (UIImageView *)getView;

- (void)displayFrame:(ARSFrame *)frame;

- (void)incrementNbDisplayed;
- (int)getAndResetNbDisplayed;

@end
