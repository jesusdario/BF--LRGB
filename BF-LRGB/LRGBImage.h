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
namespace DR {
namespace SoporteBase {
namespace Imagenes {
class LRGBImage {
	cv::Mat lightness;
	std::vector<cv::Mat> channels;
	LRGBImage() {}
public:
	cv::Mat GetColorChannels() {
		cv::Mat res;
		cv::merge(channels,res);
		return res;
	}
	void ShiftLightness(float shift) {
		lightness += shift;
	}
	void ShiftColorChannel(int index,float shift) {
		channels[index] += shift;
	}
	void SetLightness(cv::Mat mat) {
		lightness=mat.clone();
	}
	void SetColorChannels(cv::Mat mat) {
		channels.clear();
		std::vector<cv::Mat>& c2=channels;
		c2.push_back(cv::Mat());
		c2.push_back(cv::Mat());
		c2.push_back(cv::Mat());
		cv::split(mat,c2);
	}
	cv::Mat GetColorChannel(int position) {
		return channels[position];
	}
	cv::Mat GetLightness() {
		return lightness;
	}
	LRGBImage Clone() {
		LRGBImage d;
		d.lightness=lightness.clone();
		for (int i=0;i<channels.size();i++) {
			d.channels.push_back(channels[i].clone());
		}
		return d;
	}
	void ScaleColor(float factor) {		
		for (int i=0;i<channels.size();i++) {
			channels[i]*=factor;
		}
	}
	void ScaleColor(int index,float factor,float shift=0, cv::MatExpr mask = cv::MatExpr()) {
		if (mask.size().height!= 0) {
			channels[index]  = (channels[index].mul(mask) + shift)*factor + channels[index].mul(~mask);			 
		}
		else {
			channels[index] = (channels[index] + shift)*factor;
		}
	}
	void GetColorStatistics(int index, double *avg = NULL, double *stddev = NULL, double *n = NULL) {
		cv::Scalar s=cv::sum(channels[index]);
		double length = channels[index].rows*channels[index].cols;
		double vavg = s[0] / length;
		cv::MatExpr exp1 = (channels[index] - vavg);
		cv::Scalar sqr=cv::sum(exp1.mul(exp1));
		double vstddev = std::sqrt(sqr[0]) / length;
		if (avg != NULL)
			*avg = vavg;
		if (stddev != NULL)
			*stddev = vstddev;
		if (n != NULL)
			*n = length;
	}
	void GetColorStatisticsPN(int index, double *avgp=NULL, double *avgn = NULL, double *stddevp = NULL, double *stddevn = NULL,double *np = NULL,double *nn = NULL) {
		double sump, sumn;
		double nump, numn;
		GetColorSum(index, &sump, &sumn, &nump, &numn);
		double meanp = nump==0?0:sump / nump;
		double meann = numn==0?0:sumn / numn;
		if (stddevp != NULL || stddevn != NULL) {
			float *ptr = (float *)(channels[index].data);
			const int pixels = channels[index].rows*channels[index].cols;
			double sp = 0;
			double sn = 0;
			for (int i = 0; i < pixels; i++) {
				float val = *ptr;
				if (val >= 0) {
					double d = (val - meanp);
					d = d*d;
					sp += d;
				}
				else {
					double d = (val - meann);
					d = d*d;
					sn += d;
				}
				*ptr++;
			}
			if (stddevp != NULL)
				*stddevp = cv::sqrt(sp) / nump;
			if (stddevn != NULL)
				*stddevn = cv::sqrt(sn) / numn;
		}		
		if (avgp != NULL)
			*avgp = meanp;
		if (avgn != NULL)
			*avgn = meann;
		if (np != NULL)
			*np = nump;
		if (nn != NULL)
			*nn = numn;
	}

	void GetColorSum(int index, double *sump = NULL, double *sumn = NULL, double *nump = NULL, double *numn = NULL) {
		float *ptr = (float *)(channels[index].data);
		const int pixels = channels[index].rows*channels[index].cols;
		int np = 0;
		int nn = 0;
		double sp = 0;
		double sn = 0;
		for (int i = 0; i < pixels; i++) {
			float val = *ptr;
			if (val >= 0) {
				sp += val;
				np++;
			}
			else {
				sn += val;
				nn++;
			}
			*ptr++;
		}
		if (sump != NULL)
			*sump = sp;
		if (sumn != NULL)
			*sumn = sn;
		if (nump != NULL)
			*nump = np;
		if (numn != NULL)
			*numn = nn;
	}
	void ScaleColorPN(int index, float factorp, float factorn) {
		float *ptr = (float *)(channels[index].data);
		const int pixels = channels[index].rows*channels[index].cols;
		for (int i = 0; i < pixels; i++) {
			*ptr = (*ptr) >= 0 ? (*ptr)*factorp : (*ptr)*factorn;
			ptr++;
		}
	}
	void ScaleLightness(float factor,float center=0) {
		if (center==0) {
			lightness=lightness*factor;
		} else {
			lightness=(lightness-center)*factor+center;		
		}
	}
	void Init(cv::Mat image, int datatype= CV_32F) {
		std::vector<cv::Mat>& c2 = channels;
		c2.clear();
		c2.push_back(cv::Mat());
		c2.push_back(cv::Mat());
		c2.push_back(cv::Mat());
		cv::split(image, c2);
		c2[0].convertTo(c2[0], datatype/*CV_32F*/);
		c2[1].convertTo(c2[1], datatype/*CV_32F*/);
		c2[2].convertTo(c2[2], datatype/*CV_32F*/);
		cv::Mat mt2;
		image.convertTo(mt2, datatype);
		cv::cvtColor(mt2, lightness, CV_BGR2GRAY);
		//mt2.convertTo(lightness,datatype/*CV_32F*/);
		c2[0] = c2[0] - lightness;
		c2[1] = c2[1] - lightness;
		c2[2] = c2[2] - lightness;
	}
	LRGBImage(cv::Mat image,int datatype=CV_32F) {
		Init(image, datatype);
		
	}
	cv::Mat GetImage(int datatype=CV_8UC3,double scale=1.0,double shift=0.0) {
		cv::Mat result(lightness.rows,lightness.cols,datatype);
		std::vector<cv::Mat> c2;
		c2.push_back(lightness+channels[0]);
		c2.push_back(lightness+channels[1]);
		c2.push_back(lightness+channels[2]);
		cv::merge(c2,result);
		result.convertTo(result,datatype,scale,shift);
		return result;
	}
	cv::Mat GetNeutral(int datatype = CV_8UC3, double scale = 1.0, double shift = 0.0) {
		cv::Mat result(lightness.rows, lightness.cols, datatype);
		std::vector<cv::Mat> c2;
		cv::Mat m128 = cv::Mat::ones(lightness.rows,lightness.cols,CV_32FC1) * 128;
		c2.push_back(m128 + channels[0]);
		c2.push_back(m128 + channels[1]);
		c2.push_back(m128 + channels[2]);
		cv::merge(c2, result);
		result.convertTo(result, datatype, scale, shift);
		return result;
	}
};
}
}
}