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
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;

import javax.swing.JPanel;


public class PlotVisualizer extends JPanel {

	private static final long serialVersionUID = -6754436015453195809L;
	
	private Object locker = new Object();
	private float[] data = null;
	private float[] visdata = null;
	private int size = 0;
	private float max_val = 0;
	private float min_val = 0;
	private float range_val = 0;
	private int min_index = 0;
	private boolean enabled;
	private int mouse_x = -1;
	private Integer index_selected = null;
	private Float initial_value = null;

	private long samplerate;
	private int offset;
	private int shift = 0;
	private int shift2 = 0;
	
	private int area_around_mouse = 0;
	
	private final TransformerAndCallback trans;
	
	private int nwidth = 1, nheight = 1;
	
	public PlotVisualizer(final TransformerAndCallback trans) {
		this.trans = trans;
		
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
					final int id_selected = getBestIdAround(mouse_x, area_around_mouse);
					setSelectedIndex(id_selected);
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
	
	public void setSelectedValue(float val) {
		if (data == null) {
			synchronized (locker) {
				this.initial_value = val;
			}
		} else
			setSelectedIndex(trans.toIndex(val, offset, samplerate));
		
		repaint();
	}
	
	public void setSelectedIndex(int val) {
		synchronized (locker) {
			this.index_selected = val;
		}
		repaint();
	}
	
	public Float getSelectedValue() {
		if (data == null)
			return initial_value;
		else
			return trans.fromIndex(index_selected, offset, samplerate);
	}
	
	public Float getValueFromId(int id) {
		return trans.fromIndex(id, offset, samplerate);
	}
	
	private String getValueAt(int id) {
		final float val = trans.fromIndex(id, offset, samplerate);
		return trans.getDescription(val, id);
	}

	public void plot(float[] incoming_data, int offset, final int size, long samplerate) {
		if (size <= 0) return;
		
		synchronized (locker) {
			if (data == null || data.length < size)
				data = new float[size];
			
			shift = ((size < nwidth) ? (nwidth / size) : 0);
			shift2 = shift/2;
			
			System.arraycopy(incoming_data, 0, data, 0, size);
			
			if (visdata == null || visdata.length != nwidth)
				visdata = new float[nwidth];
			
			this.size = size;
			this.offset = offset;
			this.samplerate = samplerate;
			
			max_val = data[0];
			min_val = data[0];
			
			float localmin = data[0];
			min_index = 0;
			int old_px = 0;
			
			for (int i = 1; i < size; i++) {
				final int curr_px = nwidth * i / size;
				final float val = data[i];

				if (old_px != curr_px) {

					for (int px = old_px; px < curr_px; px++)
						visdata[px] = localmin;

					localmin = val;
				}
				
				old_px = curr_px;
				
				if (val < localmin)
					localmin = val;
				
				if (val > max_val)
					max_val = val;
				else if (val < min_val) {
					min_val = val;
					min_index = i;
				}
			}
			
			range_val = max_val - min_val;
			
			if (range_val == 0 || Float.isInfinite(range_val) || Float.isNaN(range_val)) return;
			
			for (int x = 0; x < nwidth; x++)
				visdata[x] = nheight * (visdata[x] - min_val) / range_val;
			
			// calculate selected initial index
			if (index_selected == null && initial_value != null) {
				index_selected = trans.toIndex(initial_value, offset, samplerate);
				initial_value= null;
			}
		}
		
		repaint();
	}
	
	public int getMinIndex() {
		return min_index;
	}
	
	public int getOffset() {
		return offset;
	}
	
	public long getSamplerate() {
		return samplerate;
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

			if (range_val == 0 || Float.isInfinite(range_val) || Float.isNaN(range_val)) return;

			g.setColor(enabled ? Color.black : Color.DARK_GRAY);
			g.fillRect(0, 0, nwidth, nheight);
			
			if (mouse_x != -1) {
				g.setColor(Color.darkGray);
				
				if (area_around_mouse != 0) g.fillRect(mouse_x-area_around_mouse/2, 0, area_around_mouse, nheight);
			}

			if (visdata.length == nwidth) {
				
				g.setColor(Color.gray);

				for (int x = 0; x < nwidth; x++)
					g.drawLine(x, (int) visdata[x], x, nheight);

			} else {
				g.setColor(Color.blue);
				g.fillRect(0, 0, nwidth, nheight);
				return;
			}
			
			if (mouse_x != -1) {
				
				g.setColor(Color.gray);
				g.drawLine(mouse_x, 0, mouse_x, nheight);
				
				final int id = getBestIdAround(mouse_x, area_around_mouse);
				
				if (id >= 0 && id < size) {
					final int x = nwidth * id / size+shift2;
					final int y = (int) (nheight * (data[id] - min_val) / range_val);
					
					g.drawString(getValueAt(id), mouse_x+nheight / 10, nheight / 5);
					
					//g.fillOval(x-3-shift2, y-3-shift2, 6+shift, 6+shift);
					g.setColor(Color.yellow);
					g.fillRect(x-shift2, y, shift+1, nheight-y);
				}
			}
			
			if (index_selected != null) {

				if (index_selected >= 0 || index_selected < size) {
					g.setColor(Color.green);

					final int x = nwidth * index_selected / size+shift2;

					g.drawLine(x, 0, x, nheight);

					if (index_selected >= 0 && index_selected < size)
						g.drawString(getValueAt(index_selected), x+nheight / 10, nheight / 9);
				}
			}
			

		}
	}
	
	public static abstract class TransformerAndCallback {
		public abstract float fromIndex(final int id, final int offset, final long samplerate);
		public abstract int toIndex(final float val, final int offset, final long samplerate);
		public abstract String getDescription(final float val, final int id);
		
		public void onMouseMoved(final int m_id, final int offset, final long samplerate) {};
		public void onMouseExited() {};
		public void executeIdSelected(final int sel_id, final int offset, final long samplerate) {};
	}
}
