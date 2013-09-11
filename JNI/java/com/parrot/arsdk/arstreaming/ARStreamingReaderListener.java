package com.parrot.arsdk.arstreaming;

import com.parrot.arsdk.arsal.ARNativeData;

/**
 * This interface describes a listener of ARStreamingReader events
 */
public interface ARStreamingReaderListener
{
    /**
     * This callback can be called in different cases:<br>
     *  - Frame complete:<br>
     *    - 'currentFrame' contains a complete frame and won't be used by ARStreamingReader again<br>
     *    - 'isFlushFrame' is true if the complete frame occured after a buffer flush (on most cases, the complete frame is an I-Frame)<br>
     *    - 'nbSkippedFrames' contains the number of skipped frames since the last complete frame<br>
     *    - Return value is the storing location of the next frame<br>
     *  - Frame too small:<br>
     *    - 'currentFrame' should not be modified (it is still used by the ARStreamingReader)<br>
     *    - 'isFlushFrame' is undefined<br>
     *    - 'nbSkippedFrames' is undefined<br>
     *    - Return value is a new ARNativeData, greater than the currentFrame buffer.<br>
     *      -- If the returned value is null or smaller than currentFrame, the frame is discarded<br>
     *  - Copy complete: (only called after a Frame too small)<br>
     *    - 'currentFrame' is the previous frame buffer, which can now be freed (or reused)<br>
     *    - 'isFlushFrame' is undefined<br>
     *    - 'nbSkippedFrames' is undefined<br>
     *    - Return value is unused and should be null<br>
     *  - Cancel:<br>
     *    - 'currentFrame' is the frame buffer which is cancelled, and which can be freed<br>
     *    - 'isFlushFrame' is undefined<br>
     *    - 'nbSkippedFrames' in undefined<br>
     *    - Return value is unused and should be null
     * @param cause The event that triggered this call (see global func description)
     * @param currentFrame The frame buffer for the event (see global func description)
     * @param isFlushFrame Indicates if the frame forced a sender flush. This is typically set on I-Frames (see global func description)
     * @param nbSkippedFrames The number of frames skipped since last complete frame (see global func description)
     * @return The new frame buffer, or null (depending on cause, see global func description)
     */
    public ARNativeData didUpdateFrameStatus (ARSTREAMING_READER_CAUSE_ENUM cause, ARNativeData currentFrame, boolean isFlushFrame, int nbSkippedFrames);
}