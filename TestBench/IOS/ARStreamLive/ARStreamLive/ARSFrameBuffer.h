//
//  ARSFrameBuffer.h
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ARSFrame : NSObject

@property (nonatomic) uint8_t *data;
@property (nonatomic) uint32_t capacity;
@property (nonatomic) uint32_t used;
@property (nonatomic) uint32_t missed;
@property (nonatomic) BOOL isIFrame;

- (id)initWithCapacity:(uint32_t)defaultCapacity;

- (uint32_t)doubleCapacity;

@end

@interface ARSFrameBuffer : NSObject

- (id)initWithCapacity:(int)capacity;

- (BOOL)addElement:(ARSFrame *)element;

- (ARSFrame *)getElement;

@end
