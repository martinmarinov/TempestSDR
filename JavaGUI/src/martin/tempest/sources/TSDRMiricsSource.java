package martin.tempest.sources;

import javax.swing.JDialog;

public class TSDRMiricsSource extends TSDRSource {
	
	public TSDRMiricsSource() {
		super("Mirics dongle", "TSDRPlugin_Mirics", "", false);
	}
	
	public boolean populate(final JDialog dialog) {
		return false;
	}

}
