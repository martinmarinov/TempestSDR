package martin.tempest.gui;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.prefs.Preferences;

import javax.swing.JToggleButton;

import martin.tempest.core.TSDRLibrary;

public class ParametersToggleButton extends JToggleButton {

	private static final long serialVersionUID = -7115465123485780108L;
	private final TSDRLibrary.PARAM param;
	private OnTSDRParamChangedCallback callback;
	private final String prefname;
	private final Preferences prefs;
	
	private void notifyCallback(final boolean selected) {
		if (callback != null)
			callback.onSetParam(param, selected ? 1 : 0);
		if (prefs != null) prefs.putBoolean(prefname, selected);
	}
	
	public ParametersToggleButton(final TSDRLibrary.PARAM param, final String text, final Preferences prefs, final boolean defaultvalue) {
		super(text, (prefs != null)  ? prefs.getBoolean("PARAM"+param, defaultvalue) : defaultvalue);
		this.param = param;
		this.prefs = prefs;
		prefname = "PARAM"+param;
		addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent arg0) {
				notifyCallback(isSelected());
			}
		});
	}
	
	public void setParaChangeCallback(final OnTSDRParamChangedCallback callback) {
		this.callback = callback;
		notifyCallback(isSelected());
	}
	
	@Override
	public void setBounds(int x, int y, int width, int height) {
		notifyCallback(isSelected());
		super.setBounds(x, y, width, height);
	}
	
	@Override
	public void setSelected(boolean arg0) {
		super.setSelected(arg0);
		notifyCallback(arg0);
	}
		
}
