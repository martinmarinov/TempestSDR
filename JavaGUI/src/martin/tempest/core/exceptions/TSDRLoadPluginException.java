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

public class TSDRLoadPluginException extends TSDRException {
	
	private static final long serialVersionUID = 5210950769308579938L;

	public TSDRLoadPluginException(final String msg) {
		super(msg);
	}
	
	public TSDRLoadPluginException() {
		super();
	}
}
