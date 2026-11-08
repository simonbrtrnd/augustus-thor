package com.github.Keriew.augustus;

import android.app.Activity;
import android.app.Presentation;
import android.content.Context;
import android.hardware.display.DisplayManager;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLMotionListener;

/**
 * A touch on the second screen gives it the focus. The keys and the controller then come to this
 * window, which passes them on to the game.
 */
class BottomScreen implements SurfaceHolder.Callback, View.OnTouchListener, View.OnKeyListener {
    private Presentation presentation;

    void show(Activity activity) {
        if (presentation != null) {
            return;
        }
        Display display = findSecondScreen(activity);
        if (display == null) {
            return;
        }
        presentation = new Presentation(activity, display);
        presentation.setCancelable(false);
        Context context = presentation.getContext();
        RelativeLayout layout = new RelativeLayout(context);
        layout.setFocusable(true);
        layout.setFocusableInTouchMode(true);
        layout.setOnKeyListener(this);
        layout.setOnGenericMotionListener(SDLMotionListener.get());
        SurfaceView view = new SurfaceView(context);
        view.getHolder().addCallback(this);
        view.setOnTouchListener(this);
        layout.addView(view, new RelativeLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        presentation.setContentView(layout);
        presentation.show();
        layout.requestFocus();
    }

    void hide() {
        if (presentation != null) {
            presentation.dismiss();
            presentation = null;
        }
    }

    private static Display findSecondScreen(Activity activity) {
        DisplayManager manager = (DisplayManager) activity.getSystemService(Context.DISPLAY_SERVICE);
        int gameDisplayId = activity.getWindowManager().getDefaultDisplay().getDisplayId();
        for (Display display : manager.getDisplays(DisplayManager.DISPLAY_CATEGORY_PRESENTATION)) {
            if (display.getDisplayId() != gameDisplayId) {
                return display;
            }
        }
        return null;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        nativeSetSurface(holder.getSurface(), width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeSetSurface(null, 0, 0);
    }

    @Override
    public boolean onTouch(View view, MotionEvent event) {
        nativeTouch(event.getActionMasked(), event.getX() / view.getWidth(), event.getY() / view.getHeight());
        return true;
    }

    /**
     * Passes on the keys the game's own window takes, and leaves to Android those it keeps (SDLActivity.dispatchKeyEvent).
     */
    @Override
    public boolean onKey(View view, int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_CAMERA:
            case KeyEvent.KEYCODE_ZOOM_IN:
            case KeyEvent.KEYCODE_ZOOM_OUT:
                return false;
            default:
                return SDLActivity.handleKeyEvent(view, keyCode, event, null);
        }
    }

    private static native void nativeSetSurface(Surface surface, int width, int height);

    private static native void nativeTouch(int action, float x, float y);

}
