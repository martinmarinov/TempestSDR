package martin.tempest.sources;

import java.awt.Container;

/**
 * This is a source that works with the Mirics dongle, for more information see <a href="http://www.mirics.com/">mirics.com</a>
 * 
 * @author Martin Marinov
 *
 */
public class TSDRMiricsSource extends TSDRSource {
	
	public TSDRMiricsSource() {
		super("Mirics dongle", "TSDRPlugin_Mirics", false);
	}
	
	@Override
	public void populateGUI(final Container cont, final String defaultprefs) {
		setParams(defaultprefs);
	}

}
