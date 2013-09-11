/*
 * GENERATED FILE
 *  Do not modify this file, it will be erased during the next configure run
 */

package com.parrot.arsdk.arstreaming;

import java.util.HashMap;

/**
 * Java copy of the eARSTREAMING_READER_CAUSE enum
 */
public enum ARSTREAMING_READER_CAUSE_ENUM {
   /** Frame is complete (no error) */
    ARSTREAMING_READER_CAUSE_FRAME_COMPLETE (0, "Frame is complete (no error)"),
   /** Frame buffer is too small for the frame on the network */
    ARSTREAMING_READER_CAUSE_FRAME_TOO_SMALL (1, "Frame buffer is too small for the frame on the network"),
   /** Copy of previous frame buffer is complete (called only after ARSTREAMING_READER_CAUSE_FRAME_TOO_SMALL) */
    ARSTREAMING_READER_CAUSE_COPY_COMPLETE (2, "Copy of previous frame buffer is complete (called only after ARSTREAMING_READER_CAUSE_FRAME_TOO_SMALL)"),
   /** Reader is closing, so buffer is no longer used */
    ARSTREAMING_READER_CAUSE_CANCEL (3, "Reader is closing, so buffer is no longer used"),
   ARSTREAMING_READER_CAUSE_MAX (4);

    private final int value;
    private final String comment;
    static HashMap<Integer, ARSTREAMING_READER_CAUSE_ENUM> valuesList;

    ARSTREAMING_READER_CAUSE_ENUM (int value) {
        this.value = value;
        this.comment = null;
    }

    ARSTREAMING_READER_CAUSE_ENUM (int value, String comment) {
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
     * Gets the ARSTREAMING_READER_CAUSE_ENUM instance from a C enum value
     * @param value C value of the enum
     * @return The ARSTREAMING_READER_CAUSE_ENUM instance, or null if the C enum value was not valid
     */
    public static ARSTREAMING_READER_CAUSE_ENUM getFromValue (int value) {
        if (null == valuesList) {
            ARSTREAMING_READER_CAUSE_ENUM [] valuesArray = ARSTREAMING_READER_CAUSE_ENUM.values ();
            valuesList = new HashMap<Integer, ARSTREAMING_READER_CAUSE_ENUM> (valuesArray.length);
            for (ARSTREAMING_READER_CAUSE_ENUM entry : valuesArray) {
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
