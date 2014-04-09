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
 * This is a source that works with the Mirics dongle, for more information see <a href="http://www.mirics.com/">mirics.com</a>
 * 
 * @author Martin Marinov
 *
 */
public class TSDRMiricsSource extends TSDRSource {
	
	public TSDRMiricsSource() {
		super("Mirics dongle", "TSDRPlugin_Mirics", false);
	}
	
	@Override
	public boolean populateGUI(final Container cont, final String defaultprefs, final ActionListenerRegistrator okbutton) {
		setParams(defaultprefs);
		return false;
	}

}
