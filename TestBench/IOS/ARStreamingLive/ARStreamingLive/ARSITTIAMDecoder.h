//
//  ARSITTIAMDecoder.h
//  ARStreamingLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "ARSFrameBuffer.h"

#define MAX_RES_X (1920)
#define MAX_RES_Y (1080)
#define BPP          (2)

@interface ARSITTIAMDecoder : NSObject

- (void)initializeWithWidth:(int)w andHeight:(int)h;
- (void)close;

- (void)incrementNbDecoded;
- (int)getAndResetNbDecoded;

- (ARSFrame *)decodeFrame:(ARSFrame *)frame;

@end
