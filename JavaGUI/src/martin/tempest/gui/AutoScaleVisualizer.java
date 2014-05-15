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

import javax.swing.JPanel;

import martin.tempest.gui.scale.LogScale;

public class AutoScaleVisualizer extends JPanel {
	
	private static final double FONT_SPACING_COEFF = 1.5;
	private static final double LOWEST_DB = -50.7;
	private static final double HIGHEST_DB = 0.6;
	private static final double FONT_SIZE_COEFF = 0.8;

	private static final long serialVersionUID = 6629300250729955406L;
	
	private int nwidth = 1, nheight = 1;
	private int fontsize = 12;
	
	private volatile double min = 0, max = 0, span = 1;
	
	private boolean font_set = false;
	private Font font;
	
	private final static Color background = new Color(150, 170, 130);
	private final static Color default_txt_colour_background = Color.DARK_GRAY;
	
	private static final Color colour_map[] = new Color[256];
	{
		for (int i = 0; i < 256; i++) colour_map[i] = new Color(i, i, i);
	}
	
	private final LogScale scale_y = new LogScale(default_txt_colour_background, FONT_SPACING_COEFF ,LOWEST_DB, HIGHEST_DB);
	
	private final Object locker = new Object();
	
	@Override
	public void setBounds(int x, int y, int width, int height) {
		this.nwidth = width;
		this.nheight = height;
		scale_y.setDimentions(this.nwidth, this.nheight);
		
		super.setBounds(x, y, width, height);
	}
	
	@Override
	public void setBounds(Rectangle r) {
		this.nwidth = r.width;
		this.nheight = r.height;
		scale_y.setDimentions(this.nwidth, this.nheight);
		super.setBounds(r);
	}

	
	public void setValue(final double min, final double max) {
		
		synchronized (locker) {
			this.min = min;
			this.max = max;
			this.span = max - min;
		}
		
		repaint();
	}
	
	private int pxtocol(final int px) {
		final double val = scale_y.pxtoval(px);
		final int col = (int) (255 * (val - min) / span);
		return (col < 0) ? (0) : ((col > 255) ? (255) : (col));
	}
	
	@Override
	public void paint(Graphics g) {
		
		synchronized (locker) {

			final int width = this.nwidth;
			final int height = this.nheight;

			final int minpx = scale_y.valtopx(this.min);
			final int maxpx = scale_y.valtopx(this.max);

			if (!font_set) {
				final Font existing = g.getFont();
				font = new Font(existing.getFontName(), Font.PLAIN, (int) (existing.getSize()*FONT_SIZE_COEFF));
				g.setFont(font);
				font_set = true;
				fontsize = g.getFont().getSize();
			} else {
				g.setFont(font);
			}

			// start drawing
			g.setColor(background);
			g.fillRect(0, 0, width, height);
			
			scale_y.paintScale(g);

			final int maxvalidpx = Math.min(minpx, nheight);
			for (int y = (maxpx < 0) ? 0 : maxpx ; y < maxvalidpx; y++) {
				g.setColor(colour_map[pxtocol(y)]);
				g.drawLine(0, y, nwidth, y);
			}
			
			if (this.min >= scale_y.getLowestValue() && this.min <= scale_y.getHighestValue()) {
				final double min_db = scale_y.valtodb(this.min);
				g.setColor(Color.lightGray);
				g.drawString(String.format("%.2f", min_db), 0, minpx - fontsize/2);
			}
			
			if (this.max >= scale_y.getLowestValue() && this.max <= scale_y.getHighestValue()) {
				final double max_db = scale_y.valtodb(this.max);
				g.setColor(Color.DARK_GRAY);
				g.drawString(String.format("%.1f", max_db), 0, maxpx + fontsize);
			}

		}
	}
	
}
