package com.parrot.arsdk.arvideo;

import java.util.Map;
import java.util.HashMap;

import com.parrot.arsdk.arsal.ARNativeData;
import com.parrot.arsdk.arsal.ARSALPrint;

import com.parrot.arsdk.arnetwork.ARNetworkManager;
import com.parrot.arsdk.arnetwork.ARNetworkIOBufferParam;
import com.parrot.arsdk.arnetwork.ARNetworkIOBufferParamBuilder;

/**
 * Wrapper class for the ARVIDEO_Sender C object.<br>
 * <br>
 * To create an ARVideoSender, the application must provide a suitable
 * <code>ARVideoSenderListener</code> to handle the events.<br>
 * <br>
 * The two ARVideoSender Runnables must be run in independant threads<br>
 */
public class ARVideoSender
{
    private static final String TAG = ARVideoSender.class.getSimpleName ();

    /* *********************** */
    /* INTERNAL REPRESENTATION */
    /* *********************** */

    /**
     * Storage of the C pointer
     */
    private long cSender;

    /**
     * Current frames buffer storage
     */
    private Map<Long, ARNativeData> frames;

    /**
     * Event listener
     */
    private ARVideoSenderListener eventListener;

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
     * Constructor for ARVideoSender object<br>
     * Create a new instance of an ARVideoSender for a given ARNetworkManager,
     * and given buffer ids within the ARNetworkManager.
     * @param netManager The ARNetworkManager to use (must be initialized and valid)
     * @param dataBufferId The id to use for data transferts on network (must be a valid buffer id in <code>netManager</code>
     * @param ackBufferId The id to use for ack transferts on network (must be a valid buffer id in <code>netManager</code>
     * @param theEventListener The event listener to use for this instance
     * @param frameBufferSize Capacity of the internal frameBuffer
     */
    public ARVideoSender (ARNetworkManager netManager, int dataBufferId, int ackBufferId, ARVideoSenderListener theEventListener, int frameBufferSize) {
        this.cSender = nativeConstructor (netManager.getManager (), dataBufferId, ackBufferId, frameBufferSize);
        if (this.cSender != 0) {
            this.valid = true;
            this.eventListener = theEventListener;
            // HashMap will realloc when over 75% of its capacity, so put a greater capacity than needed to avoid that
            this.frames = new HashMap<Long, ARNativeData> (frameBufferSize * 2);
            this.dataRunnable = new Runnable () {
                    public void run () {
                        nativeRunDataThread (ARVideoSender.this.cSender);
                    }
                };
            this.ackRunnable = new Runnable () {
                    public void run () {
                        nativeRunAckThread (ARVideoSender.this.cSender);
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
     * Stops the internal threads of the ARVideoSender.<br>
     * Calling this function allow the ARVideoSender Runnables to end
     */
    public void stop () {
        nativeStop (cSender);
    }

    /**
     * Deletes the ARVideoSender.<br>
     * This function should only be called after <code>stop()</code><br>
     * <br>
     * Warning: If this function returns <code>false</code>, then the ARVideoSender was not disposed !
     * @return <code>true</code> if the Runnables are not running.<br><code>false</code> if the ARVideoSender could not be disposed now.
     */
    public boolean dispose () {
        boolean ret = nativeDispose (cSender);
        if (ret) {
            this.valid = false;
        }
        return ret;
    }

    /**
     * Sends a new frame with the ARVideoSender.<br>
     * The application should not modify <code>frame</code> until an event
     * is called for it (either cancel or sent).<br>
     * Modifying <code>frame</code> before that leads to undefined behavior.
     * @param frame The frame to send
     * @param flush If active, the ARVideoSender will cancel any remaining prevous frame, and start sending this one immediately
     */
    ARVIDEO_ERROR_ENUM SendNewFrame (ARNativeData frame, boolean flush) {
        return ARVIDEO_ERROR_ENUM.ARVIDEO_OK;
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
        return nativeGetEfficiency (cSender);
    }

    /* ***************** */
    /* PRIVATE FUNCTIONS */
    /* ***************** */

    /**
     * Callback wrapper for the listener
     */
    private void callbackWrapper (int istatus, long ndPointer, int ndSize) {
        ARVIDEO_SENDER_STATUS_ENUM status = ARVIDEO_SENDER_STATUS_ENUM.getFromValue (istatus);
        if (status == null) {
            return;
        }

        switch (status) {
        case ARVIDEO_SENDER_STATUS_FRAME_SENT:
        case ARVIDEO_SENDER_STATUS_FRAME_CANCEL:
            ARNativeData data = null;
            synchronized (this)
            {
                data = frames.remove (ndPointer);
            }
            eventListener.didUpdateFrameStatus (status, data);
            break;

        default:
            ARSALPrint.e (TAG, "Unknown status :" + status);
            break;
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
     * This function created a C-Managed ARVIDEO_Sender object
     * @param cNetManager C-Pointer to the ARNetworkManager internal object
     * @param dataBufferId id of the data buffer to use
     * @param ackBufferId id of the ack buffer to use
     * @param nbFramesToBuffer number of frames that the object can keep in its internal buffer
     * @return C-Pointer to the ARVIDEO_Sender object (or null if any error occured)
     */
    private native long nativeConstructor (long cNetManager, int dataBufferId, int ackBufferId, int nbFramesToBuffer);

    /**
     * Entry point for the data thread<br>
     * This function never returns until <code>stop</code> is called
     * @param cSender C-Pointer to the ARVIDEO_Sender C object
     */
    private native void nativeRunDataThread (long cSender);

    /**
     * Entry point for the ack thread<br>
     * This function never returns until <code>stop</code> is called
     * @param cSender C-Pointer to the ARVIDEO_Sender C object
     */
    private native void nativeRunAckThread (long cSender);

    /**
     * Stops the internal thread loops
     * @param cSender C-Pointer to the ARVIDEO_Sender C object
     */
    private native void nativeStop (long cSender);

    /**
     * Marks the sender as invalid and frees it if needed
     * @param cSender C-Pointer to the ARVIDEO_Sender C object
     */
    private native boolean nativeDispose (long cSender);

    /**
     * Gets the estimated efficiency of the sender
     * @param cSender C-Pointer to the ARVIDEO_Sender C object
     */
    private native float nativeGetEfficiency (long cSender);

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