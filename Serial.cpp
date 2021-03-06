#include "stdafx.h"
#include "Serial.h"

#include <algorithm>

namespace
{
	const size_t MaxSampleCount = 1024 * 100;
	const size_t BufferSize = 1024;

	const int SampleBits = 8;
	const int UsbSpeed = 230400;
	const int SamplePeriod = 50;
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
	return 1000000.0 / SamplePeriod;
}

WORD Serial::GetMaxValue()
{
	return  (1 << SampleBits) - 1;
}

std::wstring Serial::FindPortName() const
{
	WCHAR devs[1 << 16];
	DWORD chars = ::QueryDosDevice(nullptr, devs, 1 << 16);

	for (const WCHAR* p = devs; *p;)
	{
		std::wstring s = p;
		if (s.substr(0, 3) == L"COM")
			return s;
		p += s.length() + 1;
	}
	return std::wstring();
}

bool Serial::Open()
{
	if (_file)
		return false;

	std::wstring path = L"\\\\.\\";
	std::wstring portName = _portName.empty() ? FindPortName() : _portName;
	path += portName;
		
	HANDLE file = CreateFile(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (!file || file == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(L"CreateFile failed");
		return false;
	}
	DCB dcb;
	::memset(&dcb, 0, sizeof dcb);
	dcb.fBinary = true;
	dcb.BaudRate = UsbSpeed;
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
	_portName = portName;

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
	_oldValRange = _valRange;
	_valRange.first = Serial::GetMaxValue();
	_valRange.second = 0;
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

bool Serial::ProcessByte8(byte b, WORD& value)
{
	value = b;
	return true;
}

bool Serial::ProcessByte10(byte b, WORD& value)
{
	bool ok = false;

	if (b & 0x80) // First byte
	{
		_unpack = WORD(b & 0x7f) << 3; // High 7.
		_unpacked = 1;
		ok = true;
	}
	else
	{
		WORD value;
		bool ok = false;
		if (_unpacked == 1)
		{
			value = _unpack | b >> 4; // Low 3.
			_unpack = WORD(b & 0xf) << 6; // High 4.
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
	}
	return ok;
}

size_t Serial::ReadSamples(DWORD bytesRead)
{
	std::lock_guard<std::mutex> lock(_samplesMutex);

	size_t oldSampleCount = _sampleCount;

	for (size_t i = 0; i < bytesRead; ++i)
	{
		WORD value = 0;
		if (SampleBits == 10 ? ProcessByte10(_buffer[i], value) : ProcessByte8(_buffer[i], value))
		{
			if (_sampleCount == MaxSampleCount)
				ResetSamples();

			if (_valRange.first > value)
				_valRange.first = value;

			if (_valRange.second < value)
				_valRange.second = value;

			_smoothedValRange.first = std::min(_valRange.first, _oldValRange.first);
			_smoothedValRange.second = std::max(_valRange.second, _oldValRange.second);

			Serial::Sample::value_t threshold = _smoothedValRange.first + (_smoothedValRange.second - _smoothedValRange.first) / 2;

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

	return _sampleCount - oldSampleCount;
}

Serial::Sample Serial::GetSample(size_t i) const
{
	std::lock_guard<std::mutex> lock(_samplesMutex);
	return _samples[i];
}

Serial::SampleVec Serial::HarvestSamples()
{
//	return Serial::SampleVec(600, Sample{ 511 });

	std::lock_guard<std::mutex> lock(_samplesMutex);

	//int first = count > _sampleCount ? 0 : _sampleCount - count;
	//samples.insert(samples.begin(), _samples.begin() + first, _samples.begin() + _sampleCount);
	Serial::SampleVec samples(_samples.begin(), _samples.begin() + _sampleCount);
	
	ResetSamples();

	return samples;
}

double Serial::GetTemporalError() const
{
	std::lock_guard<std::mutex> lock(_chunkMutex);
	return _lastChunkTime ? _lastChunkSamples * 1000 / (GetSampleFrequency() * _lastChunkTime) - 1 : 0.0;
}
