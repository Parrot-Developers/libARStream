//
//  ARVLogger.m
//  ARVideoTB
//
//  Created by Nicolas BRULEZ on 09/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARVLogger.h"

#define ARVIDEO_DEFAULT_FILENAME @"ARVLog"
#define ARVIDEO_DATE_FORMAT @"_yyyyMMdd_HHmmss"

@interface ARVLogger()

@property (nonatomic, strong) NSFileHandle *file;

@end

@implementation ARVLogger

- (id)init
{
    self = [super init];
    if (self)
    {
        NSString *filename = [NSString stringWithFormat:@"%@/%@%@.log", [self applicationDocumentsDirectory], ARVIDEO_DEFAULT_FILENAME, [self nowTime:YES]];
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

- (void)log:(NSString *)message
{
    NSLog (@"Adding message : %@", message);
    [_file writeData:[message dataUsingEncoding:NSUTF8StringEncoding]];
    [_file writeData:[@"\n" dataUsingEncoding:NSUTF8StringEncoding]];
}

- (void)close
{
    [_file closeFile];
}

- (void)dealloc
{
    [self close];
}

- (NSString *)nowTime:(BOOL)useDate
{
    if (! useDate)
    {
        return @"";
    }
    NSDate *now = [NSDate date];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:ARVIDEO_DATE_FORMAT];
    return [formatter stringFromDate:now];
}

- (NSString *)applicationDocumentsDirectory {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *basePath = ([paths count] > 0) ? [paths objectAtIndex:0] : nil;
    return basePath;
}


@end
