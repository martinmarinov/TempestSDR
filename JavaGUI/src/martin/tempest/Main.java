package martin.tempest;

import java.awt.Component;
import java.awt.EventQueue;

import javax.swing.JFrame;

import java.awt.GridBagLayout;

import javax.swing.JComboBox;

import java.awt.GridBagConstraints;
import java.awt.Insets;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.JTextField;
import javax.swing.DefaultComboBoxModel;

import martin.tempest.core.TSDRLibrary;
import martin.tempest.core.exceptions.TSDRException;
import martin.tempest.sources.TSDRSource;

import javax.swing.JSlider;

import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.image.BufferedImage;

public class Main implements TSDRLibrary.FrameReadyCallback {

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
		frmTempestSdr.setTitle("TempestSDR");
		frmTempestSdr.setResizable(false);
		frmTempestSdr.setBounds(100, 100, 734, 561);
		frmTempestSdr.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		GridBagLayout gridBagLayout = new GridBagLayout();
		gridBagLayout.columnWidths = new int[] {0, 0, 0, 10, 0};
		gridBagLayout.rowHeights = new int[] {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		gridBagLayout.columnWeights = new double[]{1.0, 1.0, Double.MIN_VALUE, 0.0, 0.0};
		gridBagLayout.rowWeights = new double[]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
		frmTempestSdr.getContentPane().setLayout(gridBagLayout);
		
		cbDevice = new JComboBox<TSDRSource>();
		cbDevice.setModel(new DefaultComboBoxModel<TSDRSource>(TSDRSource.getAvailableSources()));
		GridBagConstraints gbc_cbDevice = new GridBagConstraints();
		gbc_cbDevice.insets = new Insets(0, 0, 5, 5);
		gbc_cbDevice.fill = GridBagConstraints.HORIZONTAL;
		gbc_cbDevice.gridx = 0;
		gbc_cbDevice.gridy = 0;
		frmTempestSdr.getContentPane().add(cbDevice, gbc_cbDevice);
		
		textArgs = new JTextField();
		GridBagConstraints gbc_textArgs = new GridBagConstraints();
		gbc_textArgs.insets = new Insets(0, 0, 5, 5);
		gbc_textArgs.fill = GridBagConstraints.HORIZONTAL;
		gbc_textArgs.gridx = 1;
		gbc_textArgs.gridy = 0;
		frmTempestSdr.getContentPane().add(textArgs, gbc_textArgs);
		textArgs.setColumns(10);
		
		btnStartStop = new JButton("Start");
		btnStartStop.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				performStartStop();
			}
		});
		GridBagConstraints gbc_btnStart = new GridBagConstraints();
		gbc_btnStart.insets = new Insets(0, 0, 5, 0);
		gbc_btnStart.fill = GridBagConstraints.BOTH;
		gbc_btnStart.gridwidth = 3;
		gbc_btnStart.gridx = 2;
		gbc_btnStart.gridy = 0;
		frmTempestSdr.getContentPane().add(btnStartStop, gbc_btnStart);
		
		visualizer = new ImageVisualizer();
		GridBagConstraints gbc_visualizer = new GridBagConstraints();
		gbc_visualizer.fill = GridBagConstraints.BOTH;
		gbc_visualizer.gridwidth = 2;
		gbc_visualizer.insets = new Insets(0, 0, 5, 5);
		gbc_visualizer.gridx = 0;
		gbc_visualizer.gridy = 1;
		gbc_visualizer.gridheight = 15;
		frmTempestSdr.getContentPane().add(visualizer, gbc_visualizer);
		
		cbVideoModes = new JComboBox<VideoMode>();
		cbVideoModes.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				onVideoModeSelected((VideoMode) cbVideoModes.getSelectedItem());
			}
		});
		cbVideoModes.setModel(new DefaultComboBoxModel<VideoMode>(VideoMode.getVideoModes()));
		GridBagConstraints gbc_cbVideoModes = new GridBagConstraints();
		gbc_cbVideoModes.gridwidth = 3;
		gbc_cbVideoModes.insets = new Insets(0, 0, 5, 0);
		gbc_cbVideoModes.fill = GridBagConstraints.HORIZONTAL;
		gbc_cbVideoModes.gridx = 2;
		gbc_cbVideoModes.gridy = 1;
		frmTempestSdr.getContentPane().add(cbVideoModes, gbc_cbVideoModes);
		
		JLabel lblWidth = new JLabel("Width:");
		GridBagConstraints gbc_lblWidth = new GridBagConstraints();
		gbc_lblWidth.anchor = GridBagConstraints.EAST;
		gbc_lblWidth.insets = new Insets(0, 0, 5, 5);
		gbc_lblWidth.gridx = 2;
		gbc_lblWidth.gridy = 2;
		frmTempestSdr.getContentPane().add(lblWidth, gbc_lblWidth);
		
		spWidth = new JSpinner();
		spWidth.setModel(new SpinnerNumberModel(576, 1, 10000, 1));
		GridBagConstraints gbc_spWidth = new GridBagConstraints();
		gbc_spWidth.gridwidth = 2;
		gbc_spWidth.fill = GridBagConstraints.BOTH;
		gbc_spWidth.insets = new Insets(0, 0, 5, 0);
		gbc_spWidth.gridx = 3;
		gbc_spWidth.gridy = 2;
		frmTempestSdr.getContentPane().add(spWidth, gbc_spWidth);
		
		JLabel lblHeight = new JLabel("Height:");
		GridBagConstraints gbc_lblHeight = new GridBagConstraints();
		gbc_lblHeight.anchor = GridBagConstraints.EAST;
		gbc_lblHeight.insets = new Insets(0, 0, 5, 5);
		gbc_lblHeight.gridx = 2;
		gbc_lblHeight.gridy = 3;
		frmTempestSdr.getContentPane().add(lblHeight, gbc_lblHeight);
		
		spHeight = new JSpinner();
		spHeight.setModel(new SpinnerNumberModel(625, 1, 10000, 1));
		GridBagConstraints gbc_spHeight = new GridBagConstraints();
		gbc_spHeight.gridwidth = 2;
		gbc_spHeight.fill = GridBagConstraints.BOTH;
		gbc_spHeight.insets = new Insets(0, 0, 5, 0);
		gbc_spHeight.gridx = 3;
		gbc_spHeight.gridy = 3;
		frmTempestSdr.getContentPane().add(spHeight, gbc_spHeight);
		
		JLabel lblFramerate = new JLabel("Framerate:");
		GridBagConstraints gbc_lblFramerate = new GridBagConstraints();
		gbc_lblFramerate.anchor = GridBagConstraints.EAST;
		gbc_lblFramerate.insets = new Insets(0, 0, 5, 5);
		gbc_lblFramerate.gridx = 2;
		gbc_lblFramerate.gridy = 4;
		frmTempestSdr.getContentPane().add(lblFramerate, gbc_lblFramerate);
		
		spFramerate = new JSpinner();
		spFramerate.setModel(new SpinnerNumberModel(50.0, 1.0, 120.0, 0.0));
		GridBagConstraints gbc_spFramerate = new GridBagConstraints();
		gbc_spFramerate.gridwidth = 2;
		gbc_spFramerate.fill = GridBagConstraints.BOTH;
		gbc_spFramerate.insets = new Insets(0, 0, 5, 0);
		gbc_spFramerate.gridx = 3;
		gbc_spFramerate.gridy = 4;
		frmTempestSdr.getContentPane().add(spFramerate, gbc_spFramerate);
		
		lblGain = new JLabel("Gain:");
		GridBagConstraints gbc_lblGain = new GridBagConstraints();
		gbc_lblGain.anchor = GridBagConstraints.EAST;
		gbc_lblGain.insets = new Insets(0, 0, 0, 5);
		gbc_lblGain.gridx = 0;
		gbc_lblGain.gridy = 17;
		frmTempestSdr.getContentPane().add(lblGain, gbc_lblGain);
		
		slGain = new JSlider();
		GridBagConstraints gbc_slGain = new GridBagConstraints();
		gbc_slGain.fill = GridBagConstraints.BOTH;
		gbc_slGain.insets = new Insets(0, 0, 0, 5);
		gbc_slGain.gridx = 1;
		gbc_slGain.gridy = 17;
		frmTempestSdr.getContentPane().add(slGain, gbc_slGain);
		
		lblFrequency = new JLabel("Frequency:");
		GridBagConstraints gbc_lblFrequency = new GridBagConstraints();
		gbc_lblFrequency.anchor = GridBagConstraints.EAST;
		gbc_lblFrequency.insets = new Insets(0, 0, 5, 5);
		gbc_lblFrequency.gridx = 0;
		gbc_lblFrequency.gridy = 16;
		frmTempestSdr.getContentPane().add(lblFrequency, gbc_lblFrequency);
		
		spFrequency = new JSpinner();
		spFrequency.setModel(new SpinnerNumberModel(new Long(200000000), new Long(0), new Long(2147483647), new Long(1)));
		GridBagConstraints gbc_spFrequency = new GridBagConstraints();
		gbc_spFrequency.fill = GridBagConstraints.BOTH;
		gbc_spFrequency.insets = new Insets(0, 0, 5, 5);
		gbc_spFrequency.gridx = 1;
		gbc_spFrequency.gridy = 16;
		frmTempestSdr.getContentPane().add(spFrequency, gbc_spFrequency);
	}
	
	private void onVideoModeSelected(final VideoMode m) {
		spWidth.setValue(m.width);
		spHeight.setValue(m.height);
		spFramerate.setValue(m.refreshrate);
	}
	
	private void performStartStop() {
		
		if (mSdrlib.isRunning()) {
			
			btnStartStop.setText("Start");
		} else {
			final TSDRSource src = (TSDRSource) cbDevice.getSelectedItem();
		
			src.setParams(textArgs.getText());
			
			new Thread() {
				public void run() {
					try {
						mSdrlib.startAsync(src, (Integer) spWidth.getValue(), (Integer) spHeight.getValue(), (Double) spFramerate.getValue());
					} catch (TSDRException e1) {
						btnStartStop.setText("Start");
						displayException(frmTempestSdr, e1);
					}
					
				};
			}.start();
			
			
			btnStartStop.setText("Stop");
		}
		
	}

	@Override
	public void onFrameReady(TSDRLibrary lib, BufferedImage frame) {
		visualizer.drawImage(frame);
	}

	@Override
	public void onException(TSDRLibrary lib, Exception e) {
		btnStartStop.setText("Start");
		displayException(frmTempestSdr, e);
	}

	@Override
	public void onClosed(TSDRLibrary lib) {
		btnStartStop.setText("Start");
	}
 }
