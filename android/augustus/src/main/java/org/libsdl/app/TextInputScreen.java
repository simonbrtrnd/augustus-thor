package org.libsdl.app;

import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;

/**
 * Moves SDL's hidden text editor to the window of another screen.
 * Android opens the keyboard on the screen whose window holds the focused editor.
 * This class lives in SDL's package because the editor and its fields are package-private.
 */
public final class TextInputScreen {
    private TextInputScreen() {
    }

    /**
     * Puts the editor in the given layout, so the next keyboard opens on its screen.
     * An open keyboard keeps its editor.
     */
    public static void moveTo(ViewGroup layout) {
        if (layout == null || SDLActivity.mScreenKeyboardShown) {
            return;
        }
        DummyEdit edit = SDLActivity.mTextEdit;
        if (edit != null) {
            if (edit.getParent() == layout) {
                return;
            }
            ViewGroup parent = (ViewGroup) edit.getParent();
            if (parent != null) {
                parent.removeView(edit);
            }
        }
        edit = new DummyEdit(layout.getContext());
        edit.setOnGenericMotionListener(SDLActivity.getMotionListener());
        layout.addView(edit, new RelativeLayout.LayoutParams(0, 0));
        SDLActivity.mTextEdit = edit;
    }

    /**
     * Puts the editor back in the game's own window.
     */
    public static void moveToGameScreen() {
        moveTo(SDLActivity.mLayout);
    }

    /**
     * Whether the game has the keyboard open.
     */
    public static boolean isKeyboardShown() {
        return SDLActivity.mScreenKeyboardShown;
    }

    /**
     * SDL's handler of controller and mouse motion, for another window to pass its events on.
     */
    public static View.OnGenericMotionListener motionListener() {
        return SDLActivity.getMotionListener();
    }
}
