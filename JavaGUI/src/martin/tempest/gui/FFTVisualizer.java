/*******************************************************************************
 * Copyright (c) 2014 Martin Marinov.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 *     Martin Marinov - initial API and implementation
 ******************************************************************************/
package martin.tempest.gui;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.Rectangle;

import javax.swing.JPanel;


public class FFTVisualizer extends JPanel {
	
	private static final long serialVersionUID = -6754436015453195809L;
	
	private Object locker = new Object();
	float[] data = null;

	private int nwidth = 1, nheight = 1, nheighthalf = 1;
	
	public void drawFFT(float[] fft, long samplerate) {
		
		synchronized (locker) {
			if (data == null || data.length != fft.length)
				data = new float[fft.length];
			System.arraycopy(fft, 0, data, 0, fft.length);
		}
		
		repaint();
	}
	
	@Override
	public void setBounds(int x, int y, int width, int height) {
		this.nwidth = width;
		this.nheight = height;
		this.nheighthalf = height/2;
		super.setBounds(x, y, width, height);
	}
	
	@Override
	public void setBounds(Rectangle r) {
		this.nwidth = r.width;
		this.nheight = r.height;
		this.nheighthalf = r.height/2;
		super.setBounds(r);
	}
	
	@Override
	public void paint(Graphics g) {
		synchronized (locker) {
			g.setColor(Color.black);
			g.fillRect(0, 0, nwidth, nheight);
			if (data != null) {
				g.setColor(Color.white);
				int ly = nheight + (int) (Math.log(data[0]*data[0]));
				int lx = 0;
				for (int i = 1; i < data.length; i++) {
					final int x = nwidth * i / data.length;
					final int y = nheight + (int) (Math.log(data[i]*data[i]));
					g.drawLine(lx, ly, x, y);
					lx = x;
					ly = y;
				}
			}
		}
	}
	
	
}
