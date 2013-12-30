package martin.tempest.sources;

import java.io.File;

public class TSDRSource {
	
	private final static TSDRSource[] SOURCES = new TSDRSource[] {
		new TSDRFileSource(""),
	};
	
	public final String libname;
	private String params;
	public final boolean absolute;
	public final String descr;
	
	public static TSDRSource[] getAvailableSources() {
		return SOURCES;
	}
	
	TSDRSource(final String desc, final String libname, final String params, final boolean absolute) {
		this.descr = desc;
		this.libname = libname;
		this.params = params;
		this.absolute = absolute;
	}
	
	public void setParams(final String params) {
		this.params = params;
	}
	
	public String getParams() {
		return params;
	}
	
	public static TSDRSource fromPlugin(final String desc, final File full_path_to_library, final String params) {
		return new TSDRSource(desc, full_path_to_library.getAbsolutePath(), params, true);
	}
	
	@Override
	public String toString() {
		return descr;
	}
}
