#include "stdafx.h"
#include "Serial.h"

#include <algorithm>

namespace
{
	const size_t MaxSampleCount = 1024 * 100;
	const size_t BufferSize = 1024;
}

Serial::Serial() : _file(nullptr), _abort(false), _lastChunkTime(0), _lastChunkSamples(0), _unpacked(0), _unpack(0)
{
	_samples.resize(MaxSampleCount);
	_buffer = new byte[BufferSize];

	ResetSamples();
}

Serial::~Serial()
{
	Close();
	delete [] _buffer;
}

double Serial::GetSampleFrequency()
{
	return 1000000.0 / 160;
}

bool Serial::Open()
{
	if (_file)
		return false;

	HANDLE file = CreateFile(L"COM6", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (!file || file == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(L"CreateFile failed");
		return false;
	}
	DCB dcb;
	::memset(&dcb, 0, sizeof dcb);
	dcb.fBinary = true;
	dcb.BaudRate = 115200;
	dcb.ByteSize = 8;
	if (!SetCommState(file, &dcb))
	{
		AfxMessageBox(L"SetCommState failed");
		return false;
	}

	COMMTIMEOUTS timeouts;
	if (!GetCommTimeouts(file, &timeouts))
	{
		AfxMessageBox(L"GetCommTimeouts failed");
		return false;
	}

	//timeouts.ReadIntervalTimeout = 5;
	//timeouts.ReadTotalTimeoutConstant = 5;
	//timeouts.ReadTotalTimeoutMultiplier = 10;

	timeouts.ReadIntervalTimeout = 1;
	timeouts.ReadTotalTimeoutConstant = 1;
	timeouts.ReadTotalTimeoutMultiplier = 1;

	if (!SetCommTimeouts(file, &timeouts))
	{
		AfxMessageBox(L"SetCommTimeouts failed");
		return false;
	}

	_file = file;
	_abort = false;
	_thread = std::thread(&Serial::Go, this);

	return true;
}

bool Serial::Close()
{
	if (!_file)
		return false;
	
	_abort = true;
	_thread.join();

	if (!CloseHandle(_file))
	{
		AfxMessageBox(L"CloseHandle failed");
		return false;
	}

	_file = nullptr;
	return true;
}

void Serial::ResetSamples()
{
	_sampleCount = 0;
	_minVal = 1023;
	_maxVal = 0;
	_lastThresholdCross = 0;
	_frequency = 0;
	_beenDown = false;
}

void Serial::Go()
{
	ResetSamples();
	_lastChunkTime = _lastChunkSamples = 0;
	_unpacked = 0;

	DWORD chunkStartTime = ::GetTickCount();
	int chunkSamples = 0;

	const int sampleSize = sizeof(Sample::value_t);
	
	while (!_abort)
	{
		DWORD bytesRead = 0;
		ReadFile(_file, _buffer, BufferSize, &bytesRead, nullptr);
		
		if (bytesRead)
		{
			chunkSamples += ReadSamples(bytesRead);

			DWORD now = ::GetTickCount();
			if (now > chunkStartTime + 250)
			{
				std::lock_guard<std::mutex> lock(_chunkMutex);
				_lastChunkSamples = chunkSamples;
				_lastChunkTime = now - chunkStartTime;
				chunkSamples = 0;
				chunkStartTime = now;
			}
		}
	}
}

size_t Serial::ReadSamples(DWORD bytesRead)
{
	std::lock_guard<std::mutex> lock(_samplesMutex);

	size_t oldSampleCount = _sampleCount;

	for (size_t i = 0; i < bytesRead; ++i)
	{
		byte b = _buffer[i];

		if (b & 0x80) // First byte
		{
			_unpack = short(b & 0x7f) << 3; // High 7.
			_unpacked = 1;
		}
		else
		{
			short value;
			bool ok = false;
			if (_unpacked == 1)
			{
				value = _unpack | b >> 4; // Low 3.
				_unpack = short(b & 0xf) << 6; // High 4.
				_unpacked = 2;
				ok = true;
			}
			else if (_unpacked == 2)
			{
				if ((b & 0x40) == 0)
				{
					value = _unpack | b; // Low 6.
					ok = true;
				}
				_unpacked = 0;
			}
			else
				continue;

			if (ok)
			{
				if (_sampleCount == MaxSampleCount)
					ResetSamples();

				if (_minVal > value)
					_minVal = value;

				if (_maxVal < value)
					_maxVal = value;

				Serial::Sample::value_t threshold = _minVal + (_maxVal - _minVal) / 2;

				if (_sampleCount)
				{
					if (_samples[_sampleCount - 1].value >= threshold && value < threshold)
						_beenDown = true;
					else if (_beenDown && _samples[_sampleCount - 1].value < threshold && value >= threshold)
					{
						if (_lastThresholdCross)
							_frequency = GetSampleFrequency() / (_sampleCount - _lastThresholdCross);

						_lastThresholdCross = _sampleCount;
					}
				}
				_samples[_sampleCount++] = Sample{ value };
			}
		}
	}

	return _sampleCount - oldSampleCount;
}

Serial::Sample Serial::GetSample(size_t i) const
{
	std::lock_guard<std::mutex> lock(_samplesMutex);
	return _samples[i];
}

Serial::SampleVec Serial::HarvestSamples(Sample::value_t& minVal, Sample::value_t& maxVal)
{
//	return Serial::SampleVec(600, Sample{ 511 });

	std::lock_guard<std::mutex> lock(_samplesMutex);

	//int first = count > _sampleCount ? 0 : _sampleCount - count;
	//samples.insert(samples.begin(), _samples.begin() + first, _samples.begin() + _sampleCount);
	Serial::SampleVec samples(_samples.begin(), _samples.begin() + _sampleCount);
	
	minVal = _minVal, maxVal = _maxVal;

	ResetSamples();

	return samples;
}

double Serial::GetTemporalError() const
{
	std::lock_guard<std::mutex> lock(_chunkMutex);
	return _lastChunkTime ? _lastChunkSamples * 1000 / (GetSampleFrequency() * _lastChunkTime) - 1 : 0.0;
}
