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
package martin.tempest.sources;

import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.DataInputStream;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.util.ArrayList;

import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JTextField;

/**
 * This plugin allows for playback of prerecorded files.
 * 
 * @author Martin Marinov
 *
 */
public class TSDRFileSource extends TSDRSource {
	private final static String[] filetype = new String[] {"float", "int8", "int16", "uint8", "uint16"};
	
	final JFileChooser fc = new JFileChooser();

	public TSDRFileSource() {
		super("From file", "TSDRPlugin_RawFile", false);
	}
	
	private static ParsedTSDRFileSource getWavFile(String file) {
		DataInputStream in = null;
		try {
			
			in = new DataInputStream(new BufferedInputStream(new FileInputStream(file)));
			
			if (in.readInt() != 0x52494646) return null;
			
			in.skipBytes(4);
			
			if (in.readInt() != 0x57415645) return null;
			if (in.readInt() != 0x666d7420) return null;
			
			in.skipBytes(8);
			
			final int samplerate = in.readByte() | (in.readByte() << 8) | (in.readByte() << 16) | (in.readByte() << 24);
			
			in.skipBytes(6);
			
			final int bitspersample = in.readByte() | (in.readByte() << 8);
			
			
			if (bitspersample == 8 || bitspersample == 16) {
				final String type = (bitspersample == 8) ? "int8" : "int16";
				
				int type_id = -1;
				for (int i = 0; i < filetype.length; i++) if (type.equals(filetype[i])) {type_id = i; break;};
				if (type_id == -1) return null;
				
				return new ParsedTSDRFileSource(file, type_id, samplerate);

			} else
				return null;
			
		} catch (Throwable t) {
			return null;
		} finally {
			if (in != null)
				try {
					in.close();
				} catch (Throwable t) {}
		}
	}

	@SuppressWarnings({ "rawtypes", "unchecked" })
	@Override
	public boolean populateGUI(final Container cont, final String defaultprefs, final ActionListenerRegistrator okbutton) {
		
		final ParsedTSDRFileSource parsed = new ParsedTSDRFileSource(defaultprefs);
		
		final JFileChooser fc = parsed.filename.isEmpty() ? new JFileChooser() : new JFileChooser(parsed.filename);
		
		final JLabel description = new JLabel("Choose pre-recorded file, type and sample rate:");
	    cont.add(description);
	    description.setBounds(12, 12, 350, 24);
		
		final JTextField filename = new JTextField(parsed.filename);
	    cont.add(filename);
	    filename.setBounds(12, 12+32, 150, 24);
		
		final JTextField samplerate = new JTextField(String.valueOf(parsed.samplerate));
	    cont.add(samplerate);
	    samplerate.setBounds(12*2+150+100, 12+32, 150, 24);
		
		final JComboBox type = new JComboBox();
		type.setModel(new DefaultComboBoxModel(filetype));
		type.setSelectedIndex(parsed.type_id);
		type.setBounds(12*3+150+150+100, 12+32, 100, 24);
		cont.add(type);
	    
		final JButton browse = new JButton("Browse");
		cont.add(browse);
		browse.setBounds(150+12, 12+32, 100, 24);
		browse.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				if (fc.showOpenDialog(cont) == JFileChooser.APPROVE_OPTION) {
					final String absoluteName = fc.getSelectedFile().getAbsolutePath();
					final ParsedTSDRFileSource wavData = getWavFile(absoluteName);
					
					filename.setText(absoluteName);
					
					if (wavData != null) {
						type.setSelectedIndex(wavData.type_id);
						samplerate.setText(String.valueOf(wavData.samplerate));
					}
				}
			}	
		});
		
		okbutton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				final String params = "\""+filename.getText()+"\" "+samplerate.getText()+" "+type.getSelectedItem();
				setParams(params);
			}
		});
	
		return true;
	}
	
	private static class ParsedTSDRFileSource {
		private String filename = "";
		private int type_id = 0;
		private long samplerate = 8000000;
		
		private final ArrayList<String> parse(final String input) {
			final ArrayList<String> answer = new ArrayList<String>();
			if (input == null || input.trim().isEmpty()) return answer;
			
			StringBuilder builder = new StringBuilder();
			boolean inside = false;
			
			for (int i = 0; i < input.length(); i++) {
				final char c = input.charAt(i);
				
				if (!inside && c == ' ') {
					if (builder.length() != 0) answer.add(builder.toString());
					builder = new StringBuilder();
				} else if (c != '"') builder.append(c);
				if (c == '"') inside = !inside;
			}
			if (builder.length() != 0) answer.add(builder.toString());
			
			return answer;
		}
		
		public ParsedTSDRFileSource(String filename, int type_id, long samplerate) {
			this.filename = filename;
			this.type_id = type_id;
			this.samplerate = samplerate;
		}
		
		public ParsedTSDRFileSource(final String command) {
			final ArrayList<String> parsed = parse(command);
			if (parsed.size() == 3) {
				final String supposed_type = parsed.get(2);
				
				int id = -1;
				for (int i = 0; i < filetype.length; i++) if (filetype[i].equals(supposed_type)) id = i;
				
				if (id == -1) return;
				
				long samplerate = -1;
				try {
					samplerate = Long.parseLong(parsed.get(1));
				} catch (Throwable e) {
					return;
				}
				
				this.filename = parsed.get(0);
				this.type_id = id;
				this.samplerate = samplerate;
				
			}
		}
		
		@Override
		public String toString() {
			if (type_id < 0 || type_id >= filetype.length)
				return "ParsedTSDRFileSource filename: "+filename+"; type: INVALID; samplerate: "+samplerate;
			return "ParsedTSDRFileSource filename: "+filename+"; type: "+filetype[type_id]+"; samplerate: "+samplerate;
		}
	}
}
