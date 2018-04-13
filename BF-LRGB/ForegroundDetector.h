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
#include <intrin.h>
extern double chiIntercoefficientDistance;
extern double zetaValue;
extern double tauSqrt3Value;
extern float limdetforeground;
extern bool deteccionsombras;
extern bool withoperationclose;
extern bool removeIlluminationArtifacts;
namespace DR {
namespace SoporteBase {
namespace Imagenes {
	void MedianBlur5(cv::Mat mat,cv::Mat& rest,unsigned char val);
	void RemoveIsolatedPoints2(cv::Mat mat, cv::Mat& rest);
	cv::Mat GetMorphologyMask(int width);
	
class ForegroundDetector {
	cv::Mat foreground;
	cv::Mat shadows;
public:
	virtual cv::Mat GetForeground() {
		return foreground;
	}
	virtual cv::Mat GetShadows() {
		return shadows;
	}
	inline bool coefficientDistance(const double dist1, const double dist2, const double dist3, const double vintercoef) {
		if (dist1 < vintercoef) {
			if (dist2 < vintercoef) {
				return true;
			}
			else if (dist3 < vintercoef)
				return true;			
		}
		else {
			if (dist2 < vintercoef)
			{
				if (dist3 < vintercoef)
					return true;
			}			
		}
		return false;			
	}
	virtual cv::Mat FindLightness(cv::Mat background) {
		cv::Mat Lightness(background.rows, background.cols, CV_32FC3);
		const unsigned int valblue = (unsigned int)(0.114f * 256 * 256 * 256); //1912603;//0.114f*256*256*256;
		const unsigned int valgreen = (unsigned int)(0.587f * 256 * 256 * 256); ;// 9848226;//0.587f*256*256*256;
		const unsigned int valred = (unsigned int)(0.299f * 256 * 256 * 256); //5016388;//0.299f*256*256*256;
		for (int i = 0;i<background.rows;i++) {
			unsigned char *ptrBackground = background.ptr<unsigned char>(i);
			float *ptrLightness = Lightness.ptr<float>(i);
			for (int o = 0;o<background.cols;o++) {
				unsigned int color = (((unsigned int)ptrBackground[0])*valblue +
					((unsigned int)ptrBackground[1])*valgreen +
					((unsigned int)ptrBackground[2])*valred) >> 24;
				ptrLightness[2] = ptrLightness[1] = ptrLightness[0] = color;
				ptrBackground += 3;
				ptrLightness += 3;
			}
		}
		return Lightness;
	}
	cv::Mat morphologymask;
	virtual void FindForeground(cv::Mat currentImage,BackgroundGenerator *generador,bool addImages,ListaImagenes &imageList) {
		if (!generador->WithData()) return;
		bool RemoveShadows= removeIlluminationArtifacts;
					
		cv::Mat source=currentImage.clone();
		cv::Mat background=generador->GetCurrentBackground().clone();
		
		cv::Mat k2(source.rows,source.cols,CV_32FC3);
		const int limitptr=source.cols*3;
		const int quotientptr=limitptr/4;
		const int remainderptr=limitptr%4;		
		for (int i=0;i<source.rows;i++) {
			unsigned char *ptrSource=source.ptr<unsigned char>(i);
			unsigned char *ptrBackground=background.ptr<unsigned char>(i);
			float *ptrk2=k2.ptr<float>(i);
			
			for (int o=0;o<limitptr;o++) {
				ptrk2[0]=ptrSource[0]-(float)ptrBackground[0];
				ptrk2++;
				ptrBackground++;
				ptrSource++;
			}
		}
		cv::Mat k1=FindLightness(background);//2.3ms 800x600
		cv::Mat constant(background.rows,background.cols,CV_32FC3);
		constant = k2 / k1;
		cv::Mat changes(constant.rows,constant.cols,CV_8UC1);
		const float i1=(float)((1-limdetforeground)-1);
		const float i2=(float)((1+limdetforeground)-1);
		
		for (int i=0;i<constant.rows;i++) {
			float *ptrConstant=constant.ptr<float>(i);
			unsigned char *ptrChanges=changes.ptr<unsigned char>(i);
			for (int o=0;o<constant.cols;o++) {
				*ptrChanges=!((ptrConstant[0]>=i1&&ptrConstant[0]<=i2)&&
					(ptrConstant[1]>=i1&&ptrConstant[1]<=i2)&&
					(ptrConstant[2]>=i1&&ptrConstant[2]<=i2))
					;
				ptrConstant+=3;
				ptrChanges++;
			}
		}
		
		cv::Mat illuminationArtifacts(constant.rows,constant.cols,CV_8UC1);
		const int numberOfColumns=constant.cols;
		//const float zeta=(float)(0.8f);//This value is used to avoid detecting hair as a shadow
		const float LengthThreshold=zetaValue*zetaValue;//thresh*thresh;
		const float interCoefficientDistance=chiIntercoefficientDistance;
		const float tausqrt3_1=tauSqrt3Value;
		const float tausqrt3_2=-tauSqrt3Value;
		const int isIlluminationArtifact = 1;
		const int isForeground = 0;

		for (int i=0;i<constant.rows;i++) {
			float *ptrConstant=constant.ptr<float>(i);
			unsigned char *ptrChanges=changes.ptr<unsigned char>(i);
			unsigned char *ptrResult=illuminationArtifacts.ptr<unsigned char>(i);
			unsigned char *ptrSource = source.ptr<unsigned char>(i);
			unsigned char *ptrBackground = background.ptr<unsigned char>(i);
				
			for (int o=0;o<numberOfColumns;o++) {	
				float length=ptrConstant[0]*ptrConstant[0]+ptrConstant[1]*ptrConstant[1]+ptrConstant[2]*ptrConstant[2];
				float projection=ptrConstant[0]+ptrConstant[1]+ptrConstant[2];
				if ((*ptrChanges)) {
					if (length < LengthThreshold &&
						coefficientDistance(std::abs(ptrConstant[1] - ptrConstant[0]), std::abs(ptrConstant[2] - ptrConstant[0]), std::abs(ptrConstant[2] - ptrConstant[1]), interCoefficientDistance)
						&&
						(projection >= tausqrt3_2 && projection <= tausqrt3_1)
						) {
						*ptrResult = isIlluminationArtifact; //Artefacto de iluminaci¨®n o sombra
					}
					else {							
						*ptrResult = isForeground;						
					}
				}
				else {
					*ptrResult = isForeground;
				}
					
				ptrConstant += 3;
				ptrResult++;
				ptrChanges++;
				ptrSource += 3;
				ptrBackground += 3;
			}
		}
		
		
		if (RemoveShadows) {
			changes-= illuminationArtifacts;
		}
		this->shadows = illuminationArtifacts;
		if (addImages) {
			imageList.Add32F("Foreground_ScalingCoefficients [ki]",constant,-1,1);
			imageList.Add32F("Foreground_Shadow/Illumination Artifacts",illuminationArtifacts,-1,1);			
		}
		cv::Mat result;
		MedianBlur5(changes,changes,1);		//1.5ms (800x600)
		RemoveIsolatedPoints2(changes, changes);//2.5ms (800x600)
		
		if (withoperationclose) {
			if (morphologymask.rows == 0)
				morphologymask = GetMorphologyMask( 10 * source.rows / 600.0);
			cv::morphologyEx(changes,result,cv::MORPH_CLOSE,morphologymask,cv::Point(morphologymask.rows / 2, morphologymask.cols/2));//2.8 ms		
		}
		else {
			result = changes;
		}
		
		foreground=result;				
		if (addImages) {
			imageList.Add("Foreground_Foreground",foreground*255);
			std::vector<cv::Mat> mergedResult;
			mergedResult.push_back(foreground);
			mergedResult.push_back(foreground);
			mergedResult.push_back(foreground);
			cv::Mat mergedMask;
			cv::merge(mergedResult, mergedMask);
			cv::Mat segmentedImage;
			cv::multiply(mergedMask, currentImage,segmentedImage);
			imageList.Add("Foreground_Segmented Foreground", segmentedImage);
		}

	}
	
};
}
}
}