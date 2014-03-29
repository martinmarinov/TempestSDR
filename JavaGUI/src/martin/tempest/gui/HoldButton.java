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

import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import javax.swing.JButton;

public class HoldButton extends JButton {
	private static final long DEFAULT_REPEAT_INTERVAL = 50;
	private static final long serialVersionUID = 7176946053407595449L;

	private final ArrayList<HoldListener> callbacks = new ArrayList<HoldButton.HoldListener>();
	private final Timer timer = new Timer();
	private int clickssofar = 0;
	
	volatile boolean running = false;
	
	private TimerTask task = null;
	private final long repeat_interval;
	
	public HoldButton(final String caption) {
		this(DEFAULT_REPEAT_INTERVAL, caption);
	}
	
	public void doHold() {
		if (!isEnabled()) return;
		if (running) return;
		running = true;
		clickssofar = 0;
		timer.scheduleAtFixedRate(task = new TimerTask() {
			  @Override
			  public void run() {
				  clickssofar++;
				  notifyHoldListeners(clickssofar);
			  }
		}, 0, repeat_interval);
	}
	
	public void doRelease() {
		if (!isEnabled()) return;
		if (!running) return;
		if (task != null) {
			task.cancel();
			task = null;
		}
		running = false;
	}
	
	public HoldButton(final long repeat_interval, final String caption) {
		super(caption);
		this.repeat_interval = repeat_interval;
		addMouseListener(new MouseListener() {
			
			@Override
			public void mouseReleased(MouseEvent arg0) {
				doRelease();
			}
			
			@Override
			public void mousePressed(MouseEvent arg0) {
				doHold();
			}

			public void mouseExited(MouseEvent arg0) {}
			public void mouseEntered(MouseEvent arg0) {}
			
			@Override
			public void mouseClicked(MouseEvent arg0) {
				notifyHoldListeners(1);
			}
		});
	}
	
	private void notifyHoldListeners(final int clickssofar) {
		for (final HoldListener listener : callbacks) listener.onHold(clickssofar);
	}
	
	public void addHoldListener(final HoldListener listener) {
		if (!callbacks.contains(listener))
			callbacks.add(listener);
	}
	
	public void removeHoldListener(final HoldListener listener) {
		callbacks.remove(listener);
	}
	
	public interface HoldListener {
		void onHold(final int clickssofar);
	}
}
