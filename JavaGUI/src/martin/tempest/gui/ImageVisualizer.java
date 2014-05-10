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
import java.awt.Graphics2D;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.sql.Timestamp;

import javax.imageio.ImageIO;
import javax.swing.JPanel;


public class ImageVisualizer extends JPanel {
	
	private static final long serialVersionUID = -6754436015453195809L;
	private static final int COUNT_TO_AVG = 50;
	private static final int OSD_SIZE_FRACT_OF_HEIGHT = 20;
	
	private Font default_osd_font = null;
	private int current_osd_height;
	
	private BufferedImage todraw = null;
	private Object locker = new Object();
	
	private long prev = System.currentTimeMillis();
	private long osdtime = System.currentTimeMillis();
	private int count = 0;
	private int fps;
	
	private String OSD;
	
	private int width = -1, height = -1, nwidth = 1, nheight = 1, im_width = -1, im_height = -1, todraw_width = 1, todraw_height = 1, todraw_x = 0, todraw_y = 0;
	private int desired_image_width = -1;
	
	private Graphics theonewith_hints = null;
	private volatile boolean rendering_quality_high = false;
	
	public void drawImage(final BufferedImage image, int width) {
		
		synchronized (locker) {
			desired_image_width = width;
			todraw = image;
		}
		
		repaint();
	}
	
	@Override
	public void setBounds(int x, int y, int width, int height) {
		this.nwidth = width;
		this.nheight = height;
		current_osd_height = height / OSD_SIZE_FRACT_OF_HEIGHT;
		default_osd_font = new Font(null, Font.PLAIN, current_osd_height);
		super.setBounds(x, y, width, height);
	}
	
	@Override
	public void setBounds(Rectangle r) {
		this.nwidth = r.width;
		this.nheight = r.height;
		current_osd_height = height / OSD_SIZE_FRACT_OF_HEIGHT;
		default_osd_font = new Font(null, Font.PLAIN, current_osd_height);
		super.setBounds(r);
	}
	
	public void setRenderingQualityHigh(boolean high) {
		this.rendering_quality_high = high;
		theonewith_hints = null;
	}
	
	@Override
	public void paint(Graphics g) {
		
		if (theonewith_hints != g) {
			try {
				final Graphics2D g2D = (Graphics2D) g;
				
				if (rendering_quality_high) {
					g2D.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BICUBIC);
					g2D.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
					g2D.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
				} else {
					g2D.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR);
					g2D.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_SPEED);
					g2D.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
				}

				theonewith_hints = g;
			} catch( Throwable t) {};
		}

		if (todraw != null) {
			if (desired_image_width != im_width || todraw.getHeight() != im_height || nwidth != width || nheight != height) {
				width = nwidth;
				height = nheight;
				
				im_width = desired_image_width;
				im_height = todraw.getHeight();
				
				todraw_width = width;
				todraw_height = height;
				
				if (width * im_height < im_width * height)
					todraw_height = (width * im_height) / im_width;
				else
					todraw_width = (height * im_width) / im_height;
				
				todraw_x = (width - todraw_width) / 2;
				todraw_y = (height - todraw_height) / 2;	
			}
			
			g.setColor(Color.BLACK);
			g.fillRect(0, 0, width, height);
			
			synchronized (locker) {
				g.drawImage(todraw, todraw_x, todraw_y, todraw_width, todraw_height, null);
			}
			
			drawFPS(g);
			drawOSD(g);
		} else {
			g.setColor(Color.BLUE);
			g.fillRect(0, 0, getWidth(), getHeight());
		}
	}
	
	private void drawFPS(Graphics g) {
		count++;
		if (count > COUNT_TO_AVG) {
			final long now = System.currentTimeMillis();
			fps = (int) Math.round((1000.0 * COUNT_TO_AVG) / (double) (now - prev));
			count = 0;
			prev = now;
		}
		
		g.setColor(Color.white);
		g.fillRect(width-60, 0, 60, 20);
		g.setColor(Color.black);
		g.drawString(fps+" fps", width-60, 15);
	}
	
	private void drawOSD(Graphics g) {
		if (OSD != null && !OSD.isEmpty()) {
			g.setColor(Color.green);
			final Font original = g.getFont();
			g.setFont(default_osd_font);
			g.drawString(OSD, 5, current_osd_height);
			g.setFont(original);
			if (System.currentTimeMillis() > osdtime)
				OSD = null;
		}
	}
	
	public void setOSD(final String text, final long timems) {
		OSD = text;
		osdtime = System.currentTimeMillis()+timems;
	}
	
	public void saveImageToPNGFile(final File dir) {
		final File file = new File(dir, new Timestamp((new java.util.Date()).getTime()).toString().replace(" ", "").replace(":", "")+".png");
		if (todraw != null) {
			synchronized (todraw) {
				try {
					ImageIO.write(todraw, "png", file);
					System.out.println("File "+file+" saved!");
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		} else
			System.err.println("No file on screen to write!");
		
	}
}
