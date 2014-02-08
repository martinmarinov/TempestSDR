package martin.tempest.sources;

import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.util.ArrayList;

import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;

public class TSDRExtIOSource extends TSDRSource {
	
	public TSDRExtIOSource() {
		super("ExtIO source", "TSDRPlugin_ExtIO", false);
	}
	
	private ArrayList<File> findExtIOpluginsInFolder(final File dir) {
		final ArrayList<File> answer = new ArrayList<File>();
		if (dir.exists()) {
			final File[] files = dir.listFiles();
			
			for (final File file : files) {
				final String fname = file.getName().toLowerCase();
				if (fname.startsWith("extio_") && fname.endsWith(".dll"))
					answer.add(file);
			}
		}
		return answer;
	}
	
	@SuppressWarnings({ "unchecked", "rawtypes" })
	private void makeDeviceSelectionDialog(final Container cont, final ArrayList<File> availableplugins) {
		
		final JComboBox options = new JComboBox();
		final String[] filenames = new String[availableplugins.size()];
		for (int i = 0; i < filenames.length; i++) filenames[i] = availableplugins.get(i).getName();
		cont.add(options);
		options.setBounds(12, 12, 159, 22);
		options.setModel(new DefaultComboBoxModel(filenames));
		
		final JButton ok = new JButton("Choose");
		cont.add(ok);
		ok.setBounds(12, 12+2*32, 84, 24);

		ok.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				new Thread() {
					public void run() {
						ok.setEnabled(false);
						setParams(availableplugins.get(options.getSelectedIndex()).getAbsolutePath());
						ok.setEnabled(true);
					};
				}.start();
			}
		});
	}
	
	private void makeFileSelectionDialog(final Container cont) {
		
		final JFileChooser fc = new JFileChooser();
		
		if (fc.showOpenDialog(cont) == JFileChooser.APPROVE_OPTION)
			setParams(fc.getSelectedFile().getAbsolutePath());
		
		final JButton ok = new JButton("Choose ExtIO dll file");
		cont.add(ok);
		ok.setBounds(12, 12+2*32, 84, 24);

		ok.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				new Thread() {
					public void run() {
						ok.setEnabled(false);
						if (fc.showOpenDialog(cont) == JFileChooser.APPROVE_OPTION)
							setParams(fc.getSelectedFile().getAbsolutePath());
						ok.setEnabled(true);
					};
				}.start();
			}
		});
	}

	@Override
	public boolean populateGUI(final Container cont, final String defaultprefs) {
		
		final String[] paths = System.getProperty("java.library.path").split(File.pathSeparator);
		for (final String path : paths) {
			final ArrayList<File> plugins = findExtIOpluginsInFolder(new File(path));
			if (plugins.size() == 1) {
				setParams(plugins.get(0).getAbsolutePath());
				return false;
			} else if (plugins.size() > 1) {
				makeDeviceSelectionDialog(cont, plugins);
				return true;
			}
		}
		
		{
			final ArrayList<File> plugins = findExtIOpluginsInFolder(new File("."));
			if (plugins.size() == 1) {
				setParams(plugins.get(0).getAbsolutePath());
				return false;
			} else if (plugins.size() > 1) {
				makeDeviceSelectionDialog(cont, plugins);
				return true;
			}
		}
		
		makeFileSelectionDialog(cont);
		return true;
	}
}
