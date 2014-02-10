package martin.tempest.sources;

/**
 * This plugin allows for playback of prerecorded files.
 * 
 * @author Martin Marinov
 *
 */
public class TSDRUHDSource extends TSDRSource {

	public TSDRUHDSource() {
		super("USRP (via UHD)", "TSDRPlugin_UHD", false);
	}

}
