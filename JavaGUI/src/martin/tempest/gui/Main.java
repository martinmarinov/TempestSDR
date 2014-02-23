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

import java.awt.Component;
import java.awt.EventQueue;
import java.awt.Rectangle;
import java.awt.Toolkit;

import javax.imageio.ImageIO;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JComboBox;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JSpinner;
import javax.swing.SpinnerModel;
import javax.swing.SpinnerNumberModel;
import javax.swing.JTextField;
import javax.swing.DefaultComboBoxModel;

import martin.tempest.core.TSDRLibrary;
import martin.tempest.core.TSDRLibrary.PARAM;
import martin.tempest.core.TSDRLibrary.PARAM_DOUBLE;
import martin.tempest.core.TSDRLibrary.SYNC_DIRECTION;
import martin.tempest.core.exceptions.TSDRException;
import martin.tempest.core.exceptions.TSDRLoadPluginException;
import martin.tempest.gui.HoldButton.HoldListener;
import martin.tempest.sources.TSDRSource;
import martin.tempest.sources.TSDRSource.TSDRSourceParamChangedListener;

import javax.swing.JSlider;

import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.image.BufferedImage;

import javax.swing.SwingConstants;
import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;
import javax.swing.JCheckBox;

import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.prefs.Preferences;

import javax.swing.JPanel;
import javax.swing.border.TitledBorder;
import javax.swing.UIManager;
import javax.swing.JScrollPane;

public class Main implements TSDRLibrary.FrameReadyCallback, TSDRSourceParamChangedListener, OnTSDRParamChangedCallback {
	
	private final static String SNAPSHOT_FORMAT = "png";
	private final static int OSD_TIME = 2000;
	
	private final static int FRAMERATE_SIGNIFICANT_FIGURES = 6;
	private final static long FREQUENCY_STEP = 1000000;
	
	private final static double FRAMERATE_MIN_CHANGE = 1.0/Math.pow(10, FRAMERATE_SIGNIFICANT_FIGURES);
	private final static String FRAMERATE_FORMAT = "%."+FRAMERATE_SIGNIFICANT_FIGURES+"f";

	private final Preferences prefs = Preferences.userNodeForPackage(this.getClass());
	
	private final static String PREF_WIDTH = "width";
	private final static String PREF_HEIGHT = "height";
	private final static String PREF_FRAMERATE = "framerate";
	private final static String PREF_COMMAND_PREFIX = "command";
	private final static String PREF_FREQ = "frequency";
	private final static String PREF_GAIN = "gain";
	private final static String PREF_MOTIONBLUR = "motionblur";
	private final static String PREF_SOURCE_ID = "source_id";

	private final JComponent additionalTweaks[] = new JComponent[] {
			new ParametersCheckbox(PARAM.AUTOPIXELRATE, "Auto pixel rate", this, prefs, false),
			new ParametersCheckbox(PARAM.AUTOSHIFT, "Auto shift", this, prefs, false),
	};

	private final SpinnerModel frequency_spinner_model = new SpinnerNumberModel(new Long(prefs.getLong(PREF_FREQ, 400000000)), new Long(0), new Long(2147483647), new Long(FREQUENCY_STEP));
	
	private JFrame frmTempestSdr;
	private JFrame fullscreenframe;
	private JSpinner spWidth;
	private JSpinner spHeight;
	@SuppressWarnings("rawtypes")
	private JComboBox cbVideoModes;
	@SuppressWarnings("rawtypes")
	private JComboBox cbDevice;
	private JSpinner spFrequency;
	private JLabel lblFrequency;
	private JSlider slGain;
	private JSlider slMotionBlur;
	private JLabel lblGain;
	private JButton btnStartStop;
	private final TSDRLibrary mSdrlib;
	private ImageVisualizer visualizer;
	private Rectangle visualizer_bounds;
	private double framerate = 25;
	private JTextField txtFramerate;
	private HoldButton btnLowerFramerate, btnHigherFramerate, btnUp, btnDown, btnLeft, btnRight;
	private String current_plugin_name = "";
	private JPanel pnInputDeviceSettings;
	
	private final TSDRSource[] souces = TSDRSource.getAvailableSources();
	private final VideoMode[] videomodes = VideoMode.getVideoModes();
	
	private volatile boolean snapshot = false;
	
	private boolean video_mode_change_manually_triggered = false;
	
	/**
	 * Launch the application.
	 */
	public static void main(String[] args) {
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				try {
					Main window = new Main();
					window.frmTempestSdr.setVisible(true);
				} catch (Exception e) {
					displayException(null, e);
				}
			}
		});
	}

	private static void displayException(final Component panel, final Throwable t) {
		final String msg = t.getMessage();
		final String exceptionname = t.getClass().getSimpleName();
		JOptionPane.showMessageDialog(panel, (msg == null) || (msg.trim().isEmpty()) ? "A "+exceptionname+" occured." : msg, exceptionname, JOptionPane.ERROR_MESSAGE);
		t.printStackTrace();
	}
	
	/**
	 * Create the application.
	 * @throws TSDRException 
	 */
	public Main() throws TSDRException {
		mSdrlib = new TSDRLibrary();
		mSdrlib.registerFrameReadyCallback(this);
		initialize();
	}

	/**
	 * Initialize the contents of the frame.
	 */
	@SuppressWarnings({ "rawtypes", "unchecked" })
	private void initialize() {
		final int width_initial = prefs.getInt(PREF_WIDTH, 576);
		final int height_initial = prefs.getInt(PREF_HEIGHT, 625);
		final double framerate_initial = prefs.getDouble(PREF_FRAMERATE, framerate);
		final int closest_videomode_id = findClosestVideoModeId(width_initial, height_initial, framerate_initial, videomodes);
		
		frmTempestSdr = new JFrame();
		frmTempestSdr.setFocusable(true);
		frmTempestSdr.setFocusableWindowState(true);
		frmTempestSdr.addKeyListener(keyhook);
		frmTempestSdr.setResizable(false);
		frmTempestSdr.setTitle("TempestSDR");
		frmTempestSdr.setBounds(100, 100, 749, 654);
		frmTempestSdr.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frmTempestSdr.addMouseListener(new MouseAdapter() {
			@Override
			public void mouseClicked(MouseEvent e) {
				frmTempestSdr.requestFocus();
			}
		});
		
		visualizer = new ImageVisualizer();
		visualizer.setBounds(12, 13, 551, 422);
		visualizer.addKeyListener(keyhook);
		visualizer.setFocusable(true);
		visualizer.addMouseListener(new MouseAdapter() {
			@Override
			public void mouseClicked(MouseEvent e) {
				if (e.getClickCount() == 2) {
					if (fullscreenframe.isVisible()) {
						fullscreenframe.remove(visualizer);
						visualizer.setBounds(visualizer_bounds);
						frmTempestSdr.getContentPane().add(visualizer);
						fullscreenframe.setVisible(false);
						frmTempestSdr.requestFocus();
					} else {
						visualizer_bounds = visualizer.getBounds();
						frmTempestSdr.remove(visualizer);
						fullscreenframe.getContentPane().add(visualizer);
						fullscreenframe.setVisible(true);
						visualizer.requestFocus();
					}
				} else
					visualizer.requestFocus();
				
			}
		});
		frmTempestSdr.getContentPane().setLayout(null);
		frmTempestSdr.getContentPane().add(visualizer);
		
		cbDevice = new JComboBox();
		cbDevice.setBounds(12, 448, 218, 22);
		final int cbDeviceIndex = prefs.getInt(PREF_SOURCE_ID, 0);
		current_plugin_name = souces[cbDeviceIndex < souces.length ? cbDeviceIndex : 0].descr;
		cbDevice.setModel(new DefaultComboBoxModel(souces));
		cbDevice.setSelectedIndex(cbDeviceIndex);
		cbDevice.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				onPluginSelected();
			}
		});
		frmTempestSdr.getContentPane().add(cbDevice);
		
		btnStartStop = new JButton("Start");
		btnStartStop.setBounds(575, 13, 159, 25);
		btnStartStop.setEnabled(false);
		btnStartStop.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				performStartStop();
			}
		});
		frmTempestSdr.getContentPane().add(btnStartStop);
		
		cbVideoModes = new JComboBox();
		cbVideoModes.setBounds(575, 43, 159, 22);
		cbVideoModes.setModel(new DefaultComboBoxModel(videomodes));
		if (closest_videomode_id != -1) cbVideoModes.setSelectedIndex(closest_videomode_id);
		cbVideoModes.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				onVideoModeSelected((VideoMode) cbVideoModes.getSelectedItem());
			}
		});
		frmTempestSdr.getContentPane().add(cbVideoModes);
		
		JLabel lblWidth = new JLabel("Width:");
		lblWidth.setBounds(575, 73, 65, 16);
		lblWidth.setHorizontalAlignment(SwingConstants.RIGHT);
		frmTempestSdr.getContentPane().add(lblWidth);
		
		spWidth = new JSpinner();
		spWidth.setBounds(645, 70, 89, 22);
		spWidth.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onResolutionChange();
			}
		});
		spWidth.setModel(new SpinnerNumberModel(width_initial, 1, 10000, 1));
		frmTempestSdr.getContentPane().add(spWidth);
		
		JLabel lblHeight = new JLabel("Height:");
		lblHeight.setBounds(575, 100, 65, 16);
		lblHeight.setHorizontalAlignment(SwingConstants.RIGHT);
		frmTempestSdr.getContentPane().add(lblHeight);
		
		spHeight = new JSpinner();
		spHeight.setBounds(645, 97, 89, 22);
		spHeight.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onResolutionChange();
			}
		});
		spHeight.setModel(new SpinnerNumberModel(height_initial, 1, 10000, 1));
		frmTempestSdr.getContentPane().add(spHeight);
		
		JLabel lblFramerate = new JLabel("Framerate:");
		lblFramerate.setBounds(575, 127, 65, 16);
		lblFramerate.setHorizontalAlignment(SwingConstants.RIGHT);
		frmTempestSdr.getContentPane().add(lblFramerate);
		
		lblGain = new JLabel("Gain:");
		lblGain.setBounds(575, 374, 65, 16);
		lblGain.setHorizontalAlignment(SwingConstants.LEFT);
		frmTempestSdr.getContentPane().add(lblGain);
		
		slGain = new JSlider();
		slGain.setBounds(575, 399, 159, 26);
		slGain.setValue((int) (prefs.getFloat(PREF_GAIN, 0.5f) * (slGain.getMaximum() - slGain.getMinimum()) + slGain.getMinimum()));
		slGain.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				onGainLevelChanged();
			}
		});
		frmTempestSdr.getContentPane().add(slGain);
		
		lblFrequency = new JLabel("Frequency:");
		lblFrequency.setBounds(242, 448, 65, 16);
		lblFrequency.setHorizontalAlignment(SwingConstants.RIGHT);
		frmTempestSdr.getContentPane().add(lblFrequency);
		
		spFrequency = new JSpinner();
		spFrequency.setBounds(319, 448, 244, 22);
		spFrequency.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onCenterFreqChange();
			}
		});
		spFrequency.setModel(frequency_spinner_model);
		frmTempestSdr.getContentPane().add(spFrequency);
		
		final JCheckBox chckbxInvertedColours = new JCheckBox("Inverted colours");
		chckbxInvertedColours.setBounds(575, 180, 159, 25);
		chckbxInvertedColours.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				mSdrlib.setInvertedColors(chckbxInvertedColours.isSelected());
			}
		});
		chckbxInvertedColours.setHorizontalAlignment(SwingConstants.LEFT);
		frmTempestSdr.getContentPane().add(chckbxInvertedColours);
		
		btnUp = new HoldButton("Up");
		btnUp.setBounds(615, 255, 78, 25);
		btnUp.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.UP, clickssofar);
			}
		});
		frmTempestSdr.getContentPane().add(btnUp);
		
		btnLeft = new HoldButton("Left");
		btnLeft.setBounds(575, 280, 65, 25);
		btnLeft.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.LEFT, clickssofar);
			}
		});
		frmTempestSdr.getContentPane().add(btnLeft);
		
		btnRight = new HoldButton("Right");
		btnRight.setBounds(669, 280, 65, 25);
		btnRight.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.RIGHT, clickssofar);
			}
		});
		frmTempestSdr.getContentPane().add(btnRight);
		
		btnDown = new HoldButton("Down");
		btnDown.setBounds(615, 306, 78, 25);
		btnDown.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.DOWN, clickssofar);
			}
		});
		frmTempestSdr.getContentPane().add(btnDown);
		
		txtFramerate = new JTextField();
		txtFramerate.setBounds(645, 124, 89, 22);
		txtFramerate.setText(String.format(FRAMERATE_FORMAT, framerate_initial));
		framerate = framerate_initial;
		txtFramerate.addFocusListener(new FocusAdapter() {
			@Override
			public void focusLost(FocusEvent e) {
				onFrameRateTextChanged();
			}
		});
		txtFramerate.addKeyListener(new KeyAdapter() {
			@Override
			public void keyReleased(KeyEvent evt) {
				if(evt.getKeyCode() == KeyEvent.VK_ENTER)
					onFrameRateTextChanged();
			}
		});
		frmTempestSdr.getContentPane().add(txtFramerate);
		txtFramerate.setColumns(10);
		
		btnLowerFramerate = new HoldButton("<");
		btnLowerFramerate.setBounds(645, 146, 41, 25);
		btnLowerFramerate.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onFrameRateChanged(true, clickssofar);
			}
		});
		frmTempestSdr.getContentPane().add(btnLowerFramerate);
		
		btnHigherFramerate = new HoldButton(">");
		btnHigherFramerate.setBounds(693, 146, 41, 25);
		btnHigherFramerate.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onFrameRateChanged(false, clickssofar);
			}
		});
		frmTempestSdr.getContentPane().add(btnHigherFramerate);
		
		slMotionBlur = new JSlider();
		slMotionBlur.setBounds(645, 212, 89, 22);
		slMotionBlur.setValue((int) (prefs.getFloat(PREF_MOTIONBLUR, 0.0f) * (slMotionBlur.getMaximum() - slMotionBlur.getMinimum()) + slMotionBlur.getMinimum()));
		slMotionBlur.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				onMotionBlurLevelChanged();
			}
		});
		frmTempestSdr.getContentPane().add(slMotionBlur);
		
		JButton btnSnapshot = new HoldButton("Snapshot");
		btnSnapshot.setBounds(575, 336, 159, 25);
		btnSnapshot.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				snapshot = true;
			}
		});
		frmTempestSdr.getContentPane().add(btnSnapshot);
		
		JLabel lblMotionBlur = new JLabel("Lowpass:");
		lblMotionBlur.setBounds(575, 212, 65, 16);
		lblMotionBlur.setHorizontalAlignment(SwingConstants.RIGHT);
		frmTempestSdr.getContentPane().add(lblMotionBlur);
		
		pnInputDeviceSettings = new JPanel();
		pnInputDeviceSettings.setBounds(12, 483, 551, 123);
		pnInputDeviceSettings.setBorder(new TitledBorder(UIManager.getBorder("TitledBorder.border"), "Input device settings", TitledBorder.LEADING, TitledBorder.TOP, null, null));
		frmTempestSdr.getContentPane().add(pnInputDeviceSettings);
		pnInputDeviceSettings.setLayout(null);
		
		JScrollPane scrAdvancedTweaks = new JScrollPane();
		scrAdvancedTweaks.setBounds(575, 448, 159, 158);
		scrAdvancedTweaks.setViewportBorder(new TitledBorder(null, "Advanced tweaks", TitledBorder.LEADING, TitledBorder.TOP, null, null));
		frmTempestSdr.getContentPane().add(scrAdvancedTweaks);
		
		JPanel pnAdvancedTweaksContainer = new JPanel();
		scrAdvancedTweaks.setViewportView(pnAdvancedTweaksContainer);
		pnAdvancedTweaksContainer.setLayout(null);
		
		int containery = 0;
		for (int i = 0; i < additionalTweaks.length; i++) {
			additionalTweaks[i].setBounds(0, containery, 159, 25);
			containery += 25;
			pnAdvancedTweaksContainer.add(additionalTweaks[i]);
		}
		
		frmTempestSdr.setFocusableWindowState(true);
		frmTempestSdr.requestFocus();
		
		onGainLevelChanged();
		onMotionBlurLevelChanged();
		
		// full screen frame
		fullscreenframe = new JFrame("Video display");
		fullscreenframe.setFocusable(true);
		fullscreenframe.addKeyListener(keyhook);
		Toolkit tk = Toolkit.getDefaultToolkit();  
		int xSize = ((int) tk.getScreenSize().getWidth());  
		int ySize = ((int) tk.getScreenSize().getHeight());  
		fullscreenframe.setSize(xSize,ySize);
		fullscreenframe.setUndecorated(true);
		fullscreenframe.setLocation(0, 0);
		
		onPluginSelected();
	}
	
	private int findClosestVideoModeId(final int width, final int height, final double framerate, final VideoMode[] modes) {
		int mode = -1;
		double diff = -1;
		for (int i = 0; i < modes.length; i++) {
			final VideoMode m = modes[i];
			if (m.height == height && m.width == width) {
				final double delta = Math.abs(m.refreshrate-framerate);
				if (diff == -1 || delta < diff) {
					diff = delta;
					mode = i;
				}
			}
		}
		return mode;
	}
	
	private void onVideoModeSelected(final VideoMode m) {
		if (video_mode_change_manually_triggered) return;
		
		spWidth.setValue(m.width);
		spHeight.setValue(m.height);
		framerate = m.refreshrate;
		setFrameRate(framerate);
	}
	
	private void performStartStop() {
		
		if (mSdrlib.isRunning()) {
			
			new Thread() {
				@Override
				public void run() {
					try {
						if (!mSdrlib.isRunning()) btnStartStop.setEnabled(false);
						mSdrlib.stop();
					} catch (TSDRException e) {
						displayException(frmTempestSdr, e);
					}
				}
			}.start();
			
		} else {

			try {

				final Long newfreq = (Long) spFrequency.getValue();
				if (newfreq != null && newfreq > 0)
					mSdrlib.setBaseFreq(newfreq);

				float gain = (slGain.getValue() - slGain.getMinimum()) / (float) (slGain.getMaximum() - slGain.getMinimum());
				if (gain < 0.0f) gain = 0.0f; else if (gain > 1.0f) gain = 1.0f;
				mSdrlib.setGain(gain);

			} catch (TSDRException e) {
				displayException(frmTempestSdr, e);
				return;
			}

			try {
				mSdrlib.startAsync((Integer) spWidth.getValue(), (Integer) spHeight.getValue(), framerate);
			} catch (TSDRException e1) {
				displayException(frmTempestSdr, e1);
				btnStartStop.setText("Start");
				return;
			}

			btnStartStop.setText("Stop");
			cbDevice.setEnabled(false);
		}
		
	}
	
	private void onSync(final SYNC_DIRECTION dir, final int repeatssofar) {
		mSdrlib.sync(repeatssofar, dir);
	}
	
	private void onResolutionChange() {
		try {
			final int width = (Integer) spWidth.getValue();
			final int height = (Integer) spHeight.getValue();
			mSdrlib.setResolution(width, height, framerate);
			
			prefs.putInt(PREF_WIDTH, width);
			prefs.putInt(PREF_HEIGHT, height);
			prefs.putDouble(PREF_FRAMERATE, framerate);
			
			final int closest_videomode_id = findClosestVideoModeId(width, height, framerate, videomodes);
			video_mode_change_manually_triggered = true;
			if (closest_videomode_id != -1) cbVideoModes.setSelectedIndex(closest_videomode_id);
			video_mode_change_manually_triggered = false;
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onCenterFreqChange() {
		final Long newfreq = (Long) spFrequency.getValue();
		
		if (newfreq == null || newfreq < 0) return;
		
		try {
			mSdrlib.setBaseFreq(newfreq);
			visualizer.setOSD("Freq: "+newfreq+" Hz", OSD_TIME);
			prefs.putLong(PREF_FREQ, newfreq);
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onGainLevelChanged() {
		float gain = (slGain.getValue() - slGain.getMinimum()) / (float) (slGain.getMaximum() - slGain.getMinimum());
		if (gain < 0.0f) gain = 0.0f; else if (gain > 1.0f) gain = 1.0f;
		
		try {
			mSdrlib.setGain(gain);
			prefs.putFloat(PREF_GAIN, gain);
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onMotionBlurLevelChanged() {
		float mblur = slMotionBlur.getValue() / 100.0f;
		
		try {
			mSdrlib.setMotionBlur(mblur);
			prefs.putFloat(PREF_MOTIONBLUR, mblur);
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onFrameRateTextChanged() {
		try {
			final Double val = Double.parseDouble(txtFramerate.getText().trim());
			if (val != null && val > 0) framerate = val;
		} catch (NumberFormatException e) {}
		setFrameRate(framerate);
	}
	
	private void setFrameRate(final double val) {
		final String frameratetext = String.format(FRAMERATE_FORMAT, val);
		txtFramerate.setText(frameratetext);
		try {
			final int width = (Integer) spWidth.getValue();
			final int height = (Integer) spHeight.getValue();
			mSdrlib.setResolution(width, height, val);
			visualizer.setOSD("Framerate: "+frameratetext+" fps", OSD_TIME);
			
			prefs.putInt(PREF_WIDTH, width);
			prefs.putInt(PREF_HEIGHT, height);
			prefs.putDouble(PREF_FRAMERATE, val);
			
			final int closest_videomode_id = findClosestVideoModeId(width, height, val, videomodes);
			video_mode_change_manually_triggered = true;
			if (closest_videomode_id != -1) cbVideoModes.setSelectedIndex(closest_videomode_id);
			video_mode_change_manually_triggered = false;
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onKeyboardKeyPressed(final KeyEvent e) {
		final int keycode = e.getKeyCode();

		if (e.isShiftDown()) {
			switch (keycode) {
			case KeyEvent.VK_LEFT:
				btnLeft.doHold();
				visualizer.setOSD("Move: Left", OSD_TIME);
				break;
			case KeyEvent.VK_RIGHT:
				btnRight.doHold();
				visualizer.setOSD("Move: Right", OSD_TIME);
				break;
			case KeyEvent.VK_UP:
				btnUp.doHold();
				visualizer.setOSD("Move: Up", OSD_TIME);
				break;
			case KeyEvent.VK_DOWN:
				btnDown.doHold();
				visualizer.setOSD("Move: Down", OSD_TIME);
				break;
			}
		} else {
			switch (keycode) {
			case KeyEvent.VK_RIGHT:
				btnHigherFramerate.doHold();
				break;
			case KeyEvent.VK_LEFT:
				btnLowerFramerate.doHold();
				break;
			case KeyEvent.VK_UP:
				spFrequency.setValue(frequency_spinner_model.getNextValue());
				break;
			case KeyEvent.VK_DOWN:
				spFrequency.setValue(frequency_spinner_model.getPreviousValue());
				break;
			}
		}
	}
	
	private void onKeyboardKeyReleased(final KeyEvent e) {
		final int keycode = e.getKeyCode();
		
		if (e.isShiftDown()) {
			switch (keycode) {
			case KeyEvent.VK_LEFT:
				btnLeft.doRelease();
				btnHigherFramerate.doRelease();
				break;
			case KeyEvent.VK_RIGHT:
				btnRight.doRelease();
				btnLowerFramerate.doRelease();
				break;
			case KeyEvent.VK_UP:
				btnUp.doRelease();
				break;
			case KeyEvent.VK_DOWN:
				btnDown.doRelease();
				break;
			}
		} else {
			switch (keycode) {
			case KeyEvent.VK_RIGHT:
				btnLeft.doRelease();
				btnHigherFramerate.doRelease();
				break;
			case KeyEvent.VK_LEFT:
				btnRight.doRelease();
				btnLowerFramerate.doRelease();
				break;
			}
		}
	}
	
	private void onFrameRateChanged(boolean left, int clicksofar) {
		double amount = clicksofar * clicksofar * FRAMERATE_MIN_CHANGE;
		if (amount > 0.05) amount = 0.05;
		if (left && framerate > amount)
			framerate -= amount;
		else if (!left)
			framerate += amount;
		setFrameRate(framerate);
	}
	
	private void onPluginSelected() {
		
		if (!mSdrlib.isRunning()) btnStartStop.setEnabled(false);
		try {
			mSdrlib.unloadPlugin();
		} catch (TSDRException e) {};
		
		pnInputDeviceSettings.removeAll();
		pnInputDeviceSettings.revalidate();
		pnInputDeviceSettings.repaint(); 
		
		final int id = cbDevice.getSelectedIndex();
		prefs.putInt(PREF_SOURCE_ID, id);
		current_plugin_name = souces[id].descr;
		
		final String preferences = prefs.get(PREF_COMMAND_PREFIX+current_plugin_name, "");
		final TSDRSource current = (TSDRSource) cbDevice.getSelectedItem();
		current.setOnParameterChangedCallback(this);
		
		current.populateGUI(pnInputDeviceSettings, preferences);
		pnInputDeviceSettings.revalidate();
		pnInputDeviceSettings.repaint(); 
	}

	@Override
	public void onFrameReady(TSDRLibrary lib, BufferedImage frame) {
		if (snapshot) {
			snapshot = false;
			try {
				final int freq = (int) Math.abs(((Long) spFrequency.getValue())/1000000.0d);
				final String filename = "TSDR_"+(new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss")).format(new Date())+"_"+freq+"MHz"+"."+SNAPSHOT_FORMAT;
				final File outputfile = new File(filename);
				ImageIO.write(frame, SNAPSHOT_FORMAT, outputfile);
				visualizer.setOSD("Saved to "+outputfile.getAbsolutePath(), OSD_TIME);
			} catch (Throwable e) {
				visualizer.setOSD("Failed to capture snapshot", OSD_TIME);
				e.printStackTrace();
			}
			
		}
		visualizer.drawImage(frame);
	}

	@Override
	public void onException(TSDRLibrary lib, Exception e) {
		btnStartStop.setEnabled(true);
		btnStartStop.setText("Start");
		cbDevice.setEnabled(true);
		displayException(frmTempestSdr, e);
		System.gc();
	}

	@Override
	public void onStopped(TSDRLibrary lib) {
		btnStartStop.setEnabled(true);
		btnStartStop.setText("Start");
		cbDevice.setEnabled(true);
		System.gc();
	}
	
	private final KeyAdapter keyhook = new KeyAdapter() {
		@Override
		public void keyPressed(KeyEvent e) {
			onKeyboardKeyPressed(e);
			super.keyPressed(e);
		}
		@Override
		public void keyReleased(KeyEvent e) {
			onKeyboardKeyReleased(e);
			super.keyReleased(e);
		}
	};

	@Override
	public void onParametersChanged(TSDRSource source) {
		cbDevice.setEnabled(false);
		if (!mSdrlib.isRunning())  btnStartStop.setEnabled(false);
		
		try {
			try {
				mSdrlib.unloadPlugin();
			} catch (TSDRLoadPluginException e) {};
			
			mSdrlib.loadPlugin(source);
		} catch (Throwable t) {
			if (!mSdrlib.isRunning())  btnStartStop.setEnabled(false);
			displayException(frmTempestSdr, t);
			cbDevice.setEnabled(true);
			return;
		}
		cbDevice.setEnabled(true);
		btnStartStop.setEnabled(true);
		
		// set the field with the text arguments
		prefs.put(PREF_COMMAND_PREFIX+current_plugin_name, source.getParams());
	}

	@Override
	public void onSetParam(PARAM param, long value) {
		try {
			if (mSdrlib != null)
				mSdrlib.setParam(param, value);
		} catch (Throwable e) {
			displayException(frmTempestSdr, e);
			return;
		}
	}

	@Override
	public void onSetParam(PARAM_DOUBLE param, double value) {
		try {
			if (mSdrlib != null)
				mSdrlib.setParamDouble(param, value);
		} catch (Throwable e) {
			displayException(frmTempestSdr, e);
			return;
		}
	}

 }
