/*
#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Martin Marinov - initial API and implementation
#-------------------------------------------------------------------------------
*/
#ifdef OS_WINDOWS
	#define WINHEAD (1)
#elif defined(_WIN64)
	#define WINHEAD (1)
#elif defined(_WIN32)
	#define WINHEAD (1)
#elif defined(_MSC_VER) // Microsoft compiler
	#define WINHEAD (1)
#else
	#define WINHEAD (0)
#endif
