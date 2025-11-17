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

/**
 * This plugin provides support for RTL-SDR devices using the Soapy SDR API.
 *
 * @author Martin Marinov
 *
 */
public class TSDRSoapyRTLSDRSource extends TSDRSource {

	public TSDRSoapyRTLSDRSource() {
		super("RTL-SDR (via SoapySDR)", "TSDRPlugin_SoapyRTLSDR", false);
	}

}
