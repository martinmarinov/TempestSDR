package martin.tempest.core.exceptions;

public class TSDRCannotOpenDeviceException extends TSDRException {

	private static final long serialVersionUID = -7807615914689137600L;

	public TSDRCannotOpenDeviceException(final Exception e) {
		super(e);
	}
	
	public TSDRCannotOpenDeviceException(final String msg) {
		super(msg);
	}
}
