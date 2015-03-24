// ChordExtraction.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "include/vamp-hostsdk/PluginHostAdapter.h"
#include "include/vamp-hostsdk/PluginInputDomainAdapter.h"
#include "include/vamp-hostsdk/PluginLoader.h"
#include "include\sndfile.h"
#include "include\sndfile.hh"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <set>

using namespace std;
using namespace Vamp;
using namespace Vamp::HostExt;



#define KEY "nnls-chroma:chordino"
#define WAV_TEST "sample/suynghitronganh.wav"
static const float rate = 44100.00f;
//typedef std::vector<Result> Results;

void appendFeatures(Plugin::FeatureSet &a, const Plugin::FeatureSet &b)
{
	for (Plugin::FeatureSet::const_iterator i = b.begin(); i != b.end(); ++i) {
		int output = i->first;
		const Plugin::FeatureList &fl = i->second;
		Plugin::FeatureList &target = a[output];
		for (Plugin::FeatureList::const_iterator j = fl.begin(); j != fl.end(); ++j) {
			target.push_back(*j);
		}
	}
}
void destroyTestAudio(float **b, size_t channels)
{
	for (size_t c = 0; c < channels; ++c) {
		delete[] b[c];
	}
	delete[] b;
}

float** createTestAudio(size_t channels, size_t blocksize, size_t blocks)
{
	float **b = new float *[channels];
	for (size_t c = 0; c < channels; ++c) {
		b[c] = new float[blocksize * blocks];
		for (int i = 0; i < int(blocksize * blocks); ++i) {
			b[c][i] = sinf(float(i) / 10.f);
			if (i == 5005 || i == 20002) {
				b[c][i - 2] = 0;
				b[c][i - 1] = -1;
				b[c][i] = 1;
			}
		}
	}
	return b;
}

float** loadWaveAudio_mono(string filepath, size_t &channels, size_t blocksize, size_t step, size_t &blocks){
	SndfileHandle file;
	file = SndfileHandle(WAV_TEST);
	channels = 1;
	blocks = ceil(file.frames() / step);
	int buffersz = file.frames();
	float** buffer = new float*[channels];
	buffer[0] = new float[buffersz];
	file.readf(buffer[0], buffersz);

	return buffer;
}

int _tmain(int argc, _TCHAR* argv[])
{
	//return 0;
	// Get names of all VAMP Plugins
	Vamp::HostExt::PluginLoader::PluginKeyList keys =
		Vamp::HostExt::PluginLoader::getInstance()->listPlugins();
	// Get Chordino plugin (nnls-chroma:chordino)
	Plugin *p = PluginLoader::getInstance()->loadPlugin
		(KEY, rate, PluginLoader::ADAPT_ALL);
	if (!p)
		return 1;
	PluginInputDomainAdapter *pa = new PluginInputDomainAdapter(p);
	Vamp::Plugin::InputDomain inp = pa->getInputDomain();
	// Features data
	
	Plugin::FeatureSet f;
	//Results r;
	float **data = 0;
	size_t channels = pa->getMaxChannelCount();
	size_t step = pa->getPreferredStepSize();
	//size_t step = pa->getPreferredBlockSize();
	size_t block = pa->getPreferredBlockSize();
	size_t count = 100; //= number of blocks

	// Init plugin

	//pa->initialise(channels, step, block);
	pa->initialise(channels, step, block);

	bool real = true;
	// Option 1:Create a test audio
	if (!real)
		data = createTestAudio(channels, block, count);
	else
		data = loadWaveAudio_mono(WAV_TEST, channels, block, step, count);
	// Option 2: Load wave file
	// Run the plugin

	for (size_t i = 0; i < count; ++i) {

		float **ptr = (float **)alloca(channels * sizeof(float));
		size_t idx = i * step;
		for (size_t c = 0; c < channels; ++c) ptr[c] = data[c] + idx;
		RealTime timestamp = RealTime::frame2RealTime(idx, rate);
		Plugin::FeatureSet fs = pa->process(ptr, timestamp);
		appendFeatures(f, fs);
	}
	Plugin::FeatureSet fs = pa->getRemainingFeatures();
	appendFeatures(f, fs);
	if (data) destroyTestAudio(data, channels);
	
	return 0;
}
