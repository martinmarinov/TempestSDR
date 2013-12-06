package martin.tempest;

import martin.tempest.core.TSDRLibrary;

public class Main {

	public static void main(String[] args) {
		try {
			TSDRLibrary sdrlib = new TSDRLibrary();
			sdrlib.loadSource(TSDRLibrary.getAllSources()[0]);
			sdrlib.setBaseFreq(0);
		} catch(Exception e) {
			System.err.println("Cannot load! Reason: "+e.getLocalizedMessage());
			e.printStackTrace();
		}
	}

}
