
// ChildView.cpp : implementation of the CChildView class
//

#include "stdafx.h"
#include "Oscilloscope.h"
#include "ChildView.h"
#include "MainFrm.h"

#undef max

#include <cmath>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

CChildView::CChildView() : _offset(0), _trigger(false), _zoom(1024)
{
}

CChildView::~CChildView()
{
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

double CChildView::DevToLog(long pixels) const 
{
	return (_offset + pixels) * GetSamplesPerPixel();
}

long CChildView::LogToDev(double samples) const
{
	return long(samples / GetSamplesPerPixel() - _offset);
}

void CChildView::OnPaint() 
{
	CPaintDC dc(this); 

	CRect rect;
	GetClientRect(rect);

	CPen pen1(PS_SOLID, 1, 0xc0c0c0);
	dc.SelectObject(pen1);

	CRect updateRect;
	dc.GetClipBox(&updateRect);

	// Draw all divs, not just ones in updateRect, so div positions are identical for timer redraws and window redraws. 
	const double samplesPerDiv = Serial::GetSampleFrequency() / double(GetDivsPerSecond());
	double div = samplesPerDiv * long(DevToLog(0) / samplesPerDiv);
	long divPos = LogToDev(div);
	while (divPos < updateRect.right)
	{
		if (divPos >= updateRect.left)
		{
			dc.MoveTo(divPos, rect.top);
			dc.LineTo(divPos, rect.bottom);
		}

		div += samplesPerDiv;
		divPos = LogToDev(div);
	}

	rect.DeflateRect(0, 10);

	for (int v = 0; v <= 5; ++v)
	{
		int y = rect.bottom - int(rect.Height() * v / 5.0);
		dc.MoveTo(updateRect.left, y);
		dc.LineTo(updateRect.right, y);
	}

	CPen pen2(PS_SOLID, 1, 0xff8000);
	dc.SelectObject(pen2);

	const long firstSample = long(DevToLog(updateRect.left));
	long sample = firstSample;
	long samplePos = LogToDev(sample);
	while (samplePos < updateRect.right && sample < (long)_samples.size())
	{
		int y = rect.bottom - int((_samples[sample].value / 1023.0) * rect.Height());
		if (sample == firstSample)
			dc.MoveTo(samplePos, y);
		else
			dc.LineTo(samplePos, y);

		sample = long(DevToLog(++samplePos));
	}

	dc.SelectStockObject(NULL_PEN);
}

void CChildView::Zoom(int steps)
{
	if (steps == -1)
	{
		if (_zoom > 1)
		{
			_zoom /= 2;
			_offset /= 2;
		}
	}
	else if (steps == 1)
	{
		_zoom *= 2;
		_offset *= 2;
	}
	Invalidate();
}

int CChildView::GetPixelsPerSecond() const
{
	return _zoom;
}

int CChildView::GetDivsPerSecond() const
{
	return _zoom <= 4096 ? 1 : 1000;
}

double CChildView::GetSamplesPerPixel() const
{
	return Serial::GetSampleFrequency() / double(GetPixelsPerSecond());
}

void CChildView::Reset()
{
	_samples.clear();
	_offset = 0;
	Invalidate();
}

void CChildView::Update()
{
	Serial::Sample::value_t minVal, maxVal;
	Serial::SampleVec newSamples = CMainFrame::Instance().GetSerial().HarvestSamples(minVal, maxVal);

	CRect clientRect;
	GetClientRect(clientRect);

	auto firstNewSample = newSamples.begin();

	if (_trigger)
	{
		if (_samples.size() / GetSamplesPerPixel() >= clientRect.Width())
		{
			Reset();

			Serial::Sample::value_t threshold = minVal + (maxVal - minVal) / 2;

			for (; firstNewSample != newSamples.end(); ++firstNewSample)
				if (firstNewSample->value < threshold)
					break;
			for (; firstNewSample != newSamples.end(); ++firstNewSample)
				if (firstNewSample->value >= threshold)
					break;
		}
	}
	else
	{
		size_t oldOffset = _offset;

		_offset = std::max(0l, long((_samples.size() + newSamples.size()) / GetSamplesPerPixel()) - clientRect.Width());

		long scrollPixels = _offset - oldOffset;

		CRect updateRect;
		ScrollWindowEx(-scrollPixels, 0, nullptr, nullptr, nullptr, &updateRect, 0);
	}

	CRect updateRect = clientRect;
	updateRect.left = LogToDev(_samples.size()) - 1;

	_samples.insert(_samples.end(), firstNewSample, newSamples.end());

	updateRect.right = LogToDev(_samples.size()) + 1;

	InvalidateRect(updateRect);
}
