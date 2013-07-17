//
//  ARVVideoReader.h
//  ARVideoLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARVFrameBuffer.h"

@interface ARVVideoReader : NSObject

@property (nonatomic, strong) ARVFrameBuffer *buffer;
@property (strong, nonatomic) ARVFrame *frame;

- (void)start:(NSString *)ip;
- (void)stop;

@end
