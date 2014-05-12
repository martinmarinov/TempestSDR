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
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Rectangle;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;

import javax.swing.JPanel;


public class PlotVisualizer extends JPanel {

	private static final long serialVersionUID = -6754436015453195809L;
	
	private static final double FONT_SPACING_COEFF = 1.3;
	private static final double FONT_SIZE_COEFF = 0.8;
	private static final double DB_MULTIPLIER = 10;
	private final static Color default_txt_colour_background = Color.lightGray;
	
	private final static double[] POSSIBLE_SCALE_VALUES_DB = new double[] {50, 20, 10, 5, 2, 1, 0.5, 0.2, 0.1}; 
	
	private Object locker = new Object();
	private float[] data = null;
	private double[] visdata = null;
	private int size = 0;
	private int max_index = 0;
	private boolean enabled;
	private int mouse_x = -1;
	private Integer index_selected = null;
	private Float initial_value = null;

	private long samplerate;
	private int offset;
	private int shift = 0;
	private int shift2 = 0;
	
	private double lowest_db = -20;
	private double highest_db = 0;
	private double span_db = highest_db-lowest_db;
	private double lowest_val = dbtoval(lowest_db), highest_val = dbtoval(highest_db);
	
	private int area_around_mouse = 0;
	
	private final TransformerAndCallback trans;
	
	private int nwidth = 1, nheight = 1;
	
	private boolean font_set = false;
	private int half_fontsize;
	private Font font;
	
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
		float max = data[bestid];
		for (int id = start_id+1; id < end_id; id++)
			if (data[id] > max) {
				max = data[id];
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
	
	private double valtodb(final double val) {
		return DB_MULTIPLIER*Math.log10(val);
	}
	
	private double dbtoval(final double db) {
		return Math.pow(10, db/DB_MULTIPLIER);
	}
	
	private int valtologpx(final double val) {
		if (val <= lowest_val) return nheight;
		else if (val >= highest_val) return 0;
		return (int) (nheight - (valtodb(val) - lowest_db) * nheight / span_db);
	}
	
	private int dbtopx(final double db) {
		final int px = (int) (nheight - (db - lowest_db) * nheight / span_db);
		return (px < 0) ? 0 : (px >= nheight ? nheight - 1 : px);
	}
//	
//	private double pxtoval(final int px) {
//		final double db = LOWEST_DB * px / (double) nheight;
//		return dbtoval(db);
//	}
	
	private void populateData() {
		highest_val = data[0];
		lowest_val = highest_val;
		max_index = 0;
		double max_val = highest_val;
		
		double localmax = highest_val;
		max_index = 0;
		int old_px = 0;
		
		// do the auto scaling by finding the maximum and minimum local values
		for (int i = 1; i < size; i++) {
			final int curr_px = nwidth * i / size;
			final float val = data[i];

			if (old_px != curr_px) {

				for (int px = old_px; px < curr_px; px++)
					visdata[px] = localmax;

				if (localmax > highest_val)
					highest_val = localmax;
				else if (localmax < lowest_val)
					lowest_val = localmax;
				
				localmax = val;
			}
			
			old_px = curr_px;
			
			if (val > localmax)
				localmax = val;
			
			if (val > max_val) {
				max_val = val;
				max_index = i;
			}
			
		}
		
		visdata[nwidth-1] = localmax;
		
		// set the values required to do the conversations
		lowest_db = valtodb(lowest_val);
		highest_db = valtodb(highest_val);
		span_db = highest_db-lowest_db;
		
		
		if (span_db == 0 || Double.isInfinite(span_db) || Double.isNaN(span_db)) return;
		
		for (int x = 0; x < nwidth; x++)
			visdata[x] = valtologpx(visdata[x]);
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
				visdata = new double[nwidth];
			
			this.size = size;
			this.offset = offset;
			this.samplerate = samplerate;
			
			populateData();
			
			// calculate selected initial index
			if (index_selected == null && initial_value != null) {
				index_selected = trans.toIndex(initial_value, offset, samplerate);
				initial_value= null;
			}
		}
		
		repaint();
	}
	
	public int getMaxIndex() {
		return max_index;
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
	
	private static final int getAccuracy(double number) {
		if (Math.abs(((int) (number)) - number) < 1e-10)
			return 0;
		
		int accuracy = 0;
		int i = 1;
		while((i*=10) < 1e9) {
			accuracy++;
			if (Math.abs(((int) (number*i)/(double) i) - number) < 1e-10)
				break;
		}
		return accuracy;
	}
	
	public void paintScale(Graphics g) {
		final int linewidth = nwidth / 200;
		final int small_linewidth = nwidth / 400;
		final int textoffset = linewidth + 2*small_linewidth;
		
		final double dvision = FONT_SPACING_COEFF * span_db / half_fontsize;
		double db_step = POSSIBLE_SCALE_VALUES_DB[POSSIBLE_SCALE_VALUES_DB.length-1];
		for (final double v : POSSIBLE_SCALE_VALUES_DB) if (v < dvision) {db_step = v; break;};
		final double line_db_step = db_step/5;
		
		final double firstval = ((int) (highest_db / db_step)) * db_step;
		final double firstval_line = ((int) (highest_db / line_db_step)) * line_db_step;

		g.setColor(default_txt_colour_background);
		
		for (double db = firstval_line; db >= lowest_db; db -= line_db_step) {
			final int y = dbtopx ( db );
			g.drawLine(0, y, small_linewidth, y);
		}
		
		
		final int accuracy = getAccuracy(db_step);
		final String formatter_plus = String.format("+%%.%df dB", accuracy);
		final String formatter_minus = String.format("%%.%df dB", accuracy);
		for (double db = firstval; db >= lowest_db; db -= db_step) {
			final int y = dbtopx ( db );
			g.drawString((db > 0) ? String.format(formatter_plus, db) : String.format(formatter_minus, db), textoffset, y+half_fontsize);
			g.drawLine(0, y, linewidth, y);
		}
		
	}
	
	private void setFont(Graphics g) {
		if (!font_set) {
			final Font existing = g.getFont();
			font = new Font(existing.getFontName(), Font.PLAIN, (int) (existing.getSize()*FONT_SIZE_COEFF));
			half_fontsize = font.getSize()/2;
			g.setFont(font);
			font_set = true;
		} else {
			g.setFont(font);
		}
	}
	
	@Override
	public void paint(Graphics g) {
		
		setFont(g);
		
		if (data == null) {
			g.setColor(Color.black);
			g.fillRect(0, 0, nwidth, nheight);
			paintScale(g);
			return;
		}
		
		synchronized (locker) {

			if (span_db == 0 || Double.isInfinite(span_db) || Double.isNaN(span_db)) return;
			
			g.setColor(enabled ? Color.black : Color.DARK_GRAY);
			g.fillRect(0, 0, nwidth, nheight);
			
			if (mouse_x != -1) {
				g.setColor(Color.darkGray);
				
				if (area_around_mouse != 0) g.fillRect(mouse_x-area_around_mouse/2, 0, area_around_mouse, nheight);
			}

			if (visdata.length == nwidth) {
				
				g.setColor(Color.gray);

				for (int x = 0; x < nwidth; x++)
					g.drawLine(x, (int) (visdata[x]), x, nheight);

			} else {
				g.setColor(Color.blue);
				g.fillRect(0, 0, nwidth, nheight);
				return;
			}
			
			if (mouse_x != -1) {
				
				g.setColor(default_txt_colour_background);
				g.drawLine(mouse_x, 0, mouse_x, nheight);
				
				final int id = getBestIdAround(mouse_x, area_around_mouse);
				
				if (id >= 0 && id < size) {
					final int x = nwidth * id / size+shift2;
					final int y = valtologpx(data[id]);
					final double db = valtodb(data[id]);
					
					g.drawString(getValueAt(id), mouse_x+nheight / 10, nheight / 5);
					g.drawString(String.format("%.1f dB", db), mouse_x+nheight / 10, nheight / 5 + 3*half_fontsize);
					
					g.setColor(Color.yellow);
					g.fillRect(x-shift2, y, shift+1, nheight);
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
			
			paintScale(g);
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
