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

	void Zoom(int steps);
	int GetZoom() const { return _zoom; }

	int GetDivsPerSecond() const;

private:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	double GetSamplesPerPixel() const;
	void UpdateDivsPerSecond();

	double DevToLog(long pixels) const;
	long LogToDev(double samples) const;

	size_t _offset; // Pixels.
	Serial::SampleVec _samples;
	bool _trigger;
	int _zoom;
	int _divsPerSecond;
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};

