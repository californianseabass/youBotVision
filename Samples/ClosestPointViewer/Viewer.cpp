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
#include "Viewer.h"
#include <vector>
#include <iostream>

#if (ONI_PLATFORM == ONI_PLATFORM_MACOSX)
        #include <GLUT/glut.h>
#else
        #include <GL/glut.h>
#endif

#include "../Common/OniSampleUtilities.h"

#define GL_WIN_SIZE_X	1280
#define GL_WIN_SIZE_Y	1024
#define TEXTURE_SIZE	512

#define DEFAULT_DISPLAY_MODE	DISPLAY_MODE_DEPTH

#define MIN_NUM_CHUNKS(data_size, chunk_size)	((((data_size)-1) / (chunk_size) + 1))
#define MIN_CHUNKS_SIZE(data_size, chunk_size)	(MIN_NUM_CHUNKS(data_size, chunk_size) * (chunk_size))

SampleViewer* SampleViewer::ms_self = NULL;

void SampleViewer::glutIdle()
{
	glutPostRedisplay();
}
void SampleViewer::glutDisplay()
{
	SampleViewer::ms_self->display();
}
void SampleViewer::glutKeyboard(unsigned char key, int x, int y)
{
	SampleViewer::ms_self->onKey(key, x, y);
}

SampleViewer::SampleViewer(const char* strSampleName, const char* deviceUri) :
	m_pClosestPoint(NULL), m_pClosestPointListener(NULL)

{
	ms_self = this;
	strncpy(m_strSampleName, strSampleName, ONI_MAX_STR);

	m_pClosestPoint = new closest_point::ClosestPoint(deviceUri);
}
SampleViewer::~SampleViewer()
{
	finalize();

	delete[] m_pTexMap;

	ms_self = NULL;
}

void SampleViewer::finalize()
{
	if (m_pClosestPoint != NULL)
	{
		m_pClosestPoint->resetListener();
		delete m_pClosestPoint;
		m_pClosestPoint = NULL;
	}
	if (m_pClosestPointListener != NULL)
	{
		delete m_pClosestPointListener;
		m_pClosestPointListener = NULL;
	}
}

openni::Status SampleViewer::init(int argc, char **argv)
{
	m_pTexMap = NULL;

	if (!m_pClosestPoint->isValid())
	{
		return openni::STATUS_ERROR;
	}

	m_pClosestPointListener = new MyMwListener;
	m_pClosestPoint->setListener(*m_pClosestPointListener);

	return initOpenGL(argc, argv);

}
openni::Status SampleViewer::run()	//Does not return
{
	glutMainLoop();

	return openni::STATUS_OK;
}

int SampleViewer::getWidth() {
	openni::VideoFrameRef depthFrame = m_pClosestPointListener->getFrame();
	return depthFrame.getWidth();
}

int SampleViewer::getHeight() {
	openni::VideoFrameRef depthFrame = m_pClosestPointListener->getFrame();
	return depthFrame.getHeight();
}

double SampleViewer::getXResolution(){
	double resolution = 0;
	const openni::VideoFrameRef* depthFrame = &m_pClosestPointListener->getFrame();
	if (depthFrame->isValid()) {
		resolution = static_cast<double>(depthFrame->getVideoMode().getResolutionX());
	}
	return resolution;
}

double SampleViewer::getYResolution(){
	double resolution = 0;
	const openni::VideoFrameRef* depthFrame = &m_pClosestPointListener->getFrame();
	if (depthFrame->isValid()) {
		resolution = static_cast<double>(depthFrame->getVideoMode().getResolutionY());
	}
	return resolution;
}

double SampleViewer::getFOV() {
	double fov = m_pClosestPoint->getFOV();
	return fov;
}

bool SampleViewer::returnDepthFrame (std::vector<uint16_t>& pixels) {
	openni::VideoFrameRef depthFrame;

	while (!depthFrame.isValid()){ 
		depthFrame = m_pClosestPointListener->getFrame();
		sleep(2);
	}


	if (depthFrame.isValid())
	{
		std::cout << "depth frame is valid" << std::endl;
		const uint16_t *pDepthRow = (const uint16_t*)depthFrame.getData();
		int depthFrameSize = depthFrame.getWidth() * depthFrame.getHeight();
		pixels = std::vector<uint16_t>(pDepthRow, pDepthRow + depthFrame.getDataSize()); 
		return true;
	} 
	return false;
}


void SampleViewer::display()
{
	if (!m_pClosestPointListener->isAvailable())
	{
		return;
	}

	openni::VideoFrameRef depthFrame = m_pClosestPointListener->getFrame();
	const closest_point::IntPoint3D& closest = m_pClosestPointListener->getClosestPoint();
	m_pClosestPointListener->setUnavailable();

	if (m_pTexMap == NULL)
	{
		// Texture map init
		m_nTexMapX = MIN_CHUNKS_SIZE(depthFrame.getWidth(), TEXTURE_SIZE);
		m_nTexMapY = MIN_CHUNKS_SIZE(depthFrame.getHeight(), TEXTURE_SIZE);
		m_pTexMap = new openni::RGB888Pixel[m_nTexMapX * m_nTexMapY];
	}



	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, GL_WIN_SIZE_X, GL_WIN_SIZE_Y, 0, -1.0, 1.0);

	if (depthFrame.isValid())
	{
		calculateHistogram(m_pDepthHist, MAX_DEPTH, depthFrame);
	}

	memset(m_pTexMap, 0, m_nTexMapX*m_nTexMapY*sizeof(openni::RGB888Pixel));

	float factor[3] = {1, 1, 1};
	// check if we need to draw depth frame to texture
	if (depthFrame.isValid())
	{
		const openni::DepthPixel* pDepthRow = (const openni::DepthPixel*)depthFrame.getData();
		openni::RGB888Pixel* pTexRow = m_pTexMap + depthFrame.getCropOriginY() * m_nTexMapX;
		int rowSize = depthFrame.getStrideInBytes() / sizeof(openni::DepthPixel);
		int width = depthFrame.getWidth();
		int height = depthFrame.getHeight();

		for (int y = 0; y < height; ++y)
		{
			const openni::DepthPixel* pDepth = pDepthRow;
			openni::RGB888Pixel* pTex = pTexRow + depthFrame.getCropOriginX();

			for (int x = 0; x < width; ++x, ++pDepth, ++pTex)
			{
				if (*pDepth != 0)
				{
					if (*pDepth == closest.Z)
					{
						factor[0] = factor[1] = 0;
					}
//					// Add debug lines - every 10cm
// 					else if ((*pDepth / 10) % 10 == 0)
// 					{
// 						factor[0] = factor[2] = 0;
// 					}

					int nHistValue = m_pDepthHist[*pDepth];
					pTex->r = nHistValue*factor[0];
					pTex->g = nHistValue*factor[1];
					pTex->b = nHistValue*factor[2];

					factor[0] = factor[1] = factor[2] = 1;
				}
			}

			pDepthRow += rowSize;
			pTexRow += m_nTexMapX;
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_nTexMapX, m_nTexMapY, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pTexMap);

	// Display the OpenGL texture map
	glColor4f(1,1,1,1);

	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);

	int nXRes = depthFrame.getWidth();
	int nYRes = depthFrame.getHeight();

	// upper left
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	// upper right
	glTexCoord2f((float)nXRes/(float)m_nTexMapX, 0);
	glVertex2f(GL_WIN_SIZE_X, 0);
	// bottom right
	glTexCoord2f((float)nXRes/(float)m_nTexMapX, (float)nYRes/(float)m_nTexMapY);
	glVertex2f(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	// bottom left
	glTexCoord2f(0, (float)nYRes/(float)m_nTexMapY);
	glVertex2f(0, GL_WIN_SIZE_Y);

	glEnd();
	glDisable(GL_TEXTURE_2D);

	float closestCoordinates[3] = {closest.X*GL_WIN_SIZE_X/float(depthFrame.getWidth()), closest.Y*GL_WIN_SIZE_Y/float(depthFrame.getHeight()), 0};

	glVertexPointer(3, GL_FLOAT, 0, closestCoordinates);
	glColor3f(1.f, 0.f, 0.f);
	glPointSize(10);
	glDrawArrays(GL_POINTS, 0, 1);
	glFlush();

	// Swap the OpenGL display buffers
	glutSwapBuffers();

}

void SampleViewer::onKey(unsigned char key, int /*x*/, int /*y*/)
{
	switch (key)
	{
	case 27:
		finalize();
		exit (1);
	}

}

openni::Status SampleViewer::initOpenGL(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	glutCreateWindow (m_strSampleName);
	// 	glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	initOpenGLHooks();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	return openni::STATUS_OK;

}
void SampleViewer::initOpenGLHooks()
{
	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);
}



using namespace openni;

namespace closest_point
{

class StreamListener;

struct ClosestPointInternal
{
	ClosestPointInternal(ClosestPoint* pClosestPoint) :
		m_pDevice(NULL), m_pDepthStream(NULL), m_pListener(NULL), m_pStreamListener(NULL), m_pClosesPoint(pClosestPoint)
		{}

	void Raise()
	{
		if (m_pListener != NULL)
			m_pListener->readyForNextData(m_pClosesPoint);
	}
	bool m_oniOwner;
	Device* m_pDevice;
	VideoStream* m_pDepthStream;

	ClosestPoint::Listener* m_pListener;

	StreamListener* m_pStreamListener;

	ClosestPoint* m_pClosesPoint;
};

class StreamListener : public VideoStream::NewFrameListener
{
public:
	StreamListener(ClosestPointInternal* pClosestPoint) : m_pClosestPoint(pClosestPoint)
	{}
	virtual void onNewFrame(VideoStream& stream)
	{
		m_pClosestPoint->Raise();
	}
private:
	ClosestPointInternal* m_pClosestPoint;
};

ClosestPoint::ClosestPoint(const char* uri)
{
	m_pInternal = new ClosestPointInternal(this);

	m_pInternal->m_pDevice = new Device;
	m_pInternal->m_oniOwner = true;

	OpenNI::initialize();
	Status rc = m_pInternal->m_pDevice->open(uri);
	if (rc != STATUS_OK)
	{
		printf("Open device failed:\n%s\n", OpenNI::getExtendedError());
		return;
	}
	initialize();
}

ClosestPoint::ClosestPoint(openni::Device* pDevice)
{
	m_pInternal = new ClosestPointInternal(this);

	m_pInternal->m_pDevice = pDevice;
	m_pInternal->m_oniOwner = false;

	OpenNI::initialize();

	if (pDevice != NULL)
	{
		initialize();
	}
}

void ClosestPoint::initialize()
{
	m_pInternal->m_pStreamListener = NULL;
	m_pInternal->m_pListener = NULL;

	m_pInternal->m_pDepthStream = new VideoStream;
	Status rc = m_pInternal->m_pDepthStream->create(*m_pInternal->m_pDevice, SENSOR_DEPTH);
	if (rc != STATUS_OK)
	{
		printf("Created failed\n%s\n", OpenNI::getExtendedError());
		return;
	}

	m_pInternal->m_pStreamListener = new StreamListener(m_pInternal);

	rc = m_pInternal->m_pDepthStream->start();
	if (rc != STATUS_OK)
	{
		printf("Start failed:\n%s\n", OpenNI::getExtendedError());
	}

	m_pInternal->m_pDepthStream->addNewFrameListener(m_pInternal->m_pStreamListener);
}

float ClosestPoint::getFOV(){
	float fov = 0.0;
	openni::VideoStream* stream = m_pInternal->m_pDepthStream;
	fov = stream->getVerticalFieldOfView();
	return fov;
}

ClosestPoint::~ClosestPoint()
{
	if (m_pInternal->m_pDepthStream != NULL)
	{
		m_pInternal->m_pDepthStream->removeNewFrameListener(m_pInternal->m_pStreamListener);

		m_pInternal->m_pDepthStream->stop();
		m_pInternal->m_pDepthStream->destroy();

		delete m_pInternal->m_pDepthStream;
	}

	if (m_pInternal->m_pStreamListener != NULL)
	{
		delete m_pInternal->m_pStreamListener;
	}

	if (m_pInternal->m_oniOwner)
	{
		if (m_pInternal->m_pDevice != NULL)
		{
			m_pInternal->m_pDevice->close();

			delete m_pInternal->m_pDevice;
		}
	}

	OpenNI::shutdown();


	delete m_pInternal;
}

bool ClosestPoint::isValid() const
{
	if (m_pInternal == NULL)
		return false;
	if (m_pInternal->m_pDevice == NULL)
		return false;
	if (m_pInternal->m_pDepthStream == NULL)
		return false;
	if (!m_pInternal->m_pDepthStream->isValid())
		return false;

	return true;
}

Status ClosestPoint::setListener(Listener& listener)
{
	m_pInternal->m_pListener = &listener;
	return STATUS_OK;
}
void ClosestPoint::resetListener()
{
	m_pInternal->m_pListener = NULL;
}

Status ClosestPoint::getNextData(IntPoint3D& closestPoint, VideoFrameRef& rawFrame)
{
	Status rc = m_pInternal->m_pDepthStream->readFrame(&rawFrame);
	if (rc != STATUS_OK)
	{
		printf("readFrame failed\n%s\n", OpenNI::getExtendedError());
	}

	DepthPixel* pDepth = (DepthPixel*)rawFrame.getData();
	bool found = false;
	closestPoint.Z = 0xffff;
	int width = rawFrame.getWidth();
	int height = rawFrame.getHeight();

	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x, ++pDepth)
		{
			if (*pDepth < closestPoint.Z && *pDepth != 0)
			{
				closestPoint.X = x;
				closestPoint.Y = y;
				closestPoint.Z = *pDepth;
				found = true;
			}
		}

	if (!found)
	{
		return STATUS_ERROR;
	}

	return STATUS_OK;
}

}