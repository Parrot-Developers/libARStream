//
//  ARSFrameBuffer.m
//  ARStreamingLive
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
