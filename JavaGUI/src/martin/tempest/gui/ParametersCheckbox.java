package martin.tempest.gui;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.prefs.Preferences;

import javax.swing.JCheckBox;

import martin.tempest.core.TSDRLibrary;

public class ParametersCheckbox extends JCheckBox {

	private static final long serialVersionUID = 6282512242403837521L;
	private final TSDRLibrary.PARAM param;
	private final OnTSDRParamChangedCallback callback;
	
	public ParametersCheckbox(final TSDRLibrary.PARAM param, final String text, final OnTSDRParamChangedCallback callback, final Preferences prefs, final boolean defaultvalue) {
		super(text, prefs.getBoolean("PARAM"+param, defaultvalue));
		this.param = param;
		this.callback = callback;
		final String prefname = "PARAM"+param;
		callback.onSetParam(param, isSelected() ? 1 : 0);
		addActionListener(new ActionListener() {
			
			@Override
			public void actionPerformed(ActionEvent arg0) {
				callback.onSetParam(param, isSelected() ? 1 : 0);
				prefs.putBoolean(prefname, isSelected());
			}
		});
	}
	
	@Override
	public void setBounds(int x, int y, int width, int height) {
		callback.onSetParam(param, isSelected() ? 1 : 0);
		super.setBounds(x, y, width, height);
	}
		
}
