//
//  ARSDecoderViewController.m
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSDecoderViewController.h"
#import "ARSStreamReader.h"
#import <libARCodecs/libARCodecs.h>

#import "GLView.h"

@interface ARSDecoderViewController ()

@property (nonatomic, strong) ARSStreamReader *reader;
@property (nonatomic, strong) NSThread *decodeThread;
@property (nonatomic) dispatch_semaphore_t threadEnded;
@property (nonatomic)ARCODECS_Manager_t *decoder;
@property (nonatomic, weak) ARSFrame *decodedFrame;

@property (nonatomic, strong) NSTimer *fpsTimer;

@property (nonatomic) AROpenGLTexture texture;
@property (nonatomic, strong) GLView *glview;

@property (nonatomic, strong) NSString *filePath;
@property (nonatomic, strong) NSMutableString *printString;
@end

@implementation ARSDecoderViewController

@synthesize ip;
@synthesize streamHeight;
@synthesize streamWidth;

static ARSFrame *_frame = NULL;
int ARDecoder_GetNextDataCallback(uint8_t **dataPtrAddr)
{
    int readBytes = 0;
    if(dataPtrAddr != NULL)
    {
        readBytes = _frame.used;
        *dataPtrAddr = _frame.data;
    }
    else
    {
        readBytes = 0;
    }
    
    return readBytes;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        _reader = [[ARSStreamReader alloc] init];
        _threadEnded = dispatch_semaphore_create(0);
        
        //CREATE LOG FILE
        NSDate *date = [[NSDate alloc] init];
        NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
        [dateFormatter setDateFormat:@"yyyy-MM-dd-hh:mm:ss"];
        NSString *dateString = [dateFormatter stringFromDate:date];
        
        _filePath = [[NSHomeDirectory()stringByAppendingPathComponent:@"Documents"] stringByAppendingPathComponent:[NSString stringWithFormat:@"Log_%@.txt",dateString]];
        
        _printString = [NSMutableString stringWithString:@"Timestamp;DecodeFPS;\n"];
        [_printString writeToFile:_filePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
    }
    return self;
}

- (void)viewDidLoad
{
    _reader.delegate = self;
    [super viewDidLoad];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self addDisplay];
}

- (void)addDisplay
{
    _glview = [[GLView alloc] initWithFrame:self.view.frame rendererClass:[GLRenderer_YUV class]];
    [self.view addSubview:_glview];
    [self.view sendSubviewToBack:_glview];
}

- (void)removeDisplay
{
    [_glview removeFromSuperview];
    _glview = nil;
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
    
    int nbDec = [self getAndResetNbDecoded];
    [_decFps setText:[NSString stringWithFormat:@"DecodeFPS : %d", nbDec + prevNbDec]];
    
    int nbDis = [self getAndResetNbDisplayed];
    [_disFps setText:[NSString stringWithFormat:@"DisplayFPS : %d", nbDis + prevNbDis]];
    
    prevNbNet = nbNet;
    prevNbDec = nbDec;
    prevNbDis = nbDis;
   
    long long timeStamp = (long long)([[NSDate date] timeIntervalSince1970] * 1000.0);
    
    [_printString appendString:[NSString stringWithFormat:@"%lld;%d;\n",timeStamp, nbDec + prevNbDec]];
    [_printString  writeToFile:_filePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
}

- (int)getAndResetNbDecoded
{
    int res = _texture.num_picture_decoded;
    _texture.num_picture_decoded = 0;
    return res;
}

- (int)getAndResetNbDisplayed
{
    int res = _texture.nbDisplayed;
    _texture.nbDisplayed = 0;
    return res;
}

- (void)createDecoder
{
    eARCODECS_ERROR error = ARCODECS_OK;
    _decoder = ARCODECS_Manager_New(ARCODECS_TYPE_VIDEO_H264_PARROT,ARDecoder_GetNextDataCallback, &error);
    if(_decoder == nil)
    {
        NSLog(@"%s", ARCODECS_Error_ToString(error));
    }
}

- (void)closeDecoder
{
    ARCODECS_Manager_Delete(&(_decoder));
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [_reader start:ip];
    [self createDecoder];
    _decodeThread = [[NSThread alloc] initWithTarget:self selector:@selector(decodeLoop) object:nil];
    [_decodeThread start];
    _fpsTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(fpsTimerTick) userInfo:nil repeats:YES];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [_fpsTimer invalidate];
    [_decodeThread cancel];
    dispatch_semaphore_wait(_threadEnded, DISPATCH_TIME_FOREVER);
    ARCODECS_Manager_Delete(&(_decoder));
    [_reader stop];
    [self removeDisplay];
    [super viewWillDisappear:animated];
}

- (void)decodeLoop
{
    while (! [_decodeThread isCancelled])
    {
        ARSFrame *frame = [_reader.buffer getElement];
        if (frame)
        {
            _frame = frame;
            eARCODECS_ERROR error = ARCODECS_OK;
            ARCODECS_Manager_Frame_t *arcodecsOutputFrame = ARCODECS_Manager_Decode(_decoder,&error);
            if (arcodecsOutputFrame != NULL)
            {
                _texture.num_picture_decoded++;
                _texture.image_size.height = streamHeight;
                _texture.image_size.width  = streamWidth;
                _texture.texture_size.height = streamHeight;
                _texture.texture_size.width  = streamWidth;
                _texture.dataSize = arcodecsOutputFrame->size;
                _texture.data = arcodecsOutputFrame->data;

                dispatch_sync(dispatch_get_main_queue(), ^{
                    [_glview render:&_texture];
                });
            }
            else
            {
                NSLog(@"Decoding error %s", ARCODECS_Error_ToString(error));
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

- (void)newResolutionWidth:(int)w Height:(int)h
{
    streamHeight = h;
    streamWidth = w;
    [self closeDecoder];
    [self createDecoder];
}

@end
