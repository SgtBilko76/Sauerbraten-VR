package org.sauerquest.vr;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

// Lifecycle/JNI bridge Activity, structurally adapted from QuakeQuest's
// GLES3JNIActivity.java (github.com/Team-Beef-Studios/QuakeQuest, GPLv2).
// Phase 1 trims out asset-copying and storage-permission handling, which
// belong to Phase 9 once there is real Sauerbraten game data to package.
@SuppressLint("SdCardPath")
public class SauerQuestActivity extends Activity implements SurfaceHolder.Callback
{
	private static final String TAG = "SauerQuest";

	static {
		try {
			System.loadLibrary("openxr_loader");
		} catch (Throwable e) {
			Log.w(TAG, "openxr_loader not preloaded (expected: it's dlopen'd at runtime)", e);
		}
		System.loadLibrary("sauerquest");
	}

	private SurfaceHolder mSurfaceHolder;
	private long mNativeHandle;

	// SDL2's Android backend (third_party/SDL2/src/core/android/SDL_android.c)
	// resolves a handful of platform services (storage paths, clipboard,
	// toast, etc.) via static methods it looks up by name on whatever
	// Activity class calls its nativeSetupJNI(). Upstream SDL apps get this
	// for free because they subclass org.libsdl.app.SDLActivity, which
	// implements the full set; this port uses its own Activity instead, so
	// nativeSetupJNI() is invoked directly from native code (see
	// vr_glue/sauerquest_vr_bootstrap.c's onCreate handler) against this
	// class. getContext() is the one static method actually exercised by
	// this port's code paths so far (SDL_AndroidGetInternalStoragePath(),
	// needed by every relative-path file load) -- the many other lookups
	// SDL_android.c performs against methods this class doesn't implement
	// are expected to fail (SDL_android.c's nativeSetupJNI clears the
	// resulting exceptions instead of leaving them pending).
	private static SauerQuestActivity sInstance;
	public static android.content.Context getContext() { return sInstance; }

	// Estado térmico del SO (API 29+, minSdk es 29): 0 normal, 1 leve, 2 moderado,
	// 3 severo, 4 crítico, 5 emergencia, 6 apagado. -1 si no hay datos. Lo lee la
	// telemetría (therm=) desde vr_glue; se llama una vez por segundo.
	public static int thermalStatus() {
		try {
			if (sInstance == null) return -1;
			android.os.PowerManager pm = (android.os.PowerManager) sInstance.getSystemService(android.content.Context.POWER_SERVICE);
			return pm != null ? pm.getCurrentThermalStatus() : -1;
		} catch (Throwable e) {
			return -1;
		}
	}

	@Override protected void onCreate(Bundle icicle)
	{
		Log.v(TAG, "SauerQuestActivity::onCreate()");
		super.onCreate(icicle);

		sInstance = this;

		// Sauerbraten's own file I/O (src/shared/stream.cpp's openfile())
		// uses plain fopen() with paths relative to the process's working
		// directory, not SDL_RWFromFile() -- so it has no idea APK assets
		// exist at all (only SDL2_image's IMG_Load, used for texture
		// loading, goes through SDL_RWFromFile's asset-manager fallback).
		// Extract the bundled data/packages assets to internal storage
		// once at startup, then chdir() there in native code (see
		// vr_glue/sauerquest_vr_bootstrap.c's AppThreadFunction, right
		// before it calls android_sauer_main()) so every relative fopen()
		// the engine does just works against real files.
		//
		// extractAssetsIfNeeded() skips this entirely on a plain relaunch
		// of the same installed version -- copyAssetTree() itself always
		// overwrites unconditionally with no existence check, which was
		// fine at the original ~100MB curated bundle but became a real
		// multi-minute stall on every single launch once the bundle grew
		// to include every stock map/sound (~1GB+).
		extractAssetsIfNeeded();

		SurfaceView view = new SurfaceView(this);
		setContentView(view);
		view.getHolder().addCallback(this);

		// org.libsdl.app.SDLAudioManager (AudioTrack-based playback glue
		// for SDL2's Android audio backend) needs this same
		// initialize()/setContext()/nativeSetupJNI() sequence stock SDL
		// apps get from SDL.java's setupJNI()/initialize() -- this port
		// has no such helper, so it's called directly here, before the
		// native engine thread starts (SauerQuestJNILib.onCreate() below)
		// and can reach initsound()'s SDL_InitSubSystem(SDL_INIT_AUDIO)/
		// Mix_OpenAudio() calls.
		org.libsdl.app.SDLAudioManager.initialize();
		org.libsdl.app.SDLAudioManager.setContext(this);
		org.libsdl.app.SDLAudioManager.nativeSetupJNI();

		mNativeHandle = SauerQuestJNILib.onCreate(this);
	}

	// Marker file recording which app versionCode's assets are currently
	// extracted at getFilesDir() -- lets a plain relaunch of the same
	// installed version skip re-copying entirely (copyAssetTree() itself
	// has no existence check, so without this every launch re-copied the
	// full bundle, harmless at the original ~100MB curated set but a
	// real multi-minute stall once it grew to include every stock
	// map/sound, ~1GB+). A new install/update (different versionCode)
	// always re-extracts, so asset changes always take effect.
	private static final String ASSETS_VERSION_MARKER = ".assets_version";

	private void extractAssetsIfNeeded()
	{
		int currentVersion = 0;
		try {
			currentVersion = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
		} catch (android.content.pm.PackageManager.NameNotFoundException e) {
			Log.e(TAG, "Could not read own versionCode; extracting assets unconditionally", e);
		}

		File marker = new File(getFilesDir(), ASSETS_VERSION_MARKER);
		String markedVersion = null;
		if (marker.exists()) {
			try (java.io.BufferedReader r = new java.io.BufferedReader(new java.io.FileReader(marker))) {
				markedVersion = r.readLine();
			} catch (IOException e) {
				Log.w(TAG, "Could not read assets version marker", e);
			}
		}

		if (markedVersion != null && markedVersion.equals(String.valueOf(currentVersion))) {
			Log.v(TAG, "Assets already extracted for versionCode " + currentVersion + ", skipping copy");
			return;
		}

		Log.v(TAG, "Extracting bundled assets (versionCode " + currentVersion + ")...");
		copyAssetTree("data", getFilesDir());
		copyAssetTree("packages", getFilesDir());

		try (java.io.FileWriter w = new java.io.FileWriter(marker)) {
			w.write(String.valueOf(currentVersion));
		} catch (IOException e) {
			// Non-fatal: assets did extract, just won't skip next launch.
			Log.e(TAG, "Could not write assets version marker -- assets will re-extract every launch", e);
		}
	}

	// Recursively copies assetSubdir (and everything under it) from the
	// APK's assets/ into destDir/assetSubdir. AssetManager.list() (unlike
	// the native AAssetManager C API) reliably enumerates subdirectories,
	// which is why this runs on the Java side rather than in native code.
	private void copyAssetTree(String assetSubdir, File destDir)
	{
		AssetManager assets = getAssets();
		try {
			String[] entries = assets.list(assetSubdir);
			if (entries == null || entries.length == 0) {
				// A leaf file, not a directory: list() returns empty for those.
				File outFile = new File(destDir, assetSubdir);
				outFile.getParentFile().mkdirs();
				try (InputStream in = assets.open(assetSubdir);
				     OutputStream out = new FileOutputStream(outFile)) {
					byte[] buf = new byte[64 * 1024];
					int n;
					while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
				}
				return;
			}
			for (String entry : entries) {
				copyAssetTree(assetSubdir + "/" + entry, destDir);
			}
		} catch (IOException e) {
			Log.e(TAG, "copyAssetTree failed for " + assetSubdir, e);
		}
	}

	@Override protected void onStart()
	{
		super.onStart();
		if (mNativeHandle != 0) SauerQuestJNILib.onStart(mNativeHandle, this);
	}

	@Override protected void onResume()
	{
		super.onResume();
		if (mNativeHandle != 0) SauerQuestJNILib.onResume(mNativeHandle);
	}

	@Override protected void onPause()
	{
		if (mNativeHandle != 0) SauerQuestJNILib.onPause(mNativeHandle);
		super.onPause();
	}

	@Override protected void onStop()
	{
		if (mNativeHandle != 0) SauerQuestJNILib.onStop(mNativeHandle);
		super.onStop();
	}

	@Override protected void onDestroy()
	{
		if (mSurfaceHolder != null && mNativeHandle != 0) {
			SauerQuestJNILib.onSurfaceDestroyed(mNativeHandle);
		}
		if (mNativeHandle != 0) SauerQuestJNILib.onDestroy(mNativeHandle);
		org.libsdl.app.SDLAudioManager.release(this);
		super.onDestroy();
		mNativeHandle = 0;
	}

	@Override public void surfaceCreated(SurfaceHolder holder)
	{
		if (mNativeHandle != 0) {
			SauerQuestJNILib.onSurfaceCreated(mNativeHandle, holder.getSurface());
			mSurfaceHolder = holder;
		}
	}

	@Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
	{
		if (mNativeHandle != 0) {
			SauerQuestJNILib.onSurfaceChanged(mNativeHandle, holder.getSurface());
			mSurfaceHolder = holder;
		}
	}

	@Override public void surfaceDestroyed(SurfaceHolder holder)
	{
		if (mNativeHandle != 0) {
			SauerQuestJNILib.onSurfaceDestroyed(mNativeHandle);
			mSurfaceHolder = null;
		}
	}

	// Called from native VR_Shutdown() via JNI CallVoidMethod.
	public void shutdown()
	{
		finishAffinity();
		System.exit(0);
	}
}
