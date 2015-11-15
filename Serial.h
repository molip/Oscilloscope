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
	SampleVec HarvestSamples();
	double GetTemporalError() const;
	double GetFrequency() const { return _frequency; }
	Sample::value_t GetMinVal() const { return _minVal; }
	Sample::value_t GetMaxVal() const { return _maxVal; }
	std::wstring GetPortName() const;

	static double GetSampleFrequency();
	static WORD GetMaxValue();

private:
	void Go();
	size_t ReadSamples(DWORD bytesRead);
	void ResetSamples();
	bool ProcessByte8(byte b, WORD& value);
	bool ProcessByte10(byte b, WORD& value);

	HANDLE _file;
	mutable std::mutex _samplesMutex, _chunkMutex;
	bool _abort;
	std::thread _thread;

	SampleVec _samples;
	size_t _sampleCount;
	int _lastChunkTime, _lastChunkSamples;
	byte* _buffer;
	WORD _unpack;
	size_t _unpacked;
	Sample::value_t _minVal, _maxVal;

	size_t _lastThresholdCross;
	double _frequency;
	bool _beenDown;
};
