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
#define SININCREMENTODENOM 1
#include "opencv2\opencv.hpp"
#include "opencv2\opencv_modules.hpp"
#include "ListaImagenes.h"
#include "Timer.h"
#include <stack>
#include <intrin.h>
extern double thetaThresholdProbabilityTrue;
extern double etaThresholdProbabilityFalse;
extern int xiThresholdNumberOfTrueEvents;
extern int rhoThresholdNumberOfFalseEvents;
extern double gammaValue;
extern double psivalue;
extern double betavalue;
extern float deltabackground;
extern bool detectSuddenChangesInBackground;
extern double thresholdSuddenChanges;
extern int numberOfAveragedFrames;
//cv::Mat QuitarPuntosAislados(cv::Mat mat);
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {
			class BackgroundGenerator {
				
				bool withData;				
			public:
				cv::Mat background;
				cv::Mat testingBackground;
				cv::Mat verifiedBackground;
				int framenumber;
				BackgroundGenerator() {
					withData = false;
					framenumber = 0;
				}
				virtual bool WithData() {
					return withData;
				}
				cv::Mat isVerified;
				cv::Mat eventCounter;
				cv::Mat totalEventCounter;
				std::vector<cv::Mat> previousImages;
				cv::Mat testingBackgroundChanged;
				virtual cv::Mat GetCurrentBackground() {
					return background;
				}
				inline bool twoConditions(bool cond1, bool cond2, bool cond3) {
					if (cond1) {
						if (cond2) {
							return true;
						}
						else if (cond3)
							return true;
					}
					else {
						if (cond2) {
							if (cond3)
								return true;
						}
					}
					return false;
				}
				virtual cv::Mat FindDifferences(cv::Mat background, cv::Mat source, bool addImages, ListaImagenes &imageList,float deltaBackground) {
					cv::Mat brightness(source.rows, source.cols, CV_32FC3);

					const unsigned int valblue = 0.114f * 256 * 256 * 256;
					const unsigned int valgreen = 0.587f * 256 * 256 * 256;
					const unsigned int valred = 0.299f * 256 * 256 * 256;
					for (int i = 0; i<source.rows; i++) {
						unsigned char *ptrfondo = background.ptr<unsigned char>(i);
						float *ptrbrightness = brightness.ptr<float>(i);
						for (int o = 0; o<source.cols; o++) {
							unsigned int color = (((unsigned int)ptrfondo[0])*valblue +
								((unsigned int)ptrfondo[1])*valgreen +
								((unsigned int)ptrfondo[2])*valred) >> (8 * 3);
							ptrbrightness[0] = color;
							ptrbrightness[1] = color;
							ptrbrightness[2] = color;
							ptrfondo += 3;
							ptrbrightness += 3;
						}
					}

					cv::Mat k1 = brightness;
					//cv::Mat k2=foregroundf-fondof;//<1.6 ms
					cv::Mat k2(source.rows, source.cols, CV_32FC3);
					int limptr = source.cols * 3;
					for (int i = 0; i<source.rows; i++) {
						unsigned char *ptrorigen = source.ptr<unsigned char>(i);
						unsigned char *ptrfondo = background.ptr<unsigned char>(i);
						float *ptrk2 = k2.ptr<float>(i);
						float *ptrfondof = k2.ptr<float>(i);
						for (int o = 0; o<limptr; o++) {
							ptrk2[0] = ptrorigen[0] - (float)ptrfondo[0];
							ptrk2++;
							ptrfondo++;
							ptrorigen++;
						}
					}

					//cv::Mat constante=k2/(k1+0.1f);//<4 milisegundos
					cv::Mat constant(background.rows, background.cols, CV_32FC3);
					for (int i = 0; i<constant.rows; i++) {
						float *ptrk2 = k2.ptr<float>(i);
						float *ptrk1 = k1.ptr<float>(i);
						float *ptrconstant = constant.ptr<float>(i);

						int delta = limptr / 4;
						int resto = limptr % 4;

						for (int o = 0; o < delta; o++) {
							_mm_storeu_ps(ptrconstant, _mm_div_ps(_mm_loadu_ps(ptrk2), _mm_loadu_ps(ptrk1)));
							ptrk1 += 4;
							ptrk2 += 4;
							ptrconstant += 4;
						}

						for (int o = delta * 4; o<limptr; o++) {
							*ptrconstant = *ptrk2 / (*ptrk1);
							ptrconstant++;
							ptrk1++;
							ptrk2++;
						}
					}
					if (addImages) {
						imageList.Add32F("Fondo_Constante Diferencia [ki]", constant, -1, 1);
					}
					
					cv::Mat changes(constant.rows, constant.cols, CV_8UC1);

					//The formula is 
					//      k_i=deltaN/L+1
					//constant is deltaN/L=k2(x,y)/k1(x,y), therefore, one must be subtracted from the limits. This reduces the subtraction computation step.
					float ct1 = (float)(1 - deltaBackground) - 1;
					float ct2 = (float)(1 + deltaBackground) - 1;


					//<1 ms
					for (int i = 0; i<constant.rows; i++) {
						float *ptrconstant = constant.ptr<float>(i);
						unsigned char *ptrchanges = changes.ptr<unsigned char>(i);
						for (int o = 0; o<constant.cols; o++) {
							*ptrchanges = ((ptrconstant[0] >= ct1&&ptrconstant[0] <= ct2)||
								(ptrconstant[1] >= ct1&&ptrconstant[1] <= ct2)||
								(ptrconstant[2] >= ct1&&ptrconstant[2] <= ct2)) ? 1 : 0;//255:0;
							ptrconstant += 3;
							ptrchanges++;
						}
					}
					if (framenumber % 2 == 0) {
						cv::medianBlur(changes, changes, 3);//<0.5ms
					}
					framenumber++;
					return changes;//(~cambios)/128;
				}

				int totalresetevents;
				virtual void AddImage(cv::Mat image, bool addImages, ListaImagenes &imageList) {
					if (this->background.rows==0) {
						this->background = image.clone();
						testingBackground = image.clone();
						verifiedBackground = cv::Mat::zeros(image.rows, image.cols, CV_8UC3);
						isVerified = cv::Mat::zeros(image.rows, image.cols, CV_8UC1);
						eventCounter = cv::Mat::zeros(image.rows, image.cols, CV_32FC1);
						totalEventCounter = cv::Mat::zeros(image.rows, image.cols, CV_32FC1);
						testingBackgroundChanged = cv::Mat::ones(image.rows, image.cols, CV_8UC1);
						totalresetevents = 0;
						withData = true;
						return;
					}
					withData = true;

					cv::Mat source = image;
					cv::Mat background = this->background;
					bool withRecomputing = false;
				recompute:
					cv::Mat differenceThreshold;
					
					differenceThreshold = FindDifferences(testingBackground, source, addImages, imageList,deltabackground);
					
					float beta = betavalue;
					float deltabeta = (beta - 1) / beta;
					for (int i = 0; i<totalEventCounter.rows; i++) {
						float *ptrTotal = totalEventCounter.ptr<float>(i);
						float *ptrIsBackground = eventCounter.ptr<float>(i);
						unsigned char *ptrDifference = differenceThreshold.ptr<unsigned char>(i);
						for (int o = 0; o<totalEventCounter.cols; o++) {
							if (*ptrTotal >= beta) {
								*ptrTotal = beta;//*= deltabeta;
								*ptrIsBackground *= deltabeta;
								(*ptrIsBackground) += *ptrDifference;
								//(*ptrtotal)++;
							}
							else {
								(*ptrIsBackground) += *ptrDifference;
								(*ptrTotal)++;
							}
							ptrTotal++;
							ptrIsBackground++;
							ptrDifference++;
						}
					}
					float thetaProbabilityTrue = thetaThresholdProbabilityTrue;
					float etaProbabilityFalse = etaThresholdProbabilityFalse;
					int xiNumberOfTrueEvents = xiThresholdNumberOfTrueEvents;
					int rhoNumberOfFalseEvents = rhoThresholdNumberOfFalseEvents;
					bool detectSuddenChanges = detectSuddenChangesInBackground;
					cv::Mat probab(totalEventCounter.rows, totalEventCounter.cols, CV_32FC1);
					if (detectSuddenChanges) {
						int differenceTotal = 0;
						for (int i = 0; i<totalEventCounter.rows; i++) {
							unsigned char *ptrdif = differenceThreshold.ptr<unsigned char>(i);
							for (int o = 0; o<totalEventCounter.cols; o++) {
								if (*ptrdif) differenceTotal++;
								ptrdif++;
							}
						}
						if (!withRecomputing&&differenceTotal<thresholdSuddenChanges*probab.rows*probab.cols&&totalresetevents>60) {
							std::cout << "Sudden Illumination Change:" << differenceTotal << ":" << thresholdSuddenChanges*probab.rows*probab.cols << "&" << std::endl;
							testingBackground = image.clone();
							verifiedBackground = cv::Mat::zeros(image.rows, image.cols, CV_8UC3);
							isVerified = cv::Mat::zeros(image.rows, image.cols, CV_8UC1);
							eventCounter = cv::Mat::zeros(image.rows, image.cols, CV_32FC1);
							totalEventCounter = cv::Mat::zeros(image.rows, image.cols, CV_32FC1);
							testingBackgroundChanged = cv::Mat::ones(image.rows, image.cols, CV_8UC1);
							withRecomputing = true;
							totalresetevents = 0;
							previousImages.clear();
							goto recompute;
						}
					}
					float gamma = gammaValue;
					float compgamma = 1 - gamma;
					float psi = psivalue;
					float comppsi = 1 - psi;
					
					while (previousImages.size() >= numberOfAveragedFrames)
						previousImages.erase(previousImages.begin());
					previousImages.push_back(source.clone());

					for (int i = 0; i<totalEventCounter.rows; i++) {
						float *ptrTotal = totalEventCounter.ptr<float>(i);
						float *ptrIsBackground = eventCounter.ptr<float>(i);
						float *ptrProbability = probab.ptr<float>(i);
						cv::Point3_<unsigned char> *ptrSource = source.ptr<cv::Point3_<unsigned char>>(i);
						cv::Point3_<unsigned char> *ptrBackground = background.ptr<cv::Point3_<unsigned char>>(i);
						cv::Point3_<unsigned char> *ptrVerifiedBackground = verifiedBackground.ptr<cv::Point3_<unsigned char> >(i);
						cv::Point3_<unsigned char> *ptrTestingBackground = testingBackground.ptr<cv::Point3_<unsigned char> >(i);
						unsigned char *ptrIsVerified = isVerified.ptr<unsigned char>(i);
						unsigned char *ptrDifference = differenceThreshold.ptr<unsigned char>(i);
						unsigned char *ptrTestingBackgroundChanged = testingBackgroundChanged.ptr<unsigned char>(i);
						for (int o = 0; o<totalEventCounter.cols; o++) {
							float probability = (*ptrIsBackground + 1) / (*ptrTotal + 2);
							*ptrProbability = probability;

							if ((*ptrTotal) >= xiNumberOfTrueEvents&&probability >= thetaProbabilityTrue) {
								if (*ptrDifference) {
									(*ptrVerifiedBackground) = (*ptrTestingBackground);
									(*ptrIsVerified) = 1;
									*ptrTestingBackgroundChanged = false;
								}								
							}
							else {
								if ((*ptrTotal) >= rhoNumberOfFalseEvents&&probability<etaProbabilityFalse) {
									cv::Point3_<float> suma(0, 0, 0);
									float weighttotal = 0;
									for (int j = 0; j < previousImages.size(); j++) {
										cv::Point3_<unsigned char> pt = previousImages[j].at<cv::Point3_<unsigned char>>(i, o);
										float weight = j + 1;
										suma.x += weight*pt.x;
										suma.y += weight*pt.y;
										suma.z += weight*pt.z;
										weighttotal += weight;
									}
									suma.x /= weighttotal;
									suma.y /= weighttotal;
									suma.z /= weighttotal;
									cv::Point3_<unsigned char> r(suma.x, suma.y, suma.z);
									(*ptrTestingBackground) = r;
									(*ptrIsBackground) = 0;
									(*ptrTotal) = 0;
									*ptrTestingBackgroundChanged = true;
								}
							}

							if ((*ptrIsVerified)) {
								(*ptrBackground) = (*ptrVerifiedBackground);
							}
							else {
								(*ptrBackground) = (*ptrTestingBackground);
							}
							if (*ptrDifference) {
								(*ptrTestingBackground).x = cv::saturate_cast<unsigned char>((float)(*ptrTestingBackground).x*gamma + (float)(*ptrSource).x*compgamma);
								(*ptrTestingBackground).y = cv::saturate_cast<unsigned char>((float)(*ptrTestingBackground).y*gamma + (float)(*ptrSource).y*compgamma);
								(*ptrTestingBackground).z = cv::saturate_cast<unsigned char>((float)(*ptrTestingBackground).z*gamma + (float)(*ptrSource).z*compgamma);
								if (!*ptrTestingBackgroundChanged&&*ptrIsVerified) {
									(*ptrVerifiedBackground).x = cv::saturate_cast<unsigned char>((float)(*ptrVerifiedBackground).x*psi + (float)(*ptrTestingBackground).x*comppsi);
									(*ptrVerifiedBackground).y = cv::saturate_cast<unsigned char>((float)(*ptrVerifiedBackground).y*psi + (float)(*ptrTestingBackground).y*comppsi);
									(*ptrVerifiedBackground).z = cv::saturate_cast<unsigned char>((float)(*ptrVerifiedBackground).z*psi + (float)(*ptrTestingBackground).z*comppsi);
								}
							}

							ptrTestingBackgroundChanged++;

							ptrProbability++;
							ptrTotal++;
							ptrIsBackground++;
							ptrSource++;
							ptrBackground++;
							ptrVerifiedBackground++;
							ptrTestingBackground++;
							ptrIsVerified++;
							ptrDifference++;
						}
					}
					if (addImages) {
						imageList.Add("Background_Probability Et=1 [P]", probab);
						imageList.Add("Background_Verified Background [S]", verifiedBackground);
						imageList.Add("Background_Testing Background [C]", testingBackground);
						imageList.Add("Background_Relation between verified background and testing background [D]", testingBackgroundChanged * 255);
						imageList.Add("Background_Final Background [B]", background);
						imageList.Add("Background_IsVerified [A]", isVerified * 255);
						imageList.Add("Background_Event [E]", differenceThreshold * 255);
					}
					totalresetevents++;
					

				}
			};
		}
	}
}

