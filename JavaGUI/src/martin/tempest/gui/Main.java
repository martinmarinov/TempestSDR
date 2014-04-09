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
import java.awt.Container;
import java.awt.Dialog;
import java.awt.EventQueue;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.Toolkit;

import javax.imageio.ImageIO;
import javax.swing.JComponent;
import javax.swing.JDialog;
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
import martin.tempest.sources.TSDRSource.ActionListenerRegistrator;
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
import java.util.ArrayList;
import java.util.Date;
import java.util.prefs.Preferences;

import javax.swing.JPanel;
import javax.swing.JToggleButton;
import javax.swing.JTabbedPane;
import javax.swing.JMenuBar;
import javax.swing.JMenu;
import javax.swing.JMenuItem;

public class Main implements TSDRLibrary.FrameReadyCallback, TSDRLibrary.IncomingValueCallback, TSDRSourceParamChangedListener, OnTSDRParamChangedCallback {
	
	private final static String SNAPSHOT_FORMAT = "png";
	private final static int OSD_TIME = 2000;
	//private final static int OSD_TIME_LONG = 5000;
	
	private final static int FRAMERATE_SIGNIFICANT_FIGURES = 6;
	private final static long FREQUENCY_STEP = 5000000;
	
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
	private final static String PREF_HEIGHT_LOCK = "height_lock";

	private final JComponent additionalTweaks[] = new JComponent[] {
			// new ParametersCheckbox(PARAM.AUTOSHIFT, "Auto shift", this, prefs, false),
	};

	private final SpinnerModel frequency_spinner_model = new SpinnerNumberModel(new Long(prefs.getLong(PREF_FREQ, 400000000)), new Long(0), new Long(2147483647), new Long(FREQUENCY_STEP));
	
	private JFrame frmTempestSdr;
	private JFrame fullscreenframe;
	private JDialog deviceframe;
	private JSpinner spWidth;
	private JSpinner spHeight;
	@SuppressWarnings("rawtypes")
	private JComboBox cbVideoModes;
	private JSpinner spFrequency;
	private JLabel lblFrequency;
	private JSlider slGain;
	private JSlider slMotionBlur;
	private JLabel lblGain;
	private JButton btnStartStop;
	private final TSDRLibrary mSdrlib;
	private ImageVisualizer visualizer;
	private PlotVisualizer line_plotter, frame_plotter;
	private Rectangle visualizer_bounds;
	private double framerate = 25;
	private JTextField txtFramerate;
	private HoldButton btnLowerFramerate, btnHigherFramerate, btnUp, btnDown, btnLeft, btnRight;
	private JPanel pnInputDeviceSettings;
	private ParametersToggleButton tglbtnAutoResolution, tglbtnAutoPosition, tglbtnPllFramerate;
	private JToggleButton tglbtnLockHeightAndFramerate;
	private JOptionPane optpaneDevices;
	
	private final TSDRSource[] souces = TSDRSource.getAvailableSources();
	private final JMenuItem[] souces_menues = new JMenuItem[souces.length];
	private final VideoMode[] videomodes = VideoMode.getVideoModes();

	
	private volatile boolean snapshot = false;
	private volatile boolean height_change_from_auto = false;
	
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
		mSdrlib.registerValueChangedCallback(this);
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
		final int closest_videomode_id = VideoMode.findClosestVideoModeId(width_initial, height_initial, framerate_initial, videomodes);
		final boolean heightlock_enabled = prefs.getBoolean(PREF_HEIGHT_LOCK, true);
		
		frmTempestSdr = new JFrame();
		frmTempestSdr.setFocusable(true);
		frmTempestSdr.setFocusableWindowState(true);
		frmTempestSdr.addKeyListener(keyhook);
		frmTempestSdr.setResizable(false);
		frmTempestSdr.setTitle("TempestSDR");
		frmTempestSdr.setBounds(100, 100, 813, 704);
		frmTempestSdr.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frmTempestSdr.addMouseListener(new MouseAdapter() {
			@Override
			public void mouseClicked(MouseEvent e) {
				frmTempestSdr.requestFocus();
			}
		});
		
		visualizer = new ImageVisualizer();
		visualizer.setBounds(10, 33, 551, 411);
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
		
		line_plotter = new PlotVisualizer();
		line_plotter.setBounds(10, 490, 793, 81);
		frmTempestSdr.getContentPane().add(line_plotter);
		
		btnStartStop = new JButton("Start");
		btnStartStop.setBounds(573, 33, 230, 25);
		btnStartStop.setEnabled(false);
		btnStartStop.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				performStartStop();
			}
		});
		frmTempestSdr.getContentPane().add(btnStartStop);

		
		lblFrequency = new JLabel("Frequency:");
		lblFrequency.setBounds(240, 456, 139, 16);
		lblFrequency.setHorizontalAlignment(SwingConstants.RIGHT);
		frmTempestSdr.getContentPane().add(lblFrequency);
		
		spFrequency = new JSpinner();
		spFrequency.setBounds(379, 456, 182, 22);
		spFrequency.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onCenterFreqChange();
			}
		});
		spFrequency.setModel(frequency_spinner_model);
		frmTempestSdr.getContentPane().add(spFrequency);
		framerate = framerate_initial;
		
		frame_plotter = new PlotVisualizer();
		frame_plotter.setBounds(10, 583, 793, 81);
		frmTempestSdr.getContentPane().add(frame_plotter);
		
		tabbedPane = new JTabbedPane(JTabbedPane.TOP);
		tabbedPane.setBounds(573, 64, 230, 414);
		frmTempestSdr.getContentPane().add(tabbedPane);
		
		JPanel panel = new JPanel();
		tabbedPane.addTab("Basic", null, panel, null);
		panel.setLayout(null);
		
		cbVideoModes = new JComboBox();
		cbVideoModes.setBounds(8, 12, 172, 22);
		panel.add(cbVideoModes);
		cbVideoModes.setModel(new DefaultComboBoxModel(videomodes));
		if (closest_videomode_id != -1 && closest_videomode_id < videomodes.length && closest_videomode_id >= 0) cbVideoModes.setSelectedIndex(closest_videomode_id);

		
		tglbtnAutoResolution = new ParametersToggleButton(PARAM.AUTORESOLUTION, "A", prefs, true);
		tglbtnAutoResolution.setBounds(192, 12, 25, 22);
		panel.add(tglbtnAutoResolution);
		tglbtnAutoResolution.setToolTipText("Automatically detect resolution");
		tglbtnAutoResolution.setParaChangeCallback(this);
		tglbtnAutoResolution.setMargin(new Insets(0, 0, 0, 0));
		
		spWidth = new JSpinner();
		spWidth.setBounds(78, 39, 102, 22);
		panel.add(spWidth);
		spWidth.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onResolutionChange((Integer) spWidth.getValue(), (Integer) spHeight.getValue(), framerate);
			}
		});
		spWidth.setModel(new SpinnerNumberModel(width_initial, 1, 10000, 1));
		
		JLabel lblWidth = new JLabel("Width:");
		lblWidth.setBounds(8, 42, 65, 16);
		panel.add(lblWidth);
		lblWidth.setHorizontalAlignment(SwingConstants.RIGHT);
		
		JLabel lblHeight = new JLabel("Height:");
		lblHeight.setBounds(8, 69, 65, 16);
		panel.add(lblHeight);
		lblHeight.setHorizontalAlignment(SwingConstants.RIGHT);
		
		spHeight = new JSpinner();
		spHeight.setBounds(78, 66, 102, 22);
		panel.add(spHeight);
		spHeight.addChangeListener(new ChangeListener() {
			private Integer oldheight = null;
			public void stateChanged(ChangeEvent arg0) {
				
				final Integer width = (Integer) spWidth.getValue();
				final Integer newheight = (Integer) spHeight.getValue();
				
				if (oldheight != null && tglbtnLockHeightAndFramerate.isSelected() && !height_change_from_auto)
					onResolutionChange(width, newheight, framerate * oldheight / (double) newheight);
				else
					onResolutionChange(width, newheight, framerate);
				
				oldheight = newheight;
			}
		});
		spHeight.setModel(new SpinnerNumberModel(height_initial, 1, 10000, 1));
		
		tglbtnLockHeightAndFramerate = new JToggleButton("L");
		tglbtnLockHeightAndFramerate.setBounds(192, 65, 25, 22);
		panel.add(tglbtnLockHeightAndFramerate);
		tglbtnLockHeightAndFramerate.setToolTipText("Link the framerate with the height");
		tglbtnLockHeightAndFramerate.setSelected(heightlock_enabled);
		tglbtnLockHeightAndFramerate.setMargin(new Insets(0, 0, 0, 0));
		
		tglbtnPllFramerate = new ParametersToggleButton(PARAM.PLLFRAMERATE, "A", prefs, true);
		tglbtnPllFramerate.setBounds(192, 93, 25, 22);
		panel.add(tglbtnPllFramerate);
		tglbtnPllFramerate.setToolTipText("Automatically adjust the FPS to keep the video stable");
		tglbtnPllFramerate.setParaChangeCallback(this);
		tglbtnPllFramerate.setMargin(new Insets(0, 0, 0, 0));
		
		txtFramerate = new JTextField();
		txtFramerate.setBounds(78, 93, 102, 22);
		panel.add(txtFramerate);
		txtFramerate.setText(String.format(FRAMERATE_FORMAT, framerate_initial));
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
		txtFramerate.setColumns(10);
		
		JLabel lblFramerate = new JLabel("FPS:");
		lblFramerate.setBounds(8, 96, 65, 16);
		panel.add(lblFramerate);
		lblFramerate.setHorizontalAlignment(SwingConstants.RIGHT);
		
		btnLowerFramerate = new HoldButton("<");
		btnLowerFramerate.setBounds(78, 115, 41, 25);
		panel.add(btnLowerFramerate);
		btnLowerFramerate.setMargin(new Insets(0, 0, 0, 0));
		
		btnHigherFramerate = new HoldButton(">");
		btnHigherFramerate.setBounds(139, 116, 41, 25);
		panel.add(btnHigherFramerate);
		btnHigherFramerate.setMargin(new Insets(0, 0, 0, 0));
		
		btnUp = new HoldButton("Up");
		btnUp.setBounds(79, 152, 70, 25);
		panel.add(btnUp);
		btnUp.setMargin(new Insets(0, 0, 0, 0));
		
		btnLeft = new HoldButton("Left");
		btnLeft.setBounds(8, 183, 65, 25);
		panel.add(btnLeft);
		btnLeft.setMargin(new Insets(0, 0, 0, 0));
		
		tglbtnAutoPosition = new ParametersToggleButton(PARAM.AUTOSHIFT, "Auto", prefs, true);
		tglbtnAutoPosition.setBounds(78, 182, 70, 26);
		panel.add(tglbtnAutoPosition);
		tglbtnAutoPosition.setToolTipText("Automatically try to center on the image");
		tglbtnAutoPosition.setParaChangeCallback(this);
		tglbtnAutoPosition.setMargin(new Insets(0, 0, 0, 0));
		
		btnRight = new HoldButton("Right");
		btnRight.setBounds(152, 183, 65, 25);
		panel.add(btnRight);
		btnRight.setMargin(new Insets(0, 0, 0, 0));
		
		btnDown = new HoldButton("Down");
		btnDown.setBounds(78, 213, 70, 25);
		panel.add(btnDown);
		btnDown.setMargin(new Insets(0, 0, 0, 0));
		
		JLabel lblMotionBlur = new JLabel("Lpass:");
		lblMotionBlur.setBounds(8, 266, 65, 16);
		panel.add(lblMotionBlur);
		lblMotionBlur.setHorizontalAlignment(SwingConstants.RIGHT);
		
		slMotionBlur = new JSlider();
		slMotionBlur.setBounds(79, 266, 139, 22);
		panel.add(slMotionBlur);
		slMotionBlur.setValue((int) (prefs.getFloat(PREF_MOTIONBLUR, 0.0f) * (slMotionBlur.getMaximum() - slMotionBlur.getMinimum()) + slMotionBlur.getMinimum()));
		
		lblGain = new JLabel("Gain:");
		lblGain.setBounds(8, 286, 65, 16);
		panel.add(lblGain);
		lblGain.setHorizontalAlignment(SwingConstants.RIGHT);
		
		slGain = new JSlider();
		slGain.setBounds(79, 286, 138, 26);
		panel.add(slGain);
		slGain.setValue((int) (prefs.getFloat(PREF_GAIN, 0.5f) * (slGain.getMaximum() - slGain.getMinimum()) + slGain.getMinimum()));
		
		JButton btnSnapshot = new HoldButton("Snapshot");
		btnSnapshot.setBounds(8, 324, 209, 25);
		panel.add(btnSnapshot);
		
		final JCheckBox chckbxInvertedColours = new JCheckBox("Inverted colours");
		chckbxInvertedColours.setBounds(8, 361, 159, 25);
		panel.add(chckbxInvertedColours);
		chckbxInvertedColours.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				mSdrlib.setInvertedColors(chckbxInvertedColours.isSelected());
			}
		});
		chckbxInvertedColours.setHorizontalAlignment(SwingConstants.LEFT);
		btnSnapshot.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				snapshot = true;
			}
		});
		slGain.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				onGainLevelChanged();
			}
		});
		slMotionBlur.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				onMotionBlurLevelChanged();
			}
		});
		btnDown.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.DOWN, clickssofar);
			}
		});
		btnRight.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.RIGHT, clickssofar);
			}
		});
		tglbtnAutoPosition.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				onAutoPostionChanged();
			}
		});
		btnLeft.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.LEFT, clickssofar);
			}
		});
		btnUp.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.UP, clickssofar);
			}
		});
		btnHigherFramerate.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onFrameRateChanged(false, clickssofar);
			}
		});
		btnLowerFramerate.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onFrameRateChanged(true, clickssofar);
			}
		});
		tglbtnPllFramerate.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				onPllFramerateChanged();
			}
		});
		tglbtnLockHeightAndFramerate.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				prefs.putBoolean(PREF_HEIGHT_LOCK, tglbtnLockHeightAndFramerate.isSelected());
			}
		});
		tglbtnAutoResolution.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				onAutoResolutionChanged();
			}
		});
		cbVideoModes.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				final VideoMode selected = (VideoMode) cbVideoModes.getSelectedItem();
				for (int i = 0; i < videomodes.length; i++)
					if (videomodes[i].equals(selected)) {
						onVideoModeSelected(i);
						return;
					}
			}
		});
		
		panel_1 = new JPanel();
		tabbedPane.addTab("Advanced", null, panel_1, null);
		
		menuBar = new JMenuBar();
		menuBar.setBounds(0, 0, 825, 21);
		frmTempestSdr.getContentPane().add(menuBar);
		
		JMenu mnFile = new JMenu("File");
		menuBar.add(mnFile);
		
		for (int i = 0; i < souces.length; i++) {
			final TSDRSource src = souces[i];
			
			souces_menues[i] = new JMenuItem("Load "+src.toString());
			souces_menues[i].addActionListener(new ActionListener() {
				
				@Override
				public void actionPerformed(ActionEvent e) {
					onPluginSelected(src);
				}
			});
			mnFile.add(souces_menues[i]);
		}
		
		mnFile.addSeparator();
		
		JMenuItem exit = new JMenuItem("Exit");
		exit.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				System.exit(0);
			}
		});
		mnFile.add(exit);
        
		int containery = 0;
		for (int i = 0; i < additionalTweaks.length; i++) {
			additionalTweaks[i].setBounds(0, containery, 159, 25);
			containery += 25;
			panel_1.add(additionalTweaks[i]);
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
		
		

		
		pnInputDeviceSettings = new JPanel();
		pnInputDeviceSettings.setBounds(10, 68, 551, 74);
		pnInputDeviceSettings.setLayout(null);
		
		optpaneDevices = new JOptionPane();
		optpaneDevices.setMessage(pnInputDeviceSettings);
		optpaneDevices.setMessageType(JOptionPane.PLAIN_MESSAGE);
		optpaneDevices.setOptionType(JOptionPane.OK_CANCEL_OPTION);
		
		// dialog frame
		deviceframe = optpaneDevices.createDialog(frmTempestSdr, "");
		deviceframe.setResizable(false);
		
		deviceframe.getContentPane().add(optpaneDevices);
		
		onAutoResolutionChanged();
		onAutoPostionChanged();
	}
	
	private void onVideoModeSelected(final int modeid) {
		if (video_mode_change_manually_triggered) return;
		onResolutionChange(modeid);
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
			setPluginMenuEnabled(false);
		}
		
	}
	
	private void onSync(final SYNC_DIRECTION dir, final int repeatssofar) {
		mSdrlib.sync(repeatssofar, dir);
	}
	
	private void onResolutionChange(int id, double refreshrate, int height) {
		if (id < 0 || id >= videomodes.length) return;
		
		final VideoMode mode = videomodes[id];
		onResolutionChange(mode.width, height, refreshrate, id);
	}
	
	private void onResolutionChange(int id) {
		if (id < 0 || id >= videomodes.length) return;
		
		final VideoMode mode = videomodes[id];
		onResolutionChange(mode.width, mode.height, mode.refreshrate, id);
	}
	
	private void onResolutionChange(int width, int height, double framerate) {
		onResolutionChange(width, height, framerate, VideoMode.findClosestVideoModeId(width, height, framerate, videomodes));
	}
	
	private void onResolutionChange(int width, int height, double framerate, int closest_videomode_id) {
		this.framerate = framerate;
		final String frameratetext = String.format(FRAMERATE_FORMAT, framerate);
		txtFramerate.setText(frameratetext);
		
		try {
			mSdrlib.setResolution(width, height, framerate);
			prefs.putDouble(PREF_FRAMERATE, framerate);
			prefs.putInt(PREF_WIDTH, width);
			spWidth.setValue(width);
			prefs.putInt(PREF_HEIGHT, height);
			spHeight.setValue(height);
			
			video_mode_change_manually_triggered = true;
			if (closest_videomode_id >= 0 && closest_videomode_id < videomodes.length) {
				cbVideoModes.setSelectedIndex(closest_videomode_id);
				cbVideoModes.repaint();
			}
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
	
	private void setFramerateValButDoNotSyncWithLibrary(final double val) {
		framerate = val;
		final String frameratetext = String.format(FRAMERATE_FORMAT, framerate);
		txtFramerate.setText(frameratetext);
		prefs.putDouble(PREF_FRAMERATE, framerate);
	}
	
	private void setFrameRate(final double val) {

			final int width = (Integer) spWidth.getValue();
			final int height = (Integer) spHeight.getValue();
			onResolutionChange(width, height, val);
			
			final String frameratetext = String.format(FRAMERATE_FORMAT, framerate);
			visualizer.setOSD("Framerate: "+frameratetext+" fps", OSD_TIME);
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
	
	private void onAutoResolutionChanged() {
		if (tglbtnAutoResolution.isSelected()) {
			txtFramerate.setEnabled(false);
			txtFramerate.setText("----");
			btnLowerFramerate.setEnabled(false);
			btnHigherFramerate.setEnabled(false);
			cbVideoModes.setEnabled(false);
			spWidth.setEnabled(false);
			spHeight.setEnabled(false);
		} else {
			txtFramerate.setText(String.format(FRAMERATE_FORMAT, framerate));
			if (!tglbtnPllFramerate.isSelected()) txtFramerate.setEnabled(true);
			btnLowerFramerate.setEnabled(true);
			btnHigherFramerate.setEnabled(true);
			cbVideoModes.setEnabled(true);
			spWidth.setEnabled(true);
			spHeight.setEnabled(true);
		}
	}
	
	private void onPllFramerateChanged() {
		if (tglbtnPllFramerate.isSelected()) {
			txtFramerate.setEnabled(false);
		} else {
			if (!tglbtnAutoResolution.isSelected()) txtFramerate.setEnabled(true);
		}
	}
	
	private void onAutoPostionChanged() {
		if (tglbtnAutoPosition.isSelected()) {
			btnDown.setEnabled(false);
			btnUp.setEnabled(false);
			btnLeft.setEnabled(false);
			btnRight.setEnabled(false);
		} else {
			btnDown.setEnabled(true);
			btnUp.setEnabled(true);
			btnLeft.setEnabled(true);
			btnRight.setEnabled(true);
		}
	}
	
	private void setPluginMenuEnabled(boolean value) {
		for (int i = 0; i < souces_menues.length; i++)
			souces_menues[i].setEnabled(value);
	}
	
	private void onPluginSelected(final TSDRSource current) {
		
		if (!mSdrlib.isRunning()) btnStartStop.setEnabled(false);
		try {
			mSdrlib.unloadPlugin();
		} catch (TSDRException e) {};
		
		pnInputDeviceSettings.removeAll();
		pnInputDeviceSettings.revalidate();
		pnInputDeviceSettings.repaint(); 
		
		
		final String preferences = prefs.get(PREF_COMMAND_PREFIX+current, "");
		
		current.setOnParameterChangedCallback(this);
		
		final ActRegistrator reg = new ActRegistrator();
		final boolean usesdialog = current.populateGUI(pnInputDeviceSettings, preferences, reg);
		
		if (usesdialog) {
			pnInputDeviceSettings.revalidate();
			pnInputDeviceSettings.repaint(); 

			deviceframe.setTitle("Load "+current);
			deviceframe.setSize(570, 170);
			deviceframe.setLocationByPlatform(true);
			deviceframe.setVisible(true);

			try {
				if (((Integer)optpaneDevices.getValue()).intValue() == JOptionPane.OK_OPTION)
					reg.action();
			} catch (NullPointerException e) {};
		}
	
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
		setPluginMenuEnabled(true);
		displayException(frmTempestSdr, e);
		System.gc();
	}

	@Override
	public void onStopped(TSDRLibrary lib) {
		btnStartStop.setEnabled(true);
		btnStartStop.setText("Start");
		setPluginMenuEnabled(true);
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
	private JTabbedPane tabbedPane;
	private JPanel panel_1;
	private JMenuBar menuBar;

	@Override
	public void onParametersChanged(TSDRSource source) {
		setPluginMenuEnabled(false);
		if (!mSdrlib.isRunning())  btnStartStop.setEnabled(false);
		
		try {
			try {
				mSdrlib.unloadPlugin();
			} catch (TSDRLoadPluginException e) {};
			
			mSdrlib.loadPlugin(source);
		} catch (Throwable t) {
			if (!mSdrlib.isRunning())  btnStartStop.setEnabled(false);
			displayException(frmTempestSdr, t);
			setPluginMenuEnabled(true);
			return;
		}
		setPluginMenuEnabled(true);
		btnStartStop.setEnabled(true);
		
		// set the field with the text arguments
		prefs.put(PREF_COMMAND_PREFIX+source, source.getParams());
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

	@Override
	public void onValueChanged(VALUE_ID id, double arg0, int arg1) {
		switch (id) {
//		case AUTO_RESOLUTION_RESULT: {
//			
//			tglbtnAutoResolution.setSelected(false);
//			onAutoResolutionChanged();
//			
//			height_change_from_auto = true;
//			final int modeid = VideoMode.findClosestVideoModeId(arg0, arg1, videomodes);
//			if (modeid >= 0 && modeid < videomodes.length) {
//				onResolutionChange(modeid, arg0, arg1);
//				visualizer.setOSD("Detected "+videomodes[modeid], OSD_TIME_LONG);
//			} else {
//				setFrameRate(arg0);
//			}
//			height_change_from_auto = false;
//
//		} break;
		case PLL_FRAMERATE:
			setFramerateValButDoNotSyncWithLibrary(arg0);
			break;
		default:
			System.out.println("Java Main received notification that value "+id+" has changed to arg0="+arg0+" and arg1="+arg1);
			break;
		}

	}

	@Override
	public void onIncommingPlot(PLOT_ID id, int offset, float[] data, int size, long samplerate) {
		switch (id) {
		case LINE:
			line_plotter.plot(data, offset, size, samplerate);
			break;
		case FRAME:
			frame_plotter.plot(data, offset, size, samplerate);
			break;
		default:
			System.out.println("Java Main received unimplemented notification plot value "+id+" with size "+size);
			break;
		}
	}
	
	private class ActRegistrator implements ActionListenerRegistrator {
		
		private final ArrayList<ActionListener> listeners = new ArrayList<ActionListener>();
		private final ActionEvent e = new ActionEvent(Main.this, 0, null);
		
		@Override
		public void addActionListener(ActionListener l) {
			listeners.add(l);
		}
		
		public void action() {
			for (final ActionListener l : listeners)
				l.actionPerformed(e);
		}
	};
}
