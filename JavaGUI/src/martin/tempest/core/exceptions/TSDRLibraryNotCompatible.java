package martin.tempest.core.exceptions;

public class TSDRLibraryNotCompatible extends TSDRException {
	private static final long serialVersionUID = -7001791780564950322L;
	
	public TSDRLibraryNotCompatible(final Exception e) {
		super(e);
	}
	
	public TSDRLibraryNotCompatible(final String msg) {
		super(msg);
	}
}
