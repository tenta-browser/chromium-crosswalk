// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

/**
 * Handles extracting the necessary resources bundled in an APK and moving them to a location on
 * the file system accessible from the native code.
 */
public class ResourceExtractor {
    private static final String TAG = "base";
    private static final String ICU_DATA_FILENAME = "icudtl.dat";
    private static final String V8_NATIVES_DATA_FILENAME = "natives_blob.bin";
    private static final String V8_SNAPSHOT_DATA_FILENAME = "snapshot_blob.bin";
    private static final String APP_VERSION_PREF = "org.chromium.base.ResourceExtractor.Version";
    public static final String FALLBACK_LOCALE = "en-US";

     private static ResourceInterceptor sInterceptor = null;

    public interface ResourceInterceptor {
        public boolean shouldInterceptLoadRequest(String resource);
        public InputStream openRawResource(String resource);
    }

    private static boolean isAppDataFile(String file) {
        return ICU_DATA_FILENAME.equals(file)
                || V8_NATIVES_DATA_FILENAME.equals(file)
                || V8_SNAPSHOT_DATA_FILENAME.equals(file);
    }

     /**
     * Allow embedders to intercept the resource loading process. Embedders may
     * want to load paks from res/raw instead of assets, since assets are not
     * supported in Android library project.
     * @param intercepter The instance of intercepter which provides the files list
     * to intercept and the inputstream for the files it wants to intercept with.
     */
    public static void setResourceInterceptor(ResourceInterceptor interceptor) {
        assert (sInstance == null || sInstance.mExtractTask == null)
                : "Must be called before startExtractingResources is called";
        sInterceptor = interceptor;
    }

    private class ExtractTask extends AsyncTask<Void, Void, Void> {
        private static final int BUFFER_SIZE = 16 * 1024;

        private final List<Runnable> mCompletionCallbacks = new ArrayList<Runnable>();

        private void extractResourceHelper(InputStream is, File outFile, byte[] buffer)
                throws IOException {
            OutputStream os = null;
            File tmpOutputFile = new File(outFile.getPath() + ".tmp");
            try {
                os = new FileOutputStream(tmpOutputFile);
                Log.i(TAG, "Extracting resource %s", outFile);

                int count = 0;
                while ((count = is.read(buffer, 0, BUFFER_SIZE)) != -1) {
                    os.write(buffer, 0, count);
                }
            } finally {
                StreamUtil.closeQuietly(os);
                StreamUtil.closeQuietly(is);
            }
            if (!tmpOutputFile.renameTo(outFile)) {
                throw new IOException();
            }
        }

        private void doInBackgroundImpl() {
            Log.d(TAG, "ResourceExtractor running .... %s", Arrays.toString(mAssetsToExtract));
            final File outputDir = getOutputDir();
            final File appDataDir = getAppDataDir();

            if (!outputDir.exists() && !outputDir.mkdirs()) {
                throw new RuntimeException();
            }

            // Use a suffix for extracted files in order to guarantee that the version of the file
            // on disk matches up with the version of the APK.
            String extractSuffix = BuildInfo.getExtractedFileSuffix();
            String[] existingFileNames = outputDir.list();
            boolean allFilesExist = existingFileNames != null;
            if (allFilesExist) {
                List<String> existingFiles = Arrays.asList(existingFileNames);
                for (String assetName : mAssetsToExtract) {
                    allFilesExist &= existingFiles.contains(assetName + extractSuffix);
                }
            }
            // This is the normal case.
            if (allFilesExist) {
                return;
            }
            // A missing file means Chrome has updated. Delete stale files first.
            deleteFiles(existingFileNames);

            byte[] buffer = new byte[BUFFER_SIZE];
            for (String assetName : mAssetsToExtract) {
                File dir = isAppDataFile(assetName) ? appDataDir : outputDir;
                File output = new File(dir, assetName + extractSuffix);
                TraceEvent.begin("ExtractResource");
                try {
                    InputStream inputStream;
                    if (sInterceptor != null
                            && sInterceptor.shouldInterceptLoadRequest(assetName)) {
                        inputStream = sInterceptor.openRawResource(assetName);
                    } else {
                        inputStream = ContextUtils.getApplicationContext().getAssets().open(assetName);
                    }
                    extractResourceHelper(inputStream, output, buffer);
                } catch (IOException e) {
                    // The app would just crash later if files are missing.
                    throw new RuntimeException(e);
                } finally {
                    TraceEvent.end("ExtractResource");
                }
            }
        }

        @Override
        protected Void doInBackground(Void... unused) {
            TraceEvent.begin("ResourceExtractor.ExtractTask.doInBackground");
            try {
                doInBackgroundImpl();
            } finally {
                TraceEvent.end("ResourceExtractor.ExtractTask.doInBackground");
            }
            return null;
        }

        private void onPostExecuteImpl() {
            for (int i = 0; i < mCompletionCallbacks.size(); i++) {
                mCompletionCallbacks.get(i).run();
            }
            mCompletionCallbacks.clear();
        }

        @Override
        protected void onPostExecute(Void result) {
            TraceEvent.begin("ResourceExtractor.ExtractTask.onPostExecute");
            try {
                onPostExecuteImpl();
            } finally {
                TraceEvent.end("ResourceExtractor.ExtractTask.onPostExecute");
            }
        }

        /** Returns a number that is different each time the apk changes. */
        private long getApkVersion() {
            PackageManager pm = ContextUtils.getApplicationContext().getPackageManager();
            try {
                // Use lastUpdateTime since versionCode does not change when developing locally,
                // but also use versionCode since it is possible for Chrome to be updated without
                // the lastUpdateTime being changed (http://crbug.org/673458).
                PackageInfo pi =
                        pm.getPackageInfo(ContextUtils.getApplicationContext().getPackageName(), 0);
                // Xor'ing versionCode into upper half of the long to ensure it doesn't somehow
                // exactly offset an increase in time.
                return pi.lastUpdateTime ^ (((long) pi.versionCode) << 32);
            } catch (PackageManager.NameNotFoundException e) {
                throw new RuntimeException(e);
            }
        }
    }

    private ExtractTask mExtractTask;
    private static String[] mAssetsToExtract = detectFilesToExtract();

    private static ResourceExtractor sInstance;

    public static ResourceExtractor get() {
        if (sInstance == null) {
            sInstance = new ResourceExtractor();
        }
        return sInstance;
    }

    private static String[] detectFilesToExtract() {
        Locale defaultLocale = Locale.getDefault();
        String language = LocaleUtils.getUpdatedLanguageForChromium(defaultLocale.getLanguage());
        // Currenty (Oct 2016), this array can be as big as 4 entries, so using a capacity
        // that allows a bit of growth, but is still in the right ballpark..
        ArrayList<String> activeLocalePakFiles = new ArrayList<String>(6);
        for (String locale : BuildConfig.COMPRESSED_LOCALES) {
            if (locale.startsWith(language)) {
                activeLocalePakFiles.add(locale + ".pak");
            }
        }
        if (activeLocalePakFiles.isEmpty() && BuildConfig.COMPRESSED_LOCALES.length > 0) {
            assert Arrays.asList(BuildConfig.COMPRESSED_LOCALES).contains(FALLBACK_LOCALE);
            activeLocalePakFiles.add(FALLBACK_LOCALE + ".pak");
        }
        return activeLocalePakFiles.toArray(new String[activeLocalePakFiles.size()]);
    }

    /**
     * Specifies the files that should be extracted from the APK.
     * and moved to {@link #getOutputDir()}.
     */
    public static void setResourcesToExtract(String[] entries) {
        assert (sInstance == null || sInstance.mExtractTask == null)
                : "Must be called before startExtractingResources is called";
        mAssetsToExtract = entries;
    }

    /**
     * Synchronously wait for the resource extraction to be completed.
     * <p>
     * This method is bad and you should feel bad for using it.
     *
     * @see #addCompletionCallback(Runnable)
     */
    public void waitForCompletion() {
        if (shouldSkipPakExtraction()) {
            return;
        }

        assert mExtractTask != null;

        try {
            mExtractTask.get();
        } catch (Exception e) {
            assert false;
        }
    }

    /**
     * Adds a callback to be notified upon the completion of resource extraction.
     * <p>
     * If the resource task has already completed, the callback will be posted to the UI message
     * queue.  Otherwise, it will be executed after all the resources have been extracted.
     * <p>
     * This must be called on the UI thread.  The callback will also always be executed on
     * the UI thread.
     *
     * @param callback The callback to be enqueued.
     */
    public void addCompletionCallback(Runnable callback) {
        ThreadUtils.assertOnUiThread();

        Handler handler = new Handler(Looper.getMainLooper());
        if (shouldSkipPakExtraction()) {
            handler.post(callback);
            return;
        }

        assert mExtractTask != null;
        assert !mExtractTask.isCancelled();
        if (mExtractTask.getStatus() == AsyncTask.Status.FINISHED) {
            handler.post(callback);
        } else {
            mExtractTask.mCompletionCallbacks.add(callback);
        }
    }

    /**
     * This will extract the application pak resources in an
     * AsyncTask. Call waitForCompletion() at the point resources
     * are needed to block until the task completes.
     */
    public void startExtractingResources() {
        if (mExtractTask != null) {
            return;
        }

        // If a previous release extracted resources, and the current release does not,
        // deleteFiles() will not run and some files will be left. This currently
        // can happen for ContentShell, but not for Chrome proper, since we always extract
        // locale pak files.
        if (shouldSkipPakExtraction()) {
            return;
        }

        mExtractTask = new ExtractTask();
        mExtractTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private File getAppDataDir() {
        return new File(PathUtils.getDataDirectory());
    }

    private File getOutputDir() {
        return new File(getAppDataDir(), "paks");
    }

    private static void deleteFile(File file) {
        if (file.exists() && !file.delete()) {
            Log.w(TAG, "Unable to remove %s", file.getName());
        }
    }

    private void deleteFiles(String[] existingFileNames) {
        // These used to be extracted, but no longer are, so just clean them up.
        deleteFile(new File(getAppDataDir(), ICU_DATA_FILENAME));
        deleteFile(new File(getAppDataDir(), V8_NATIVES_DATA_FILENAME));
        deleteFile(new File(getAppDataDir(), V8_SNAPSHOT_DATA_FILENAME));
        if (existingFileNames != null) {
            for (String fileName : existingFileNames) {
                deleteFile(new File(getOutputDir(), fileName));
            }
        }
    }

    /**
     * Pak extraction not necessarily required by the embedder.
     */
    private static boolean shouldSkipPakExtraction() {
        return get().mAssetsToExtract.length == 0;
    }
}
