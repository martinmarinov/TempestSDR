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

public class SNRVisualizer extends JPanel {
	
	private static final double DB_PADDING = 2;
	private static final double FONT_SPACING_COEFF = 1.5;
	private static final double LOWEST_DB = -50.7;
	private static final double HIGHEST_DB = 0.6;
	private static final double FONT_SIZE_COEFF = 0.8;

	private static final long serialVersionUID = 6629300250729955406L;
	
	private int nwidth = 1, nheight = 1;
	
	private double snr_db = LOWEST_DB-10;
	private double max_snr_db, min_snr_db;
	private boolean max_min_set = false;
	
	private boolean font_set = false;
	private Font font;
	
	private final static Color background = new Color(150, 170, 130);
	private final static Color default_txt_colour_background = Color.DARK_GRAY;
	private final static Color snr_txt_colour_background = default_txt_colour_background;
	
	private int fontsize = 12;
	
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

	
	public void setSNRValue(final double snr) {
		synchronized (locker) {
			this.snr_db = scale_y.valtodb( snr );
			
			if (!max_min_set) {
				max_min_set = true;
				min_snr_db = snr_db;
				max_snr_db = snr_db;
			} else if (snr_db > max_snr_db)
				max_snr_db = snr_db;
			else if (snr_db < min_snr_db)
				min_snr_db = snr_db;
			
			scale_y.setLowestHighestDb(min_snr_db - DB_PADDING, max_snr_db + DB_PADDING);
			
		}
		
		repaint();
	}

	@Override
	public void paint(Graphics g) {
		
		synchronized (locker) {

			final int width = this.nwidth;
			final int height = this.nheight;

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
			
			if (snr_db < scale_y.getHighestDb() && snr_db > scale_y.getLowestDb()) {
				final int snrposy = scale_y.dbtopx(snr_db);
				
				g.setColor(snr_txt_colour_background);
				g.drawLine(0, snrposy, nwidth, snrposy);
				
				g.drawString(String.format("%.2f", snr_db), 0, (int) (snrposy+1.5*fontsize));
				g.drawString("dB", 0, (int) (snrposy-0.5*fontsize));
			}

			scale_y.paintScale(g);

		}
	}
	
}
