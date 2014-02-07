package martin.tempest.core.exceptions;

public class TSDRIncompatiblePluginException extends TSDRException {
	private static final long serialVersionUID = -8416953642010129494L;
	
	public TSDRIncompatiblePluginException(final String msg) {
		super(msg);
	}
	
	public TSDRIncompatiblePluginException() {
		super();
	}
}
