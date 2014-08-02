#pragma once

#include "Serial.h"

class CChildView : public CWnd
{
public:
	CChildView();
	virtual ~CChildView();

	void Update();
	void Reset();

	bool IsTrigger() const { return _trigger; }
	void SetTrigger(bool trigger) { _trigger = trigger; Reset(); }

private:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	double GetSamplesPerPixel() const;
	double GetPixelsPerSecond() const;
	double GetDivsPerSecond() const;

	double DevToLog(long pixels) const;
	long LogToDev(double samples) const;

	size_t _offset; // Pixels.
	Serial::SampleVec _samples;
	bool _trigger;
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};

