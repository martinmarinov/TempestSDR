/*
# SPDX-License-Identifier: GPL-3.0-or-later
# TempestSDR Plugin Architecture:
#  Copyright (c) 2014 Martin Marinov.
# librtlsdr library:
#  Copyright (C) 2012-2013 by Steve Markgraf <steve@steve-m.de>
#  Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
# Plugin implementation:
#  Copyright (C) 2025 Tymoteusz Vogt
*/
package martin.tempest.sources;

/**
 * This plugin allows for RTL2832U based SDR USB dongles to be used with the application.
 */
public class TSDRRTLSource extends TSDRSource {

	public TSDRRTLSource() {
		super("RTL2832U SDR dongle", "TSDRPlugin_RTL", false);
	}

}
