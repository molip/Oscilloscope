#pragma once

#include <mutex>
#include <thread>
#include <vector>

class Serial
{
public:
	Serial();
	~Serial();

	bool Open();
	bool Close();
	int Read() const;

	bool IsOpen() const { return _file != nullptr; }

	struct Sample
	{
		typedef WORD value_t;
		value_t value;
	};
	typedef std::vector<Sample> SampleVec;

	Sample GetSample(size_t i) const;
	SampleVec HarvestSamples(Sample::value_t& minVal, Sample::value_t& maxVal);
	double GetTemporalError() const;
	double GetFrequency() const { return _frequency; }

	static int GetSampleFrequency();

private:
	void Go();
	size_t ReadSamples(DWORD bytesRead);
	void ResetSamples();

	HANDLE _file;
	mutable std::mutex _samplesMutex, _chunkMutex;
	bool _abort;
	std::thread _thread;

	SampleVec _samples;
	size_t _sampleCount;
	int _lastChunkTime, _lastChunkSamples;
	byte* _buffer;
	short _unpack;
	size_t _unpacked;
	Sample::value_t _minVal, _maxVal;

	size_t _lastThresholdCross;
	double _frequency;
	bool _beenDown;
};
