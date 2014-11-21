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
//  ARSLogger.m
//  ARStreamTB
//
//  Created by Nicolas BRULEZ on 09/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSLogger.h"

#define ARSTREAM_DEFAULT_FILENAME @"ARSLog"
#define ARSTREAM_DATE_FORMAT @"_yyyyMMdd_HHmmss"

@interface ARSLogger()

@property (nonatomic, strong) NSFileHandle *file;

@end

@implementation ARSLogger

- (id)init
{
    self = [super init];
    if (self)
    {
        NSString *filename = [NSString stringWithFormat:@"%@/%@%@.log", [self applicationDocumentsDirectory], ARSTREAM_DEFAULT_FILENAME, [self nowTime:YES]];
        NSLog (@"Opening file %@", filename);
        NSFileManager *manager = [NSFileManager defaultManager];
        [manager createFileAtPath:filename contents:nil attributes:nil];
        _file = [NSFileHandle fileHandleForWritingAtPath:filename];
        if (_file == nil)
        {
            exit (0);
        }
    }
    return self;
}

- (id)initWithFileName:(NSString *)fileName withDate:(BOOL)useDate
{
    self = [super init];
    if (self)
    {
        NSString *filename = [NSString stringWithFormat:@"%@/%@%@.log", [self applicationDocumentsDirectory], fileName, [self nowTime:useDate]];
        NSFileManager *manager = [NSFileManager defaultManager];
        [manager createFileAtPath:filename contents:nil attributes:nil];
        _file = [NSFileHandle fileHandleForWritingAtPath:filename];
    }
    return self;
}

- (id)initWithoutFile
{
    self = [super init];
    if (self)
    {
        _file = nil;
    }
    return self;
}

- (void)log:(NSString *)message
{
    NSLog (@"Adding message : %@", message);
    [_file writeData:[message dataUsingEncoding:NSUTF8StringEncoding]];
    [_file writeData:[@"\n" dataUsingEncoding:NSUTF8StringEncoding]];
}

- (void)close
{
    [_file closeFile];
    _file = nil;
}

- (void)dealloc
{
    [self close];
}

- (void)deleteAllLogs
{
    [self close];
    UIAlertView *av = [[UIAlertView alloc] initWithTitle:@"Delete all logs" message:@"Are you sure ?" delegate:self cancelButtonTitle:@"YES" otherButtonTitles:@"NO", nil];
    [av show];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    switch (buttonIndex)
    {
        case 0:
            [self confirmDeleteAllLogs];
            break;
        default:
            break;
    }
}

- (void)confirmDeleteAllLogs
{
    NSFileManager *manager = [NSFileManager defaultManager];
    NSArray *content = [manager contentsOfDirectoryAtPath:[self applicationDocumentsDirectory] error:nil];
    for (NSString *path in content)
    {
        if ([[path pathExtension] isEqualToString:@"log"])
        {            [manager removeItemAtPath:[NSString stringWithFormat:@"%@/%@", [self applicationDocumentsDirectory],path] error:nil];
        }
    }
}

- (NSString *)nowTime:(BOOL)useDate
{
    if (! useDate)
    {
        return @"";
    }
    NSDate *now = [NSDate date];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:ARSTREAM_DATE_FORMAT];
    return [formatter stringFromDate:now];
}

- (NSString *)applicationDocumentsDirectory {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *basePath = ([paths count] > 0) ? [paths objectAtIndex:0] : nil;
    return basePath;
}


@end
