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

/**
 * Wrapper class for the ARSTREAM_Reader2_Resender C object.<br>
 */
public class ARStreamReader2Resender
{
    private static final String TAG = ARStreamReader2Resender.class.getSimpleName ();

    /* *********************** */
    /* INTERNAL REPRESENTATION */
    /* *********************** */

    /**
     * Storage of the C pointer
     */
    final private long cResender;
    private Thread streamThread;
    private Thread controlThread;

    /**
     * Check validity before all function calls
     */
    private boolean valid;

    /* *********** */
    /* CONSTRUCTOR */
    /* *********** */

    /**
     * Constructor for ARStreamReader2Resender object<br>
     * Create a new instance of an ARStreamReader for a given ARNetworkManager,
     * and given buffer ids within the ARNetworkManager.
     * @param remoteAddress device address
     * @param remotePort device arstream2 port
     * @param naluBufferSize buffer size
     * @param listener ARStream 2 listener
      */
    public ARStreamReader2Resender(ARStreamReader2 reader, String clientAddress, int clientStreamPort, int clientControlPort, int serverStreamPort, int serverControlPort,
                                   int maxPacketSize, int targetPacketSize, int maxLatency, int maxNetworkLatency)
    {
        Log.e(TAG, "ARStreamReader2Resender " + clientAddress);
        this.cResender = nativeConstructor(reader.getCReader(), clientAddress, clientStreamPort, clientControlPort, serverStreamPort, serverControlPort,
                                           maxPacketSize, targetPacketSize, maxLatency, maxNetworkLatency);
        this.valid =  (this.cResender != 0);
        if (this.valid) {
            streamThread = new Thread(new Runnable()
            {
                @Override
                public void run()
                {
                    Log.e(TAG, "streamThread running");
                    nativeRunStreamThread(cResender);
                    Log.e(TAG, "streamThread terminated");
                }
            }, "Resender streamThread");
            controlThread = new Thread(new Runnable()
            {
                @Override
                public void run()
                {
                    Log.e(TAG, "controlThread running");
                    nativeRunControlThread(cResender);
                    Log.e(TAG, "controlThread terminated");
                }
            }, "Resender controlThread");

            streamThread.start();
            controlThread.start();
        }
        else
        {
            Log.e(TAG, "ARStreamReader2Resender error creating native resender");
        }
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
     * Stops the internal threads of the ARStreamReader2Resender.<br>
     * Calling this function allow the ARStreamReader2Resender Runnables to end
     */
    public void stop () {
        nativeStop(cResender);
        try
        {
            if (streamThread != null)
            {
                streamThread.join();
            }
            if (controlThread != null)
            {
                controlThread.join();
            }
        }
        catch (InterruptedException e)
        {
            e.printStackTrace();
        }
    }

    /**
     * Deletes the ARStreamReader2Resender.<br>
     * This function should only be called after <code>stop()</code><br>
     * <br>
     * Warning: If this function returns <code>false</code>, then the ARStreamReader2Resender was not disposed !
     * @return <code>true</code> if the Runnables are not running.<br><code>false</code> if the ARStreamReader2Resender could not be disposed now.
     */
    public boolean dispose () {
        boolean ret = nativeDispose(cResender);
        if (ret) {
            this.valid = false;
        }
        return ret;
    }

    /* **************** */
    /* NATIVE FUNCTIONS */
    /* **************** */

    /**
     * Constructor in native memory space<br>
     * This function created a C-Managed ARSTREAM_Reader object
     * @param naluBufferSize size of the initial nalu buffer
     * @param naluBuffer C-Pointer to the initial nalu buffer
     * @param targetIp
     * @return C-Pointer to the ARSTREAM_Reader object (or null if any error occured)
     */
    private native long nativeConstructor(long cReader, String clientAddress, int clientStreamPort, int clientControlPort, int serverStreamPort, int serverControlPort,
                                          int maxPacketSize, int targetPacketSize, int maxLatency, int maxNetworkLatency);

    /**
     * Entry point for the stream thread<br>
     * This function never returns until <code>stop</code> is called
     * @param cReader C-Pointer to the ARSTREAM_Reader C object
     */
    private native void nativeRunStreamThread (long cReader);

    /**
     * Entry point for the control thread<br>
     * This function never returns until <code>stop</code> is called
     * @param cReader C-Pointer to the ARSTREAM_Reader C object
     */
    private native void nativeRunControlThread (long cReader);

    /**
     * Stops the internal thread loops
     * @param cReader C-Pointer to the ARSTREAM_Reader C object
     */
    private native void nativeStop (long cReader);

    /**
     * Marks the reader as invalid and frees it if needed
     * @param cReader C-Pointer to the ARSTREAM_Reader C object
     */
    private native boolean nativeDispose (long cReader);
}
