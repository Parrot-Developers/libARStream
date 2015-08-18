/*
    Copyright (C) 2015 Parrot SA

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
package com.parrot.arsdk.arstream;

import android.util.Log;

import com.parrot.arsdk.arsal.ARNativeData;
import com.parrot.arsdk.arsal.ARSALPrint;

import org.apache.http.protocol.RequestUserAgent;

/**
 * Wrapper class for the ARSTREAM_Reader2 C object.<br>
 * <br>
 * To create an ARStreamReader2, the application must provide a suitable
 * <code>ARStreamReader2Listener</code> to handle the events.<br>
 * <br>
 */
public class ARStreamReader2
{
    private static final String TAG = ARStreamReader2.class.getSimpleName ();

    /* *********************** */
    /* INTERNAL REPRESENTATION */
    /* *********************** */

    /**
     * Storage of the C pointer
     */
    final private long cReader;

    /**
     * Event listener
     */
    final private ARStreamReader2Listener listener;

    /**
     * Current nalu buffer storage
     */
    private ARNativeData naluBuffer;

    /**
     * Previous nalu buffer storage (only for "too small" events)
     */
    private ARNativeData previousnaluBuffer;


    /**
     * Check validity before all function calls
     */
    private boolean valid;

    /* *********** */
    /* CONSTRUCTOR */
    /* *********** */

    /**
     * Constructor for ARStreamReader2 object<br>
     * 
     * @param remoteAddress device address
     * @param naluBufferSize buffer size
     * @param listener ARStream 2 listener
      */
    public ARStreamReader2(String serverAddress, int serverStreamPort, int serverControlPort, int clientStreamPort, int clientControlPort,
                           int maxPacketSize, int maxBitrate, int maxLatency, int maxNetworkLatency, int naluBufferSize, ARStreamReader2Listener listener)
    {
        this.listener = listener;
        this.naluBuffer = new ARNativeData(naluBufferSize); //TODO
        this.cReader = nativeConstructor(serverAddress, serverStreamPort, serverControlPort, clientStreamPort, clientControlPort, maxPacketSize, maxBitrate, maxLatency, maxNetworkLatency);
        this.valid =  (this.cReader != 0);
    }

    /* ********** */
    /* DESTRUCTOR */
    /* ********** */
    /**
     * Destructor<br>
     * This destructor tries to avoid leaks if the object was not disposed
     */
    protected void finalize () throws Throwable {
        try {
            if (valid) {
                ARSALPrint.e (TAG, "Object " + this + " was not disposed !");
                stop();
                if (!dispose()) {
                    ARSALPrint.e (TAG, "Unable to dispose object " + this + " ... leaking memory !");
                }
            }
        } finally {
            super.finalize ();
        }
    }

    /* **************** */
    /* PUBLIC FUNCTIONS */
    /* **************** */

    /**
     * Checks if the current manager is valid.<br>
     * A valid manager is a manager which can be used to receive video frames.
     * @return The validity of the manager.
     */
    public boolean isValid()
    {
        return this.valid;
    }

    /**
     * Reader stream thread function
     */
    public void runReaderStream()
    {
        nativeRunStreamThread(cReader);
    }

    /**
     * Reader control thread function
     */
    public void runReaderControl()
    {
        nativeRunControlThread(cReader);
    }

    /**
     * Stops the internal threads of the ARStreamReader2.<br>
     * Calling this function allow the ARStreamReader2 Runnables to end
     */
    public void stop () {
        nativeStop (cReader);
    }

    /**
     * Deletes the ARStreamReader2.<br>
     * This function should only be called after <code>stop()</code><br>
     * <br>
     * Warning: If this function returns <code>false</code>, then the ARStreamReader2 was not disposed !
     * @return <code>true</code> if the Runnables are not running.<br><code>false</code> if the ARStreamReader2 could not be disposed now.
     */
    public boolean dispose () {
        boolean ret = nativeDispose (cReader);
        if (ret) {
            this.valid = false;
        }
        return ret;
    }

    /* ***************** */
    /* PACKAGE FUNCTIONS */
    /* ***************** */
    long getCReader() {
        return cReader;
    }

    /* ***************** */
    /* PRIVATE FUNCTIONS */
    /* ***************** */

    /**
     * Callback wrapper for the listener
     */
    private long[] callbackWrapper (int icause, long naluBufferPtr, int naluSize, boolean isFirstNaluInAu, boolean sLastNaluInAu,
                                    long auTimeStamp, int missingPacketsBefore, int newBufferCapacity) {
        ARSTREAM_READER2_CAUSE_ENUM cause = ARSTREAM_READER2_CAUSE_ENUM.getFromValue (icause);
        if (cause == null) {
            ARSALPrint.e (TAG, "Bad cause : " + icause);
            return null;
        }

        switch (cause) {
        case ARSTREAM_READER2_CAUSE_NALU_COMPLETE:
            naluBuffer.setUsedSize(naluSize);
            try
            {
                listener.onNaluReceived(naluBuffer, isFirstNaluInAu, sLastNaluInAu, auTimeStamp, missingPacketsBefore);
            }
            catch (Throwable t)
            {
                ARSALPrint.e (TAG, "Exception in onNaluReceived callback" + t.getMessage());
            }
            naluBuffer.setUsedSize(0);
            break;
        case ARSTREAM_READER2_CAUSE_NALU_BUFFER_TOO_SMALL:
            previousnaluBuffer = naluBuffer;
            naluBuffer = new ARNativeData(newBufferCapacity);
            ARSALPrint.e (TAG, "ARSTREAM_READER2_CAUSE_NALU_BUFFER_TOO_SMALL");
            break;
        case ARSTREAM_READER2_CAUSE_NALU_COPY_COMPLETE:
            previousnaluBuffer.dispose();
            break;
        case ARSTREAM_READER2_CAUSE_CANCEL:
            break;
        default:
            ARSALPrint.e (TAG, "Unknown cause :" + cause);
            break;
        }
        if (this.naluBuffer != null) {
            long retVal[] = { this.naluBuffer.getData(), this.naluBuffer.getCapacity() };
            return retVal;
        } else {
            long retVal[] = { 0, 0 };
            return retVal;
        }
    }

    /* **************** */
    /* NATIVE FUNCTIONS */
    /* **************** */

    /**
     * Constructor in native memory space<br>
     * This function created a C-Managed ARSTREAM_Reader2 object
     * @param remoteAddress device address
     * @param naluBuffer C-Pointer to the initial nalu buffer
     * @param naluBufferSize size of the initial nalu buffer
     * @return C-Pointer to the ARSTREAM_Reader2 object (or null if any error occured)
     */
    private native long nativeConstructor (String serverAddress, int serverStreamPort, int serverControlPort, int clientStreamPort, int clientControlPort,
                                           int maxPacketSize, int maxBitrate, int maxLatency, int maxNetworkLatency);

    /**
     * Entry point for the data thread<br>
     * This function never returns until <code>stop</code> is called
     * @param cReader C-Pointer to the ARSTREAM_Reader2 C object
     */
    private native void nativeRunStreamThread (long cReader);

    /**
     * Entry point for the data thread<br>
     * This function never returns until <code>stop</code> is called
     * @param cReader C-Pointer to the ARSTREAM_Reader2 C object
     */
    private native void nativeRunControlThread (long cReader);

    /**
     * Stops the internal thread loops
     * @param cReader C-Pointer to the ARSTREAM_Reader2 C object
     */
    private native void nativeStop (long cReader);

    /**
     * Marks the reader as invalid and frees it if needed
     * @param cReader C-Pointer to the ARSTREAM_Reader2 C object
     */
    private native boolean nativeDispose (long cReader);

    /**
     * Initializes global static references in native code
     */
    private native static void nativeInitClass ();

    /* *********** */
    /* STATIC BLOC */
    /* *********** */
    static {
        nativeInitClass ();
    }
}
