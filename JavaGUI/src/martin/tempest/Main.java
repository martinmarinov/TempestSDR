package martin.tempest;

import martin.tempest.core.TSDRLibrary;

public class Main {

	public static void main(String[] args) {
		try {
			TSDRLibrary sdrlib = new TSDRLibrary();
			sdrlib.test();
		} catch(Exception e) {
			System.out.println("Cannot load! Reason: "+e.getLocalizedMessage());
		}
	}

}
