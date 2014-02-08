package martin.tempest.sources;

import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JTextField;

import martin.tempest.core.TSDRLibrary;
import martin.tempest.core.exceptions.TSDRLibraryNotCompatible;

public class TSDRSource {
	
	private final static TSDRSource[] SOURCES = new TSDRSource[] {
		new TSDRFileSource(),
		new TSDRMiricsSource(),
		new TSDRExtIOSource(),
	};
	
	public final String libname;
	private String params = "";
	public final boolean absolute;
	public final String descr;
	private TSDRSourceParamChangedListener callback = null;
	
	public static TSDRSource[] getAvailableSources() {
		return SOURCES;
	}
	
	TSDRSource(final String desc, final String libname, final boolean absolute) {
		this.descr = desc;
		this.libname = libname;
		this.absolute = absolute;
	}
	
	public void setParams(final String params) {
		this.params = params;
		if (callback != null) callback.onParametersChanged(this);
	}
	
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
	 * it can be loaded to {@link TSDRLibrary}
	 * @param cont the container that will receive the components
	 * @param defaultprefs the default parameters stored for this plugin. The plugin can use them to build the GUI.
	 * @return true if GUI was populated so that the user can input parameters manually
	 * 			false if the plugin does not require manual adjustements or the default preferences are ok, the caller than needs to call {@link #setParams(String)}
	 */
	public boolean populateGUI(final Container cont, final String defaultprefs) {
	    final JLabel description = new JLabel("Type in the parameters:");
	    cont.add(description);
	    description.setBounds(12, 12, 400-12, 24);
	    
	    final JTextField params = new JTextField(defaultprefs);
	    cont.add(params);
	    params.setBounds(12, 12+32, 400-12, 24);
	    
	    final JButton ok = new JButton("OK");
	    cont.add(ok);
	    ok.setBounds(12, 12+2*32, 84, 24);
	    
	    ok.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				new Thread() {
					public void run() {
						ok.setEnabled(false);
						setParams(params.getText());
						ok.setEnabled(true);
					};
				}.start();
			}
		});
		return true;
	}
	
	/**
	 * Interface that allows for events to be generated when a new parameter is applied to the plugin.
	 * The plugin will need to be reloaded so that they take place.
	 * 
	 * @author Martin
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
}
