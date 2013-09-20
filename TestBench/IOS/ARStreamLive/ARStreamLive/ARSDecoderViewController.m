//
//  ARSDecoderViewController.m
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSDecoderViewController.h"
#import "ARSStreamReader.h"
#import "ARSITTIAMDecoder.h"

#if USE_OPENGL
#import "EAGLView.h"
#else
#import "ARSDisplayRGB.h"
#endif

@interface ARSDecoderViewController ()

@property (nonatomic, strong) ARSStreamReader *reader;
@property (nonatomic, strong) NSThread *decodeThread;
@property (nonatomic) dispatch_semaphore_t threadEnded;
@property (nonatomic, strong) ARSITTIAMDecoder *decoder;
@property (nonatomic, weak) ARSFrame *decodedFrame;

@property (nonatomic, strong) NSTimer *fpsTimer;

#if USE_OPENGL
@property (nonatomic) ARDroneOpenGLTexture texture;
@property (nonatomic, strong) EAGLView *glview;
#else
@property (nonatomic, strong) ARSDisplayRGB *display;
#endif

@end

@implementation ARSDecoderViewController

@synthesize ip;
@synthesize streamHeight;
@synthesize streamWidth;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        _reader = [[ARSStreamReader alloc] init];
        _threadEnded = dispatch_semaphore_create(0);
        _decoder = [[ARSITTIAMDecoder alloc] init];
    }
    return self;
}

- (void)viewDidLoad
{
    CGRect frame = [[UIScreen mainScreen] bounds];
    CGFloat tmp = frame.size.width;
    frame.size.width = frame.size.height;
    frame.size.height = tmp;
#if USE_OPENGL
    _glview = [[EAGLView alloc] initWithFrame:frame];
    ESRenderer *renderer = [[ESRenderer alloc] initWithFrame:frame withData:&_texture];
    [_glview setRenderer:renderer];
    [_glview changeState:YES];
    [_glview setScreenOrientationRight:NO];
    [self.view addSubview:_glview];
    [self.view sendSubviewToBack:_glview];
#else
    CGSize resolution = { streamWidth, streamHeight };
    _display = [[ARSDisplayRGB alloc] initWithFrame:frame andResolution:resolution];
    UIImageView *iw = [_display getView];
    [self.view addSubview:iw];
    [self.view sendSubviewToBack:iw];
#endif
    [super viewDidLoad];
}

- (BOOL)shouldAutorotate
{
    return NO;
}

- (void)fpsTimerTick
{
    static int prevNbNet = 0;
    static int prevNbDec = 0;
    static int prevNbDis = 0;
    
    int nbNet = [_reader getAndResetNbReceived];
    [_netFps setText:[NSString stringWithFormat:@"NetworkFPS : %d", nbNet + prevNbNet]];
    
    int nbDec = [_decoder getAndResetNbDecoded];
    [_decFps setText:[NSString stringWithFormat:@"DecodeFPS : %d", nbDec + prevNbDec]];
    
#if USE_OPENGL
    int nbDis = [_glview getAndResetNbDisplayed];
#else
    int nbDis = [_display getAndResetNbDisplayed];
#endif
    [_disFps setText:[NSString stringWithFormat:@"DisplayFPS : %d", nbDis + prevNbDis]];
    
    prevNbNet = nbNet;
    prevNbDec = nbDec;
    prevNbDis = nbDis;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [_reader start:ip];
    [_decoder initializeWithWidth:streamWidth andHeight:streamHeight];
    _decodeThread = [[NSThread alloc] initWithTarget:self selector:@selector(decodeLoop) object:nil];
    [_decodeThread start];
    _fpsTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(fpsTimerTick) userInfo:nil repeats:YES];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [_fpsTimer invalidate];
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
        ARSFrame *frame = [_reader.buffer getElement];
        if (frame)
        {
            _decodedFrame = [_decoder decodeFrame:frame];
            if (_decodedFrame)
            {
#if USE_OPENGL
                int newSize = [_decodedFrame used];
                _texture.image_size.height = streamHeight;
                _texture.image_size.width  = streamWidth;
                _texture.texture_size.height = streamHeight;
                _texture.texture_size.width  = streamWidth;
                if (newSize > _texture.dataSize)
                {
                    _texture.data = realloc(_texture.data, newSize);
                }
                _texture.dataSize = newSize;
                memcpy(_texture.data, [_decodedFrame data], newSize);
                _texture.num_picture_decoded++;
#else
                [_display performSelectorOnMainThread:@selector(displayFrame:) withObject:_decodedFrame waitUntilDone:NO];
#endif
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
