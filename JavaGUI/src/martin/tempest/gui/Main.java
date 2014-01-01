package martin.tempest.gui;

import java.awt.Component;
import java.awt.EventQueue;

import javax.swing.JFrame;
import javax.swing.JComboBox;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.JTextField;
import javax.swing.DefaultComboBoxModel;

import martin.tempest.core.TSDRLibrary;
import martin.tempest.core.TSDRLibrary.SYNC_DIRECTION;
import martin.tempest.core.exceptions.TSDRException;
import martin.tempest.sources.TSDRSource;

import javax.swing.JSlider;

import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.image.BufferedImage;

import javax.swing.SwingConstants;
import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;
import javax.swing.JCheckBox;

public class Main implements TSDRLibrary.FrameReadyCallback {
	
	private static final int SYNC_AMOUNT = 10;

	private JFrame frmTempestSdr;
	private JTextField textArgs;
	private JSpinner spWidth;
	private JSpinner spHeight;
	private JSpinner spFramerate;
	private JComboBox<VideoMode> cbVideoModes;
	private JComboBox<TSDRSource> cbDevice;
	private JSpinner spFrequency;
	private JLabel lblFrequency;
	private JSlider slGain;
	private JLabel lblGain;
	private JButton btnStartStop;
	private final TSDRLibrary mSdrlib;
	private ImageVisualizer visualizer;

	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\lcd1680x1050x60_8bit_8000000.wav 80000000 int8";
	private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\tvpal8bit8000000.wav 8000000 int8";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\tvpal8bit2048000.wav 2048000 int8";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\tvpal16bit8000000.wav 8000000 int16";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\mphilproj\\martin-vaio-h-200.dat 25000000 int16";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\mphilproj\\cdxdemo-rf.dat 25000000 int16";
	//private static final String COMMAND = "D:\\Dokumenti\\Cambridge\\project\\mphilproj\\Toshiba-440CDX\\toshiba.iq 25000000 float";
	
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
		frmTempestSdr.setResizable(false);
		frmTempestSdr.setTitle("TempestSDR");
		frmTempestSdr.setBounds(100, 100, 734, 561);
		frmTempestSdr.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frmTempestSdr.getContentPane().setLayout(null);
		
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
		
		visualizer = new ImageVisualizer();
		visualizer.setBounds(0, 32, 563, 433);
		frmTempestSdr.getContentPane().add(visualizer);
		
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
		
		spFramerate = new JSpinner();
		spFramerate.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				onResolutionChange();
			}
		});
		spFramerate.setBounds(638, 113, 89, 22);
		spFramerate.setModel(new SpinnerNumberModel(new Double(50), new Double(1), new Double(500), new Double(0.001)));
		frmTempestSdr.getContentPane().add(spFramerate);
		
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
		spFrequency.setModel(new SpinnerNumberModel(new Long(113095000), new Long(0), new Long(2147483647), new Long(1000000)));
		frmTempestSdr.getContentPane().add(spFrequency);
		
		final JCheckBox chckbxInvertedColours = new JCheckBox("Inverted colours");
		chckbxInvertedColours.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				mSdrlib.setInvertedColors(chckbxInvertedColours.isSelected());
			}
		});
		chckbxInvertedColours.setHorizontalAlignment(SwingConstants.LEFT);
		chckbxInvertedColours.setBounds(568, 141, 159, 25);
		frmTempestSdr.getContentPane().add(chckbxInvertedColours);
		
		JButton btnUp = new JButton("Up");
		btnUp.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				mSdrlib.sync(SYNC_AMOUNT, SYNC_DIRECTION.UP);
			}
		});
		btnUp.setBounds(608, 186, 78, 25);
		frmTempestSdr.getContentPane().add(btnUp);
		
		JButton btnLeft = new JButton("Left");
		btnLeft.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				mSdrlib.sync(SYNC_AMOUNT, SYNC_DIRECTION.LEFT);
			}
		});
		btnLeft.setBounds(568, 211, 65, 25);
		frmTempestSdr.getContentPane().add(btnLeft);
		
		JButton btnRight = new JButton("Right");
		btnRight.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				mSdrlib.sync(SYNC_AMOUNT, SYNC_DIRECTION.RIGHT);
			}
		});
		btnRight.setBounds(662, 211, 65, 25);
		frmTempestSdr.getContentPane().add(btnRight);
		
		JButton btnDown = new JButton("Down");
		btnDown.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				mSdrlib.sync(SYNC_AMOUNT, SYNC_DIRECTION.DOWN);
			}
		});
		btnDown.setBounds(608, 237, 78, 25);
		frmTempestSdr.getContentPane().add(btnDown);
		
		onVideoModeSelected(videomodes[0]);
	}
	
	private void onVideoModeSelected(final VideoMode m) {
		spWidth.setValue(m.width);
		spHeight.setValue(m.height);
		spFramerate.setValue(m.refreshrate);
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
				mSdrlib.startAsync(src, (Integer) spWidth.getValue(), (Integer) spHeight.getValue(), Double.parseDouble(spFramerate.getValue().toString()));
			} catch (TSDRException e1) {
				displayException(frmTempestSdr, e1);
				btnStartStop.setText("Start");
			}

			btnStartStop.setText("Stop");
		}
		
	}
	
	private void onResolutionChange() {
		try {
			mSdrlib.setResolution((Integer) spWidth.getValue(), (Integer) spHeight.getValue(), Double.parseDouble(spFramerate.getValue().toString()));
		} catch (TSDRException e) {
			displayException(frmTempestSdr, e);
		}
	}
	
	private void onCenterFreqChange() {
		final Long newfreq = (Long) spFrequency.getValue();
		
		if (newfreq == null || newfreq < 0) return;
		
		try {
			mSdrlib.setBaseFreq(newfreq);
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
 }
