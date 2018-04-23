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
#include "LRGBImage.h"
extern bool consuavizado;
extern bool connormalizacionLRGB;
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {
			std::string type2str(int type);
			class Preprocessor {
				double shiftlightness;
				//double colorscalevaluep[3];
				//double colorscalevaluen[3];
				bool initialized;
				//bool correctcolorscales;
				cv::Mat previousimage;
				bool colorblur;
			public:
				Preprocessor() {
					initialized = false;
					colorblur = true;
					//correctcolorscales = false;//true       
					//colorscalevaluep[0] = colorscalevaluep[1] = colorscalevaluep[2] = 1;
					//colorscalevaluen[0] = colorscalevaluen[1] = colorscalevaluen[2] = 1;
					shiftlightness = 0;
				}
				cv::Mat Preprocesar(cv::Mat img, bool *salir) {
					cv::Mat input = img;
					cv::Mat output = img.clone();
					cv::Mat image = output;
					if (consuavizado) {
						cv::GaussianBlur(input, image, cv::Size(3, 3), 1, 1);
					}
					else {
						input.copyTo(image);
					}
					if (salir != NULL) {
						*salir = false;
					}
					if (connormalizacionLRGB) {
						if (!initialized) {
							LRGBImage img4d(image);
							cv::Mat L = img4d.GetLightness();
							cv::Scalar s = cv::sum(L);
							s = s / (image.rows*image.cols);
							shiftlightness = 0;
							if (s[0] > 150) {
								shiftlightness = -(s[0] - 150);
							}
							if (s[0] < 100) {
								shiftlightness = (100 - s[0]);
							}

							if (colorblur) {
								for (int i = 0; i < 3; i++) {
									cv::Mat m = img4d.GetColorChannel(i);
									cv::GaussianBlur(m, m, cv::Size(5, 5), 2, 2);
								}
							}
							/*
							std::vector<float> promediocomponentespos;
							std::vector<float> promediocomponentesneg;
							for (int i = 0; i < 3; i++) {
								cv::Mat m = img4d.GetColorChannel(i);
								double promp,promn;
								img4d.GetColorStatistics(i, &promp, &promn);
								promediocomponentespos.push_back(promp);
								promediocomponentesneg.push_back(promn);
								
							}
							float promediototalpos = (std::abs(promediocomponentespos[0]) + std::abs(promediocomponentespos[1]) + std::abs(promediocomponentespos[2])) / 3;
							float promediototalneg= (std::abs(promediocomponentesneg[0]) + std::abs(promediocomponentesneg[1]) + std::abs(promediocomponentesneg[2])) / 3;
							std::cout << "AveragePositive:" << promediototalpos << ";AverageNegative:" << promediototalneg << std::endl;
							
							promediototalpos *= 1.5;
							promediototalneg *= 1.5;
							for (int i = 0; i < 3; i++) {
								colorscalevaluep[i] = promediototalpos / std::abs(promediocomponentespos[i]);
								colorscalevaluen[i] = promediototalneg / std::abs(promediocomponentesneg[i]);
								if (colorscalevaluep[i] > 5)
									colorscalevaluep[i] = 5;
								if (colorscalevaluep[i] < 0.3)
									colorscalevaluep[i] = 0.3;
								if (colorscalevaluen[i] > 5)
									colorscalevaluen[i] = 5;
								if (colorscalevaluen[i] < 0.3)
									colorscalevaluen[i] = 0.3;
							}
							*/
							initialized = true;							
							std::cout << "Corrections in the image: L:" << shiftlightness << std::endl;// << ";C1:" << colorscalevaluep[0] << ";C2:" << colorscalevaluep[1] << ";C2:" << colorscalevaluep[2] << ";N1:" << colorscalevaluen[0] << ";N2:" << colorscalevaluen[1] << ";N3:" << colorscalevaluen[2] << std::endl;
						}
						//the algorithm was initially formulated with color scales for positive and negative values. However this was removed in the final implementation (correctcolorscales=false)
						if (initialized&&(colorblur||shiftlightness != 0 /*|| colorscalevaluep[0] != 1 || colorscalevaluep[1] != 1 || colorscalevaluep[2] != 1)*/)) {
							LRGBImage imglrgb(image);
							if (shiftlightness!=0)
								imglrgb.ShiftLightness(shiftlightness);
							if (colorblur) {
								for (int i = 0; i < 3; i++) {
									cv::Mat m = imglrgb.GetColorChannel(i);
									cv::GaussianBlur(m, m, cv::Size(5, 5), 2, 2);
								}
							}
							/*
							if (correctcolorscales&&(colorscalevaluep[0]!=1||colorscalevaluep[1]!=1||colorscalevaluep[2]!=1)) {
								for (int i = 0; i < 3; i++) {
									imglrgb.ScaleColorPN(i, colorscalevaluep[i], colorscalevaluen[i]);
								}
							}*/
							cv::Mat img2 = imglrgb.GetImage();
							cv::imshow("Before", image);
							img2.copyTo(image);				
							cv::imshow("Current Image", image);
						}
					}
					int lim = image.cols * 3;

					for (int i = 0; i < image.rows; i++) {
						unsigned char *rowptr = image.ptr<unsigned char>(i);
						for (int o = 0; o < lim; o++) {
							if (*rowptr < 8) *rowptr = 8; 
							rowptr++;
						}
					}
					

					return output;
				}
			};
		}
	}
}
