package martin.tempest.gui.scale;

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


/**
 * Implements a zoomable scale that can map values from min_val to max_val to
 * 0 to max_value pixels. It also has a maximum zoomable value that you can set up.
 * 
 * @author martinmarinov
 *
 */
public class ZoomableXScale {

	private int max_pixels = 800;
	private double min_value, max_value, offset_val = 0.0;
	private int offset_px = 0;
	private double scale = 1.0; // one raw pixels = scale * screen pixel
	private double max_zoom_val = 1.0;
	private double one_val_in_pixels_relative = max_pixels / (max_value - min_value);
	private double one_px_in_values_relative = (max_value - min_value) / max_pixels;
	
	private boolean autofixzoomandoffset = true;

	private final Object locker = new Object();

	/**
	 * Set the values that will show up
	 * @param min_value
	 * @param max_value
	 * @param max_zoom_val the maximum size of maximum zoom. If this is 0.2 then the maximum amount you can zoom is having 0.2 value accross the whole screen
	 */
	public ZoomableXScale(final double min_value, final double max_value, final double max_zoom_val) {
		setMinMaxValue(min_value, max_value, max_zoom_val);
	}
	
	public ZoomableXScale(final double min_value, final double max_value) {
		this(min_value, max_value, 1.0);
	}
	
	public ZoomableXScale(final double max_zoom_val) {
		this(0, 100, max_zoom_val);
	}
	
	public ZoomableXScale() {
		this(0, 100, 1.0);
	}
	
	public void autofixZoomAndOffsetEnabled(final boolean enforce) {
		synchronized (locker) {
			this.autofixzoomandoffset = enforce;
		}
	}

	public void setMaxPixels(final int max_pixels) {
		synchronized (locker) {
			this.max_pixels = max_pixels;

			calculateValues_unsafe();
		}
	}

	public void setMinMaxValue(final double min_value, final double max_value, final double max_zoom_val) {
		synchronized (locker) {
			this.max_zoom_val = max_zoom_val;
			setMinMaxValue(min_value, max_value);
		}
	}
	
	public void setMinMaxValue(final double min_value, final double max_value) {
		synchronized (locker) {
			this.min_value = min_value;
			this.max_value = max_value;

			calculateValues_unsafe();
		}
	}

	public void moveOffsetWithPixels(final int offset) {
		synchronized (locker) {
			setPxOffset_unsafe(offset_px - offset);
			
			if (autofixzoomandoffset)
				autoFixOffset_unsafe();
		}
	}

	public void moveOffsetWithValue(final double value) {
		synchronized (locker) {
			setValOffset_unsafe(offset_val - value);
			
			if (autofixzoomandoffset)
				autoFixOffset_unsafe();
		}
	}

	public void zoomAround(final int px, final double coeff) {
		synchronized (locker) {
			final double val = pixels_to_value_absolute(px);
			scale *= coeff;
			calculateValues_unsafe();

			final double newval = pixels_to_value_absolute(px);

			setValOffset_unsafe(offset_val - newval + val);
			
			if (autofixzoomandoffset)
				autoFixOffset_unsafe();
		}
	}
	
	public void fixOffset() {
		synchronized (locker) {
			autoFixOffset_unsafe();
		}
	}
	
	public void reset() {
		synchronized (locker) {
			reset_unsafe();
		}
	}

	public double pixels_to_value_absolute(final int pixels) {
		synchronized (locker) {
			return pixels * one_px_in_values_relative + offset_val + min_value;
		}
	}

	public double pixels_to_value_relative(final int pixels) {
		synchronized (locker) {
			return pixels * one_px_in_values_relative;
		}
	}

	public int value_to_pixel_absolute(final double val) {
		synchronized (locker) {
			return (int) ((val - min_value) * one_val_in_pixels_relative) - offset_px;
		}
	}

	public int value_to_pixel_relative(final double val) {
		synchronized (locker) {
			return (int) (val * one_val_in_pixels_relative);
		}
	}
	
	private void setPxOffset_unsafe(final int offset_px) {
		this.offset_px = offset_px;
		offset_val = pixels_to_value_relative(offset_px);
	}
	
	private void setValOffset_unsafe(final double offset_val) {
		this.offset_val = offset_val;
		offset_px = value_to_pixel_relative(offset_val);
	}
	
	private void calculateValues_unsafe() {

		one_val_in_pixels_relative = max_pixels / ((max_value - min_value)*scale);
		one_px_in_values_relative = ((max_value - min_value)*scale) / max_pixels;
		
		final double values_in_screen = pixels_to_value_relative(max_pixels);
		if (values_in_screen < max_zoom_val) {
			scale = max_zoom_val / (max_value - min_value);
			one_val_in_pixels_relative = max_pixels / ((max_value - min_value)*scale);
			one_px_in_values_relative = ((max_value - min_value)*scale) / max_pixels;
		}
	}
	
	private void reset_unsafe() {
		scale = 1;
		offset_val = 0;
		offset_px = 0;

		calculateValues_unsafe();
	}
	
	private void autoFixOffset_unsafe() {
		
		if (offset_px < 0)
			setPxOffset_unsafe(0);
		
		final double max_val = pixels_to_value_absolute(max_pixels);
		if (max_val > this.max_value)
			 setValOffset_unsafe(this.max_value - pixels_to_value_relative(max_pixels) - this.min_value);

		if (offset_px < 0)
			reset_unsafe();
	}
}
