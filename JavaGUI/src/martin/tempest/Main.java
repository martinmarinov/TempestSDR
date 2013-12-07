package martin.tempest;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;

import martin.tempest.core.TSDRLibrary;
import martin.tempest.core.exceptions.TSDRException;

public class Main implements TSDRLibrary.FrameReadyCallback {

	public static void main(String[] args) {
		try {
			new Main();
		} catch(Exception e) {
			System.err.println("Cannot load! Reason: "+e.getLocalizedMessage());
			e.printStackTrace();
		}
	}

	public Main() throws TSDRException {
		TSDRLibrary sdrlib = new TSDRLibrary();
		sdrlib.loadSource(TSDRLibrary.getAllSources()[0]);
		sdrlib.registerFrameReadyCallback(this);
		sdrlib.start();

	}

	@Override
	public void onFrameReady(TSDRLibrary lib, BufferedImage frame) {
		System.out.println("Ouch :) "+frame.getWidth()+" x "+frame.getHeight());
		try {
			ImageIO.write(frame, "png", new File("D:\\marto.png"));
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}
