package martin.tempest.gui;

import java.awt.Component;
import java.awt.EventQueue;
import java.awt.Rectangle;
import java.awt.Toolkit;

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
import martin.tempest.core.TSDRLibrary.SYNC_DIRECTION;
import martin.tempest.core.exceptions.TSDRException;
import martin.tempest.gui.HoldButton.HoldListener;
import martin.tempest.sources.TSDRSource;

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

public class Main implements TSDRLibrary.FrameReadyCallback {
	
	private final static int OSD_TIME = 2000;
	
	private final static int FRAMERATE_SIGNIFICANT_FIGURES = 6;
	private final static long FREQUENCY_STEP = 1000000;
	
	private final static double FRAMERATE_MIN_CHANGE = 1.0/Math.pow(10, FRAMERATE_SIGNIFICANT_FIGURES);
	private final static String FRAMERATE_FORMAT = "%."+FRAMERATE_SIGNIFICANT_FIGURES+"f";
	
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\lcd800x600x60_16bit_8000000.wav 8000000 int16";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\lcd1680x1050x60_8bit_8000000.wav 80000000 int8";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\cablepal16bit8000000.wav 8000000 int16";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\tvpal8bit8000000.wav 8000000 int8";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\tvpal8bit2048000.wav 2048000 int8";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\tvpal16bit8000000.wav 8000000 int16";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\mphilproj\\martin-vaio-h-148.dat 25000000 int16";
	private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\mphilproj\\cdxdemo-rf.dat 25000000 int16";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\mphilproj\\Toshiba-440CDX\\toshiba.iq 25000000 float";

	private final SpinnerModel frequency_spinner_model = new SpinnerNumberModel(new Long(430000000), new Long(0), new Long(2147483647), new Long(FREQUENCY_STEP));
	
	private JFrame frmTempestSdr;
	private JFrame fullscreenframe;
	private JTextField textArgs;
	private JSpinner spWidth;
	private JSpinner spHeight;
	private JComboBox<VideoMode> cbVideoModes;
	private JComboBox<TSDRSource> cbDevice;
	private JSpinner spFrequency;
	private JLabel lblFrequency;
	private JSlider slGain;
	private JSlider slMotionBlur;
	private JLabel lblGain;
	private JButton btnStartStop;
	private final TSDRLibrary mSdrlib;
	private ImageVisualizer visualizer;
	private Rectangle visualizer_bounds;
	private double framerate = 50;
	private JTextField txtFramerate;
	private HoldButton btnLowerFramerate, btnHigherFramerate, btnUp, btnDown, btnLeft, btnRight;
	
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
	private void initialize() {
		frmTempestSdr = new JFrame();
		frmTempestSdr.setFocusable(true);
		frmTempestSdr.setFocusableWindowState(true);
		frmTempestSdr.addKeyListener(keyhook);
		frmTempestSdr.setResizable(false);
		frmTempestSdr.setTitle("TempestSDR");
		frmTempestSdr.setBounds(100, 100, 734, 561);
		frmTempestSdr.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frmTempestSdr.getContentPane().setLayout(null);
		frmTempestSdr.addMouseListener(new MouseAdapter() {
			@Override
			public void mouseClicked(MouseEvent e) {
				frmTempestSdr.requestFocus();
			}
		});
		
		visualizer = new ImageVisualizer();
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
		visualizer.setBounds(0, 32, 563, 433);
		frmTempestSdr.getContentPane().add(visualizer);
		
		cbDevice = new JComboBox<TSDRSource>();
		cbDevice.setBounds(0, 3, 218, 22);
		cbDevice.setModel(new DefaultComboBoxModel<TSDRSource>(TSDRSource.getAvailableSources()));
		frmTempestSdr.getContentPane().add(cbDevice);
		
		textArgs = new JTextField(COMMAND);
		textArgs.setBounds(223, 3, 340, 22);
		frmTempestSdr.getContentPane().add(textArgs);
		textArgs.setColumns(10);
		
		btnStartStop = new JButton("Start");
		btnStartStop.setBounds(568, 2, 159, 25);
		btnStartStop.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				performStartStop();
			}
		});
		frmTempestSdr.getContentPane().add(btnStartStop);
		
		cbVideoModes = new JComboBox<VideoMode>();
		cbVideoModes.setBounds(568, 32, 159, 22);
		cbVideoModes.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				onVideoModeSelected((VideoMode) cbVideoModes.getSelectedItem());
			}
		});
		final VideoMode[] videomodes = VideoMode.getVideoModes();
		cbVideoModes.setModel(new DefaultComboBoxModel<VideoMode>(videomodes));
		frmTempestSdr.getContentPane().add(cbVideoModes);
		
		JLabel lblWidth = new JLabel("Width:");
		lblWidth.setHorizontalAlignment(SwingConstants.RIGHT);
		lblWidth.setBounds(568, 62, 65, 16);
		frmTempestSdr.getContentPane().add(lblWidth);
		
		spWidth = new JSpinner();
		spWidth.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onResolutionChange();
			}
		});
		spWidth.setBounds(638, 59, 89, 22);
		spWidth.setModel(new SpinnerNumberModel(576, 1, 10000, 1));
		frmTempestSdr.getContentPane().add(spWidth);
		
		JLabel lblHeight = new JLabel("Height:");
		lblHeight.setHorizontalAlignment(SwingConstants.RIGHT);
		lblHeight.setBounds(568, 89, 65, 16);
		frmTempestSdr.getContentPane().add(lblHeight);
		
		spHeight = new JSpinner();
		spHeight.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onResolutionChange();
			}
		});
		spHeight.setBounds(638, 86, 89, 22);
		spHeight.setModel(new SpinnerNumberModel(625, 1, 10000, 1));
		frmTempestSdr.getContentPane().add(spHeight);
		
		JLabel lblFramerate = new JLabel("Framerate:");
		lblFramerate.setHorizontalAlignment(SwingConstants.RIGHT);
		lblFramerate.setBounds(568, 116, 65, 16);
		frmTempestSdr.getContentPane().add(lblFramerate);
		
		lblGain = new JLabel("Gain:");
		lblGain.setHorizontalAlignment(SwingConstants.RIGHT);
		lblGain.setBounds(0, 502, 218, 16);
		frmTempestSdr.getContentPane().add(lblGain);
		
		slGain = new JSlider();
		slGain.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				onGainLevelChanged();
			}
		});
		slGain.setBounds(223, 497, 340, 26);
		frmTempestSdr.getContentPane().add(slGain);
		
		lblFrequency = new JLabel("Frequency:");
		lblFrequency.setHorizontalAlignment(SwingConstants.RIGHT);
		lblFrequency.setBounds(0, 473, 218, 16);
		frmTempestSdr.getContentPane().add(lblFrequency);
		
		spFrequency = new JSpinner();
		spFrequency.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onCenterFreqChange();
			}
		});
		spFrequency.setBounds(223, 470, 340, 22);
		spFrequency.setModel(frequency_spinner_model);
		frmTempestSdr.getContentPane().add(spFrequency);
		
		final JCheckBox chckbxInvertedColours = new JCheckBox("Inverted colours");
		chckbxInvertedColours.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				mSdrlib.setInvertedColors(chckbxInvertedColours.isSelected());
			}
		});
		chckbxInvertedColours.setHorizontalAlignment(SwingConstants.LEFT);
		chckbxInvertedColours.setBounds(568, 169, 159, 25);
		frmTempestSdr.getContentPane().add(chckbxInvertedColours);
		
		btnUp = new HoldButton("Up");
		btnUp.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.UP, clickssofar);
			}
		});
		btnUp.setBounds(608, 244, 78, 25);
		frmTempestSdr.getContentPane().add(btnUp);
		
		btnLeft = new HoldButton("Left");
		btnLeft.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.LEFT, clickssofar);
			}
		});
		btnLeft.setBounds(568, 269, 65, 25);
		frmTempestSdr.getContentPane().add(btnLeft);
		
		btnRight = new HoldButton("Right");
		btnRight.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.RIGHT, clickssofar);
			}
		});
		btnRight.setBounds(662, 269, 65, 25);
		frmTempestSdr.getContentPane().add(btnRight);
		
		btnDown = new HoldButton("Down");
		btnDown.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onSync(SYNC_DIRECTION.DOWN, clickssofar);
			}
		});
		btnDown.setBounds(608, 295, 78, 25);
		frmTempestSdr.getContentPane().add(btnDown);
		
		txtFramerate = new JTextField();
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
		txtFramerate.setBounds(638, 113, 89, 22);
		frmTempestSdr.getContentPane().add(txtFramerate);
		txtFramerate.setColumns(10);
		
		btnLowerFramerate = new HoldButton("<");
		btnLowerFramerate.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onFrameRateChanged(true, clickssofar);
			}
		});
		btnLowerFramerate.setBounds(638, 135, 41, 25);
		frmTempestSdr.getContentPane().add(btnLowerFramerate);
		
		btnHigherFramerate = new HoldButton(">");
		btnHigherFramerate.addHoldListener(new HoldListener() {
			public void onHold(final int clickssofar) {
				onFrameRateChanged(false, clickssofar);
			}
		});
		btnHigherFramerate.setBounds(686, 135, 41, 25);
		frmTempestSdr.getContentPane().add(btnHigherFramerate);
		
		slMotionBlur = new JSlider();
		slMotionBlur.setValue(0);
		slMotionBlur.setBounds(638, 201, 89, 22);
		slMotionBlur.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				onMotionBlurLevelChanged();
			}
		});
		frmTempestSdr.getContentPane().add(slMotionBlur);
		
		JLabel lblMotionBlur = new JLabel("Lowpass:");
		lblMotionBlur.setHorizontalAlignment(SwingConstants.RIGHT);
		lblMotionBlur.setBounds(568, 201, 65, 16);
		frmTempestSdr.getContentPane().add(lblMotionBlur);
		
		frmTempestSdr.setFocusableWindowState(true);
		frmTempestSdr.requestFocus();
		
		onVideoModeSelected(videomodes[0]);
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
	}
	
	private void onVideoModeSelected(final VideoMode m) {
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
						btnStartStop.setEnabled(false);
						mSdrlib.stop();
					} catch (TSDRException e) {
						displayException(frmTempestSdr, e);
					}
				}
			}.start();
			
		} else {
			final TSDRSource src = (TSDRSource) cbDevice.getSelectedItem();
			try {
				src.setParams(textArgs.getText());

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
				mSdrlib.startAsync(src, (Integer) spWidth.getValue(), (Integer) spHeight.getValue(), framerate);
			} catch (TSDRException e1) {
				displayException(frmTempestSdr, e1);
				btnStartStop.setText("Start");
			}

			btnStartStop.setText("Stop");
		}
		
	}
	
	private void onSync(final SYNC_DIRECTION dir, final int repeatssofar) {
		mSdrlib.sync(repeatssofar, dir);
	}
	
	private void onResolutionChange() {
		try {
			mSdrlib.setResolution((Integer) spWidth.getValue(), (Integer) spHeight.getValue(), framerate);
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
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onGainLevelChanged() {
		float gain = (slGain.getValue() - slGain.getMinimum()) / (float) (slGain.getMaximum() - slGain.getMinimum());
		if (gain < 0.0f) gain = 0.0f; else if (gain > 1.0f) gain = 1.0f;
		
		try {
			mSdrlib.setGain(gain);
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onMotionBlurLevelChanged() {
		float mblur = slMotionBlur.getValue() / 100.0f;
		
		try {
			mSdrlib.setMotionBlur(mblur);
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
			mSdrlib.setResolution((Integer) spWidth.getValue(), (Integer) spHeight.getValue(), val);
			visualizer.setOSD("Framerate: "+frameratetext+" fps", OSD_TIME);
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onKeyboardKeyPressed(final KeyEvent e) {
		final int keycode = e.getKeyCode();

		if (!e.isShiftDown()) {
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
			case KeyEvent.VK_LEFT:
				btnHigherFramerate.doHold();
				break;
			case KeyEvent.VK_RIGHT:
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
		
		if (!e.isShiftDown()) {
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
			case KeyEvent.VK_LEFT:
				btnLeft.doRelease();
				btnHigherFramerate.doRelease();
				break;
			case KeyEvent.VK_RIGHT:
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

	@Override
	public void onFrameReady(TSDRLibrary lib, BufferedImage frame) {
		visualizer.drawImage(frame);
	}

	@Override
	public void onException(TSDRLibrary lib, Exception e) {
		btnStartStop.setEnabled(true);
		btnStartStop.setText("Start");
		displayException(frmTempestSdr, e);
	}

	@Override
	public void onClosed(TSDRLibrary lib) {
		btnStartStop.setEnabled(true);
		btnStartStop.setText("Start");
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
 }
