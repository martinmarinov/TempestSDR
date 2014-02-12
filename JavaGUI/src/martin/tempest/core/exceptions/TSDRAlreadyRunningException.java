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

public class TSDRAlreadyRunningException extends TSDRException {

	private static final long serialVersionUID = 5365909402344178885L;

	public TSDRAlreadyRunningException(final Exception e) {
		super(e);
	}
	
	public TSDRAlreadyRunningException(final String msg) {
		super(msg);
	}
}
