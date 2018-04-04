#pragma once
/*Copyright 2018 Jesus D. Romero

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files(the "Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to
the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
#include <vector>
#include <string>
#include <Shlwapi.h>
class VideoSource {
public:
	virtual cv::Mat GetNextImage(cv::Mat& groundtruth) {
		return cv::Mat();
	}
};
class VideoSourceDirectory :VideoSource {
	std::string directory;

	BOOL isEmpty;
	std::string pathfilenames;
	std::string pathgroundtruth;
	std::vector<std::string> filenames;
	std::vector<std::string> groundtruth;
	std::string roi;
	cv::Mat matroi;
	int position;
	int positiongt;

	void LoadFiles(std::vector<std::string> &filenames, std::string mask) {

		HANDLE handle;
		WIN32_FIND_DATAA data;
		ZeroMemory(&data, sizeof(data));
		handle = ::FindFirstFileA(mask.c_str(), &data);
		if (handle != INVALID_HANDLE_VALUE) {
			do {
				filenames.push_back(data.cFileName);
			} while (::FindNextFileA(handle, &data));
			::FindClose(handle);
			std::sort(filenames.begin(), filenames.end());
		}
	}
	int GetIndex(std::string &str) {
		bool ext = false;
		int index = 1;
		int value = -1;
	restart:
		for (int i = str.length() - 1;i >= 0; i--) {
			if (ext) {
				if (std::isdigit(str.at(i))) {
					if (value == -1) value = 0;
					value += ((int)(str.at(i) - '0'))*index;
					index *= 10;
				}
				else if (str.at(i) == '\\' || str.at(i) == '/'||str.at(i)=='_'||std::isalpha(str.at(i))) {
					break;
				}

			}
			else {
				if (str.at(i) == '.') {
					ext = true;
				}
				else if (str.at(i) == '\\' || str.at(i) == '/') {
					ext = true;
					goto restart;
				}
			}
		}
		return value;
	}
	int deltagt;
public:
	VideoSourceDirectory(std::string backgroundmask, std::string groundtruthmask, std::string roi="", int deltagt=0) {
		this->roi = roi;
		position = 0;
		positiongt = 0;
		LoadFiles(filenames, backgroundmask);
		LoadFiles(groundtruth, groundtruthmask);
		char buf[100000];
		strcpy_s(buf, backgroundmask.c_str());
		::PathRemoveFileSpecA(buf);
		pathfilenames = buf;
		strcpy_s(buf, groundtruthmask.c_str());
		::PathRemoveFileSpecA(buf);
		pathgroundtruth = buf;
		this->deltagt = deltagt;
	}
	bool GroundTruthFinished() {
		return positiongt == groundtruth.size();
	}

	cv::Mat GetROI() {
		if (roi.size() != 0) {
			if (matroi.rows==0)
				matroi=Leer(roi);
			return matroi;
		}
		else {
			return cv::Mat();
		}
	}
	cv::Mat Leer(std::string camino) {
		HANDLE h = CreateFileA(camino.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (h == INVALID_HANDLE_VALUE) {
			return cv::imread(camino);
		}
		DWORD tam = GetFileSize(h, NULL);
		if (tam == INVALID_FILE_SIZE) {
			cv::Mat res=cv::imread(camino);
			CloseHandle(h);
			return res;
		}
		unsigned char *dat = new unsigned char[tam];
		DWORD leidos = 0;
		if (ReadFile(h, dat, tam, &leidos, NULL)&&leidos==tam) {
			cv::_InputArray entrada(dat, tam);			
			cv::Mat res=cv::imdecode(entrada, CV_LOAD_IMAGE_COLOR);
			CloseHandle(h);
			delete[] dat;
			return res;
		}
		else {
			delete[] dat;
			cv::Mat res = cv::imread(camino);
			CloseHandle(h);			
			return res;
		}
	}
	virtual cv::Mat GetNextImage(cv::Mat &groundtruth,int * fileindex, int *groundtruthindex, int *gtindexarchivo) {
		if (position == filenames.size()) {
			groundtruth = cv::Mat();
			return cv::Mat();
		}
		std::string fname = filenames[position++];
		int findex = GetIndex(fname);
		if (fileindex != NULL)
			*fileindex = findex;
		if (groundtruthindex != NULL)
			*groundtruthindex = -1;
		if (gtindexarchivo != NULL)
			*gtindexarchivo = -1;
	reintentar:
		if (positiongt < this->groundtruth.size()) {
			int gtindex = 0;
			std::string fmask;			
			fmask = this->groundtruth[positiongt];
			int indicearchivogt = GetIndex(fmask);
			gtindex = indicearchivogt + deltagt;
			if (gtindex < findex) {
				positiongt++;
				goto reintentar;
			}
			if (findex == gtindex&&gtindex != -1 && findex != -1) {
				if (groundtruthindex != NULL)
					*groundtruthindex = gtindex;
				if (gtindexarchivo != NULL)
					*gtindexarchivo = indicearchivogt;
				groundtruth = Leer(pathgroundtruth + "\\" + fmask);//cv::imread(pathgroundtruth + "\\" + fmask);
				/*unsigned char *ptr=groundtruth.ptr<unsigned char>(0);
				if (ptr[0]!=0&&ptr[1]!=0&&ptr[2]!=0&&ptr[0]!=255&&ptr[1]!=255&&ptr[2]!=255) {
					groundtruth = cv::Mat();
				} else {
					std::vector < cv::Mat> vec;
					cv::split(groundtruth, vec);
					groundtruth = vec[0]>0|vec[1]>0|vec[2]>0;
				}*/
				positiongt++;
			}
			else
				groundtruth = cv::Mat();
		}
		else
			groundtruth = cv::Mat();
		return Leer(pathfilenames + "\\" + fname);//cv::imread(pathfilenames + "\\" + fname);
	}
};