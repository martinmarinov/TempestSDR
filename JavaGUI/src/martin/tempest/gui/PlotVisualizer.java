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


public class PlotVisualizer extends JPanel {
	
	private static final long serialVersionUID = -6754436015453195809L;
	
	private Object locker = new Object();
	private float[] data = null;
	private int size = 0;
	private float max_val = 0;
	private float min_val = 0;
	private float range_val = 0;

	private int nwidth = 1, nheight = 1;
	
	public void plot(float[] incoming_data, int offset, int size, long samplerate) {
		if (size <= 0) return;
		
		synchronized (locker) {
			if (data == null || data.length < size)
				data = new float[size];
			System.arraycopy(incoming_data, 0, data, 0, size);
			this.size = size;
			
			max_val = data[0];
			min_val = data[0];
			for (int i = 1; i < this.size; i++) {
				final float val = data[i];
				if (val > max_val) max_val = val;
				else if (val < min_val) min_val = val;
			}
			range_val = max_val - min_val;
		}
		
		repaint();
	}
	
	@Override
	public void setBounds(int x, int y, int width, int height) {
		this.nwidth = width;
		this.nheight = height;
		super.setBounds(x, y, width, height);
	}
	
	@Override
	public void setBounds(Rectangle r) {
		this.nwidth = r.width;
		this.nheight = r.height;
		super.setBounds(r);
	}
	
	@Override
	public void paint(Graphics g) {
		synchronized (locker) {
			if (range_val == 0 || Float.isInfinite(range_val) || Float.isNaN(range_val)) return;
			
			g.setColor(Color.black);
			g.fillRect(0, 0, nwidth, nheight);
			if (data != null) {
				g.setColor(Color.white);
				int ly = (int) (nheight - nheight * (data[0] - min_val) / range_val);
				int lx = 0;
				for (int i = 1; i < size; i++) {
					final int x = nwidth * i / size;
					final int y = (int) (nheight - nheight * (data[i] - min_val) / range_val);
					g.drawLine(lx, ly, x, y);
					lx = x;
					ly = y;
				}
			}
		}
	}
	
	
}
