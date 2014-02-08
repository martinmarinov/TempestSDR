package martin.tempest.sources;

import java.awt.Container;

public class TSDRMiricsSource extends TSDRSource {
	
	public TSDRMiricsSource() {
		super("Mirics dongle", "TSDRPlugin_Mirics", false);
	}
	
	@Override
	public boolean populateGUI(final Container cont, final String defaultprefs) {
		return false;
	}

}
