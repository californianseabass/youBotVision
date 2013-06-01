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
#include <iostream>
#include <vector>


int main(int argc, char** argv)
{
	openni::Status rc = openni::STATUS_OK;

	const char* deviceURI = openni::ANY_DEVICE;
	if (argc > 1)
	{
		deviceURI = argv[1];
	}

	SampleViewer sampleViewer("ClosestPoint Viewer", deviceURI);

	openni::VideoFrameRef *depthFrame;


	rc = sampleViewer.init(argc, argv);
	if (rc != openni::STATUS_OK)
	{
		return 1;
	}

	// while (true) {
	// 	// openni depth pixels are type depthed to be uint16_t's
	// 	std::vector<uint16_t> depthFrame;
	// 	if (!sampleViewer.returnDepthFrame(depthFrame)) return -1;
	// 	std::cout << "depth frame size:" << depthFrame.size() << std::endl;
	// 	for (size_t i = 0; i < depthFrame.size(); ++i)
	// 	{
	// 		std::cout << depthFrame[i] << std::endl;
	// 	}


	// 	return 0;
	// }
	sampleViewer.run();
}