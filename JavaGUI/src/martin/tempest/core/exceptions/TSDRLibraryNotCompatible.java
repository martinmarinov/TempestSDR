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
package martin.tempest.core.exceptions;

public class TSDRLibraryNotCompatible extends TSDRException {
	private static final long serialVersionUID = -7001791780564950322L;
	
	public TSDRLibraryNotCompatible(final Exception e) {
		super(e);
	}
	
	public TSDRLibraryNotCompatible(final String msg) {
		super(msg);
	}
}
