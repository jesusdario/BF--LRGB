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
extern double valorInterCoef;
extern double valorDistancia;
extern double valorProyec;
extern float limdetforeground;
extern bool deteccionsombras;
extern bool conoperacionclose;
extern bool quitarartefactosilum;
//cv::Mat QuitarPuntosAislados(cv::Mat mat);

namespace DR {
namespace SoporteBase {
namespace Imagenes {
	void MedianBlur5(cv::Mat mat,cv::Mat& rest,unsigned char val);
	void QuitarPuntosAislados2(cv::Mat mat, cv::Mat& rest);
	cv::Mat ObtenerMascaraMorph(int ancho);
	
class BuscadorObjetos21 {
	cv::Mat foreground;
	cv::Mat sombras;
public:
	virtual cv::Mat ObtenerForeground() {
		return foreground;
	}
	virtual cv::Mat ObtenerSombras() {
		return sombras;
	}
	inline bool distcoef2(const double dist1, const double dist2, const double dist3, const double vintercoef) {
		return distcoefbak(std::abs(dist1 - dist2), std::abs(dist1 - dist3), std::abs(dist2 - dist3), vintercoef);
	}
	inline bool distcoefentre(const double dist1, const double dist2, const double dist3, const double menor, const double mayor) {
		return (dist1 >= menor&&dist1 <= mayor) && (dist2 >= menor&&dist2 <= mayor) && (dist3 >= menor&&dist3 <= mayor);
	}
	inline bool distcoefbak(const double dist1, const double dist2, const double dist3, const double vintercoef) {
		if (dist1 < vintercoef&&dist2 < vintercoef&&dist3 < vintercoef) {
			return true;
		}
		return false;
	}
	inline bool distcoef(const double dist1, const double dist2, const double dist3, const double vintercoef) {
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
	inline bool comparardist(const double dist1, const double dist2, const double delta) {
		if (dist1 < dist2) {
			return (dist1 / dist2) < delta;
		}
		else
			return (dist2 / dist1) < delta;
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
	inline bool menor1mayor2(double dist, double dist2, double dist3, double distmenor, double distmayor) {
		if (dist < distmenor&&dist2 < distmayor&&dist3 < distmayor)
			return true;
		if (dist2 < distmenor&&dist < distmayor&&dist3 < distmayor)
			return true;
		if (dist3 < distmenor&&dist < distmayor&&dist2 < distmayor)
			return true;
		return false;
	}
	virtual bool signoigual(double a, double b, double c) {
		return std::signbit(a) == std::signbit(b) && std::signbit(b) == std::signbit(c);
	}
	virtual cv::Mat CalcularLuminosidad(cv::Mat fondo) {
		cv::Mat brightness(fondo.rows, fondo.cols, CV_32FC3);
		const unsigned int valblue = (unsigned int)(0.114f * 256 * 256 * 256); //1912603;//0.114f*256*256*256;
		const unsigned int valgreen = (unsigned int)(0.587f * 256 * 256 * 256); ;// 9848226;//0.587f*256*256*256;
		const unsigned int valred = (unsigned int)(0.299f * 256 * 256 * 256); //5016388;//0.299f*256*256*256;
		for (int i = 0;i<fondo.rows;i++) {
			unsigned char *ptrfondo = fondo.ptr<unsigned char>(i);
			float *ptrbrightness = brightness.ptr<float>(i);
			for (int o = 0;o<fondo.cols;o++) {
				unsigned int color = (((unsigned int)ptrfondo[0])*valblue +
					((unsigned int)ptrfondo[1])*valgreen +
					((unsigned int)ptrfondo[2])*valred) >> 24;
				ptrbrightness[2] = ptrbrightness[1] = ptrbrightness[0] = color;
				ptrfondo += 3;
				ptrbrightness += 3;
			}
		}
		return brightness;
	}
	cv::Mat mascaraopmorf;
	virtual void EncontrarBlobs(cv::Mat imagenactual,GeneradorFondo22 *generador,bool agregarimagenes,ListaImagenes &listaimagenes) {
		if (!generador->ConDatos()) return;
		bool quitarsombras= quitarartefactosilum;
					
		cv::Mat origen=imagenactual.clone();
		cv::Mat fondo=generador->ObtenerFondoActual().clone();
		
		cv::Mat k2(origen.rows,origen.cols,CV_32FC3);
		const int limptr=origen.cols*3;
		const int delta=limptr/4;
		const int resto=limptr%4;		
		for (int i=0;i<origen.rows;i++) {
			unsigned char *ptrorigen=origen.ptr<unsigned char>(i);
			unsigned char *ptrfondo=fondo.ptr<unsigned char>(i);
			float *ptrk2=k2.ptr<float>(i);
			float *ptrfondof=k2.ptr<float>(i);
			for (int o=0;o<limptr;o++) {
				ptrk2[0]=ptrorigen[0]-(float)ptrfondo[0];
				ptrk2++;
				ptrfondo++;
				ptrorigen++;
			}
		}
		cv::Mat k1=CalcularLuminosidad(fondo);//2.3ms 800x600
		cv::Mat constante(fondo.rows,fondo.cols,CV_32FC3);
		//constante = k2 / k1; TIEMPO: 3.7ms
		constante = k2 / k1;
/*		for (int i = 0; i < constante.rows; i++) {
			float *ptrk2 = k2.ptr<float>(i);
			float *ptrk1 = k1.ptr<float>(i);
			float *ptrconstante = constante.ptr<float>(i);
			for (int o = 0; o<limptr; o++) {
#if SININCREMENTODENOM
				*ptrconstante = *ptrk2 / (*ptrk1);
#else
				*ptrconstante = *ptrk2 / (*ptrk1 + deltaincr);
#endif
				ptrconstante++;
				ptrk1++;
				ptrk2++;
			}
		}*/

		//ALG: 3.7ms;
		/*
		__m128 constmat;
		const float deltaincr=0.1f;
		constmat.m128_f32[0]=deltaincr;
		constmat.m128_f32[1]=deltaincr;
		constmat.m128_f32[2]=deltaincr;
		constmat.m128_f32[3]=deltaincr;
		for (int i=0;i<constante.rows;i++) {
			float *ptrk2=k2.ptr<float>(i);
			float *ptrk1=k1.ptr<float>(i);
			float *ptrconstante=constante.ptr<float>(i);
			
			
			for (int o = 0;o < delta;o++) {
#if SININCREMENTODENOM
				_mm_storeu_ps(ptrconstante, _mm_div_ps(_mm_loadu_ps(ptrk2), _mm_loadu_ps(ptrk1)));
#else
				_mm_storeu_ps(ptrconstante, _mm_div_ps(_mm_loadu_ps(ptrk2), _mm_add_ps(_mm_loadu_ps(ptrk1), constmat)));
#endif
				ptrk1 += 4;
				ptrk2 += 4;
				ptrconstante += 4;
			}
			for (int o=delta*4;o<limptr;o++) {
#if SININCREMENTODENOM
				*ptrconstante = *ptrk2 / (*ptrk1) ;
#else
				*ptrconstante=*ptrk2/(*ptrk1+deltaincr);
#endif
				ptrconstante++;
				ptrk1++;
				ptrk2++;				
			}
		}*/
		cv::Mat cambios(constante.rows,constante.cols,CV_8UC1);
		const float i1=(float)((1-limdetforeground)-1);
		const float i2=(float)((1+limdetforeground)-1);
		
		for (int i=0;i<constante.rows;i++) {
			float *resconst=constante.ptr<float>(i);
			unsigned char *rescambios=cambios.ptr<unsigned char>(i);
			for (int o=0;o<constante.cols;o++) {
				*rescambios=!((resconst[0]>=i1&&resconst[0]<=i2)&&
					(resconst[1]>=i1&&resconst[1]<=i2)&&
					(resconst[2]>=i1&&resconst[2]<=i2))
					;
				resconst+=3;
				rescambios++;
			}
		}
		
		cv::Mat sq=constante;
		cv::Mat delta1(sq.rows,sq.cols,CV_8UC1);
		const int limcol=sq.cols;
		const float thresh=(float)(0.8f);//0.8 es ok, sino detecta color cabello como sombra
		const float consttresh=valorDistancia*valorDistancia;//thresh*thresh;
		const float ang=std::cos(3.14151692/4);
		const float coefinter=valorInterCoef;
		const float coefproy1=valorProyec;
		const float coefproy2=-valorProyec;
		const int valorartefactoilum = 1;
		const int valforeground = 0;

		for (int i=0;i<sq.rows;i++) {
			float *ptr=sq.ptr<float>(i);
			unsigned char *ptrcambios=cambios.ptr<unsigned char>(i);
			unsigned char *ptrres=delta1.ptr<unsigned char>(i);
			unsigned char *ptrorigen = origen.ptr<unsigned char>(i);
			unsigned char *ptrfondo = fondo.ptr<unsigned char>(i);
				
			for (int o=0;o<limcol;o++) {	
				float longitud=ptr[0]*ptr[0]+ptr[1]*ptr[1]+ptr[2]*ptr[2];
				float proyeccion=ptr[0]+ptr[1]+ptr[2];
				if ((*ptrcambios)) {
					if (longitud < consttresh &&
						distcoef(std::abs(ptr[1] - ptr[0]), std::abs(ptr[2] - ptr[0]), std::abs(ptr[2] - ptr[1]), coefinter)
						&&
						(proyeccion >= coefproy2 && proyeccion <= coefproy1)
						) {
						*ptrres = valorartefactoilum; //Artefacto de iluminaci¨®n o sombra
					}
					else {							
						*ptrres = valforeground;						
					}
				}
				else {
					*ptrres = valforeground;
				}
					
				ptr += 3;
				ptrres++;
				ptrcambios++;
				ptrorigen += 3;
				ptrfondo += 3;
			}
		}
		
		cv::Mat sombrac=delta1;
		if (quitarsombras) {
			cambios-=sombrac;
		}
		sombras=sombrac;
		if (agregarimagenes) {
			listaimagenes.Agregar32F("Foreground_Constante Diferencia [ki]",constante,-1,1);
			listaimagenes.Agregar32F("Foreground_Sombra/efecto iluminacion",sombras,-1,1);			
		}
		cv::Mat res;
		MedianBlur5(cambios,cambios,1);		//1.5ms (800x600)
		QuitarPuntosAislados2(cambios, cambios);//2.5ms (800x600)
		
		if (conoperacionclose) {
			if (mascaraopmorf.rows == 0)
				mascaraopmorf = ObtenerMascaraMorph( 10 * origen.rows / 600.0);
			_mm_prefetch((const char *)mascaraopmorf.data, _MM_HINT_T1);			
			cv::morphologyEx(cambios,res,cv::MORPH_CLOSE,mascaraopmorf,cv::Point(mascaraopmorf.rows / 2, mascaraopmorf.cols/2));//2.8 ms		
		}
		else {
			res = cambios;
		}
		
		foreground=res;				
		if (agregarimagenes) {
			listaimagenes.Agregar("Foreground_Foreground",foreground*255);
			std::vector<cv::Mat> comb;
			comb.push_back(foreground);
			comb.push_back(foreground);
			comb.push_back(foreground);
			cv::Mat mascaracomb;
			cv::merge(comb, mascaracomb);
			cv::Mat resmult;
			cv::multiply(mascaracomb, imagenactual,resmult);
			listaimagenes.Agregar("Foreground_Foreground segmentado", resmult);
		}

	}
	
};
}
}
}