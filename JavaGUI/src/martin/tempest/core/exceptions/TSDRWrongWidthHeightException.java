package martin.tempest.core.exceptions;

public class TSDRWrongWidthHeightException extends TSDRException {

	private static final long serialVersionUID = -7807615914689137600L;

	public TSDRWrongWidthHeightException(final Exception e) {
		super(e);
	}
	
	public TSDRWrongWidthHeightException(final String msg) {
		super(msg);
	}
}
