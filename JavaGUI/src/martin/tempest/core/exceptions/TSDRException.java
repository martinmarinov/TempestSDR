package martin.tempest.core.exceptions;

public class TSDRException extends Exception {
	private static final long serialVersionUID = 6725402982063689232L;
	
	public TSDRException(final Exception e) {
		super(e);
	}
	
	public TSDRException() {
		super();
	}
	
	public TSDRException(final String msg) {
		super(msg);
	}
}
