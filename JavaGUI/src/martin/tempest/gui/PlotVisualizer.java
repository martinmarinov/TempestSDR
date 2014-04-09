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
import java.awt.Graphics2D;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;

import javax.swing.JPanel;


public class PlotVisualizer extends JPanel {
	private final static String unt_format = "%.1f";
	
	private static final long serialVersionUID = -6754436015453195809L;
	
	private Object locker = new Object();
	private float[] data = null;
	private int size = 0;
	private float max_val = 0;
	private float min_val = 0;
	private float range_val = 0;
	private boolean enabled;
	private int mouse_x = -1;
	private int id_selected = -1;

	private long samplerate;
	private int offset;
	
	private int area_around_mouse = 0;
	
	private final TransformerAndCallback trans;
	private final String text_format;
	
	private int nwidth = 1, nheight = 1;
	
	public PlotVisualizer(final TransformerAndCallback trans) {
		this.trans = trans;
		final String unit = trans.getUnit();
		text_format = (unit == null) ? unt_format : unt_format+" "+unit;
		
		addMouseMotionListener(new MouseMotionListener() {
			public void mouseDragged(MouseEvent arg0) {}
			
			public void mouseMoved(MouseEvent arg0) {
				mouse_x = arg0.getX();
				
				final int m_id = size * mouse_x / nwidth;
				if (m_id >= 0 && m_id < size)
					trans.onMouseMoved(m_id, offset, samplerate);
			
				repaint();
			}
		});
		
		addMouseListener(new MouseListener() {
			public void mouseReleased(MouseEvent arg0) {}
			public void mousePressed(MouseEvent arg0) {}
			public void mouseEntered(MouseEvent arg0) {}
			
			public void mouseClicked(MouseEvent arg0) {
				synchronized (locker) {
					selectId(getBestIdAround(mouse_x, area_around_mouse));
					trans.executeIdSelected(id_selected, offset, samplerate);
				}
			}
			
			public void mouseExited(MouseEvent arg0) {
				mouse_x = -1;
				trans.onMouseExited();
				repaint();
			}
		});
	}
	
	public void setAreaAroundMouse(int area) {
		if (area >= 0)
			area_around_mouse = area;
	}
	
	private int getBestIdAround(final int x, final int area) {
		if (area == 0) return size * x / nwidth;
		
		int start_id = size * (x - area/2) / nwidth ;
		if (start_id >= size) return -1;
		if (start_id < 0) start_id = 0;
		
		int end_id = size * (x + area/2) / nwidth ;
		if (end_id < 0) return -1;
		if (end_id > size) end_id = size;
		
		int bestid = start_id;
		float min = data[bestid];
		for (int id = start_id+1; id < end_id; id++)
			if (data[id] < min) {
				min = data[id];
				bestid = id;
			}
		
		return bestid;
	}
	
	public void selectVal(float val) {
		this.id_selected = trans.toIndex(val, offset, samplerate);
		if (this.id_selected < 0 || this.id_selected >= size) this.id_selected = -1;
		repaint();
	}
	
	public void selectId(int id) {
		this.id_selected = id;
		repaint();
	}
	
	private String getValueAt(int id) {
		final float val = trans.fromIndex(id, offset, samplerate);
		return String.format(text_format, val);
	}

	public void plot(float[] incoming_data, int offset, int size, long samplerate) {
		if (size <= 0) return;
		
		synchronized (locker) {
			if (data == null || data.length < size)
				data = new float[size];
			System.arraycopy(incoming_data, 0, data, 0, size);
			this.size = size;
			this.offset = offset;
			this.samplerate = samplerate;
			
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
		enabled = isEnabled();
		super.setBounds(x, y, width, height);
	}
	
	@Override
	public void setBounds(Rectangle r) {
		this.nwidth = r.width;
		this.nheight = r.height;
		enabled = isEnabled();
		super.setBounds(r);
	}
	
	@Override
	public void setEnabled(boolean arg0) {
		super.setEnabled(arg0);
		enabled = isEnabled();
	}
	
	public void reset() {
		synchronized (locker) {
			data = null;
			size = 0;
		}
		
		repaint();
	}
	
	@Override
	public void paint(Graphics g) {
		
		if (data == null) {
			g.setColor(Color.black);
			g.fillRect(0, 0, nwidth, nheight);
			return;
		}
		
		synchronized (locker) {

			if (g instanceof Graphics2D) {
				final Graphics2D graphics2D = (Graphics2D)g;
				graphics2D.setRenderingHint(RenderingHints.KEY_ANTIALIASING,  RenderingHints.VALUE_ANTIALIAS_ON);
			}

			if (range_val == 0 || Float.isInfinite(range_val) || Float.isNaN(range_val)) return;

			g.setColor(enabled ? Color.black : Color.DARK_GRAY);
			g.fillRect(0, 0, nwidth, nheight);
			
			
			if (mouse_x != -1) {
				g.setColor(Color.darkGray);
				
				if (area_around_mouse != 0) g.fillRect(mouse_x-area_around_mouse/2, 0, area_around_mouse, nheight);
				
				g.setColor(Color.gray);
				g.drawLine(mouse_x, 0, mouse_x, nheight);
				
				final int id = getBestIdAround(mouse_x, area_around_mouse);
				
				if (id >= 0 && id < size) {
					final int y = (int) (nheight * (data[id] - min_val) / range_val);
					
					
					g.drawString(getValueAt(id), mouse_x+nheight / 10, nheight / 5);
					
					g.setColor(Color.yellow);
					g.fillOval(nwidth * id / size-3, y-3, 6, 6);
				}
			}
			
			final int id_selected = this.id_selected;
			if (id_selected != -1) {
				g.setColor(Color.green);
				
				final int x = nwidth * id_selected / size;
				
				g.drawLine(x, 0, x, nheight);
				
				if (id_selected >= 0 && id_selected < size)
					g.drawString(getValueAt(id_selected), x+nheight / 10, nheight / 9);
			}
			
			g.setColor(Color.white);
			g.drawString(getValueAt(0), 0+nheight / 10, nheight / 5);
			
			int ly = (int) (nheight * (data[0] - min_val) / range_val);
			int lx = 0;

			if (size < nwidth) {

				for (int i = 1; i < size; i++) {
					final int x = nwidth * i / size;
					final int y = (int) (nheight * (data[i] - min_val) / range_val);
					g.drawLine(lx, ly, x, y);
					lx = x;
					ly = y;
				}

			} else {

				for (int x = 1; x < nwidth; x++) {
					final int i = size * x / nwidth;
					final int y = (int) (nheight * (data[i] - min_val) / range_val);
					g.drawLine(lx, ly, x, y);

					lx = x;
					ly = y;
				}

			}
		}
	}
	
	public static abstract class TransformerAndCallback {
		public abstract float fromIndex(final int id, final int offset, final long samplerate);
		public abstract int toIndex(final float val, final int offset, final long samplerate);
		public abstract String getUnit();
		
		public void onMouseMoved(final int m_id, final int offset, final long samplerate) {};
		public void onMouseExited() {};
		public void executeIdSelected(final int sel_id, final int offset, final long samplerate) {};
	}
}
