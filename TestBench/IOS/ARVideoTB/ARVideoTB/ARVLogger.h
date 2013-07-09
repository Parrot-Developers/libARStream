//
//  ARVLogger.h
//  ARVideoTB
//
//  Created by Nicolas BRULEZ on 09/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ARVLogger : NSObject

/**
 * Init with default file name
 */
- (id)init;

/**
 * Init with a given file name
 * @param fileName the prefix of the files created by the logged
 * @param useDate enable flag for using the date as a suffix
 */
- (id)initWithFileName:(NSString *)fileName withDate:(BOOL)useDate;

/**
 * Log a new line in the log file
 */
- (void)log:(NSString *)message;

/**
 * Close the current logfile
 */
- (void)close;

@end
