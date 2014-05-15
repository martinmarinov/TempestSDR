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

/**
 * Allows to implement zoomable graphs. 
 * 
 * If input values map from zero to width, this will allow zooming and moving around
 * Output pixels will map from zero to width but might only represent a portion of the original range
 * 
 * @author martinmarinov
 *
 */
public class ZoomableXScale {

	private double min_val, max_val, span_val;
	private double scale = 1.0;
	private int nwidth = 0;
	private double nwidthdivspan_val, span_valdivnwidth;
	
	private final Object locker = new Object();
	
	public boolean valuesValid() {
		synchronized (locker) {
			return ! Double.isInfinite(span_val) &&  ! Double.isNaN(span_val) && span_val != 0 && nwidth != 0;
		}
	}
	
	public double valtopx_relative(final double px) {
		return (px/scale);
	}
	
	public double pxtoval_relative(final double px) {
		return (px*scale);
	}
	
	public int valtopx(final double val) {
		synchronized (locker) {
			return (int) ((val - min_val) * nwidthdivspan_val);
		}
	}
	
	public double pxtoval(final int px) {
		synchronized (locker) {
			return px * span_valdivnwidth + min_val;
		}
	}
	
	public double pxtoval_unsafe(final int px) {
		synchronized (locker) {
			return px * span_valdivnwidth + min_val;
		}
	}
	
	public void zoomAround(final int px, final double coeff) {

		synchronized (locker) {
//			final double origval = pxtoval(px);
//			final double origdiff = origval - min_val;
//			final double new_diff = origdiff * coeff;
//
//			min_val -= new_diff;
//			max_val = min_val + span_val * coeff;
			scale *= coeff;

			nwidthdivspan_val  = nwidth / (scale * span_val);
			span_valdivnwidth = (scale * span_val) / nwidth;
		}
	}
	
	public void setDimentions(int width) {
		synchronized (locker) {
			nwidth = width;
			max_val = width;
			min_val = 0;
			span_val = max_val - min_val;
			
			nwidthdivspan_val  = nwidth / (scale * span_val);
			span_valdivnwidth = (scale * span_val) / nwidth;
		}
	}
	
}
