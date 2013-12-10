package martin.tempest.core.exceptions;

public class TSDRPluginParametersException extends TSDRException {
	
	private static final long serialVersionUID = -7807615914689137600L;

	public TSDRPluginParametersException(final Exception e) {
		super(e);
	}
	
	public TSDRPluginParametersException(final String msg) {
		super(msg);
	}
}
