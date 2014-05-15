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
package martin.tempest.gui.scale;

import java.awt.Color;
import java.awt.Graphics;

/**
 * This class makes it easy to visualize log scale conversions on the y axis
 * 
 * @author martinmarinov
 *
 */
public class LogScale {
	
	private final static double MAX_SPAN_DB = 500;
	
	private final static double[] POSSIBLE_SCALE_VALUES_DB = new double[] {50, 20, 10, 5, 2, 1, 0.5, 0.2, 0.1};
	private final double font_spacing_coeff;
	private static final double DB_MULTIPLIER = 10;
	
	private final Color default_txt_colour_background;
	
	private double lowest_db = -21;
	private double highest_db = 1;
	private double span_db = highest_db-lowest_db;
	private double lowest_val = dbtoval(lowest_db), highest_val = dbtoval(highest_db);
	
	private int nwidth = 0, nheight = 0;
	
	/**
	 * @param default_txt_colour_background text colour
	 * @param font_spacing a coefficent governing the distance between the ticks
	 */
	public LogScale(final Color default_txt_colour_background, final double font_spacing) {
		this.font_spacing_coeff = font_spacing;
		this.default_txt_colour_background = default_txt_colour_background;
	}
	
	/**
	 * @param default_txt_colour_background text colour
	 * @param font_spacingg a coefficent governing the distance between the ticks
	 * @param lowest_db the lowest db value on the scale (on bottom)
	 * @param highest_db the highest db value on the scale (on top)
	 */
	public LogScale(final Color default_txt_colour_background, final double font_spacing, final double lowest_db, final double highest_db) {
		this.font_spacing_coeff = font_spacing;
		this.default_txt_colour_background = default_txt_colour_background;
		setLowestHighestDb(lowest_db, highest_db);
	}
	
	public void setLowestHighestDb(final double lowest_db, final double highest_db) {
		this.lowest_db = lowest_db;
		this.highest_db = highest_db;
		span_db = highest_db-lowest_db;
		lowest_val = dbtoval(lowest_db);
		highest_val = dbtoval(highest_db);
	}
	
	public double getLowestValue() {
		return lowest_val;
	}
	
	public double getHighestValue() {
		return highest_val;
	}
	
	public double getLowestDb() {
		return lowest_db;
	}
	
	public double getHighestDb() {
		return highest_db;
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
	
	/**
	 * Set the dimensions of the log scale. This need to be the same as the component
	 * @param width
	 * @param height
	 */
	public void setDimentions(int width, int height) {
		nwidth = width;
		nheight = height;
	}
	
	/**
	 * Converta a value to decibels
	 * @param val
	 * @return
	 */
	public double valtodb(final double val) {
		return DB_MULTIPLIER*Math.log10(val);
	}
	
	/**
	 * Convert decibels to value
	 * @param db
	 * @return
	 */
	public double dbtoval(final double db) {
		return Math.pow(10, db/DB_MULTIPLIER);
	}
	
	/**
	 * Convert value to pixels
	 * @param val
	 * @return
	 */
	public int valtopx(final double val) {
		if (val <= lowest_val) return nheight;
		else if (val >= highest_val) return 0;
		return (int) (nheight - (valtodb(val) - lowest_db) * nheight / span_db);
	}
	
	/**
	 * Convert decibels to pixels
	 * @param db
	 * @return
	 */
	public int dbtopx(final double db) {
		final int px = (int) (nheight - (db - lowest_db) * nheight / span_db);
		return (px < 0) ? 0 : (px >= nheight ? nheight - 1 : px);
	}
	
	/**
	 * Convert pixels to value
	 * @param px
	 * @return
	 */
	public double pxtoval(final int px) {
		final double db = (nheight - px) * span_db / nheight + lowest_db;
		return dbtoval(db);
	}
	
	/**
	 * Set maximum and minimum value for the scale
	 * @param lowest_val
	 * @param highest_val
	 */
	public void setLowestHighestValue(final double lowest_val, final double highest_val) {
		final double newlowest_db =  valtodb(lowest_val);
		final double newhighest_db = valtodb(highest_val);
		final double newspan_db = newhighest_db-newlowest_db;
				
		if (Double.isInfinite(newspan_db) || Double.isNaN(newspan_db) || newspan_db > MAX_SPAN_DB) return;
		
		this.lowest_val = lowest_val;
		this.highest_val = highest_val;
		lowest_db =newlowest_db;
		highest_db = newhighest_db;
		span_db = newspan_db;
		
	}
	
	/**
	 * @return true if the distance from lowest_val to highest_val is not zero
	 */
	public boolean valuesValid() {
		return ! Double.isInfinite(span_db) &&  ! Double.isNaN(span_db);
	}
	
	/**
	 * Paints the scale itself
	 * @param g
	 */
	public void paintScale(Graphics g) {
		if (!valuesValid()) return;
		
		final int half_fontsize = g.getFont().getSize() / 2;
		
		final int linewidth = nwidth / 200;
		final int small_linewidth = nwidth / 400;
		final int textoffset = linewidth + 2*small_linewidth;
		
		final double dvision = font_spacing_coeff * span_db / half_fontsize;
		double db_step = POSSIBLE_SCALE_VALUES_DB[POSSIBLE_SCALE_VALUES_DB.length-1];
		for (final double v : POSSIBLE_SCALE_VALUES_DB) if (v < dvision) {db_step = v; break;};
		final double line_db_step = db_step/5;
		
		final double firstval = ((int) (highest_db / db_step)) * db_step;
		final double firstval_line = ((int) (highest_db / line_db_step)) * line_db_step;

		g.setColor(default_txt_colour_background);
		
		for (double db = firstval_line; db >= lowest_db; db -= line_db_step) {
			if (db >= lowest_db && db <= highest_db) {
				final int y = dbtopx ( db );
				g.drawLine(0, y, small_linewidth, y);
			}
		}
		
		final int accuracy = getAccuracy(db_step);
		final String formatter_plus = String.format("+%%.%df", accuracy);
		final String formatter_minus = String.format("%%.%df", accuracy);
		boolean first = true;
		for (double db = firstval; db >= lowest_db; db -= db_step) {
			if (db >= lowest_db && db <= highest_db) {
				final int y = dbtopx ( db );
				if (first) {
					first = false;
					g.drawString(((db > 0) ? String.format(formatter_plus, db) : String.format(formatter_minus, db))+" dB", textoffset, y+half_fontsize);
				} else
					g.drawString((db > 0) ? String.format(formatter_plus, db) : String.format(formatter_minus, db), textoffset, y+half_fontsize);
				g.drawLine(0, y, linewidth, y);
			}
		}
		
	}

}
