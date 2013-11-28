package martin.tempest.core;

public class TSDRLibrary {
	
	static {
	      System.loadLibrary("TSDRLibraryNDK"); 
	}
	
	public native void test();
}
