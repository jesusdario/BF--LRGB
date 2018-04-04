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
#include "Imagen4D.h"
extern bool consuavizado;
extern bool connormalizacionLRGB;
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {
			std::string type2str(int type);
			class Preprocesador {
				double shiftlightness;
				double escalacolores[3];
				double escalacoloresn[3];
				bool inicializado;
				bool corregirescalacolores;
				cv::Mat anteriorimagen;
				bool blurcolor;
			public:
				Preprocesador() {
					inicializado = false;
					blurcolor = true;
					corregirescalacolores = true;
					escalacolores[0] = escalacolores[1] = escalacolores[2] = 1;
					escalacoloresn[0] = escalacoloresn[1] = escalacoloresn[2] = 1;
					shiftlightness = 0;
				}
				cv::Mat Preprocesar(cv::Mat img, bool *salir) {
					cv::Mat entrada = img;
					cv::Mat salida = img.clone();
					cv::Mat image = salida;
					if (consuavizado) {
						cv::GaussianBlur(entrada, image, cv::Size(3, 3), 1, 1);
					}
					else {
						entrada.copyTo(image);
					}
					if (salir != NULL) {
						*salir = false;
					}
					if (connormalizacionLRGB) {
						if (!inicializado) {
							Imagen4D img4d(image);
							cv::Mat L = img4d.GetLightness();
							cv::Scalar s = cv::sum(L);
							s = s / (image.rows*image.cols);
							shiftlightness = 0;//128 - s[0];
							if (s[0] > 150) {
								shiftlightness = -(s[0] - 150);
								//escala = s[0] / 150;
							}
							if (s[0] < 100) {
								shiftlightness = (100 - s[0]);
								//escala = 1+(shiftlightness / s[0]);
							}

							if (blurcolor) {
								for (int i = 0; i < 3; i++) {
									cv::Mat m = img4d.GetColorChannel(i);
									cv::GaussianBlur(m, m, cv::Size(5, 5), 2, 2);
								}
							}

							std::vector<float> promediocomponentespos;
							std::vector<float> promediocomponentesneg;
							for (int i = 0; i < 3; i++) {
								cv::Mat m = img4d.GetColorChannel(i);
								/*
								cv::Mat positivos = m >= 0;
								cv::Mat negativos = m < 0;
								cv::Scalar p = cv::sum(m.mul(positivos));
								cv::Scalar n = cv::sum(m.mul(negativos));
								cv::Scalar totalpos = cv::sum(positivos);
								cv::Scalar totalneg = cv::sum(negativos);
								*/
								double promp,promn;
								img4d.GetColorStatistics(i, &promp, &promn);
								promediocomponentespos.push_back(promp);
								promediocomponentesneg.push_back(promn);
								//escalacolores[i] = 20 / (std::abs(s2[0]) + 0.1);
								//if (escalacolores[i] > 1) {
								//	escalacolores[i] = 1;
								//}
								//if (escalacolores[i] < 0.7)
								//	escalacolores[i] = 0.7;
							}
							float promediototalpos = (std::abs(promediocomponentespos[0]) + std::abs(promediocomponentespos[1]) + std::abs(promediocomponentespos[2])) / 3;
							float promediototalneg= (std::abs(promediocomponentesneg[0]) + std::abs(promediocomponentesneg[1]) + std::abs(promediocomponentesneg[2])) / 3;
							std::cout << "PROMPOS:" << promediototalpos << ";PROMNEG:" << promediototalneg << std::endl;
							/*if (promediototalpos < 15)
								promediototalpos = 15;
							if (promediototalneg < 15)
								promediototalneg = 15;*/
							promediototalpos *= 1.5;
							promediototalneg *= 1.5;
							for (int i = 0; i < 3; i++) {
								escalacolores[i] = promediototalpos / std::abs(promediocomponentespos[i]);
								escalacoloresn[i] = promediototalneg / std::abs(promediocomponentesneg[i]);
								if (escalacolores[i] > 5)
									escalacolores[i] = 5;
								if (escalacolores[i] < 0.3)
									escalacolores[i] = 0.3;
								if (escalacoloresn[i] > 5)
									escalacoloresn[i] = 5;
								if (escalacoloresn[i] < 0.3)
									escalacoloresn[i] = 0.3;
							}
							
							/*if (shiftlightness != 0) {
								img4d.ShiftLightness(shiftlightness);
								img4d.Init(img4d.GetImage());
							}
							//double minval, maxval;
							//cv::minMaxIdx(L, &minval, &maxval);
							/*
							std::vector<float> promedioescalas;
							for (int i = 0; i < 3; i++) {
								cv::Mat m = img4d.GetColorChannel(i);
								cv::Scalar s2 = cv::sum(m);								
								s2 = s2 / (image.rows* image.cols);
								//promedioescalas.push_back(s2[0]);
								escalacolores[i] = 20 / (std::abs(s2[0])+0.1);
								if (escalacolores[i] > 1) {
									escalacolores[i] = 1;
								}
								if (escalacolores[i] < 0.7)
									escalacolores[i] = 0.7;
							}*/
							//float escala = 1;
							


							//escalacolores[0] = escalacolores[1] = escalacolores[2] = escala;
							inicializado = true;							
							std::cout << "Correcciones imagen: L:" << shiftlightness << ";C1:" << escalacolores[0] << ";C2:" << escalacolores[1] << ";C2:" << escalacolores[2] << ";N1:" << escalacoloresn[0] << ";N2:" << escalacoloresn[1] << ";N3:"<<escalacoloresn[2]<<std::endl;							
						}
						if (inicializado&&(blurcolor||shiftlightness != 0 || escalacolores[0] != 1 || escalacolores[1] != 1 || escalacolores[2] != 1)) {
							Imagen4D img4d(image);
							if (shiftlightness!=0)
								img4d.ShiftLightness(shiftlightness);
							if (blurcolor) {
								for (int i = 0; i < 3; i++) {
									cv::Mat m = img4d.GetColorChannel(i);
									cv::GaussianBlur(m, m, cv::Size(5, 5), 2, 2);
								}
							}
							if (corregirescalacolores&&(escalacolores[0]!=1||escalacolores[1]!=1||escalacolores[2]!=1)) {
								//std::cout << "escalando";
								for (int i = 0; i < 3; i++) {
									img4d.ScaleColorPN(i, escalacolores[i], escalacoloresn[i]);
									//img4d.ScaleColor(i, escalacolores[i], 0, img4d.GetImage()>0);
									//img4d.ScaleColor(i, escalacoloresn[i], 0, img4d.GetImage()<0);
								}
							}
							cv::Mat img2 = img4d.GetImage();
							cv::imshow("ANTES", image);
							img2.copyTo(image);				
							cv::imshow("IMG", image);
						}
					}
					/*
					cv::imshow("anterior", image);
					cv::Mat a,b;
					image.convertTo(a, CV_32FC3);					
					cv::blur(a,b, cv::Size(3, 3), cv::Point(1, 1));
					cv::Mat res = a + 0.5f*(a - b);
					res.convertTo(image, CV_8UC3);
					//image = image + 0.1*(image - bl);
					cv::imshow("actual", image);

					*/
					//if (anteriorimagen.rows > 0) {
					//	cv::Mat mascara;
					//	cv::Mat difs = cv::abs(image - anteriorimagen);
					//	std::vector<cv::Mat> factores;
					//	cv::split(image, factores);
					//	factores[0].convertTo(factores[0], CV_32FC1);
					//	factores[1].convertTo(factores[1], CV_32FC1);
					//	factores[2].convertTo(factores[2], CV_32FC1);
					//	cv::sqrt(factores[0], factores[0]);
					//	cv::sqrt(factores[1], factores[1]);
					//	cv::sqrt(factores[2], factores[2]);
					//	factores[0] = 5 * factores[0] / 16;
					//	factores[1] = 5 * factores[1] / 16;
					//	factores[2] = 5 * factores[2] / 16;
					//	
					//	factores[0].convertTo(factores[0], CV_8UC1);
					//	factores[1].convertTo(factores[1], CV_8UC1);
					//	factores[2].convertTo(factores[2], CV_8UC1);
					//	cv::Mat desviacion;
					//	cv::merge(factores, desviacion);
					//	//cv::Mat suma=(factores[0]+factores[1]+factores[2])
					//	mascara = (difs > desviacion);
					//	//cv::imshow("mascara", mascara * 255);
					//	//mascara = (difs >= 5);
					//	//mascara.convertTo(mascara, CV_32FC1);
					//	//cv::imshow("mascara", mascara);
					//	//std::cout << type2str(mascara.type()) << std::endl;
					//	cv::Mat fimg;
					//	cv::Mat aimg;
					//	image.convertTo(fimg, CV_32FC3);
					//	anteriorimagen.convertTo(aimg, CV_32FC3);
					//	cv::Mat promedio;
					//	fimg.copyTo(aimg, mascara);
					//	//aimg.mul(mascara) + aimg.mul(cv::Scalar(1, 1, 1) - mascara);
					//	promedio = (fimg + 0.5f*aimg) / 1.5f;//(fimg + 0.5*(aimg.mul(mascara) + aimg.mul(cv::Scalar(1,1,1) - mascara)))/(1.5f);
					//	//cv::imshow("imageorig", image);
					//	promedio.convertTo(image, CV_8UC3);
					//	//cv::imshow("imagen", image);
					//	anteriorimagen = salida->ObtenerMatriz().clone();

					//	// a = a/2 + b/2
					//	//image = (image + (anteriorimagen.mul(mascara) + image.mul(1 - mascara)));
					//}
					//else {
					//	anteriorimagen = salida->ObtenerMatriz().clone();
					//}
					int lim = image.cols * 3;

					for (int i = 0; i < image.rows; i++) {
						unsigned char *pfila = image.ptr<unsigned char>(i);
						for (int o = 0; o < lim; o++) {
							//if (*pfila < 1) *pfila = 1;
							//if (*pfila < 4) *pfila = 4;
							if (*pfila < 8) *pfila = 8; //PARA WALLFLOWER OK
							pfila++;
						}
					}
					

					return salida;
				}
			};
		}
	}
}
