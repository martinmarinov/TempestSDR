package martin.tempest.sources;

import java.awt.Frame;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JTextField;

import martin.tempest.core.TSDRLibrary;
import martin.tempest.core.exceptions.TSDRLibraryNotCompatible;

public class TSDRSource {
	
	private final static TSDRSource[] SOURCES = new TSDRSource[] {
		new TSDRFileSource(""),
		new TSDRMiricsSource(),
		new TSDRExtIOSource(""),
	};
	
	public final String libname;
	private String params;
	public final boolean absolute;
	public final String descr;
	private TSDRSourceParamChangedListener callback = null;
	
	public static TSDRSource[] getAvailableSources() {
		return SOURCES;
	}
	
	TSDRSource(final String desc, final String libname, final String params, final boolean absolute) {
		this.descr = desc;
		this.libname = libname;
		this.params = params;
		this.absolute = absolute;
	}
	
	public void setParams(final String params) {
		this.params = params;
		if (callback != null) callback.onParametersChanged(this);
	}
	
	public String getParams() {
		return params;
	}
	
	public static TSDRSource fromPlugin(final String desc, final File full_path_to_library, final String params) {
		return new TSDRSource(desc, full_path_to_library.getAbsolutePath(), params, true);
	}
	
	@Override
	public String toString() {
		return descr;
	}
	
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
	

	public boolean populate(final JDialog dialog, final String defaultprefs) {
		dialog.setTitle(descr);
		dialog.getContentPane().setLayout(null);
		dialog.setSize(400, 150);
		dialog.setVisible(false);
		dialog.setResizable(false);
		
	    final JLabel description = new JLabel("Type in the parameters:");
	    dialog.getContentPane().add(description);
	    description.setBounds(12, 12, 400-12, 24);
	    
	    final JTextField params = new JTextField(defaultprefs);
	    dialog.getContentPane().add(params);
	    params.setBounds(12, 12+32, 400-12, 24);
	    
	    final JButton ok = new JButton("OK");
	    dialog.getContentPane().add(ok);
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
	
	public void setOnParameterChangedCallback(final TSDRSourceParamChangedListener callback) {
		this.callback = callback;
	}
	
	public JDialog invokeGUIDialog(final Frame frame, final String defaultprefs) {
		final JDialog dialog = new JDialog(frame, false);
		if (!populate(dialog, defaultprefs))
			return null;
		else
			return dialog;
	}
	
	public static interface TSDRSourceParamChangedListener {
		void onParametersChanged(final TSDRSource source);
	}
}
