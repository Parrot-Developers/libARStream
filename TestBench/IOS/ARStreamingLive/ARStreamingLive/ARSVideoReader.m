//
//  ARSStreamingReader.m
//  ARStreamingLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSStreamingReader.h"

#import <libARStreaming/ARSTREAMING_Reader.h>
#import <libARSAL/ARSAL_Thread.h>

#define CD_PORT     (43210)
#define DC_PORT     (54321)
#define NET_TO          (3)
#define DATA_BUF_ID   (125)
#define ACK_BUF_ID     (13)

uint8_t* streamingCallback (eARSTREAMING_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, int isFlushFrame, uint32_t *newBufferCapacity, void *custom)
{
    ARSStreamingReader *this = (__bridge ARSStreamingReader *)custom;
    static ARSFrame *oldFrame = nil;
    switch (cause)
    {
        case ARSTREAMING_READER_CAUSE_FRAME_COMPLETE:
            this.frame.used = frameSize;
            this.frame.isIFrame = (isFlushFrame == 1) ? YES : NO;
            this.frame.missed = numberOfSkippedFrames;
            [this incrementNbReceived];
            if (numberOfSkippedFrames > 0)
            {
                NSLog (@"Network skipped %d frame(s)", numberOfSkippedFrames);
            }
            if (![this.buffer addElement:this.frame])
            {
                NSLog (@"Impossible to add frame !");
            }
            break;
        case ARSTREAMING_READER_CAUSE_FRAME_TOO_SMALL:
            oldFrame = this.frame;
            this.frame = [[ARSFrame alloc] initWithCapacity:(2*oldFrame.capacity)];
            break;
        case ARSTREAMING_READER_CAUSE_COPY_COMPLETE:
            oldFrame = nil;
            break;
        case ARSTREAMING_READER_CAUSE_CANCEL:
            break;
        default:
            break;
    }
    *newBufferCapacity = this.frame.capacity;
    return this.frame.data;
}

@interface ARSStreamingReader () {
}
@property (nonatomic) ARNETWORKAL_Manager_t *alm;
@property (nonatomic) ARNETWORK_Manager_t *netManager;
@property (nonatomic) ARSTREAMING_Reader_t *streamingReader;

@property (nonatomic) ARSAL_Thread_t netSendThread;
@property (nonatomic) ARSAL_Thread_t netRecvThread;
@property (nonatomic) ARSAL_Thread_t vidDataThread;
@property (nonatomic) ARSAL_Thread_t vidAckThread;

@property (nonatomic) int nbReceived;

@property (nonatomic) BOOL isStarted;

@end

@implementation ARSStreamingReader

@synthesize buffer;
@synthesize frame;

- (id)init
{
    self = [super init];
    if (self)
    {
        _nbReceived = 0;
        frame = [[ARSFrame alloc] init];
    }
    return self;
}

- (void)start:(NSString *)ip
{
    @synchronized (self)
    {
        if (! _isStarted)
        {
            buffer = [[ARSFrameBuffer alloc] initWithCapacity:30];
            
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
            ARSTREAMING_Reader_InitStreamingDataBuffer(&outParam, DATA_BUF_ID);
            ARSTREAMING_Reader_InitStreamingAckBuffer(&inParam, ACK_BUF_ID);
            
            eARNETWORK_ERROR neterr = ARNETWORK_OK;
            _netManager = ARNETWORK_Manager_New(_alm, 1, &inParam, 1, &outParam, 0, &neterr);
            if (neterr != ARNETWORK_OK)
            {
                NSLog (@"Error while creating ARNetwork : %s", ARNETWORK_Error_ToString(neterr));
            }
            
            ARSAL_Thread_Create(&_netSendThread, ARNETWORK_Manager_SendingThreadRun, _netManager);
            ARSAL_Thread_Create(&_netRecvThread, ARNETWORK_Manager_ReceivingThreadRun, _netManager);
            
            eARSTREAMING_ERROR verr = ARSTREAMING_OK;
            _streamingReader = ARSTREAMING_Reader_New(_netManager, DATA_BUF_ID, ACK_BUF_ID, streamingCallback, frame.data, frame.capacity, (__bridge void *)(self), &verr);
            if (verr != ARSTREAMING_OK)
            {
                NSLog (@"Error while creating ARStreaming : %s", ARSTREAMING_Error_ToString(verr));
            }
            ARSAL_Thread_Create(&_vidDataThread, ARSTREAMING_Reader_RunDataThread, _streamingReader);
            ARSAL_Thread_Create(&_vidAckThread, ARSTREAMING_Reader_RunAckThread, _streamingReader);
            
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
            ARSTREAMING_Reader_StopReader(_streamingReader);
            
            ARSAL_Thread_Join(_vidDataThread, NULL);
            ARSAL_Thread_Join(_vidAckThread, NULL);
            
            ARSAL_Thread_Destroy(&_vidDataThread);
            ARSAL_Thread_Destroy(&_vidAckThread);
            
            ARSTREAMING_Reader_Delete(&_streamingReader);
            
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

- (void)incrementNbReceived
{
    @synchronized (self)
    {
        _nbReceived++;
    }
}

- (int)getAndResetNbReceived
{
    int res = 0;
    @synchronized (self)
    {
        res = _nbReceived;
        _nbReceived = 0;
    }
    return res;
}

@end
