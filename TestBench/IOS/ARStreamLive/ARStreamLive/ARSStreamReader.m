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
//  ARSStreamReader.m
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSStreamReader.h"

#import <libARCommands/ARCOMMANDS_Decoder.h>
#import <libARStream/ARSTREAM_Reader.h>
#import <libARSAL/ARSAL_Thread.h>

#define CD_PORT     (43210)
#define DC_PORT     (54321)
#define NET_TO          (3)
#define EVENT_ID      (126)
#define DATA_BUF_ID   (125)
#define ACK_BUF_ID     (13)

uint8_t* streamCallback (eARSTREAM_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, int isFlushFrame, uint32_t *newBufferCapacity, void *custom)
{
    ARSStreamReader *this = (__bridge ARSStreamReader *)custom;
    static ARSFrame *oldFrame = nil;
    switch (cause)
    {
        case ARSTREAM_READER_CAUSE_FRAME_COMPLETE:
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
        case ARSTREAM_READER_CAUSE_FRAME_TOO_SMALL:
            oldFrame = this.frame;
            this.frame = [[ARSFrame alloc] initWithCapacity:(2*oldFrame.capacity)];
            break;
        case ARSTREAM_READER_CAUSE_COPY_COMPLETE:
            oldFrame = nil;
            break;
        case ARSTREAM_READER_CAUSE_CANCEL:
            break;
        default:
            break;
    }
    *newBufferCapacity = this.frame.capacity;
    return this.frame.data;
}

void resolutionUpdate(uint16_t width, uint16_t height, void *custom)
{
    ARSStreamReader *this = (__bridge ARSStreamReader *)custom;
    [this updateResolutionWithWidth:(int)width andHeight:(int)height];
}

@interface ARSStreamReader ()
@property (nonatomic) ARNETWORKAL_Manager_t *alm;
@property (nonatomic) ARNETWORK_Manager_t *netManager;
@property (nonatomic) ARSTREAM_Reader_t *streamReader;

@property (nonatomic) ARSAL_Thread_t netSendThread;
@property (nonatomic) ARSAL_Thread_t netRecvThread;
@property (nonatomic) ARSAL_Thread_t vidDataThread;
@property (nonatomic) ARSAL_Thread_t vidAckThread;

@property (nonatomic, strong) dispatch_group_t myGroup;

@property (nonatomic) int nbReceived;

@property (nonatomic) BOOL isStarted;
@property (nonatomic) BOOL dispatchMustRun;

@end

@implementation ARSStreamReader

@synthesize buffer;
@synthesize frame;
@synthesize delegate;

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
            _myGroup = dispatch_group_create();
            
            eARNETWORKAL_ERROR alerr = ARNETWORKAL_OK;
            _alm = ARNETWORKAL_Manager_New(&alerr);
            if (alerr != ARNETWORKAL_OK)
            {
                NSLog (@"Error while creating ARNetworkAL : %s", ARNETWORKAL_Error_ToString(alerr));
            }
            alerr = ARNETWORKAL_Manager_InitWifiNetwork(_alm, [ip UTF8String], CD_PORT, DC_PORT, NET_TO);
            if (alerr != ARNETWORKAL_OK)
            {
                NSLog (@"Error while Initializing wifi network : %s", ARNETWORKAL_Error_ToString(alerr));
            }
            
            ARNETWORK_IOBufferParam_t inParam;
            ARNETWORK_IOBufferParam_t outParam [2];
            ARSTREAM_Reader_InitStreamDataBuffer(&outParam[0], DATA_BUF_ID);
            outParam[1].ID = EVENT_ID;
            outParam[1].dataType = ARNETWORKAL_FRAME_TYPE_DATA_WITH_ACK;
            outParam[1].numberOfCell = 20;
            outParam[1].dataCopyMaxSize = 128;
            outParam[1].isOverwriting = 0;
            ARSTREAM_Reader_InitStreamAckBuffer(&inParam, ACK_BUF_ID);
            
            eARNETWORK_ERROR neterr = ARNETWORK_OK;
            _netManager = ARNETWORK_Manager_New(_alm, 1, &inParam, 2, outParam, 0, &neterr);
            if (neterr != ARNETWORK_OK)
            {
                NSLog (@"Error while creating ARNetwork : %s", ARNETWORK_Error_ToString(neterr));
            }
            
            ARSAL_Thread_Create(&_netSendThread, ARNETWORK_Manager_SendingThreadRun, _netManager);
            ARSAL_Thread_Create(&_netRecvThread, ARNETWORK_Manager_ReceivingThreadRun, _netManager);
            
            eARSTREAM_ERROR verr = ARSTREAM_OK;
            _streamReader = ARSTREAM_Reader_New(_netManager, DATA_BUF_ID, ACK_BUF_ID, streamCallback, frame.data, frame.capacity, (__bridge void *)(self), &verr);
            if (verr != ARSTREAM_OK)
            {
                NSLog (@"Error while creating ARStream : %s", ARSTREAM_Error_ToString(verr));
            }
            ARSAL_Thread_Create(&_vidDataThread, ARSTREAM_Reader_RunDataThread, _streamReader);
            ARSAL_Thread_Create(&_vidAckThread, ARSTREAM_Reader_RunAckThread, _streamReader);

            _dispatchMustRun = YES;
            dispatch_group_async(_myGroup, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [self ackDataReaderLoop];
            });
            
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

            _dispatchMustRun = NO;
            ARNETWORKAL_Manager_SignalWifiNetwork(_alm);
            dispatch_group_wait(_myGroup, DISPATCH_TIME_FOREVER);
            _myGroup = nil;

            ARSTREAM_Reader_StopReader(_streamReader);
            
            ARSAL_Thread_Join(_vidDataThread, NULL);
            ARSAL_Thread_Join(_vidAckThread, NULL);
            
            ARSAL_Thread_Destroy(&_vidDataThread);
            ARSAL_Thread_Destroy(&_vidAckThread);
            
            ARSTREAM_Reader_Delete(&_streamReader);
            
            ARNETWORK_Manager_Stop(_netManager);
            ARNETWORKAL_Manager_SignalWifiNetwork(_alm);
            
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

- (void)ackDataReaderLoop
{
    ARCOMMANDS_Decoder_SetCommonDebugVideoSetResolutionCallback(resolutionUpdate, (__bridge void *)self);
    eARNETWORK_ERROR net_err = ARNETWORK_OK;
    uint8_t *data = malloc (256);
    int dataSize = 0;
    if (NULL == data)
    {
        return;
    }
    while (_dispatchMustRun)
    {

        net_err = ARNETWORK_Manager_ReadDataWithTimeout(_netManager, EVENT_ID, data, 256, &dataSize, 3);
        if (net_err == ARNETWORK_OK)
        {
            ARCOMMANDS_Decoder_DecodeBuffer(data, dataSize);
        }
    }
    free (data);
    ARCOMMANDS_Decoder_SetCommonDebugVideoSetResolutionCallback(NULL, NULL);
}

- (void)updateResolutionWithWidth:(int)w andHeight:(int)h
{
    [delegate newResolutionWidth:w Height:h];
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
