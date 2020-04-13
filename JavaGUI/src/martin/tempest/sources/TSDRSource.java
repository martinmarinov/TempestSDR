/*******************************************************************************
 * Copyright (c) 2014 Martin Marinov.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 *     Martin Marinov - initial API and implementation
 ******************************************************************************/
package martin.tempest.sources;

import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.JLabel;
import javax.swing.JTextField;

import martin.tempest.core.TSDRLibrary;
import martin.tempest.core.exceptions.TSDRLibraryNotCompatible;

/**
 * This class allows {@link TSDRLibrary} to use various devices as SDR frontend inputs. Note that all of these are backed up 
 * by native dynamic libraries so it can be platform dependent.
 * 
 * This class attempts to provide a flexible way of managing plugins for multiple OSes and architectues.
 * 
 * @author Martin Marinov
 *
 */
public class TSDRSource {
	
	/**
	 * This is the official TSDRSource register. All available sources are to be registered here.
	 */
	private final static TSDRSource[] SOURCES = new TSDRSource[] {
		new TSDRFileSource(),
		new TSDRMiricsSource(),
		new TSDRUHDSource(),
		new TSDRExtIOSource(),
		new TSDRSDRPlaySource(),
	};
	
	/** The native name of the dynamic library. It should not contain prefixes or extensions.
	 * For example a library could be called "abc", so libname would be "abc". When the library
	 * needs to be loaded, the system will make sure it loads abs.dll on Windows or libabs.so on Linux.
	 * An exception to this rule is if the libname contains full absolute path to the dll and {@link #absolute} is set to true. */
	public final String libname;
	
	/**
	 * If true, {@link #libname} will have the full path to the file that is the native library that this source represents
	 */
	public final boolean absolute;
	
	/**
	 * A user friendly description of the library. Use this for displaying in GUI.
	 */
	public final String descr;
	
	private TSDRSourceParamChangedListener callback = null;
	private String params = "";
	
	/**
	 * Return all of the available built-in TSDRSources. 
	 * @return a list of all available TSDRSources. Some of them might be incompatible with the current user's system, but this will raise the appropriate exception
	 */
	public static TSDRSource[] getAvailableSources() {
		return SOURCES;
	}
	
	/**
	 * Create a {@link TSDRSource} on the fly. This represents a native library that {@link TSDRLibrary} will load
	 * so that it can access the input of various SDR devices. The library supplied must be compatible with {@link TSDRLibrary},
	 * otherwise it won't be loaded.
	 * @param desc a user friendly description
	 * @param libname let's assume your library is called "abc". If you supply absolute equals to false, then setting libname to "abc" will mean 
	 * the system will look for "abc.dll" on Windows or "libabc.so" on Linux, etc in the LD_LIBRARY_PATH or the java.library.path. This is the best way
	 * to be platform independent. If, however, your library is not in any of these folders, you can supply the absolute path, for example "D:\abc.dll" here and set absolute to true.
	 * NOTE however that if any dependent libraries are not in the LD_LIBRARY_PATH, this native library will fail to load no matter whether you have set absolute to true or false.
	 * @param absolute look at information about libname
	 */
	public TSDRSource(final String desc, final String libname, final boolean absolute) {
		this.descr = desc;
		this.libname = libname;
		this.absolute = absolute;
	}
	
	/**
	 * This function can be called by the plugin itself when it's ready to be loaded/reloaded. Call it only if your application does not have a GUI or really need to force
	 * usage of a certain parameter.
	 * @param params
	 */
	public void setParams(final String params) {
		this.params = params;
		if (callback != null) callback.onParametersChanged(this);
	}
	
	/**
	 * Get the most current parameters
	 * @return
	 */
	public String getParams() {
		return params;
	}
	
	@Override
	public String toString() {
		return descr;
	}
	
	/**
	 * The functions tries to find the absolute path to the native library that implements this source.
	 * It will also look into paths that are supplied with java.library.path
	 * @return a full path to the library or just {@link #libname}
	 */
	public String getAbsolutePathToLibrary() {

		if (absolute) return libname;

		try {
			return TSDRLibrary.extractLibrary(libname).getAbsolutePath();
		} catch (TSDRLibraryNotCompatible e) {
			String fullname;
			try {
				fullname = TSDRLibrary.getNativeLibraryFullName(libname);


				// if the library is not in the jar, try looking for it somewhere else
				final File direct = new File(fullname);
				if (direct.exists()) return direct.getAbsolutePath();

				final String[] paths = System.getProperty("java.library.path").split(File.pathSeparator);
				for (final String path : paths) {
					final File temp = new File(path+File.separator+fullname);
					if (temp.exists()) return temp.getAbsolutePath();
				}

			} catch (TSDRLibraryNotCompatible e1) {}
		}

		return libname;
	}
	
	/**
	 * Register a callback that will get notified as soon as the parameters are changed. The caller
	 * may want to reload the plugin in the {@link TSDRLibrary} so that the changes would take place.
	 * @param callback
	 */
	public void setOnParameterChangedCallback(final TSDRSourceParamChangedListener callback) {
		this.callback = callback;
	}
	
	/**
	 * This should be called when the plugin is to be loaded. It will set the plugin parameters correctly according to user's desire so that once the parameters are set, 
	 * it can be loaded to {@link TSDRLibrary}. In order to get a notification when the plugin is ready to be loaded to TSDRLibrary, register your callback with {@link #setOnParameterChangedCallback(TSDRSourceParamChangedListener)}
	 * @param cont the container that will receive the components
	 * @param defaultprefs the suggested parameters for this plugin. The plugin will decide what the exact value would be.
	 */
	public boolean populateGUI(final Container cont, final String defaultprefs, final ActionListenerRegistrator okbutton) {
		if (cont == null) setParams(defaultprefs);
		
	    final JLabel description = new JLabel("Type in the parameters:");
	    cont.add(description);
	    description.setBounds(12, 12, 400-12, 24);
	    
	    final JTextField params = new JTextField(defaultprefs);
	    cont.add(params);
	    params.setBounds(12, 12+32, 400-12, 24);
	    
	    okbutton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				setParams(params.getText());
			}
		});
	    
	    return true;
	}
	
	/**
	 * Interface that allows for events to be generated when a new parameter is applied to the plugin.
	 * The plugin will need to be reloaded so that they take place.
	 * 
	 * @author Martin Marinov
	 *
	 */
	public static interface TSDRSourceParamChangedListener {
		
		/**
		 * Everytime a source changes its parameters, this function will be called.
		 * A good suggestion would be to call {@link TSDRLibrary#loadPlugin}
		 * @param source the {@link TSDRSource} that was changed
		 */
		void onParametersChanged(final TSDRSource source);
	}
	
	public static interface ActionListenerRegistrator {
		public void addActionListener(ActionListener l);
	}
}
