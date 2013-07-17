//
//  ARVDecoderViewController.m
//  ARVideoLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARVDecoderViewController.h"
#import "ARVVideoReader.h"
#import "ARVITTIAMDecoder.h"

#import "EAGLView.h"

@interface ARVDecoderViewController ()

@property (nonatomic, strong) ARVVideoReader *reader;
@property (nonatomic, strong) NSThread *decodeThread;
@property (nonatomic) dispatch_semaphore_t threadEnded;
@property (nonatomic, strong) ARVITTIAMDecoder *decoder;
@property (nonatomic, weak) ARVFrame *decodedFrame;

@property (nonatomic) ARDroneOpenGLTexture texture;

@property (nonatomic, strong) EAGLView *glview;

@end

@implementation ARVDecoderViewController

@synthesize ip;
@synthesize videoHeight;
@synthesize videoWidth;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        _reader = [[ARVVideoReader alloc] init];
        _threadEnded = dispatch_semaphore_create(0);
        _decoder = [[ARVITTIAMDecoder alloc] init];
    }
    return self;
}

- (void)viewDidLoad
{
    CGRect frame = [[UIScreen mainScreen] bounds];
    CGFloat tmp = frame.size.width;
    frame.size.width = frame.size.height;
    frame.size.height = tmp;
    _glview = [[EAGLView alloc] initWithFrame:frame];
    ESRenderer *renderer = [[ESRenderer alloc] initWithFrame:frame withData:&_texture];
    [_glview setRenderer:renderer];
    [_glview changeState:YES];
    [_glview setScreenOrientationRight:NO];
    [self.view addSubview:_glview];
    [super viewDidLoad];
}

- (BOOL)shouldAutorotate
{
    return NO;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [_reader start:ip];
    [_decoder initializeWithWidth:videoWidth andHeight:videoHeight];
    _decodeThread = [[NSThread alloc] initWithTarget:self selector:@selector(decodeLoop) object:nil];
    [_decodeThread start];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [_decodeThread cancel];
    dispatch_semaphore_wait(_threadEnded, DISPATCH_TIME_FOREVER);
    [_decoder close];
    [_reader stop];
    [super viewWillDisappear:animated];
}

- (void)decodeLoop
{
    while (! [_decodeThread isCancelled])
    {
        ARVFrame *frame = [_reader.buffer getElement];
        if (frame)
        {
            _decodedFrame = [_decoder decodeFrame:frame];
            if (_decodedFrame)
            {
                int newSize = [_decodedFrame used];
                _texture.image_size.height = videoHeight;
                _texture.image_size.width  = videoWidth;
                _texture.texture_size.height = videoHeight;
                _texture.texture_size.width  = videoWidth;
                if (newSize > _texture.dataSize)
                {
                    _texture.data = realloc(_texture.data, newSize);
                }
                _texture.dataSize = newSize;
                memcpy(_texture.data, [_decodedFrame data], newSize);
                _texture.num_picture_decoded++;
            }
        }
    }
    dispatch_semaphore_signal(_threadEnded);
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
