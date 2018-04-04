#include "stdafx.h"
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
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {
			cv::Mat ObtenerMascaraMorph(int ancho) {
				if (ancho % 2 == 0) {
					cv::Mat m = cv::Mat::zeros(ancho + 1, ancho + 1, CV_8UC1);//MODIFICACION. HACE LA MASCARA UN POCO MAS GRANDE PARA QUE ENTRE EL LADO DERECHO E INFERIOR DEL CIRCULO EN LA IMAGEN
					cv::circle(m, cv::Point(ancho / 2, ancho / 2), ancho / 2, cv::Scalar(255), -1);
					//cv::imshow("mascara",m);
					return m;
				}
				else {
					//0 1 2 3 4
					//    
					cv::Mat m = cv::Mat::zeros(ancho, ancho, CV_8UC1);//MODIFICACION. HACE LA MASCARA UN POCO MAS GRANDE PARA QUE ENTRE EL LADO DERECHO E INFERIOR DEL CIRCULO EN LA IMAGEN
					cv::circle(m, cv::Point((ancho-1) / 2, (ancho-1) / 2), (ancho-1) / 2, cv::Scalar(255), -1);
					//cv::imshow("mascara2", m);
					return m;
				}				
			}
			typedef unsigned char *pbyte;
			cv::Mat Dilate(cv::Mat mat, cv::Mat mascara, int val) {
				int inif = mascara.rows / 2;
				int finf = mat.rows - inif;

				int inic = mascara.cols / 2;
				int finc = mat.cols - inic;

				int deltaf = mascara.rows / 2;
				unsigned char **ptrs = new pbyte[mascara.rows];
				unsigned char **ptrsmascara = new pbyte[mascara.rows];

				cv::Mat res = cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);
				for (int q = 0;q<mascara.rows;q++) {
					ptrsmascara[q] = mascara.ptr<unsigned char>(q);
				}
				for (int i = inif;i<finf;i++) {
					unsigned char*fila = mat.ptr<unsigned char>(i) + inic;
					unsigned char*filares = res.ptr<unsigned char>(i) + inic;
					for (int q = 0;q<mascara.rows;q++) {
						ptrs[q] = res.ptr<unsigned char>(i - inif);
					}
					for (int o = inic;o<finc;o++) {
						if (fila[o]) {
							for (int q = 0;q<mascara.rows;q++) {
								for (int r = 0;r<mascara.cols;r++) {
									if (ptrsmascara[q][r]) {
										ptrs[q][r] = val;
									}
								}
							}
						}
						for (int q = 0;q<mascara.rows;q++) {
							ptrs[q]++;
						}
						fila++;
						filares++;
					}
				}
				delete[]ptrs;
				delete[]ptrsmascara;
				return res;
			}
			cv::Mat Erode(cv::Mat mat, cv::Mat mascara, int val) {
				int inif = mascara.rows / 2;
				int finf = mat.rows - inif;

				int inic = mascara.cols / 2;
				int finc = mat.cols - inic;

				int deltaf = mascara.rows / 2;
				unsigned char **ptrs = new unsigned char*[mascara.rows];
				unsigned char **ptrsmascara = new unsigned char*[mascara.rows];

				cv::Mat res = cv::Mat(mat.rows, mat.cols, CV_8UC1, cv::Scalar(val, 0, 0));
				for (int q = 0;q<mascara.rows;q++) {
					ptrsmascara[q] = mascara.ptr<unsigned char>(q) + inic;
				}
				for (int i = inif;i<finf;i++) {
					unsigned char*fila = mat.ptr<unsigned char>(i);
					unsigned char*filares = res.ptr<unsigned char>(i);
					for (int q = 0;q<mascara.rows;q++) {
						ptrs[q] = res.ptr<unsigned char>(i - inif);
					}
					for (int o = inic;o<finc;o++) {
						if (!fila[o]) {
							for (int q = 0;q<mascara.rows;q++) {
								for (int r = 0;r<mascara.cols;r++) {
									if (ptrsmascara[q][r]>0) {
										ptrs[q][r] = 0;
									}
								}
							}
						}
						fila++;
						filares++;
					}
				}
				return res;
			}
			cv::Mat MorphClose(cv::Mat mat, cv::Mat mascara, int val) {
				cv::Mat res = Dilate(mat, mascara, val);
				return res;
			}
			void MedianBlur5_2(cv::Mat mat, cv::Mat& rest, unsigned char val) {
				cv::medianBlur(mat, rest, 5);
				///*cv::Mat res=cv::Mat::zeros(mat.rows,mat.cols,CV_8UC1);		
				//cv::Mat expmat;
				//unsigned char * ptr[5];
				//unsigned int a,b;
				//int limc=mat.cols-2;		
				//int valc=val*13;
				//cv::copyMakeBorder(mat,expmat,2,2,2,2,cv::BORDER_CONSTANT);
				//int delta2[5];
				//for (int i=0;i<mat.rows;i++) {
				//	ptr[0]=expmat.ptr<unsigned char>(i)+4;
				//	ptr[1]=expmat.ptr<unsigned char>(i+1)+4;
				//	ptr[2]=expmat.ptr<unsigned char>(i+2)+4;
				//	ptr[3]=expmat.ptr<unsigned char>(i+3)+4;
				//	ptr[4]=expmat.ptr<unsigned char>(i+4)+4;
				//	unsigned char *ptrres=res.ptr<unsigned char>(i);
				//	int suma=0;
				//	delta2[0]=0;
				//	for (int s=0;s<4;s++) {
				//		int delta=0;
				//		for (int q=0;q<5;q++) {
				//			suma+=ptr[q][s-4];					
				//		}
				//		delta2[s+1]=delta;
				//	}
				//	
				//	for (int o=0;o<mat.cols;o++) {
				//		int delta=0;
				//		for (int q=0;q<5;q++) {
				//			delta+=*(ptr[q]);					
				//		}		
				//		suma+=delta;
				//		if (suma>valc) {
				//			*ptrres=val;
				//		} 
				//		delta2[0]=delta2[1];
				//		delta2[1]=delta2[2];
				//		delta2[2]=delta2[3];
				//		delta2[3]=delta2[4];
				//		delta2[4]=delta;
				//		suma-=delta2[1];
				//		ptrres++;
				//		ptr[0]++;
				//		ptr[1]++;
				//		ptr[2]++;
				//		ptr[3]++;
				//		ptr[4]++;
				//	}			
				//}		
				//rest=res;*/
			}
			/*
			ptra=expmat.ptr<unsigned char>(i);
			ptrb=expmat.ptr<unsigned char>(i+1);
			ptrc=expmat.ptr<unsigned char>(i+2);
			ptrd=expmat.ptr<unsigned char>(i+3);
			ptre=expmat.ptr<unsigned char>(i+4);
			unsigned char *ptrres=res.ptr<unsigned char>(i);
			int suma=0;
			for (int o=2;o<limc;o++) {
			suma+=ptra[4]+ptrb[4]+ptrc[4]+ptrd[4]+ptre[4];
			if (suma>valc) {
			*ptrres=val;
			}
			suma-=*ptra+*ptrb+*ptrc+*ptrd+*ptre;
			ptrres++;
			ptra++;ptrb++;ptrc++;ptrd++;ptre++;
			}
			*/
			void MedianBlur5(cv::Mat mat, cv::Mat& rest, unsigned char val) {
				cv::Mat res = cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);
				unsigned char * ptr[5];
				unsigned int a, b;
				int limc = mat.cols - 2;
				int valc = val * 13;
				for (int i = 0;i<mat.rows;i++) {
					ptr[0] = mat.ptr<unsigned char>(i - 2<0 ? (i - 1<0 ? i : i - 1) : i - 2);
					ptr[1] = mat.ptr<unsigned char>(i - 1<0 ? i : i - 1);
					ptr[2] = mat.ptr<unsigned char>(i);
					ptr[3] = mat.ptr<unsigned char>(i + 1 >= mat.rows ? i : i + 1);
					ptr[4] = mat.ptr<unsigned char>(i + 2 >= mat.rows ? (i + 1 >= mat.rows ? i : i + 1) : i + 2);
					unsigned char *ptrres = res.ptr<unsigned char>(i);
					a = 0;
					for (int q = 0;q<5;q++) {
						for (int s = 0;s <= 2;s++) {
							if (ptr[q][s]>0)
								a++;
						}
					}
					//if (a >= 13) {
					if (a > +5*3/2+1) {
						*ptrres = val;
					}
					else
						*ptrres = 0;
					ptrres++;
					ptr[0]++;
					ptr[1]++;
					ptr[2]++;
					ptr[3]++;
					ptr[4]++;
					a = 0;
					for (int q = 0;q<5;q++) {
						for (int s = -1;s <= 2;s++) {
							if (ptr[q][s]>0)
								a++;
						}
					}
					//if (a >= 13) {
					if (a>5*4/2+1) {
						*ptrres = val;
					}
					else
						*ptrres = 0;
					ptrres++;
					ptr[0]++;
					ptr[1]++;
					ptr[2]++;
					ptr[3]++;
					ptr[4]++;
					int suma = 0;
					for (int q = 0;q<5;q++) {
						for (int r = -2;r<2;r++)
							suma += ptr[q][r];
					}
					for (int o = 2;o<limc;o++) {

						for (int q = 0;q<5;q++) {
							suma += ptr[q][2];
						}
						//if (suma>valc) {
						if (suma>valc) {
							*ptrres = val;
						}
						for (int q = 0;q<5;q++) {
							suma -= ptr[q][-2];
						}
						ptrres++;
						ptr[0]++;
						ptr[1]++;
						ptr[2]++;
						ptr[3]++;
						ptr[4]++;
					}
					a = 0;
					for (int q = 0;q<5;q++) {
						for (int s = -2;s <= 1;s++) {
							if (ptr[q][s]>0)
								a++;
						}
					}
					//if (a >= 13) {
					if (a>5*4/2+1) {
						*ptrres = val;
					}
					else
						*ptrres = 0;
					ptrres++;
					ptr[0]++;
					ptr[1]++;
					ptr[2]++;
					ptr[3]++;
					ptr[4]++;
					a = 0;
					for (int q = 0;q<5;q++) {
						for (int s = -2;s <= 0;s++) {
							if (ptr[q][s]>0)
								a++;
						}
					}
					//if (a >= 13) {
					if (a>5*3/2+1) {
						*ptrres = val;
					}
					else
						*ptrres = 0;
					ptrres++;
					ptr[0]++;
					ptr[1]++;
					ptr[2]++;
					ptr[3]++;
					ptr[4]++;
				}
				rest = res;
			}

			void QuitarPuntosAislados2bak(cv::Mat mat, cv::Mat& rest) {
				cv::Mat res = mat.clone();//cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);
				unsigned char * ptr[5];
				unsigned char * ptrres[3];
				unsigned int a, b;
				int limc = mat.cols - 2;
				for (int i = 0; i < mat.rows; i++) {
					ptr[0] = mat.ptr<unsigned char>(i - 2 < 0 ? i - 1 < 0 ? i : i - 1 : i - 2);
					ptr[1] = mat.ptr<unsigned char>(i - 1 < 0 ? i : i - 1);
					ptr[2] = mat.ptr<unsigned char>(i);
					ptr[3] = mat.ptr<unsigned char>(i + 1 >= mat.rows ? i : i + 1);
					ptr[4] = mat.ptr<unsigned char>(i + 2 >= mat.rows ? i + 1 >= mat.rows ? i : i + 1 : i + 2);
					ptrres[0] = res.ptr<unsigned char>(i - 1 < 0 ? i : i - 1);
					ptrres[1] = res.ptr<unsigned char>(i);
					ptrres[2] = res.ptr<unsigned char>(i + 1 >= mat.rows ? i : i + 1);
					ptr[0] += 2;
					ptr[1] += 2;
					ptr[2] += 2;
					ptr[3] += 2;
					ptr[4] += 2;
					ptrres[0] += 2;
					ptrres[1] += 2;
					ptrres[2] += 2;
					for (int o = 2; o < limc; o++) {
						if (ptr[0][-2] == 0 && ptr[0][-1] == 0 && ptr[0][0] == 0 && ptr[0][1] == 0 && ptr[0][2] == 0 &&
							ptr[4][-2] == 0 && ptr[4][-1] == 0 && ptr[4][0] == 0 && ptr[4][1] == 0 && ptr[4][2] == 0 &&
							ptr[2][-2] == 0 && ptr[2][2] == 0 &&
							ptr[1][-2] == 0 && ptr[1][2] == 0 &&
							ptr[3][-2] == 0 && ptr[3][2] == 0) {
							for (int u = -1; u <= 1; u++) {
								ptrres[0][u] = 0;
								ptrres[1][u] = 0;
								ptrres[2][u] = 0;
							}
						}
						ptr[0]++;
						ptr[1]++;
						ptr[2]++;
						ptr[3]++;
						ptr[4]++;
						ptrres[0]++;
						ptrres[1]++;
						ptrres[2]++;
					}
				}
				rest = res;
					
			}

			void QuitarPuntosAislados2(cv::Mat mat, cv::Mat& rest) {
				cv::Mat res = mat.clone();//cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);
				unsigned char * ptr[5];
				unsigned char * ptrres[3];
				unsigned int a, b;
				int limc = mat.cols - 2;
				for (int i = 0; i < mat.rows; i++) {
					ptr[0] = mat.ptr<unsigned char>(i - 2 < 0 ? i - 1 < 0 ? i : i - 1 : i - 2);
					ptr[1] = mat.ptr<unsigned char>(i - 1 < 0 ? i : i - 1);
					ptr[2] = mat.ptr<unsigned char>(i);
					ptr[3] = mat.ptr<unsigned char>(i + 1 >= mat.rows ? i : i + 1);
					ptr[4] = mat.ptr<unsigned char>(i + 2 >= mat.rows ? i + 1 >= mat.rows ? i : i + 1 : i + 2);
					ptrres[0] = res.ptr<unsigned char>(i - 1 < 0 ? i : i - 1);
					ptrres[1] = res.ptr<unsigned char>(i);
					ptrres[2] = res.ptr<unsigned char>(i + 1 >= mat.rows ? i : i + 1);
					ptr[0] += 2;
					ptr[1] += 2;
					ptr[2] += 2;
					ptr[3] += 2;
					ptr[4] += 2;
					ptrres[0] += 2;
					ptrres[1] += 2;
					ptrres[2] += 2;
					int sumatope = ptr[0][-2] + ptr[0][-1] + ptr[0][0] + ptr[0][1] ;
					int sumaabajo= ptr[4][-2] + ptr[4][-1] + ptr[4][0] + ptr[4][1];
					int sumares = ptrres[0][-1] + ptrres[0][0] +
						ptrres[1][-1] + ptrres[1][0] +
						ptrres[2][-1] + ptrres[2][0];
						
					for (int o = 2; o < limc; o++) {
						sumatope += ptr[0][2];
						sumaabajo += ptr[4][2];
						sumares += ptrres[0][1] + ptrres[1][1] + ptrres[2][1];
						while (o<limc&&(sumatope != 0 || sumaabajo != 0||sumares==0)) {
							sumatope -= ptr[0][-2];
							sumaabajo -= ptr[4][-2];
							sumares -= ptrres[0][-1] + ptrres[1][-1] + ptrres[2][-1];
							o++;
							ptr[0]++;
							ptr[1]++;
							ptr[2]++;
							ptr[3]++;
							ptr[4]++;
							ptrres[0]++;
							ptrres[1]++;
							ptrres[2]++;
							sumatope += ptr[0][2];
							sumaabajo += ptr[4][2];
							sumares += ptrres[0][1] + ptrres[1][1] + ptrres[2][1];
						}
						if (o == limc) continue;
						if (sumares!=0&&sumatope==0&&sumaabajo==0&&
							ptr[2][-2] == 0 && ptr[2][2] == 0 &&
							ptr[1][-2] == 0 && ptr[1][2] == 0 &&
							ptr[3][-2] == 0 && ptr[3][2] == 0) {
							ptrres[0][-1] = ptrres[0][0] = ptrres[0][1] = 0;
							ptrres[1][-1] = ptrres[1][0] = ptrres[1][1] = 0;
							ptrres[2][-1] = ptrres[2][0] = ptrres[2][1] = 0;
						}
						sumares -= ptrres[0][-1] + ptrres[1][-1] + ptrres[2][-1];
						sumatope -= ptr[0][-2];
						sumaabajo -= ptr[4][-2];
						ptr[0]++;
						ptr[1]++;
						ptr[2]++;
						ptr[3]++;
						ptr[4]++;
						ptrres[0]++;
						ptrres[1]++;
						ptrres[2]++;
					}
				}
				rest = res;
				
			}

			void QuitarPuntosAislados3(cv::Mat mat, cv::Mat& rest) {
				cv::Mat res = mat.clone();//cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);
				unsigned char * ptr[5];
				unsigned char * ptrres[3];
				unsigned int a, b;
				int limc = mat.cols - 2;
				for (int i = 0; i < mat.rows; i++) {
					ptr[0] = mat.ptr<unsigned char>(i - 2 < 0 ? i - 1 < 0 ? i : i - 1 : i - 2);
					ptr[1] = mat.ptr<unsigned char>(i - 1 < 0 ? i : i - 1);
					ptr[2] = mat.ptr<unsigned char>(i);
					ptr[3] = mat.ptr<unsigned char>(i + 1 >= mat.rows ? i : i + 1);
					ptr[4] = mat.ptr<unsigned char>(i + 2 >= mat.rows ? i + 1 >= mat.rows ? i : i + 1 : i + 2);
					ptrres[0] = res.ptr<unsigned char>(i - 1 < 0 ? i : i - 1);
					ptrres[1] = res.ptr<unsigned char>(i);
					ptrres[2] = res.ptr<unsigned char>(i + 1 >= mat.rows ? i : i + 1);
					ptr[0] += 2;
					ptr[1] += 2;
					ptr[2] += 2;
					ptr[3] += 2;
					ptr[4] += 2;
					ptrres[0] += 2;
					ptrres[1] += 2;
					ptrres[2] += 2;
					for (int o = 2; o < limc; o++) {
						int sumaborde = ptr[0][-2] + ptr[0][-1] + ptr[0][0] + ptr[0][1] + ptr[0][2] +
							ptr[4][-2] + ptr[4][-1] + ptr[4][0] + ptr[4][1] + ptr[4][2] +
							ptr[2][-2] + ptr[2][2] +
							ptr[1][-2] + ptr[1][2] +
							ptr[3][-2] + ptr[3][2];
						int sumacentro = ptr[1][-1] + ptr[1][0] + ptr[1][1]+
							ptr[2][-1] + ptr[2][0] + ptr[2][1]+
							ptr[3][-1] + ptr[3][0] + ptr[3][1];
						/*if (sumacentro + sumaborde > 13) {
							if (sumaborde <= 4) {
								ptrres[1][0] = 0;
							} else
								ptrres[1][0] = 1;
						}
						else*/ {
							if (sumaborde >= 5)
								ptrres[1][0] = ptr[2][0];
							else
								ptrres[1][0] = 0;
						}
						/*if (ptr[0][-2] == 0 && ptr[0][-1] == 0 && ptr[0][0] == 0 && ptr[0][1] == 0 && ptr[0][2] == 0 &&
							ptr[4][-2] == 0 && ptr[4][-1] == 0 && ptr[4][0] == 0 && ptr[4][1] == 0 && ptr[4][2] == 0 &&
							ptr[2][-2] == 0 && ptr[2][2] == 0 &&
							ptr[1][-2] == 0 && ptr[1][2] == 0 &&
							ptr[3][-2] == 0 && ptr[3][2] == 0) {
							for (int u = -1; u <= 1; u++) {
								ptrres[0][u] = 0;
								ptrres[1][u] = 0;
								ptrres[2][u] = 0;
							}
						}*/
						
						ptr[0]++;
						ptr[1]++;
						ptr[2]++;
						ptr[3]++;
						ptr[4]++;
						ptrres[0]++;
						ptrres[1]++;
						ptrres[2]++;
						
					}
				}
				rest = res;

			}

			cv::Mat AumentarBrillo(cv::Mat actual, float escala, float centro) {
				cv::Mat originbw;
				cv::cvtColor(actual, originbw, CV_BGR2GRAY);
				std::vector<cv::Mat> c2;
				c2.push_back(cv::Mat());
				c2.push_back(cv::Mat());
				c2.push_back(cv::Mat());
				cv::split(actual, c2);
				c2[0].convertTo(c2[0], CV_32F);
				c2[1].convertTo(c2[1], CV_32F);
				c2[2].convertTo(c2[2], CV_32F);
				cv::Mat mt2;
				originbw.convertTo(mt2, CV_32F);
				c2[0] = c2[0] - mt2;
				c2[1] = c2[1] - mt2;
				c2[2] = c2[2] - mt2;
				cv::Mat mt = cv::Mat(actual.rows, actual.cols, mt2.type(), cv::Scalar(centro, centro, centro));
				mt2 = (mt2 - mt)*escala + mt;
				c2[0] = mt2 + c2[0];
				c2[1] = mt2 + c2[1];
				c2[2] = mt2 + c2[2];
				cv::Mat mt3;
				cv::merge(c2, mt3);
				mt3.convertTo(mt3, CV_8UC3);
				return mt3;
			}
			cv::Mat AumentarColor(cv::Mat actual, float escala) {
				cv::Mat originbw;
				cv::cvtColor(actual, originbw, CV_BGR2GRAY);
				std::vector<cv::Mat> c2;
				c2.push_back(cv::Mat());
				c2.push_back(cv::Mat());
				c2.push_back(cv::Mat());
				cv::split(actual, c2);
				c2[0].convertTo(c2[0], CV_32F);
				c2[1].convertTo(c2[1], CV_32F);
				c2[2].convertTo(c2[2], CV_32F);
				cv::Mat mt2;
				originbw.convertTo(mt2, CV_32F);
				c2[0] = c2[0] - mt2;
				c2[1] = c2[1] - mt2;
				c2[2] = c2[2] - mt2;
				c2[0] *= escala;
				c2[1] *= escala;
				c2[2] *= escala;
				c2[0] = mt2 + c2[0];
				c2[1] = mt2 + c2[1];
				c2[2] = mt2 + c2[2];
				cv::Mat mt3;
				cv::merge(c2, mt3);
				mt3.convertTo(mt3, CV_8UC3);
				return mt3;
			}
			cv::Mat ColorBlurFP(cv::Mat actual, cv::Size tam) {
				cv::Mat originbw;
				cv::cvtColor(actual, originbw, CV_BGR2GRAY);
				std::vector<cv::Mat> c2;
				c2.push_back(cv::Mat());
				c2.push_back(cv::Mat());
				c2.push_back(cv::Mat());
				cv::split(actual, c2);
				c2[0].convertTo(c2[0], CV_32F);
				c2[1].convertTo(c2[1], CV_32F);
				c2[2].convertTo(c2[2], CV_32F);
				cv::Mat mt2;
				originbw.convertTo(mt2, CV_32F);
				c2[0] = c2[0] - mt2;
				c2[1] = c2[1] - mt2;
				c2[2] = c2[2] - mt2;
				/*cv::imshow("w1",c2[0]*4/255);
				cv::imshow("w2",c2[1]*4/255);
				cv::imshow("w3",c2[2]*4/255);*/
				/*cv::GaussianBlur(c2[0],c2[0],tam,0);
				cv::GaussianBlur(c2[1],c2[1],tam,0);
				cv::GaussianBlur(c2[2],c2[2],tam,0);*/
				cv::blur(c2[0], c2[0], tam, cv::Point(tam.width / 2, tam.height / 2));
				cv::blur(c2[1], c2[1], tam, cv::Point(tam.width / 2, tam.height / 2));
				cv::blur(c2[2], c2[2], tam, cv::Point(tam.width / 2, tam.height / 2));
				/*cv::medianBlur(c2[0],c2[0],tam.width);
				cv::medianBlur(c2[1],c2[1],tam.width);
				cv::medianBlur(c2[2],c2[2],tam.width);*/
				/*cv::imshow("x1",c2[0]*4/255);
				cv::imshow("x2",c2[1]*4/255);
				cv::imshow("x3",c2[2]*4/255);*/
				/*cv::medianBlur(c2[0],c2[0],tam.width);
				cv::medianBlur(c2[1],c2[1],tam.width);
				cv::medianBlur(c2[2],c2[2],tam.width);*/
				c2[0] = mt2 + c2[0];
				c2[1] = mt2 + c2[1];
				c2[2] = mt2 + c2[2];
				cv::Mat mt3;
				cv::merge(c2, mt3);
				mt3.convertTo(mt3, CV_8UC3);
				return mt3;
			}
			cv::Mat ColorBlur(cv::Mat actual, cv::Size tam) {
				cv::Mat originbw;
				cv::cvtColor(actual, originbw, CV_BGR2GRAY);
				std::vector<cv::Mat> c2;
				c2.push_back(cv::Mat());
				c2.push_back(cv::Mat());
				c2.push_back(cv::Mat());
				cv::split(actual, c2);
				c2[0].convertTo(c2[0], CV_16S);
				c2[1].convertTo(c2[1], CV_16S);
				c2[2].convertTo(c2[2], CV_16S);
				cv::Mat mt2;
				originbw.convertTo(mt2, CV_16S);
				c2[0] = c2[0] - mt2;
				c2[1] = c2[1] - mt2;
				c2[2] = c2[2] - mt2;
				/*cv::imshow("w1",c2[0]*4/255);
				cv::imshow("w2",c2[1]*4/255);
				cv::imshow("w3",c2[2]*4/255);*/
				/*cv::GaussianBlur(c2[0],c2[0],tam,0);
				cv::GaussianBlur(c2[1],c2[1],tam,0);
				cv::GaussianBlur(c2[2],c2[2],tam,0);*/
				cv::blur(c2[0], c2[0], tam, cv::Point(tam.width / 2, tam.height / 2));
				cv::blur(c2[1], c2[1], tam, cv::Point(tam.width / 2, tam.height / 2));
				cv::blur(c2[2], c2[2], tam, cv::Point(tam.width / 2, tam.height / 2));
				/*cv::medianBlur(c2[0],c2[0],tam.width);
				cv::medianBlur(c2[1],c2[1],tam.width);
				cv::medianBlur(c2[2],c2[2],tam.width);*/
				/*cv::imshow("x1",c2[0]*4/255);
				cv::imshow("x2",c2[1]*4/255);
				cv::imshow("x3",c2[2]*4/255);*/
				/*cv::medianBlur(c2[0],c2[0],tam.width);
				cv::medianBlur(c2[1],c2[1],tam.width);
				cv::medianBlur(c2[2],c2[2],tam.width);*/
				c2[0] = c2[0] + mt2;
				c2[1] = c2[1] + mt2;
				c2[2] = c2[2] + mt2;
				cv::Mat mt3;
				cv::merge(c2, mt3);
				mt3.convertTo(mt3, CV_8UC3);
				return mt3;
			}
			cv::Mat backdifcolor(cv::Mat background, cv::Mat origen, int threshold, int maxval, int tipothreshold) {

				cv::Mat r1 = background;//ColorBlur(background,cv::Size(65,65));
				cv::Mat r2 = origen;//ColorBlur(origen,cv::Size(65,65));
									//cv::Mat r1=background;
									//cv::Mat r2=origen;
				cv::Mat dif;
				cv::absdiff(r1, r2, dif);
				cv::Mat acumprod = cv::Mat::zeros(origen.rows, origen.cols, CV_32FC1);
				std::vector<cv::Mat> mat;
				mat.push_back(cv::Mat());
				mat.push_back(cv::Mat());
				mat.push_back(cv::Mat());
				cv::split(dif, mat);
				cv::accumulateSquare(mat[0], acumprod);
				cv::accumulateSquare(mat[1], acumprod);
				cv::accumulateSquare(mat[2], acumprod);
				cv::Mat dist;
				//cv::sqrt(acumprod,dist);
				cv::Mat back2 = acumprod>threshold*threshold;
				/*
				cv::Mat back;
				cv::copyMakeBorder(back2,back,5,5,5,5,cv::BORDER_REPLICATE,cv::Scalar(0));
				int lim1=back.rows-5;
				int lim2=back.cols-5;
				cv::Mat res=cv::Mat::zeros(back2.rows,back2.cols,CV_8UC1);
				for (int i=5;i<lim1;i++) {
				unsigned char *fila=back.ptr<unsigned char>(i);
				unsigned char *filasig=back.ptr<unsigned char>(i+1);
				unsigned char *filasig2=back.ptr<unsigned char>(i+2);
				unsigned char *filasig3=back.ptr<unsigned char>(i+3);
				unsigned char *filasig4=back.ptr<unsigned char>(i+4);
				unsigned char *filares=res.ptr<unsigned char>(i-5);
				for (int o=5;o<lim2;o++) {
				int suma=	fila[0]+fila[1]+fila[2]+fila[3]+fila[4]+
				filasig[0]+filasig[1]+filasig[2]+filasig[3]+filasig[4]+
				filasig2[0]+filasig2[1]+filasig2[2]+filasig2[3]+filasig2[4]+
				filasig3[0]+filasig3[1]+filasig3[2]+filasig3[3]+filasig3[4]+
				filasig4[0]+filasig4[1]+filasig4[2]+filasig4[3]+filasig4[4];
				if (suma>255*12) {
				*filares=255;
				}
				filares++;
				filasig2++;
				filasig3++;
				filasig4++;
				filasig++;
				fila++;
				}
				}*/
				/*cv::copyMakeBorder(back2,back,2,2,2,2,cv::BORDER_REPLICATE,cv::Scalar(0));
				int lim1=back.rows-2;
				int lim2=back.cols-2;
				cv::Mat res=cv::Mat::zeros(back2.rows,back2.cols,CV_8UC1);
				for (int i=2;i<lim1;i++) {
				unsigned char *filaant2=back.ptr<unsigned char>(i-2);
				unsigned char *filaant=back.ptr<unsigned char>(i-1);
				unsigned char *fila=back.ptr<unsigned char>(i);
				unsigned char *filasig=back.ptr<unsigned char>(i+1);
				unsigned char *filasig2=back.ptr<unsigned char>(i+2);
				unsigned char *filares=res.ptr<unsigned char>(i-2);
				for (int o=2;o<lim2;o++) {
				int suma=	filaant2[-2]+filaant2[-1]+filaant2[0]+filaant2[1]+filaant2[2]+
				filaant[-2]+filaant[-1]+filaant[0]+filaant[1]+filaant[2]+
				fila[-2]+fila[-1]+fila[0]+fila[1]+fila[2]+
				filasig[-2]+filasig[-1]+filasig[0]+filasig[1]+filasig[2]+
				filasig2[-2]+filasig2[-1]+filasig2[0]+filasig2[1]+filasig2[2];
				if (suma>255*12) {
				*filares=255;
				}
				filares++;
				filaant2++;
				filasig2++;
				filaant++;
				filasig++;
				fila++;
				}
				}*/
				/*cv::medianBlur(back2,back2,5);
				cv::Mat resdif;
				cv::absdiff(back2,res,resdif);
				cv::imshow("df",resdif);
				cv::imshow("G",back2);
				cv::imshow("F",res);
				//*/
				//cv::medianBlur(back,back,3);
				/*if (tipothreshold==cv::THRESH_BINARY_INV) {
				res=~res
				}*/
				cv::Mat res;
				cv::medianBlur(back2, res, 5);
				cv::threshold(res, res, 128, maxval, tipothreshold);
				return res;
			}
		}
	}
}