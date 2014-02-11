#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <complex>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include <stdint.h>
#include <boost/algorithm/string.hpp>

#include "errors.hpp"

uhd::usrp::multi_usrp::sptr usrp;
namespace po = boost::program_options;

EXTERNC void __stdcall tsdrplugin_getName(char * name) {
	uhd::set_thread_priority_safe();
	strcpy(name, "TSDR UHD USRP Compatible Plugin");
}

EXTERNC int __stdcall tsdrplugin_init(const char * params) {
	uhd::set_thread_priority_safe();

	// simulate argv and argc
	std::string sparams(params);

	typedef std::vector< std::string > split_vector_type;

	split_vector_type argscounter;
	boost::split( argscounter, sparams, boost::is_any_of(" "), boost::token_compress_on );

	int argc = argscounter.size()+1;
	char * argv[argc];
	char zerothtarg[] = "TSDRPlugin_UHD";
	argv[0] = zerothtarg;
	for (int i = 0; i < argc-1; i++)
		argv[i+1] = (char *) argscounter[i].c_str();

	//variables to be set by po
	std::string args, file, ant, subdev, ref;
	double rate, freq, gain, bw;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
			("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
			("rate", po::value<double>(&rate)->default_value(100e6/16), "rate of incoming samples")
			("freq", po::value<double>(&freq)->default_value(0), "rf center frequency in Hz")
			("gain", po::value<double>(&gain)->default_value(0), "gain for the RF chain")
			("ant", po::value<std::string>(&ant), "daughterboard antenna selection")
			("subdev", po::value<std::string>(&subdev), "daughterboard subdevice specification")
			("bw", po::value<double>(&bw), "daughterboard IF filter bandwidth in Hz")
			("ref", po::value<std::string>(&ref)->default_value("internal"), "waveform type (internal, external, mimo)") ;

	try {
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	} catch (std::exception const&  ex)
	{
		std::string msg(boost::str(boost::format("Error: %s\n\nTSDRPlugin_UHD %s") % ex.what() % desc));
		RETURN_EXCEPTION(msg.c_str(), TSDR_PLUGIN_PARAMETERS_WRONG);
	}

	//if (args.size() != ARG_COUNT) RETURN_EXCEPTION("Unexpected number of arguments!", TSDR_PLUGIN_PARAMETERS_WRONG);

	printf("Freq = %.4f\n", freq);
	printf("Rate = %.4f\n", rate);
	//usrp = uhd::usrp::multi_usrp::make(args[ARG_ID_ADDRESS]);

	RETURN_OK();
}

EXTERNC uint32_t __stdcall tsdrplugin_setsamplerate(uint32_t rate) {
	//uhd::set_thread_priority_safe();
	printf("tsdrplugin_setsamplerate not implemented!\n");fflush(stdout);
	RETURN_EXCEPTION("tsdrplugin_setsamplerate not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC uint32_t __stdcall tsdrplugin_getsamplerate() {
	//uhd::set_thread_priority_safe();
	printf("tsdrplugin_getsamplerate not implemented!\n");fflush(stdout);
	RETURN_EXCEPTION("tsdrplugin_getsamplerate not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC int __stdcall tsdrplugin_setbasefreq(uint32_t freq) {
	//uhd::set_thread_priority_safe();
	printf("tsdrplugin_setbasefreq not implemented!\n");fflush(stdout);
	RETURN_EXCEPTION("tsdrplugin_setbasefreq not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC int __stdcall tsdrplugin_stop(void) {
	//uhd::set_thread_priority_safe();
	printf("tsdrplugin_stop not implemented!\n");fflush(stdout);
	RETURN_EXCEPTION("tsdrplugin_stop not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC int __stdcall tsdrplugin_setgain(float gain) {
	//uhd::set_thread_priority_safe();
	printf("tsdrplugin_setgain not implemented!\n");fflush(stdout);
	RETURN_EXCEPTION("tsdrplugin_setgain not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC int __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	//uhd::set_thread_priority_safe();
	printf("tsdrplugin_readasync not implemented!\n");fflush(stdout);
	RETURN_EXCEPTION("tsdrplugin_readasync not implemented!", TSDR_NOT_IMPLEMENTED);
}

EXTERNC void __stdcall tsdrplugin_cleanup(void) {
	//uhd::set_thread_priority_safe();
	printf("tsdrplugin_cleanup not implemented!\n");fflush(stdout);
}
