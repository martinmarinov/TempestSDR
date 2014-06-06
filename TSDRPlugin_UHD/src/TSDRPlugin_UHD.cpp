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

#define HOW_OFTEN_TO_CALL_CALLBACK_SEC (0.06)
#define FRACT_DROPPED_TO_TOLERATE (0)

uhd::usrp::multi_usrp::sptr usrp;
namespace po = boost::program_options;

uint32_t req_freq = 105e6;
float req_gain = 1;
double req_rate = 25e6;
volatile int is_running = 0;

EXTERNC TSDRPLUGIN_API void __stdcall tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR UHD USRP Compatible Plugin");
}

double tousrpgain(float gain) {
	try {
		uhd::gain_range_t range = usrp->get_rx_gain_range();
		return gain * (range.stop() - range.start()) + range.start();
	}
	catch (std::exception const&  ex)
	{
		return gain * 60;
	}
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_init(const char * params) {
	// simulate argv and argc
	std::string sparams(params);

	typedef std::vector< std::string > split_vector_type;

	split_vector_type argscounter;
	boost::split( argscounter, sparams, boost::is_any_of(" "), boost::token_compress_on );

	const int argc = argscounter.size()+1;
	char ** argv = (char **) malloc(argc*sizeof(char *));
	char zerothtarg[] = "TSDRPlugin_UHD";
	argv[0] = (char *) zerothtarg;
	for (int i = 0; i < argc-1; i++)
		argv[i+1] = (char *) argscounter[i].c_str();

	//variables to be set by po
	std::string args, file, ant, subdev, ref, tsrc;
	double bw;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
			("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
			("ant", po::value<std::string>(&ant), "daughterboard antenna selection")
			("rate", po::value<double>(&req_rate)->default_value(req_rate), "rate of incoming samples")
			("subdev", po::value<std::string>(&subdev), "daughterboard subdevice specification")
			("bw", po::value<double>(&bw), "daughterboard IF filter bandwidth in Hz")
			("ref", po::value<std::string>(&ref)->default_value("internal"), "waveform type (internal, external, mimo)")
			("tsrc", po::value<std::string>(&tsrc)->default_value("external"), "time source (none, external, _external_, mimo)") ;

	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	} catch (std::exception const&  ex)
	{
		std::string msg(boost::str(boost::format("Error: %s\n\nTSDRPlugin_UHD %s") % ex.what() % desc));
		RETURN_EXCEPTION(msg.c_str(), TSDR_PLUGIN_PARAMETERS_WRONG);
	}

	try {
		//create a usrp device
		usrp = uhd::usrp::multi_usrp::make(args);

		//Lock mboard clocks
		usrp->set_clock_source(ref);
		if (vm.count("tsrc")) usrp->set_time_source(tsrc);

		usrp->set_rx_rate(req_rate);
		req_rate = usrp->get_rx_rate();

		//set the rx center frequency
		usrp->set_rx_freq(req_freq);

		//set the rx rf gain
		usrp->set_rx_gain(tousrpgain(req_gain));
		//set the antenna
		if (vm.count("ant")) usrp->set_rx_antenna(ant);

		//set the IF filter bandwidth
		if (vm.count("bw"))
			usrp->set_rx_bandwidth(bw);

		boost::this_thread::sleep(boost::posix_time::seconds(1)); //allow for some setup time

		//Check Ref and LO Lock detect
		std::vector<std::string> rx_sensor_names;
		rx_sensor_names = usrp->get_rx_sensor_names(0);
		if (std::find(rx_sensor_names.begin(), rx_sensor_names.end(), "lo_locked") != rx_sensor_names.end()) {
			uhd::sensor_value_t lo_locked = usrp->get_rx_sensor("lo_locked",0);
			UHD_ASSERT_THROW(lo_locked.to_bool());
		}
		std::vector<std::string>  sensor_names = usrp->get_mboard_sensor_names(0);
		if ((ref == "mimo") and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked") != sensor_names.end())) {
			uhd::sensor_value_t mimo_locked = usrp->get_mboard_sensor("mimo_locked",0);
			UHD_ASSERT_THROW(mimo_locked.to_bool());
		}
		if ((ref == "external") and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end())) {
			uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked",0);
			UHD_ASSERT_THROW(ref_locked.to_bool());
		}
	} catch (std::exception const&  ex)
	{
		free(argv);
		RETURN_EXCEPTION(ex.what(), TSDR_CANNOT_OPEN_DEVICE);
	}

	free(argv);
	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API uint32_t __stdcall tsdrplugin_setsamplerate(uint32_t rate) {
	if (is_running)
		return tsdrplugin_getsamplerate();

	req_rate = rate;

	try {
		usrp->set_rx_rate(req_rate);
		double real_rate = usrp->get_rx_rate();
		req_rate = real_rate;
	}
	catch (std::exception const&  ex)
	{
	}

	return req_rate;
}

EXTERNC TSDRPLUGIN_API uint32_t __stdcall tsdrplugin_getsamplerate() {

	try {
		req_rate = usrp->get_rx_rate();
	}
	catch (std::exception const&  ex)
	{
	}

	return req_rate;
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_setbasefreq(uint32_t freq) {
	req_freq = freq;

	try {
		usrp->set_rx_freq(req_freq);
	}
	catch (std::exception const&  ex)
	{
	}

	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_stop(void) {
	is_running = 0;
	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_setgain(float gain) {
	req_gain = gain;
	try {
		usrp->set_rx_gain(tousrpgain(req_gain));
	}
	catch (std::exception const&  ex)
	{
	}
	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API int __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	uhd::set_thread_priority_safe();

	is_running = 1;

	float * buff = NULL;

	try {
		//set the rx sample rate
		usrp->set_rx_rate(req_rate);

		//set the rx center frequency
		usrp->set_rx_freq(req_freq);

		//set the rx rf gain
		usrp->set_rx_gain(tousrpgain(req_gain));

		//create a receive streamer
		uhd::stream_args_t stream_args("fc32"); //complex floats
		uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

		//loop until total number of samples reached
		// 1 sample = 2 items
		uhd::rx_metadata_t md;
		md.has_time_spec = false;

		size_t buff_size = HOW_OFTEN_TO_CALL_CALLBACK_SEC * req_rate * 2;
		const size_t samples_per_api_read = rx_stream->get_max_num_samps();
		if (buff_size < samples_per_api_read * 2) buff_size = samples_per_api_read * 2;
		buff = (float *) malloc(sizeof(float) * buff_size);
		const size_t items_per_api_read = samples_per_api_read*2;

		// initialize counters
		size_t items_in_buffer = 0;

		const uint64_t samp_rate_uint = req_rate;
		const double samp_rate_fract = req_rate - (double) samp_rate_uint;
		//setup streaming
		usrp->set_time_now(uhd::time_spec_t(0.0));
		rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

		uint64_t last_firstsample = 0;

		while(is_running){
			// if next call will overflow our buffer, call the callback
			if (items_per_api_read + items_in_buffer > buff_size) {
				int64_t dropped_samples = 0;
				const uint64_t samples_in_buffer = items_in_buffer >> 1;

				// estimate dropped samples
				if (md.has_time_spec) {
					const uint64_t roundsecs = (uint64_t) md.time_spec.get_full_secs();
					uint64_t first_sample_id = roundsecs * samp_rate_uint;
					first_sample_id += roundsecs * samp_rate_fract + 0.5;
					first_sample_id += md.time_spec.get_frac_secs() * req_rate + 0.5;

					// we should have the id of the first sample in our first_sample_id
					const uint64_t expected_first_sample_id = last_firstsample + samples_in_buffer;
					const int64_t dropped_samples_now = first_sample_id - expected_first_sample_id;

					dropped_samples = dropped_samples_now;

					// estimate next frame
					last_firstsample = first_sample_id;
				}

				if (dropped_samples <= 0)
					cb(buff, items_in_buffer, ctx, 0); // nothing was dropped, nice
				else if ((dropped_samples / ((float) samples_in_buffer)) < FRACT_DROPPED_TO_TOLERATE)
					cb(buff, items_in_buffer, ctx, dropped_samples); // some part of the data was dropped, but that's fine
				else
					cb(buff, 0, ctx, dropped_samples + samples_in_buffer); // too much dropped, abort

				// reset counters, the native buffer is empty
				items_in_buffer = 0;
			}

			size_t num_rx_samps = rx_stream->recv(
					&buff[items_in_buffer], samples_per_api_read, md
			);

			// vizualize gap
			items_in_buffer+=(num_rx_samps << 1);

			//handle the error codes
			switch(md.error_code){
			case uhd::rx_metadata_t::ERROR_CODE_NONE:
				break;

			case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
				std::cout << boost::format(
						"Got timeout before all samples received, possible packet loss."
				) << std::endl;
				break;
			case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
				//printf("Overflow!\n"); fflush(stdout);
				break;

			default:
				std::cout << boost::format(
						"Got error code 0x%x, exiting loop..."
				) % md.error_code << std::endl;
				goto done_loop;
			}

		} done_loop:

		usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

		// flush usrpbuffer
	    while(rx_stream->recv(
	        buff, samples_per_api_read, md,
	        uhd::device::RECV_MODE_ONE_PACKET
	    )){
	        /* NOP */
	    };
	}
	catch (std::exception const&  ex)
	{
		is_running = 0;
		if (buff!=NULL) free(buff);
		RETURN_EXCEPTION(ex.what(), TSDR_CANNOT_OPEN_DEVICE);
	}
	if (buff!=NULL) free(buff);
	RETURN_OK();

	return 0; // to avoid getting warning from stupid Eclpse
}

EXTERNC TSDRPLUGIN_API void __stdcall tsdrplugin_cleanup(void) {

	try {
		usrp.reset();
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	} catch (std::exception const&  ex) {

	}

	is_running = 0;
}
