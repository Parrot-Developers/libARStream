/*
 * GENERATED FILE
 *  Do not modify this file, it will be erased during the next configure run
 */

package com.parrot.arsdk.arstreaming;

import java.util.HashMap;

/**
 * Java copy of the eARSTREAMING_ERROR enum
 */
public enum ARSTREAMING_ERROR_ENUM {
   /** No error */
    ARSTREAMING_OK (0, "No error"),
   /** Bad parameters */
    ARSTREAMING_ERROR_BAD_PARAMETERS (1, "Bad parameters"),
   /** Unable to allocate data */
    ARSTREAMING_ERROR_ALLOC (2, "Unable to allocate data"),
   /** Bad parameter : frame too large */
    ARSTREAMING_ERROR_FRAME_TOO_LARGE (3, "Bad parameter : frame too large"),
   /** Object is busy and can not be deleted yet */
    ARSTREAMING_ERROR_BUSY (4, "Object is busy and can not be deleted yet"),
   /** Frame queue is full */
    ARSTREAMING_ERROR_QUEUE_FULL (5, "Frame queue is full");

    private final int value;
    private final String comment;
    static HashMap<Integer, ARSTREAMING_ERROR_ENUM> valuesList;

    ARSTREAMING_ERROR_ENUM (int value) {
        this.value = value;
        this.comment = null;
    }

    ARSTREAMING_ERROR_ENUM (int value, String comment) {
        this.value = value;
        this.comment = comment;
    }

    /**
     * Gets the int value of the enum
     * @return int value of the enum
     */
    public int getValue () {
        return value;
    }

    /**
     * Gets the ARSTREAMING_ERROR_ENUM instance from a C enum value
     * @param value C value of the enum
     * @return The ARSTREAMING_ERROR_ENUM instance, or null if the C enum value was not valid
     */
    public static ARSTREAMING_ERROR_ENUM getFromValue (int value) {
        if (null == valuesList) {
            ARSTREAMING_ERROR_ENUM [] valuesArray = ARSTREAMING_ERROR_ENUM.values ();
            valuesList = new HashMap<Integer, ARSTREAMING_ERROR_ENUM> (valuesArray.length);
            for (ARSTREAMING_ERROR_ENUM entry : valuesArray) {
                valuesList.put (entry.getValue (), entry);
            }
        }
        return valuesList.get (value);
    }

    /**
     * Returns the enum comment as a description string
     * @return The enum description
     */
    public String toString () {
        if (this.comment != null) {
            return this.comment;
        }
        return super.toString ();
    }
}
