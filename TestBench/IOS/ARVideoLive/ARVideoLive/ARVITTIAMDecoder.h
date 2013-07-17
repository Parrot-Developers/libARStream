//
//  ARVITTIAMDecoder.h
//  ARVideoLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "ARVFrameBuffer.h"

#define MAX_RES_X (1920)
#define MAX_RES_Y (1080)
#define BPP          (2)

@interface ARVITTIAMDecoder : NSObject

- (void)initializeWithWidth:(int)w andHeight:(int)h;
- (void)close;

- (ARVFrame *)decodeFrame:(ARVFrame *)frame;

@end
