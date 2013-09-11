/*
 * GENERATED FILE
 *  Do not modify this file, it will be erased during the next configure run
 */

package com.parrot.arsdk.arstreaming;

import java.util.HashMap;

/**
 * Java copy of the eARSTREAMING_SENDER_STATUS enum
 */
public enum ARSTREAMING_SENDER_STATUS_ENUM {
   /** Frame was sent and acknowledged by peer */
    ARSTREAMING_SENDER_STATUS_FRAME_SENT (0, "Frame was sent and acknowledged by peer"),
   /** Frame was not sent, and was cancelled by a new frame */
    ARSTREAMING_SENDER_STATUS_FRAME_CANCEL (1, "Frame was not sent, and was cancelled by a new frame"),
   ARSTREAMING_SENDER_STATUS_MAX (2);

    private final int value;
    private final String comment;
    static HashMap<Integer, ARSTREAMING_SENDER_STATUS_ENUM> valuesList;

    ARSTREAMING_SENDER_STATUS_ENUM (int value) {
        this.value = value;
        this.comment = null;
    }

    ARSTREAMING_SENDER_STATUS_ENUM (int value, String comment) {
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
     * Gets the ARSTREAMING_SENDER_STATUS_ENUM instance from a C enum value
     * @param value C value of the enum
     * @return The ARSTREAMING_SENDER_STATUS_ENUM instance, or null if the C enum value was not valid
     */
    public static ARSTREAMING_SENDER_STATUS_ENUM getFromValue (int value) {
        if (null == valuesList) {
            ARSTREAMING_SENDER_STATUS_ENUM [] valuesArray = ARSTREAMING_SENDER_STATUS_ENUM.values ();
            valuesList = new HashMap<Integer, ARSTREAMING_SENDER_STATUS_ENUM> (valuesArray.length);
            for (ARSTREAMING_SENDER_STATUS_ENUM entry : valuesArray) {
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
