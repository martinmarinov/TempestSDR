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
import martin.tempest.sources.TSDRSource;

/**
 * This is a Java wrapper library for TSDRLibrary
 * 
 * @author Martin
 *
 */
public class TSDRLibrary {
	
	private BufferedImage bimage;
	private volatile int[] pixels;
	
	public enum SYNC_DIRECTION {ANY, UP, DOWN, LEFT, RIGHT};
	
	volatile private boolean nativerunning = false;
	
	// If the binaries weren't loaded, this will go off
	private static TSDRLibraryNotCompatible m_e = null;
	private final List<FrameReadyCallback> callbacks = new ArrayList<FrameReadyCallback>();
	
	/**
	 * Extracts a library to a temporary path and prays for the OS to delete it after the app closes.
	 * @param name
	 * @return
	 * @throws IOException
	 */
	private static final File extractLibrary(final String name) throws TSDRLibraryNotCompatible {
		final String rawOSNAME = System.getProperty("os.name").toLowerCase();
		final String rawARCHNAME = System.getProperty("os.arch").toLowerCase();

		String OSNAME = null, EXT = null, ARCHNAME = null;

		if (rawOSNAME.contains("win")) {
			OSNAME = "WINDOWS";
			EXT = ".dll";
		} else if (rawOSNAME.contains("nix") || rawOSNAME.contains("nux") || rawOSNAME.contains("aix")) {
			OSNAME = "LINUX";
			EXT = ".so";
		} else if (rawOSNAME.contains("mac")) {
			OSNAME = "MAC";
			EXT = ".a";
		}

		if (rawARCHNAME.contains("arm"))
			ARCHNAME = "ARM";
		else if (rawARCHNAME.contains("64"))
			ARCHNAME = "X64";
		else
			ARCHNAME = "X86";

		if (OSNAME == null || EXT == null || ARCHNAME == null)
			throw new TSDRLibraryNotCompatible("Your OS or CPU is not yet supported, sorry.");

		final String relative_path = "lib/"+OSNAME+"/"+ARCHNAME+"/"+name+EXT;

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
			temp = new File(System.getProperty("java.io.tmpdir"), name+EXT);
			temp.deleteOnExit();
			final FileOutputStream fos = new FileOutputStream(temp);

			while((read = in.read(buffer)) != -1) {
				fos.write(buffer, 0, read);
			}
			fos.close();
			in.close();
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
			loadLibrary("TSDRLibrary");
			loadLibrary("TSDRLibraryNDK");
			
		} catch (TSDRLibraryNotCompatible e) {
			m_e = e;
		} 
	}
	
	public TSDRLibrary() throws TSDRException {
		if (m_e != null) throw m_e;
		init();
	}

	private native void init();
	public native void setSampleRate(long rate) throws TSDRException;
	public native void setBaseFreq(long freq) throws TSDRException;
	private native void nativeStart(String pluginfilepath, String params) throws TSDRException;
	public native void stop() throws TSDRException;
	public native void setGain(float gain) throws TSDRException;
	public native boolean isRunning();
	public native void setInvertedColors(boolean invertedEnabled);
	public native void sync(int pixels, SYNC_DIRECTION dir);
	public native void setResolution(int width, int height, double refreshrate) throws TSDRException;
	public native void setMotionBlur(float gain) throws TSDRException;
	
	public void startAsync(final TSDRSource plugin, int width, int height, double refreshrate) throws TSDRException {
		if (nativerunning) throw new TSDRAlreadyRunningException("");
		
		final String absolute_path = plugin.absolute ? plugin.libname : (extractLibrary(plugin.libname).getAbsolutePath());
		
		setResolution(width, height, refreshrate);
		
		new Thread() {
			public void run() {
				nativerunning = true;
				Runtime.getRuntime().addShutdownHook(unloaderhook);
				try {
					nativeStart(absolute_path, plugin.getParams());
				} catch (TSDRException e) {
					for (final FrameReadyCallback callback : callbacks) callback.onException(TSDRLibrary.this, e);
				} finally {
					try {
							Runtime.getRuntime().removeShutdownHook(unloaderhook);
					} catch (Throwable e) {};
				}
				for (final FrameReadyCallback callback : callbacks) callback.onClosed(TSDRLibrary.this);
				nativerunning = false;
			};
		}.start();
		
	}
	
	final private Thread unloaderhook = new Thread() {
		@Override
		public void run() {
			try {
				TSDRLibrary.this.stop();
			} catch (Throwable e) {}
		}
	};
	
	public boolean registerFrameReadyCallback(final FrameReadyCallback callback) {
		if (callbacks.contains(callback))
			return false;
		else
			return callbacks.add(callback);
	}
	
	public boolean unregisterFrameReadyCallback(final FrameReadyCallback callback) {
		return callbacks.remove(callback);
	}
	
	public interface FrameReadyCallback {
		void onFrameReady(final TSDRLibrary lib, final BufferedImage frame);
		void onException(final TSDRLibrary lib, final Exception e);
		void onClosed(final TSDRLibrary lib);
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
