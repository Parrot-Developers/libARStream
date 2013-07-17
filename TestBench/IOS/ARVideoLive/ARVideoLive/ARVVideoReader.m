//
//  ARVVideoReader.m
//  ARVideoLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARVVideoReader.h"

#import <libARVideo/ARVIDEO_Reader.h>
#import <libARSAL/ARSAL_Thread.h>

#define CD_PORT     (43210)
#define DC_PORT     (54321)
#define NET_TO          (3)
#define DATA_BUF_ID   (125)
#define ACK_BUF_ID     (13)

uint8_t* videoCallback (eARVIDEO_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, int isFlushFrame, uint32_t *newBufferCapacity, void *custom)
{
    ARVVideoReader *this = (__bridge ARVVideoReader *)custom;
    static ARVFrame *oldFrame = nil;
    switch (cause)
    {
        case ARVIDEO_READER_CAUSE_FRAME_COMPLETE:
            this.frame.used = frameSize;
            this.frame.isIFrame = (isFlushFrame == 1) ? YES : NO;
            this.frame.missed = numberOfSkippedFrames;
            if (numberOfSkippedFrames > 0)
            {
                NSLog (@"Network skipped %d frame(s)", numberOfSkippedFrames);
            }
            if (![this.buffer addElement:this.frame])
            {
                NSLog (@"Impossible to add frame !");
            }
            break;
        case ARVIDEO_READER_CAUSE_FRAME_TOO_SMALL:
            oldFrame = this.frame;
            this.frame = [[ARVFrame alloc] initWithCapacity:(2*oldFrame.capacity)];
            break;
        case ARVIDEO_READER_CAUSE_COPY_COMPLETE:
            oldFrame = nil;
            break;
        case ARVIDEO_READER_CAUSE_CANCEL:
            break;
        default:
            break;
    }
    *newBufferCapacity = this.frame.capacity;
    return this.frame.data;
}

@interface ARVVideoReader () {
}
@property (nonatomic) ARNETWORKAL_Manager_t *alm;
@property (nonatomic) ARNETWORK_Manager_t *netManager;
@property (nonatomic) ARVIDEO_Reader_t *videoReader;

@property (nonatomic) ARSAL_Thread_t netSendThread;
@property (nonatomic) ARSAL_Thread_t netRecvThread;
@property (nonatomic) ARSAL_Thread_t vidDataThread;
@property (nonatomic) ARSAL_Thread_t vidAckThread;

@property (nonatomic) BOOL isStarted;

@end

@implementation ARVVideoReader

@synthesize buffer;
@synthesize frame;

- (id)init
{
    self = [super init];
    if (self)
    {
        frame = [[ARVFrame alloc] init];
    }
    return self;
}

- (void)start:(NSString *)ip
{
    @synchronized (self)
    {
        if (! _isStarted)
        {
            buffer = [[ARVFrameBuffer alloc] initWithCapacity:30];
            
            eARNETWORKAL_ERROR alerr = ARNETWORKAL_OK;
            _alm = ARNETWORKAL_Manager_New(&alerr);
            if (alerr != ARNETWORKAL_OK)
            {
                NSLog (@"Error while creating ARNetworkAL : %s", ARNETWORKAL_Error_ToString(alerr));
            }
            alerr = ARNETWORKAL_Manager_InitWiFiNetwork(_alm, [ip UTF8String], CD_PORT, DC_PORT, NET_TO);
            if (alerr != ARNETWORKAL_OK)
            {
                NSLog (@"Error while Initializing wifi network : %s", ARNETWORKAL_Error_ToString(alerr));
            }
            
            ARNETWORK_IOBufferParam_t inParam;
            ARNETWORK_IOBufferParam_t outParam;
            ARVIDEO_Reader_InitVideoDataBuffer(&outParam, DATA_BUF_ID);
            ARVIDEO_Reader_InitVideoAckBuffer(&inParam, ACK_BUF_ID);
            
            eARNETWORK_ERROR neterr = ARNETWORK_OK;
            _netManager = ARNETWORK_Manager_New(_alm, 1, &inParam, 1, &outParam, 0, &neterr);
            if (neterr != ARNETWORK_OK)
            {
                NSLog (@"Error while creating ARNetwork : %s", ARNETWORK_Error_ToString(neterr));
            }
            
            ARSAL_Thread_Create(&_netSendThread, ARNETWORK_Manager_SendingThreadRun, _netManager);
            ARSAL_Thread_Create(&_netRecvThread, ARNETWORK_Manager_ReceivingThreadRun, _netManager);
            
            eARVIDEO_ERROR verr = ARVIDEO_OK;
            _videoReader = ARVIDEO_Reader_New(_netManager, DATA_BUF_ID, ACK_BUF_ID, videoCallback, frame.data, frame.capacity, (__bridge void *)(self), &verr);
            if (verr != ARVIDEO_OK)
            {
                NSLog (@"Error while creating ARVideo : %s", ARVIDEO_Error_ToString(verr));
            }
            ARSAL_Thread_Create(&_vidDataThread, ARVIDEO_Reader_RunDataThread, _videoReader);
            ARSAL_Thread_Create(&_vidAckThread, ARVIDEO_Reader_RunAckThread, _videoReader);
            
            _isStarted = YES;
            
            NSLog (@"All threads started");
        }
    }
}

- (void)stop
{
    @synchronized (self)
    {
        if (_isStarted)
        {
            buffer = nil;
            ARVIDEO_Reader_StopReader(_videoReader);
            
            ARSAL_Thread_Join(_vidDataThread, NULL);
            ARSAL_Thread_Join(_vidAckThread, NULL);
            
            ARSAL_Thread_Destroy(&_vidDataThread);
            ARSAL_Thread_Destroy(&_vidAckThread);
            
            ARVIDEO_Reader_Delete(&_videoReader);
            
            ARNETWORK_Manager_Stop(_netManager);
            
            ARSAL_Thread_Join(_netRecvThread, NULL);
            ARSAL_Thread_Join(_netSendThread, NULL);
            
            ARSAL_Thread_Destroy(&_netSendThread);
            ARSAL_Thread_Destroy(&_netRecvThread);
            
            ARNETWORK_Manager_Delete(&_netManager);
            
            ARNETWORKAL_Manager_Delete(&_alm);
            
            _isStarted = NO;
        }
    }
}

- (void)dealloc
{
    [self stop];
}

@end
