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
//
//  ARSFrameBuffer.m
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSFrameBuffer.h"

#define DEFAULT_CAPACITY (4096)

@implementation ARSFrame

@synthesize data;
@synthesize capacity;
@synthesize used;
@synthesize missed;
@synthesize isIFrame;

- (id)init
{
    return [self initWithCapacity:DEFAULT_CAPACITY];
}

- (id)initWithCapacity:(uint32_t)defaultCapacity
{
    self = [super init];
    if (self)
    {
        data = malloc (defaultCapacity);
        capacity = defaultCapacity;
        used = 0;
        missed = 0;
        isIFrame = NO;
    }
    return self;
}

- (uint32_t)doubleCapacity
{
    capacity *= 2;
    data = realloc(data, capacity);
    return capacity;
}

@end

@interface ARSFrameBuffer ()

@property (nonatomic, strong) NSMutableArray *frames;
@property (nonatomic) int readIndex;
@property (nonatomic) int writeIndex;
@property (nonatomic) int capacity;
@property (nonatomic) dispatch_semaphore_t sem;

@end

@implementation ARSFrameBuffer

- (id)initWithCapacity:(int)capacity
{
    self = [super init];
    if (self)
    {
        _frames = [[NSMutableArray alloc] initWithCapacity:capacity];
        for (int i = 0; i < capacity; i++)
        {
            [_frames addObject:[[ARSFrame alloc] init]];
        }
        _capacity = capacity;
        _readIndex = 0;
        _writeIndex = 0;
        _sem = dispatch_semaphore_create(0);
    }
    return self;
}

- (BOOL)addElement:(ARSFrame *)element
{
    if (element.isIFrame)
    {
        int nbElems = _writeIndex - _readIndex;
        if (nbElems > 0)
        {
            NSLog (@"Decoder flushed %d frame(s)", nbElems);
        }
        // Reset sem to zero
        while (dispatch_semaphore_wait(_sem, DISPATCH_TIME_NOW) == 0);
        _readIndex = _writeIndex;
    }
    int nbElems = _writeIndex - _readIndex;
    if (nbElems >= _capacity - 1)
    {
        return NO;
    }
    ARSFrame *localFrame = [_frames objectAtIndex:(_writeIndex % _capacity)];
    while (localFrame.capacity < element.used) {
        [localFrame doubleCapacity];
    }
    memcpy(localFrame.data, element.data, element.used);
    localFrame.used = element.used;
    localFrame.isIFrame = element.isIFrame;
    localFrame.missed = element.missed;
    _writeIndex ++;
    dispatch_semaphore_signal(_sem);
    return YES;
}

- (ARSFrame *)getElement
{
    ARSFrame *retFrame = nil;
    if (0 == dispatch_semaphore_wait(_sem, dispatch_time(DISPATCH_TIME_NOW, 100000)))
    {
        retFrame = [_frames objectAtIndex:(_readIndex % _capacity)];
        _readIndex ++;
    }
    return retFrame;
}

@end
