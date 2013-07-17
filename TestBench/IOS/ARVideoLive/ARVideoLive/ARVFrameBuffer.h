//
//  ARVFrameBuffer.h
//  ARVideoLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ARVFrame : NSObject

@property (nonatomic) uint8_t *data;
@property (nonatomic) uint32_t capacity;
@property (nonatomic) uint32_t used;
@property (nonatomic) uint32_t missed;
@property (nonatomic) BOOL isIFrame;

- (id)initWithCapacity:(uint32_t)defaultCapacity;

- (uint32_t)doubleCapacity;

@end

@interface ARVFrameBuffer : NSObject

- (id)initWithCapacity:(int)capacity;

- (BOOL)addElement:(ARVFrame *)element;

- (ARVFrame *)getElement;

@end
