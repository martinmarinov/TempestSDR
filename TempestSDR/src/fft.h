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

#ifndef FFT_H_
#define FFT_H_

// m is the log2(size). i.e. if data is 1024, m should be defined to 10
void fft_real(float * data, int m);

#endif
