package martin.tempest.sources;

import java.awt.Component;
import java.awt.Dialog;
import java.awt.Frame;
import java.awt.Window;
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
	
	public String getAbsolutePathToLibrary() throws TSDRLibraryNotCompatible {
		return absolute ? libname : (TSDRLibrary.extractLibrary(libname).getAbsolutePath());
	}
	

	public boolean populate(final JDialog dialog) {
		dialog.setTitle(descr);
		dialog.getContentPane().setLayout(null);
		dialog.setSize(400, 150);
		dialog.setVisible(false);
		dialog.setResizable(false);
		
	    final JLabel description = new JLabel("Type in the parameters:");
	    dialog.getContentPane().add(description);
	    description.setBounds(12, 12, 400-12, 24);
	    
	    final JTextField params = new JTextField(this.params);
	    dialog.getContentPane().add(params);
	    params.setBounds(12, 12+32, 400-12, 24);
	    
	    final JButton ok = new JButton("OK");
	    dialog.getContentPane().add(ok);
	    ok.setBounds(12, 12+2*32, 84, 24);
	    
	    ok.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				if (callback != null) callback.onParametersChanged(TSDRSource.this);
			}
		});
		return true;
	}
	
	public void setOnParameterChangedCallback(final TSDRSourceParamChangedListener callback) {
		this.callback = callback;
	}
	
	public JDialog invokeGUIDialog(final Frame frame) {
		final JDialog dialog = new JDialog(frame, false);
		if (!populate(dialog))
			return null;
		else
			return dialog;
	}
	
	public static interface TSDRSourceParamChangedListener {
		void onParametersChanged(final TSDRSource source);
	}
}
