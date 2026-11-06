package com.github.Keriew.augustus;

import android.app.Activity;
import android.app.Presentation;
import android.content.Context;
import android.graphics.Color;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.animation.DecelerateInterpolator;
import android.widget.RelativeLayout;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.TextInputScreen;

/**
 * The second screen, such as the lower screen of the AYN Thor: a window of its own,
 * whose surface the game draws into and whose touches it reads.
 *
 * A touch there gives that screen the focus. The keys and the controller then come to this window,
 * which passes them on to the game, and the next keyboard opens on this screen.
 */
class BottomScreen implements SurfaceHolder.Callback, View.OnTouchListener, View.OnKeyListener {
    private static final float KEYBOARD_SHADE_ALPHA = 0.6f;
    private static final int KEYBOARD_SHADE_MILLIS = 250;

    private Presentation presentation;
    private RelativeLayout layout;

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
        // The keyboard covers the panel, rather than shrinking the surface and so the panel's scale
        presentation.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING);
        Context context = presentation.getContext();
        layout = new RelativeLayout(context);
        layout.setFocusable(true);
        layout.setFocusableInTouchMode(true);
        layout.setOnKeyListener(this);
        layout.setOnGenericMotionListener(TextInputScreen.motionListener());
        SurfaceView view = new SurfaceView(context);
        view.getHolder().addCallback(this);
        view.setOnTouchListener(this);
        layout.addView(view, new RelativeLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        followKeyboard(context);
        presentation.setContentView(layout);
        presentation.show();
        layout.requestFocus();
    }

    void hide() {
        if (presentation != null) {
            presentation.dismiss();
            presentation = null;
            layout = null;
        }
    }

    /**
     * Darkens the panel while the keyboard is open on this screen, so the keyboard stands out,
     * and closes the keyboard once the game is done with it.
     */
    private void followKeyboard(Context context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            return;
        }
        View shade = new View(context);
        shade.setBackgroundColor(Color.BLACK);
        shade.setAlpha(0f);
        layout.addView(shade, new RelativeLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        Window window = presentation.getWindow();
        window.setDecorFitsSystemWindows(false);
        window.getDecorView().setOnApplyWindowInsetsListener((view, insets) -> {
            boolean isKeyboardShown = insets.isVisible(WindowInsets.Type.ime());
            shade.animate()
                .alpha(isKeyboardShown ? KEYBOARD_SHADE_ALPHA : 0f)
                .setDuration(KEYBOARD_SHADE_MILLIS)
                .setInterpolator(new DecelerateInterpolator());
            return view.onApplyWindowInsets(insets);
        });
        ViewGroup root = layout;
        root.getViewTreeObserver().addOnGlobalLayoutListener(() -> {
            if (TextInputScreen.isKeyboardShown()) {
                return;
            }
            // SDL closes the keyboard through the game's own window, which cannot close it on another screen
            WindowInsets insets = window.getDecorView().getRootWindowInsets();
            if (insets != null && insets.isVisible(WindowInsets.Type.ime())) {
                window.getInsetsController().hide(WindowInsets.Type.ime());
            }
            // While SDL's editor keeps the focus, the keyboard service takes the keys before this window
            if (root.findFocus() != root) {
                root.requestFocus();
            }
        });
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
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            TextInputScreen.moveTo(layout);
        }
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
