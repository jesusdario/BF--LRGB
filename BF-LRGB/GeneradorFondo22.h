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
extern double valorProbabilidadTrue;
extern double valorProbabilidadFalse;
extern int valorEventosTrue;
extern int valorEventosFalse;
extern double valorgama;
extern double valorpsi;
extern double valorbeta;
extern float limdetfondo;
extern bool considerarcambiosubito;
extern double thresholdcambiosubito;
extern int numframespromedio;
//cv::Mat QuitarPuntosAislados(cv::Mat mat);
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {
			class GeneradorFondo22 {
				
				bool condatos;				
			public:
				cv::Mat fondo;
				cv::Mat fondocalc;
				cv::Mat fondoalmacenado;
				int numframe;
				GeneradorFondo22() {
					condatos = false;
					numframe = 0;
				}
				virtual bool ConDatos() {
					return condatos;
				}
				cv::Mat almacenado;
				cv::Mat contadoresfondo;
				cv::Mat contadortotal;
				cv::Mat threshdifback;
				std::vector<cv::Mat> imagenesanteriores;
				cv::Mat fondopruebacambiado;
				virtual cv::Mat ObtenerFondoActual() {
					return fondo;
				}
				cv::Mat triplicar(cv::Mat a) {
					std::vector<cv::Mat> vec;
					vec.push_back(a);
					vec.push_back(a);
					vec.push_back(a);
					cv::Mat res;
					cv::merge(vec, res);
					return res;
				}
				void MaxLim(cv::Mat mat, int limite) {
					int limcol = mat.cols * 3;
					for (int i = 0; i<mat.rows; i++) {
						unsigned char *pt = mat.ptr<unsigned char>(i);
						for (int o = 0; o<limcol; o++) {
							if (*pt<limite) {
								*pt = limite;
							}
							pt++;
						}
					}
				}
				inline bool doscondiciones(bool cond1, bool cond2, bool cond3) {
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
				inline int redondear(float f) {
					int r = std::floor(f + 0.5f);
					return r;
				}
				virtual cv::Mat EncontrarDiferencias(cv::Mat fondo, cv::Mat origen, bool agregarimagenes, ListaImagenes &lista,float limdetfondo) {
					cv::Mat fondof, foregroundf;
					cv::Mat brightness(origen.rows, origen.cols, CV_32FC3);

					const unsigned int valblue = 0.114f * 256 * 256 * 256;
					const unsigned int valgreen = 0.587f * 256 * 256 * 256;
					const unsigned int valred = 0.299f * 256 * 256 * 256;
					for (int i = 0; i<origen.rows; i++) {
						unsigned char *ptrfondo = fondo.ptr<unsigned char>(i);
						float *ptrbrightness = brightness.ptr<float>(i);
						for (int o = 0; o<origen.cols; o++) {
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
					cv::Mat k2(origen.rows, origen.cols, CV_32FC3);
					int limptr = origen.cols * 3;
					for (int i = 0; i<origen.rows; i++) {
						unsigned char *ptrorigen = origen.ptr<unsigned char>(i);
						unsigned char *ptrfondo = fondo.ptr<unsigned char>(i);
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
					cv::Mat constante(fondo.rows, fondo.cols, CV_32FC3);

					__m128 constmat;
					constmat.m128_f32[0] = 0.1f;
					constmat.m128_f32[1] = 0.1f;
					constmat.m128_f32[2] = 0.1f;
					constmat.m128_f32[3] = 0.1f;
					for (int i = 0; i<constante.rows; i++) {
						float *ptrk2 = k2.ptr<float>(i);
						float *ptrk1 = k1.ptr<float>(i);
						float *ptrconstante = constante.ptr<float>(i);

						int delta = limptr / 4;
						int resto = limptr % 4;

						for (int o = 0; o < delta; o++) {
#if SININCREMENTODENOM
							_mm_storeu_ps(ptrconstante, _mm_div_ps(_mm_loadu_ps(ptrk2), _mm_loadu_ps(ptrk1)));
#else
							_mm_storeu_ps(ptrconstante, _mm_div_ps(_mm_loadu_ps(ptrk2), _mm_add_ps(_mm_loadu_ps(ptrk1), constmat)));
#endif
							ptrk1 += 4;
							ptrk2 += 4;
							ptrconstante += 4;
						}

						for (int o = delta * 4; o<limptr; o++) {
#if SININCREMENTODENOM
							*ptrconstante = *ptrk2 / (*ptrk1);
#else
							*ptrconstante = *ptrk2 / (*ptrk1 + 0.1f);
#endif
							ptrconstante++;
							ptrk1++;
							ptrk2++;
						}
					}
					if (agregarimagenes) {
						lista.Agregar32F("Fondo_Constante Diferencia [ki]", constante, -1, 1);
					}
					
					cv::Mat cambios(constante.rows, constante.cols, CV_8UC1);


					float ct1 = (float)(1 - limdetfondo) - 1;
					float ct2 = (float)(1 + limdetfondo) - 1;


					//<1 ms
					for (int i = 0; i<constante.rows; i++) {
						float *resconst = constante.ptr<float>(i);
						unsigned char *rescambios = cambios.ptr<unsigned char>(i);
						for (int o = 0; o<constante.cols; o++) {
							*rescambios = ((resconst[0] >= ct1&&resconst[0] <= ct2)||
								(resconst[1] >= ct1&&resconst[1] <= ct2)||
								(resconst[2] >= ct1&&resconst[2] <= ct2)) ? 1 : 0;//255:0;
							resconst += 3;
							rescambios++;
						}
					}
					cv::Mat res;
					if (numframe % 2 == 0) {
						cv::medianBlur(cambios, cambios, 3);//<0.5ms
					}
					numframe++;
					
					return cambios;//(~cambios)/128;
				}

				int totaleventosreset;
				virtual void AgregarImagen(cv::Mat imagen, cv::Mat mascaraforeground, bool agregarimagenes, ListaImagenes &lista) {
					if (this->fondo.rows==0) {
						this->fondo = imagen.clone();
						fondocalc = imagen.clone();
						fondoalmacenado = cv::Mat::zeros(imagen.rows, imagen.cols, CV_8UC3);
						almacenado = cv::Mat::zeros(imagen.rows, imagen.cols, CV_8UC1);
						contadoresfondo = cv::Mat::zeros(imagen.rows, imagen.cols, CV_32FC1);
						contadortotal = cv::Mat::zeros(imagen.rows, imagen.cols, CV_32FC1);
						fondopruebacambiado = cv::Mat::ones(imagen.rows, imagen.cols, CV_8UC1);
						totaleventosreset = 0;
						condatos = true;
						return;
					}
					condatos = true;

					cv::Mat origen = imagen;
					cv::Mat fondo = this->fondo;
					bool conrecalc = false;
				recalc:
					cv::Mat thresholdDiferencia;
					
					thresholdDiferencia = EncontrarDiferencias(fondocalc, origen, agregarimagenes, lista,limdetfondo);
					
					float beta = valorbeta;
					float deltabeta = (beta - 1) / beta;
					for (int i = 0; i<contadortotal.rows; i++) {
						float *ptrtotal = contadortotal.ptr<float>(i);
						float *ptresfondo = contadoresfondo.ptr<float>(i);
						unsigned char *ptrdif = thresholdDiferencia.ptr<unsigned char>(i);
						for (int o = 0; o<contadortotal.cols; o++) {
							if (*ptrtotal >= beta) {
								*ptrtotal = beta;//*= deltabeta;
								*ptresfondo *= deltabeta;
								(*ptresfondo) += *ptrdif;
								//(*ptrtotal)++;
							}
							else {
								(*ptresfondo) += *ptrdif;
								(*ptrtotal)++;
							}
							ptrtotal++;
							ptresfondo++;
							ptrdif++;
						}
					}
					float probabilidadtrue = valorProbabilidadTrue;
					float probabilidadfalse = valorProbabilidadFalse;
					int nroeventostrue = valorEventosTrue;
					int nroeventosfalse = valorEventosFalse;
					bool cambiosubito = considerarcambiosubito;
					cv::Mat probab(contadortotal.rows, contadortotal.cols, CV_32FC1);
					if (cambiosubito) {
						int totaldifs = 0;
						for (int i = 0; i<contadortotal.rows; i++) {
							unsigned char *ptrdif = thresholdDiferencia.ptr<unsigned char>(i);
							for (int o = 0; o<contadortotal.cols; o++) {
								if (*ptrdif) totaldifs++;
								ptrdif++;
							}
						}
						if (!conrecalc&&totaldifs<thresholdcambiosubito*probab.rows*probab.cols&&totaleventosreset>60) {
							std::cout << "Cambio subito iluminacion:" << totaldifs << ":" << thresholdcambiosubito*probab.rows*probab.cols << "&" << std::endl;
							fondocalc = imagen.clone();
							fondoalmacenado = cv::Mat::zeros(imagen.rows, imagen.cols, CV_8UC3);
							almacenado = cv::Mat::zeros(imagen.rows, imagen.cols, CV_8UC1);
							contadoresfondo = cv::Mat::zeros(imagen.rows, imagen.cols, CV_32FC1);
							contadortotal = cv::Mat::zeros(imagen.rows, imagen.cols, CV_32FC1);
							fondopruebacambiado = cv::Mat::ones(imagen.rows, imagen.cols, CV_8UC1);
							conrecalc = true;
							totaleventosreset = 0;
							imagenesanteriores.clear();
							goto recalc;
						}
					}
					float gamma = valorgama;
					float compgamma = 1 - gamma;
					float psi = valorpsi;
					float comppsi = 1 - psi;
					
					while (imagenesanteriores.size() >= numframespromedio)
						imagenesanteriores.erase(imagenesanteriores.begin());
					imagenesanteriores.push_back(origen.clone());

					for (int i = 0; i<contadortotal.rows; i++) {
						float *ptrtotal = contadortotal.ptr<float>(i);
						float *ptresfondo = contadoresfondo.ptr<float>(i);
						float *ptrprobabilidad = probab.ptr<float>(i);
						cv::Point3_<unsigned char> *ptrorigen = origen.ptr<cv::Point3_<unsigned char>>(i);
						cv::Point3_<unsigned char> *ptrfondo = fondo.ptr<cv::Point3_<unsigned char>>(i);
						cv::Point3_<unsigned char> *ptrfondoalmacenado = fondoalmacenado.ptr<cv::Point3_<unsigned char> >(i);
						cv::Point3_<unsigned char> *ptrfondocalc = fondocalc.ptr<cv::Point3_<unsigned char> >(i);
						unsigned char *ptralmacenado = almacenado.ptr<unsigned char>(i);
						unsigned char *ptrdif = thresholdDiferencia.ptr<unsigned char>(i);
						unsigned char *ptrfondopruebacambiado = fondopruebacambiado.ptr<unsigned char>(i);
						for (int o = 0; o<contadortotal.cols; o++) {
							float probabilidad = (*ptresfondo + 1) / (*ptrtotal + 2);
							*ptrprobabilidad = probabilidad;

							if ((*ptrtotal) >= nroeventostrue&&probabilidad >= probabilidadtrue) {
								if (*ptrdif) {
									(*ptrfondoalmacenado) = (*ptrfondocalc);
									(*ptralmacenado) = 1;
									*ptrfondopruebacambiado = false;
								}								
							}
							else {
								if ((*ptrtotal) >= nroeventosfalse&&probabilidad<probabilidadfalse) {
									cv::Point3_<float> suma(0, 0, 0);
									float totpeso = 0;
									for (int j = 0; j < imagenesanteriores.size(); j++) {
										cv::Point3_<unsigned char> pt = imagenesanteriores[j].at<cv::Point3_<unsigned char>>(i, o);
										float peso = j + 1;
										suma.x += peso*pt.x;
										suma.y += peso*pt.y;
										suma.z += peso*pt.z;
										totpeso += peso;
									}
									suma.x /= totpeso;
									suma.y /= totpeso;
									suma.z /= totpeso;
									cv::Point3_<unsigned char> r(suma.x, suma.y, suma.z);
									(*ptrfondocalc) = r;
									(*ptresfondo) = 0;
									(*ptrtotal) = 0;
									*ptrfondopruebacambiado = true;
								}
							}

							if ((*ptralmacenado)) {
								(*ptrfondo) = (*ptrfondoalmacenado);
							}
							else {
								(*ptrfondo) = (*ptrfondocalc);
							}
							if (*ptrdif) {
								(*ptrfondocalc).x = cv::saturate_cast<unsigned char>((float)(*ptrfondocalc).x*gamma + (float)(*ptrorigen).x*compgamma);
								(*ptrfondocalc).y = cv::saturate_cast<unsigned char>((float)(*ptrfondocalc).y*gamma + (float)(*ptrorigen).y*compgamma);
								(*ptrfondocalc).z = cv::saturate_cast<unsigned char>((float)(*ptrfondocalc).z*gamma + (float)(*ptrorigen).z*compgamma);
								if (!*ptrfondopruebacambiado&&*ptralmacenado) {
									(*ptrfondoalmacenado).x = cv::saturate_cast<unsigned char>((float)(*ptrfondoalmacenado).x*psi + (float)(*ptrfondocalc).x*comppsi);
									(*ptrfondoalmacenado).y = cv::saturate_cast<unsigned char>((float)(*ptrfondoalmacenado).y*psi + (float)(*ptrfondocalc).y*comppsi);
									(*ptrfondoalmacenado).z = cv::saturate_cast<unsigned char>((float)(*ptrfondoalmacenado).z*psi + (float)(*ptrfondocalc).z*comppsi);
								}
							}

							ptrfondopruebacambiado++;

							ptrprobabilidad++;
							ptrtotal++;
							ptresfondo++;
							ptrorigen++;
							ptrfondo++;
							ptrfondoalmacenado++;
							ptrfondocalc++;
							ptralmacenado++;
							ptrdif++;
						}
					}
					if (agregarimagenes) {
						lista.Agregar("Fondo_Probabilidad Et=1 [P]", probab);
						lista.Agregar("Fondo_Fondo Verificado [S]", fondoalmacenado);
						lista.Agregar("Fondo_Fondo de Prueba [C]", fondocalc);
						lista.Agregar("Fondo_Relacion entre fondo verificado y de prueba [D]", fondopruebacambiado * 255);
						lista.Agregar("Fondo_Fondo Final [B]", fondo);
						lista.Agregar("Fondo_Almacenado [A]", almacenado * 255);
						lista.Agregar("Fondo_Evento Ct-It [E]", thresholdDiferencia * 255);
					}
					totaleventosreset++;
					

				}
			};
		}
	}
}

