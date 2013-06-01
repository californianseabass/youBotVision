/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
#ifndef _ONI_SAMPLE_VIEWER_H_
#define _ONI_SAMPLE_VIEWER_H_


#ifdef _CLOSEST_POINT
#define MW_CP_API ONI_API_EXPORT
#else
#define MW_CP_API ONI_API_IMPORT
#endif

// #include "MWClosestPoint/MWClosestPoint.h"
#include <OpenNI.h>
#include <vector>

#define MAX_DEPTH 10000


/* ---------------------------- Closest Point --------------------------------*/
namespace openni
{
	class Device;
}

namespace closest_point
{

struct IntPoint3D
{
	int X;
	int Y;
	int Z;
};

struct ClosestPointInternal;

class MW_CP_API ClosestPoint
{
public:
	class Listener
	{
	public:
		virtual void readyForNextData(ClosestPoint*) = 0;
	};

	ClosestPoint(const char* uri = NULL);
	ClosestPoint(openni::Device* pDevice);
	~ClosestPoint();

	float getFOV();

	bool isValid() const;

	openni::Status setListener(Listener& listener);
	void resetListener();

	openni::Status getNextData(IntPoint3D& closestPoint, openni::VideoFrameRef& rawFrame);
private:
	void initialize();

	ClosestPointInternal* m_pInternal;
};

}





/* ------------------------- Viewer.h ...............................*/

class MyMwListener : public closest_point::ClosestPoint::Listener
{
public:
	MyMwListener() : m_ready(false) {}
	void readyForNextData(closest_point::ClosestPoint* pClosestPoint)
	{
		openni::Status rc = pClosestPoint->getNextData(m_closest, m_frame);

		if (rc == openni::STATUS_OK)
		{
//			printf("%d, %d, %d\n", m_closest.X, m_closest.Y, m_closest.Z);
		}
		else
		{
			printf("Update failed\n");
		}
		m_ready = true;
	}

	const openni::VideoFrameRef& getFrame() {return m_frame;}
	const closest_point::IntPoint3D& getClosestPoint() {return m_closest;}
	bool isAvailable() const {return m_ready;}
	void setUnavailable() {m_ready = false;}
private:
	openni::VideoFrameRef m_frame;
	closest_point::IntPoint3D m_closest;
	bool m_ready;
};


class SampleViewer
{
public:
	SampleViewer(const char* strSampleName, const char* deviceUri);
	virtual ~SampleViewer();

	virtual openni::Status init(int argc, char **argv);
	virtual openni::Status run();	//Does not return
	virtual bool returnDepthFrame(std::vector<uint16_t>& pixels);

	virtual int getWidth();
	virtual int getHeight();

	virtual double getXResolution();
	virtual double getYResolution();
	virtual double getFOV();

protected:
	virtual void display();
	virtual void displayPostDraw(){};	// Overload to draw over the screen image

	virtual void onKey(unsigned char key, int x, int y);

	virtual openni::Status initOpenGL(int argc, char **argv);
	void initOpenGLHooks();

	void finalize();

private:
	SampleViewer(const SampleViewer&);
	SampleViewer& operator=(SampleViewer&);

	static SampleViewer* ms_self;
	static void glutIdle();
	static void glutDisplay();
	static void glutKeyboard(unsigned char key, int x, int y);

	float				m_pDepthHist[MAX_DEPTH];
	char			m_strSampleName[ONI_MAX_STR];
	openni::RGB888Pixel*m_pTexMap;
	unsigned int		m_nTexMapX;
	unsigned int		m_nTexMapY;

	closest_point::ClosestPoint* m_pClosestPoint;
	MyMwListener* m_pClosestPointListener;

};


#endif // _ONI_SAMPLE_VIEWER_H_
