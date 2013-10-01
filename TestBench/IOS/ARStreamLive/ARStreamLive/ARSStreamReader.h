//
//  ARSStreamReader.h
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSFrameBuffer.h"

@protocol ARSStreamReaderDelegate <NSObject>

- (void)newResolutionWidth:(int)w Height:(int)h;

@end

@interface ARSStreamReader : NSObject

@property (nonatomic, strong) ARSFrameBuffer *buffer;
@property (strong, nonatomic) ARSFrame *frame;
@property (strong, nonatomic) id<ARSStreamReaderDelegate> delegate;

- (void)start:(NSString *)ip;
- (void)stop;

- (void)incrementNbReceived;
- (int)getAndResetNbReceived;

- (void)updateResolutionWithWidth:(int)w andHeight:(int)h;

@end
