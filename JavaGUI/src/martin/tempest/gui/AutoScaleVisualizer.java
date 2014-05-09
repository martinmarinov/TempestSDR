package martin.tempest.gui;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Rectangle;

import javax.swing.JPanel;

public class AutoScaleVisualizer extends JPanel {
	
	private static final double LOWEST_DB = -4;
	private static final double DB_STEP = 0.5;
	private static final double DB_LINE_STEP = 0.1;
	private static final double LOWEST_VAL = Math.pow(10, LOWEST_DB);
	private static final double FONT_SIZE_COEFF = 0.8;

	private static final long serialVersionUID = 6629300250729955406L;
	
	private int nwidth = 1, nheight = 1;
	
	private volatile double min = 0, max = 0, span = 1;
	
	private boolean font_set = false;
	private int half_fontsize;
	
	private final static Color background = new Color(150, 170, 130);
	private final static Color default_txt_colour_background = Color.DARK_GRAY;
	
	private static Color colour_map[];
	private static Color inverse_colour_map[];
	
	private final Object locker = new Object();
	
	private Font font;
	
	@Override
	public void setBounds(int x, int y, int width, int height) {
		this.nwidth = width;
		this.nheight = height;
		
		synchronized (locker) {
			initializeColourArray();
		}
		
		super.setBounds(x, y, width, height);
	}
	
	@Override
	public void setBounds(Rectangle r) {
		this.nwidth = r.width;
		this.nheight = r.height;
		
		synchronized (locker) {
			initializeColourArray();
		}
		super.setBounds(r);
	}
	
	private void initializeColourArray() {
		if (colour_map == null || colour_map.length != nheight) {
			colour_map = new Color[nheight];
			inverse_colour_map = new Color[nheight];
			
			for (int i = 0; i < nheight; i++) {
				colour_map[i] = default_txt_colour_background;
				inverse_colour_map[i] = default_txt_colour_background;
			}
		}
	}
	
	public void setValue(final double min, final double max) {
		
		synchronized (locker) {
			this.min = min;
			this.max = max;
			this.span = max - min;
			
			initializeColourArray();
			
			for (int i = 0; i < nheight; i++) {
				final int c = pxtocol(i);
				colour_map[i] = (c == 0 || c == 255) ? background : new Color(c, c, c);
				inverse_colour_map[i] = (c == 0 || c == 255) ? default_txt_colour_background : ( c < 170 ? Color.white : Color.black);
			}
		}
		
		repaint();
	}
	
	private int valtologpx(final double val) {
		if (val < LOWEST_VAL) return nheight;
		else if (val >= 1) return 0;
		return (int) (Math.log10(val) * nheight / LOWEST_DB);
	}
	
	private int dbtopx(final double db) {
		final int px = (int) (db * nheight / LOWEST_DB);
		return (px < 0) ? 0 : (px >= nheight ? nheight - 1 : px);
	}
	
	private double pxtoval(final int px) {
		final double db = LOWEST_DB * px / (double) nheight;
		return Math.pow(10, db);
	}
	
	private int pxtocol(final int px) {
		final double val = pxtoval(px);
		final int col = (int) (255 * (val - min) / span);
		return (col < 0) ? (0) : ((col > 255) ? (255) : (col));
	}
	
	@Override
	public void paint(Graphics g) {
		
		synchronized (locker) {

			final int width = this.nwidth;
			final int height = this.nheight;

			final int minpx = valtologpx(this.min);
			final int maxpx = valtologpx(this.max);

			if (!font_set) {
				final Font existing = g.getFont();
				font = new Font(existing.getFontName(), Font.PLAIN, (int) (existing.getSize()*FONT_SIZE_COEFF));
				half_fontsize = font.getSize()/2;
				g.setFont(font);
				font_set = true;
			} else {
				g.setFont(font);
			}
			
			if (colour_map == null) return;

			// start drawing
			g.setColor(background);
			g.fillRect(0, 0, width, height);

			final int maxvalidpx = Math.min(minpx, nheight);
			for (int y = (maxpx < 0) ? 0 : maxpx ; y < maxvalidpx; y++) {
				g.setColor(colour_map[y]);
				g.drawLine(0, y, nwidth, y);
			}

			final int linewidth = width / 8;
			final int small_linewidth = width / 20;
			final int textoffset = linewidth + 2*small_linewidth;

			g.setColor(default_txt_colour_background);
			g.drawString("dB", textoffset, 2*half_fontsize);
			for (double i = LOWEST_DB; i < 0 ; i+=DB_STEP) {
				final int y = dbtopx(i);
				g.setColor(inverse_colour_map[y]);
				g.drawString(String.valueOf(i), textoffset, y+half_fontsize);
				g.drawLine(0, y, linewidth, y);
			}

			for (double i = LOWEST_DB; i < 0 ; i+=DB_LINE_STEP) {
				final int y = dbtopx(i);
				g.setColor(inverse_colour_map[y]);
				g.drawLine(0, y, small_linewidth, y);
			}

		}
	}
	
}
