package martin.tempest.sources;

import javax.swing.JDialog;

public class TSDRMiricsSource extends TSDRSource {
	
	public TSDRMiricsSource() {
		super("Mirics dongle", "TSDRPlugin_Mirics", "", false);
	}
	
	@Override
	public boolean populate(JDialog dialog, String defaultprefs) {
		return false;
	}

}
