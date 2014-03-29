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
	
	public ParametersToggleButton(final TSDRLibrary.PARAM param, final String text, final Preferences prefs, final boolean defaultvalue) {
		super(text, prefs.getBoolean("PARAM"+param, defaultvalue));
		this.param = param;
		final String prefname = "PARAM"+param;
		addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent arg0) {
				if (callback != null)
					callback.onSetParam(param, isSelected() ? 1 : 0);
				prefs.putBoolean(prefname, isSelected());
			}
		});
	}
	
	public void setParaChangeCallback(final OnTSDRParamChangedCallback callback) {
		this.callback = callback;
		if (callback != null)
			callback.onSetParam(param, isSelected() ? 1 : 0);
	}
	
	@Override
	public void setBounds(int x, int y, int width, int height) {
		if (callback != null)
			callback.onSetParam(param, isSelected() ? 1 : 0);
		super.setBounds(x, y, width, height);
	}
		
}
