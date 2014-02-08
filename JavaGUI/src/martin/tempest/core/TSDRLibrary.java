package martin.tempest.core;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import martin.tempest.core.exceptions.TSDRAlreadyRunningException;
import martin.tempest.core.exceptions.TSDRException;
import martin.tempest.core.exceptions.TSDRLibraryNotCompatible;
import martin.tempest.gui.VideoMode;
import martin.tempest.sources.TSDRSource;

/**
 * This is a Java wrapper library for TSDRLibrary. It aims to be platform independent as long as
 * the library is compiled for the corresponding OS and the native dlls are located in lib/OSNAME/ARCH or in
 * LD_LIBRARY_PATH. It controls the native library and exposes its full functionality.
 * 
 * @author Martin Marinov
 *
 */
public class TSDRLibrary {
	
	/** The image that will be have its pixels written by the NDK */
	private BufferedImage bimage;
	/** A pointer to the pixels of {@link #bimage} */
	private volatile int[] pixels;
	
	/** The desired direction of manual synchronisation */
	public enum SYNC_DIRECTION {ANY, UP, DOWN, LEFT, RIGHT};
	
	/** Whether native is running or not */
	volatile private boolean nativerunning = false;
	
	/** If the binaries weren't loaded, this will go off */
	private static TSDRLibraryNotCompatible m_e = null;
	
	/** A list of all of the callbacks that will receive a frame when it is ready */
	private final List<FrameReadyCallback> callbacks = new ArrayList<FrameReadyCallback>();
	
	/**
	 * Returns a OS specific library filename. If you supply "abc" on Windows, this function
	 * will return abc.dll. On Linux this will return libabc.so, etc.
	 * @param name
	 * @throws TSDRLibraryNotCompatible if current OS not supported
	 * @return
	 */
	public static final String getNativeLibraryFullName(final String name) throws TSDRLibraryNotCompatible {
		final String rawOSNAME = System.getProperty("os.name").toLowerCase();
		String EXT = null, LIBPREFIX = "";

		if (rawOSNAME.contains("win"))
			EXT = ".dll";
		else if (rawOSNAME.contains("nix") || rawOSNAME.contains("nux") || rawOSNAME.contains("aix")) {
			EXT = ".so";
			LIBPREFIX = "lib";
		} else if (rawOSNAME.contains("mac")) {
			EXT = ".a";
		}

		if (EXT == null)
			throw new TSDRLibraryNotCompatible("Your OS or CPU is not yet supported, sorry.");
		
		return LIBPREFIX+name+EXT;
	}
	
	/**
	 * Extracts a library to a temporary path and prays for the OS to delete it after the app closes.
	 * @param name
	 * @return
	 * @throws TSDRLibraryNotCompatible
	 */
	public static final File extractLibrary(final String name) throws TSDRLibraryNotCompatible {


		final String rawOSNAME = System.getProperty("os.name").toLowerCase();
		final String rawARCHNAME = System.getProperty("os.arch").toLowerCase();
		String OSNAME = null, ARCHNAME = null;
		final String dllfullfilename = getNativeLibraryFullName(name);

		if (rawOSNAME.contains("win"))
			OSNAME = "WINDOWS";
		else if (rawOSNAME.contains("nix") || rawOSNAME.contains("nux") || rawOSNAME.contains("aix"))
			OSNAME = "LINUX";
		else if (rawOSNAME.contains("mac"))
			OSNAME = "MAC";

		if (rawARCHNAME.contains("arm"))
			ARCHNAME = "ARM";
		else if (rawARCHNAME.contains("64"))
			ARCHNAME = "X64";
		else
			ARCHNAME = "X86";
		
		if (OSNAME == null || ARCHNAME == null)
			throw new TSDRLibraryNotCompatible("Your OS or CPU is not yet supported, sorry.");

		final String relative_path = "lib/"+OSNAME+"/"+ARCHNAME+"/"+dllfullfilename;

		InputStream in = TSDRLibrary.class.getClassLoader().getResourceAsStream(relative_path);

		if (in == null)
			try {
				in = new FileInputStream(relative_path);
			} catch (FileNotFoundException e) {}

		if (in == null) throw new TSDRLibraryNotCompatible("The library has not been compiled for your OS/Architecture yet ("+OSNAME+"/"+ARCHNAME+").");

		File temp;
		try {
			byte[] buffer = new byte[in.available()];

			int read = -1;
			temp = new File(System.getProperty("java.io.tmpdir"), dllfullfilename);
			temp.deleteOnExit();
			final FileOutputStream fos = new FileOutputStream(temp);

			while((read = in.read(buffer)) != -1) {
				fos.write(buffer, 0, read);
			}
			fos.close();
			in.close();
			
			if (!temp.exists())
				throw new TSDRLibraryNotCompatible("Cannot extract library files to temporary directory.");
		} catch (IOException e) {
			throw new TSDRLibraryNotCompatible(e);
		}

		return temp;
	}

	/**
	 * Loads a dll library on the fly based on OS and ARCH. Do not supply extension.
	 * @param name
	 * @throws IOException 
	 */
	private static final void loadLibrary(final String name) throws TSDRLibraryNotCompatible {
		try {
			// try traditional method
			System.loadLibrary(name); 
		} catch (Throwable t) {
				final File library = extractLibrary(name);
				System.load(library.getAbsolutePath());
				library.delete();
		}
	}

	/**
	 * Load the libraries statically and detect errors
	 */
	static {
		try {
			loadLibrary("TSDRLibraryNDK");
		} catch (TSDRLibraryNotCompatible e) {
			m_e = e;
		} 
	}
	
	/**
	 * Initializes the library by attempting to load the required underlying native binaries.
	 * @throws TSDRException
	 */
	public TSDRLibrary() throws TSDRException {
		if (m_e != null) throw m_e;
		init();
	}

	/** Called on initialisation to set up native buffers and variables */
	private native void init();
	
	/** Set the sample rate on the fly */
	public native void setSampleRate(long rate) throws TSDRException;
	
	/** Set the tuned frequency */
	public native void setBaseFreq(long freq) throws TSDRException;
	
	/**
	 * Load a plugin so that it is ready to be started via {@link #nativeStart()}
	 * @param pluginfilepath
	 * @param params
	 * @throws TSDRException
	 */
	private native void loadPlugin(String pluginfilepath, String params) throws TSDRException;
	
	/**
	 * Starts the processing. After this point on, either an exception will be genrated or the callback
	 * will start receiving frames. This function DOES block and run on the same thread as the caller. 
	 * {@link #startAsync(int, int, double)} uses it so one needs to use {@link #startAsync(int, int, double)} instead of calling this directly.
	 * @throws TSDRException
	 */
	private native void nativeStart() throws TSDRException;
	
	/**
	 * Stop the processing that was started with {@link #nativeStart()} before.
	 * @throws TSDRException
	 */
	public native void stop() throws TSDRException;
	
	/**
	 * Unloads and frees any resources taken by the native plugin that was previously loaded with {@link #loadPlugin(TSDRSource)}
	 * @throws TSDRException
	 */
	public native void unloadPlugin() throws TSDRException;
	
	/**
	 * Set gain level
	 * @param gain from 0 to 1
	 * @throws TSDRException
	 */
	public native void setGain(float gain) throws TSDRException;
	
	/**
	 * Check whether the library is running
	 * @return true if {@link #nativeStart()} is still working.
	 */
	public native boolean isRunning();
	public native void setInvertedColors(boolean invertedEnabled);
	public native void sync(int pixels, SYNC_DIRECTION dir);
	public native void setResolution(int width, int height, double refreshrate) throws TSDRException;
	public native void setMotionBlur(float gain) throws TSDRException;
	
	/** Releases any resources taken by the native library. Unless {@link #init()} is called again, any call to {@link TSDRLibrary} can have unexpected
	 * result and may crash the JVM */
	public native void free();
	
	/**
	 * Loads the plugin. It is always safe to do {@link #unloadPlugin()} first before calling this.
	 * Note: If the plugin that you are trying to load is already in use, unload it first otherwise 
	 * you might receive an exception.
	 * @param plugin the plugin to be loaded
	 * @throws TSDRException
	 */
	public void loadPlugin(final TSDRSource plugin) throws TSDRException {
		loadPlugin(plugin.getAbsolutePathToLibrary(), plugin.getParams());
	}
	
	/**
	 * Starts the whole system. After this function is called, assuming there are no exceptions,
	 * the callbacks registered via {@link #registerFrameReadyCallback(FrameReadyCallback)} will start
	 * receiving video frames.
	 * Note: width and height are not what you might think. For example for 800x600@60Hz resolution, width and height might be 1056x628. Consult {@link VideoMode} for more information.
	 * @param width the desired total width
	 * @param height the desired total height
	 * @param refreshrate the desired refreshrate
	 * @throws TSDRException
	 */
	public void startAsync(int width, int height, double refreshrate) throws TSDRException {
		if (nativerunning) throw new TSDRAlreadyRunningException("");
		
		setResolution(width, height, refreshrate);
		
		new Thread() {
			public void run() {
				nativerunning = true;
				try {
				Runtime.getRuntime().removeShutdownHook(unloaderhook);
				} catch (Throwable e) {};
				
				Runtime.getRuntime().addShutdownHook(unloaderhook);
				try {
					nativeStart();
				} catch (TSDRException e) {
					for (final FrameReadyCallback callback : callbacks) callback.onException(TSDRLibrary.this, e);
				}
				
				for (final FrameReadyCallback callback : callbacks) callback.onStopped(TSDRLibrary.this);
				nativerunning = false;
			};
		}.start();
		
	}
	
	/**
	 * Unloads the native libraries just before the JVM closes.
	 */
	final private Thread unloaderhook = new Thread() {
		@Override
		public void run() {
			try {
				TSDRLibrary.this.unloadPlugin();
			} catch (Throwable e) {}
			try {
				TSDRLibrary.this.stop();
			} catch (Throwable e) {}
			try {
				TSDRLibrary.this.free();
			} catch (Throwable e) {}
		}
	};
	
	@Override
	protected void finalize() throws Throwable {
		try {
		Runtime.getRuntime().removeShutdownHook(unloaderhook);
		} catch (Throwable e) {};
		unloaderhook.run();
		super.finalize();
	}
	
	/**
	 * Register the callback that will receive the decoded video frames
	 * @param callback
	 * @return true on success, false if the callback is already registered
	 */
	public boolean registerFrameReadyCallback(final FrameReadyCallback callback) {
		if (callbacks.contains(callback))
			return false;
		else
			return callbacks.add(callback);
	}
	
	/**
	 * Unregister a callback previously registered with {@link #unregisterFrameReadyCallback(FrameReadyCallback)}
	 * @param callback
	 * @return true if it was removed, false if it was never registered, therefore not removed
	 */
	public boolean unregisterFrameReadyCallback(final FrameReadyCallback callback) {
		return callbacks.remove(callback);
	}
	
	/**
	 * This interface will allow you to asynchronously receive information about video frames and exceptions from the library.
	 * 
	 * @author Martin Marinov
	 *
	 */
	public interface FrameReadyCallback {
		
		/**
		 * When a new video frame was generated, this callback will trigger giving you an oportunity to 
		 * save it or show it to the user.
		 * This method should return quickly otherwise you risk dropping frames.
		 * After this method terminates, the {@link BufferedImage} is not guaranteed to be the same or indeed thread safe.
		 * This is because as soon as this function terminates, the system will start painting the next frame onto the same image.
		 * @param lib
		 * @param frame
		 */
		void onFrameReady(final TSDRLibrary lib, final BufferedImage frame);
		
		/**
		 * If any exception was generated while the library was running
		 * @param lib
		 * @param e
		 */
		void onException(final TSDRLibrary lib, final Exception e);
		
		/**
		 * If the {@link TSDRLibrary#stop()} was called.
		 * @param lib
		 */
		void onStopped(final TSDRLibrary lib);
	}
	
	/**
	 * The native code should call this method to initialize or resize the buffer array before accessing it
	 * @param x the width of the frame
	 * @param y the height of the frame
	 */
	private void fixSize(final int x, final int y) {
		if (bimage == null || bimage.getWidth() != x || bimage.getHeight() != y) {
			try {
				bimage = new BufferedImage(x, y, BufferedImage.TYPE_INT_RGB);
				pixels = ((DataBufferInt) bimage.getRaster().getDataBuffer()).getData();
			} catch (Throwable t) {
				t.printStackTrace();
			}
		}
	}
	
	/**
	 * The native code should invoke this method when it has written data to the buffer variable.
	 * This method writes the result into the bitmap
	 */
	private void notifyCallbacks() {
		for (final FrameReadyCallback callback : callbacks) callback.onFrameReady(this, bimage);
	}
	
}
