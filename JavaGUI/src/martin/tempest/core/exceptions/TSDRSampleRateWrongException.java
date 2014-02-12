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

public class TSDRSampleRateWrongException extends TSDRException {

	private static final long serialVersionUID = 8116802552326966327L;

	public TSDRSampleRateWrongException(final Exception e) {
		super(e);
	}
	
	public TSDRSampleRateWrongException(final String msg) {
		super(msg);
	}
}
