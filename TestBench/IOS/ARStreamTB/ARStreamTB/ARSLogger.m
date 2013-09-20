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
