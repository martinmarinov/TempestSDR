package martin.tempest.core.exceptions;

public class TSDRSampleRateWrongException extends TSDRException {

	private static final long serialVersionUID = 8116802552326966327L;

	public TSDRSampleRateWrongException(final Exception e) {
		super(e);
	}
	
	public TSDRSampleRateWrongException(final String msg) {
		super(msg);
	}
}
