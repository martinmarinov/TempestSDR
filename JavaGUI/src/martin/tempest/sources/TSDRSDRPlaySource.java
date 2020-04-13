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

/**
 * This is a source that works with the SDRPlay RSP, for more information see <a href="http://www.sdrplay.com/">sdrplay.com</a>
 * 
 * @author Martin Marinov
 *
 */
public class TSDRSDRPlaySource extends TSDRSource {
	
	public TSDRSDRPlaySource() {
		super("SDRplay RSP", "TSDRPlugin_SDRPlay", false);
	}
	
	@Override
	public boolean populateGUI(final Container cont, final String defaultprefs, final ActionListenerRegistrator okbutton) {
		setParams(defaultprefs);
		return false;
	}
}
