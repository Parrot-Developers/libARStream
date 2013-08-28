package com.parrot.arsdk.arvideo;

import com.parrot.arsdk.arsal.ARNativeData;
import com.parrot.arsdk.arsal.ARSALPrint;

import com.parrot.arsdk.arnetwork.ARNetworkManager;
import com.parrot.arsdk.arnetwork.ARNetworkIOBufferParam;
import com.parrot.arsdk.arnetwork.ARNetworkIOBufferParamBuilder;

/**
 * Wrapper class for the ARVIDEO_Reader C object.<br>
 * <br>
 * To create an ARVideoReader, the application must provide a suitable
 * <code>ARVideoReaderListener</code> to handle the events.<br>
 * <br>
 * The two ARVideoReader Runnables must be run in independant threads<br>
 */
public class ARVideoReader
{
    private static final String TAG = ARVideoReader.class.getSimpleName ();

    /* *********************** */
    /* INTERNAL REPRESENTATION */
    /* *********************** */

    /**
     * Storage of the C pointer
     */
    private long cReader;

    /**
     * Current frame buffer storage
     */
    private ARNativeData currentFrameBuffer;

    /**
     * Previous frame buffer storage (only for "too small" events)
     */
    private ARNativeData previousFrameBuffer;

    /**
     * Event listener
     */
    private ARVideoReaderListener eventListener;

    /**
     * Check validity before all function calls
     */
    private boolean valid;

    /**
     * Runnable of the data part
     */
    private Runnable dataRunnable;

    /**
     * Runnable of the ack part
     */
    private Runnable ackRunnable;

    /* **************** */
    /* STATIC FUNCTIONS */
    /* **************** */

    /**
     * Static builder for ARNetworkIOBufferParam of data buffers
     * @param bufferId Id of the new buffer
     * @return A new buffer, configured to be an ARVideo data buffer
     */
    public static ARNetworkIOBufferParam newDataARNetworkIOBufferParam (int bufferId) {
        ARNetworkIOBufferParam retParam = new ARNetworkIOBufferParamBuilder (bufferId).build ();
        nativeSetDataBufferParams (retParam.getNativePointer (), bufferId);
        return retParam;
    }

    /**
     * Static builder for ARNetworkIOBufferParam of ack buffers
     * @param bufferId Id of the new buffer
     * @return A new buffer, configured to be an ARVideo ack buffer
     */
    public static ARNetworkIOBufferParam newAckARNetworkIOBufferParam (int bufferId) {
        ARNetworkIOBufferParam retParam = new ARNetworkIOBufferParamBuilder (bufferId).build ();
        nativeSetAckBufferParams (retParam.getNativePointer (), bufferId);
        return retParam;
    }

    /* *********** */
    /* CONSTRUCTOR */
    /* *********** */

    /**
     * Constructor for ARVideoReader object<br>
     * Create a new instance of an ARVideoReader for a given ARNetworkManager,
     * and given buffer ids within the ARNetworkManager.
     * @param netManager The ARNetworkManager to use (must be initialized and valid)
     * @param dataBufferId The id to use for data transferts on network (must be a valid buffer id in <code>netManager</code>
     * @param ackBufferId The id to use for ack transferts on network (must be a valid buffer id in <code>netManager</code>
     * @param initialFrameBuffer The first frame buffer to use
     * @param theEventListener The event listener to use for this instance
     */
    public ARVideoReader (ARNetworkManager netManager, int dataBufferId, int ackBufferId, ARNativeData initialFrameBuffer, ARVideoReaderListener theEventListener) {
        this.cReader = nativeConstructor (netManager.getManager (), dataBufferId, ackBufferId, initialFrameBuffer.getData (), initialFrameBuffer.getCapacity ());
        if (this.cReader != 0) {
            this.valid = true;
            this.eventListener = theEventListener;
            this.currentFrameBuffer = initialFrameBuffer;
            this.dataRunnable = new Runnable () {
                    public void run () {
                        nativeRunDataThread (ARVideoReader.this.cReader);
                    }
                };
            this.ackRunnable = new Runnable () {
                    public void run () {
                        nativeRunAckThread (ARVideoReader.this.cReader);
                    }
                };
        } else {
            this.valid = false;
            this.dataRunnable = null;
            this.ackRunnable = null;
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
                stop ();
                if (! dispose ()) {
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
     * Stops the internal threads of the ARVideoReader.<br>
     * Calling this function allow the ARVideoReader Runnables to end
     */
    public void stop () {
        nativeStop (cReader);
    }

    /**
     * Deletes the ARVideoReader.<br>
     * This function should only be called after <code>stop()</code><br>
     * <br>
     * Warning: If this function returns <code>false</code>, then the ARVideoReader was not disposed !
     * @return <code>true</code> if the Runnables are not running.<br><code>false</code> if the ARVideoReader could not be disposed now.
     */
    public boolean dispose () {
        boolean ret = nativeDispose (cReader);
        if (ret) {
            this.valid = false;
        }
        return ret;
    }

    /**
     * Gets the Data Runnable<br>
     * Each runnable must be run exactly ONE time
     * @return The Data Runnable
     */
    public Runnable getDataRunnable () {
        return dataRunnable;
    }

    /**
     * Gets the Ack Runnable<br>
     * Each runnable must be run exactly ONE time
     * @return The Ack Runnable
     */
    public Runnable getAckRunnable () {
        return ackRunnable;
    }

    /**
     * Gets the estimated efficiency of the network link<br>
     * This methods gives the percentage of useful data packets in the
     * data stream.
     * @return Estimated network link efficiency (0.0-1.0)
     */
    public float getEstimatedEfficiency () {
        return nativeGetEfficiency (cReader);
    }

    /* ***************** */
    /* PRIVATE FUNCTIONS */
    /* ***************** */

    /**
     * Callback wrapper for the listener
     */
    private long[] callbackWrapper (int icause, long ndPointer, int ndSize, boolean isFlush, int nbSkip) {
        ARVIDEO_READER_CAUSE_ENUM cause = ARVIDEO_READER_CAUSE_ENUM.getFromValue (icause);
        if (cause == null) {
            ARSALPrint.e (TAG, "Bad cause : " + icause);
            return null;
        }

        if (ndPointer != currentFrameBuffer.getData()) {
            ARSALPrint.e (TAG, "Bad frame buffer");
            return null;
        }

        switch (cause) {
        case ARVIDEO_READER_CAUSE_FRAME_COMPLETE:
            currentFrameBuffer.setUsedSize(ndSize);
            currentFrameBuffer = eventListener.didUpdateFrameStatus (cause, currentFrameBuffer, isFlush, nbSkip);
            break;
        case ARVIDEO_READER_CAUSE_FRAME_TOO_SMALL:
            previousFrameBuffer = currentFrameBuffer;
            currentFrameBuffer = eventListener.didUpdateFrameStatus (cause, currentFrameBuffer, isFlush, nbSkip);
            break;
        case ARVIDEO_READER_CAUSE_COPY_COMPLETE:
            eventListener.didUpdateFrameStatus (cause, previousFrameBuffer, isFlush, nbSkip);
            previousFrameBuffer = null;
            break;
        case ARVIDEO_READER_CAUSE_CANCEL:
            eventListener.didUpdateFrameStatus (cause, currentFrameBuffer, isFlush, nbSkip);
            currentFrameBuffer = null;
            break;
        default:
            ARSALPrint.e (TAG, "Unknown cause :" + cause);
            break;
        }
        if (currentFrameBuffer != null) {
            long retVal[] = { currentFrameBuffer.getData(), currentFrameBuffer.getCapacity() };
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
     * Sets an ARNetworkIOBufferParams internal values to represent an
     * ARVideo data buffer.
     * @param cParams C-Pointer to the ARNetworkIOBufferParams internal representation
     * @param id The id of the param
     */
    private native static void nativeSetDataBufferParams (long cParams, int id);

    /**
     * Sets an ARNetworkIOBufferParams internal values to represent an
     * ARVideo ack buffer.
     * @param cParams C-Pointer to the ARNetworkIOBufferParams internal representation
     * @param id The id of the param
     */
    private native static void nativeSetAckBufferParams (long cParams, int id);

    /**
     * Constructor in native memory space<br>
     * This function created a C-Managed ARVIDEO_Reader object
     * @param cNetManager C-Pointer to the ARNetworkManager internal object
     * @param dataBufferId id of the data buffer to use
     * @param ackBufferId id of the ack buffer to use
     * @param frameBuffer C-Pointer to the initial frame buffer
     * @param frameBufferSize size of the initial frame buffer
     * @return C-Pointer to the ARVIDEO_Reader object (or null if any error occured)
     */
    private native long nativeConstructor (long cNetManager, int dataBufferId, int ackBufferId, long frameBuffer, int frameBufferSize);

    /**
     * Entry point for the data thread<br>
     * This function never returns until <code>stop</code> is called
     * @param cReader C-Pointer to the ARVIDEO_Reader C object
     */
    private native void nativeRunDataThread (long cReader);

    /**
     * Entry point for the ack thread<br>
     * This function never returns until <code>stop</code> is called
     * @param cReader C-Pointer to the ARVIDEO_Reader C object
     */
    private native void nativeRunAckThread (long cReader);

    /**
     * Stops the internal thread loops
     * @param cReader C-Pointer to the ARVIDEO_Reader C object
     */
    private native void nativeStop (long cReader);

    /**
     * Marks the reader as invalid and frees it if needed
     * @param cReader C-Pointer to the ARVIDEO_Reader C object
     */
    private native boolean nativeDispose (long cReader);

    /**
     * Gets the estimated efficiency of the reader
     * @param cReader C-Pointer to the ARVIDEO_Reader C object
     */
    private native float nativeGetEfficiency (long cReader);

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