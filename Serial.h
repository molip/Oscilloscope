#pragma once

#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>

#undef min
#undef max

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
	Sample::value_t GetMinVal() const { return std::min(_smoothedValRange.first, _smoothedValRange.second); }
	Sample::value_t GetMaxVal() const { return _smoothedValRange.second; }
	std::wstring GetPortName() const { return _portName; }

	static double GetSampleFrequency();
	static WORD GetMaxValue();

private:
	void Go();
	size_t ReadSamples(DWORD bytesRead);
	void ResetSamples();
	bool ProcessByte8(byte b, WORD& value);
	bool ProcessByte10(byte b, WORD& value);
	std::wstring FindPortName() const;

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
	std::wstring _portName;

	using ValPair = std::pair<Sample::value_t, Sample::value_t>;
	ValPair _valRange, _oldValRange, _smoothedValRange;

	size_t _lastThresholdCross;
	double _frequency;
	bool _beenDown;
};
