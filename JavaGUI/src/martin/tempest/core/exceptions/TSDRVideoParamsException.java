package martin.tempest.core.exceptions;

public class TSDRVideoParamsException extends TSDRException {

	private static final long serialVersionUID = -7807615914689137600L;

	public TSDRVideoParamsException(final Exception e) {
		super(e);
	}
	
	public TSDRVideoParamsException(final String msg) {
		super(msg);
	}
}
