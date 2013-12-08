package martin.tempest.core.exceptions;

public class TSDRAlreadyRunningException extends TSDRException {

	private static final long serialVersionUID = 5365909402344178885L;

	public TSDRAlreadyRunningException(final Exception e) {
		super(e);
	}
	
	public TSDRAlreadyRunningException(final String msg) {
		super(msg);
	}
}
