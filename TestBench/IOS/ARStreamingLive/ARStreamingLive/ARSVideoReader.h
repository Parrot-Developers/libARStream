//
//  ARSStreamingReader.h
//  ARStreamingLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSFrameBuffer.h"

@interface ARSStreamingReader : NSObject

@property (nonatomic, strong) ARSFrameBuffer *buffer;
@property (strong, nonatomic) ARSFrame *frame;

- (void)start:(NSString *)ip;
- (void)stop;

- (void)incrementNbReceived;
- (int)getAndResetNbReceived;

@end
