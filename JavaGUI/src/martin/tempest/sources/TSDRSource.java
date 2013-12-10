package martin.tempest.sources;

import java.io.File;

public class TSDRSource {
	public final String libname;
	public final String params;
	public final boolean absolute;
	
	TSDRSource(final String libname, final String params, final boolean absolute) {
		this.libname = libname;
		this.params = params;
		this.absolute = absolute;
	}
	
	public static TSDRSource fromRawFile(final File file) {
		return new TSDRSource("TSDRPlugin_RawFile", file.getAbsolutePath(), false);
	}
	
	public static TSDRSource fromRawFile(final String filename) {
		return new TSDRSource("TSDRPlugin_RawFile", filename, false);
	}
	
	public static TSDRSource fromPlugin(final File full_path_to_library, final String params) {
		return new TSDRSource(full_path_to_library.getAbsolutePath(), params, true);
	}
}
