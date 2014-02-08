package martin.tempest.sources;

/**
 * This plugin allows for playback of prerecorded files.
 * 
 * @author Martin Marinov
 *
 */
public class TSDRFileSource extends TSDRSource {

	public TSDRFileSource() {
		super("From file", "TSDRPlugin_RawFile", false);
	}

}
