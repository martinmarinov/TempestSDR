package martin.tempest.gui;

import martin.tempest.core.TSDRLibrary.PARAM;
import martin.tempest.core.TSDRLibrary.PARAM_DOUBLE;

interface OnTSDRParamChangedCallback {
	public void onSetParam(PARAM param, long value);
	public void onSetParam(PARAM_DOUBLE param, double value);
}
