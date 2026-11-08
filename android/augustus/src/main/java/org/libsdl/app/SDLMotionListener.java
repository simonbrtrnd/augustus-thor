package org.libsdl.app;

import android.view.View;

/**
 * Gives other windows SDL's handler of controller and mouse motion, which SDL keeps protected.
 */
public final class SDLMotionListener {
    private SDLMotionListener() {
    }

    /**
     * SDL's handler of controller and mouse motion, for another window to pass its events on.
     */
    public static View.OnGenericMotionListener get() {
        return SDLActivity.getMotionListener();
    }
}
