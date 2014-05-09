package martin.tempest.gui;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Rectangle;

import javax.swing.JPanel;

public class AutoScaleVisualizer extends JPanel {

	private static final long serialVersionUID = 6629300250729955406L;
	
	private int nwidth = 1, nheight = 1;
	
	private volatile double min = 0, max = 0;
	
	private final static Color[] colours = new Color[256];
	
	{
		for (int i = 0; i < colours.length; i++) colours[i] = new Color(i, i, i);
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
	
	public void setValue(final double min, final double max) {
		this.min = min;
		this.max = max;
		
		repaint();
	}
	
	@Override
	public void paint(Graphics g) {
		final double min = this.min;
		final double max = this.max;
		final int width = this.nwidth;
		final int height = this.nheight;
		
		final int minpx = (int) (min * height);
		final int maxpx = (int) (max * height);
		final int pxspan = maxpx-minpx;
		
		// start drawing
		g.setColor(colours[50]);
		g.fillRect(0, 0, width, height);
		
		
		for (int y = minpx; y < maxpx; y++) {
			final int colour = (255 * (y - minpx)) / pxspan;
			g.setColor(colours[colour]);
			final int inversey = height - y;
			g.drawLine(0, inversey, width, inversey);
		}
	}
	
}
