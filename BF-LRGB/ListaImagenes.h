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
#include <string>
#include "opencv2\opencv.hpp"
#include "ListaImagenes.h"
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {


struct InfoImagen {
	std::string descripcion;
	cv::Mat imagen;
	bool temporal;
	bool mostrada;
	InfoImagen() {
		temporal=false;
		mostrada=false;
	}
	InfoImagen(const InfoImagen &c):descripcion(c.descripcion) {
		this->imagen=c.imagen;
		this->temporal=c.temporal;
		this->mostrada=c.mostrada;
	}
	~InfoImagen() {
		/*IImagen *img=imagen.Puntero();
		imagen.Detach();
		if (img!=NULL) {
			delete img;
		}*/
	}
	virtual const char *ObtenerDescripcion() {
		return descripcion.c_str();
	}
	virtual cv::Mat ObtenerImagen() {
		return imagen;
	}
};
class ListaImagenes {	
	std::vector<InfoImagen> imagenes;
public:
	ListaImagenes() {
		imagenes.reserve(100);//el multithreading puede causar errores si es que el tamaño del vector cambia y se obtiene una referencia a una posicion invalida de la memoria
	}
	virtual void Limpiar() {
		imagenes.clear();
	}
	virtual int ObtenerNumImagenes() {
		return (int)imagenes.size();
	}
	virtual cv::Mat ObtenerImagen(int posicion) {
		return imagenes[posicion].imagen;
	}
	virtual const char *ObtenerDescripcion(int posicion) {
		return imagenes[posicion].descripcion.c_str();
	}
	virtual cv::Mat ObtenerImagen(const char *descripcion) {
		for (unsigned i=0;i<imagenes.size();i++) {
			if (strncmp(imagenes[i].ObtenerDescripcion(),descripcion,1000)==0)
				return imagenes[i].ObtenerImagen();
		}
		return cv::Mat();
	}
	cv::Mat normalizar(cv::Mat res,bool clonar) {
		cv::Mat resultado;
		if (res.channels()!=3) {
			std::vector<cv::Mat> vc;
			if ((res.type()&CV_8U)==0) {
				res.convertTo(resultado,CV_8U,256);
				vc.push_back(resultado);
				vc.push_back(resultado);
				vc.push_back(resultado);			
			} else {
				vc.push_back(res);vc.push_back(res);vc.push_back(res);
			}
			cv::merge(vc,resultado);
		} else {
			res.convertTo(resultado,CV_8U,256);
		}
		/*if (res.type()&CV_64F!=0||res.type()&CV_32F!=0) {
			resultado=res*256;
		} 
		if (res.channels()!=3) {
			if (res.type()&CV_8U==0) {
				res.convertTo(res,CV_8U);
			}
			std::vector<cv::Mat> vc;
			vc.push_back(res);
			vc.push_back(res);
			vc.push_back(res);
			cv::Mat res2;
			cv::merge(vc,res2);
			res=res2;
		} else {
			res.convertTo(res,CV_8U);
		}*/
		return resultado;
	}
	/*void Agregar32F(std::string descripcion, cv::Mat imagen, float valmin, float valmax) {
		InfoImagen img;
		img.descripcion=descripcion;
		cv::Mat conv;
		if (valmin==valmax)
			valmax++;
		float centro=(valmax+valmin)/2;
		float rango=(valmax-valmin);
		cv::Mat res=(imagen-valmin)/rango;
		res=normalizar(res,true);
		img.imagen=res;
		imagenes.push_back(img);
	}
	*/
	void Add32F(std::string descripcion,cv::Mat mat,float valmin,float valmax) {
		InfoImagen img;
		img.descripcion=descripcion;
		cv::Mat conv;
		if (valmin==valmax)
			valmax++;
		float centro=(valmax+valmin)/2;
		float rango=(valmax-valmin);
		cv::Mat res=(mat-valmin)/rango;
		res=normalizar(res,false);
		img.imagen=res;
		imagenes.push_back(img);
	}
	/*void Agregar(std::string descripcion,cv::Mat imagen,bool temporal) {
		InfoImagen img;
		img.descripcion=descripcion;
		if (imagen.type()!=CV_8UC3) {
			cv::Mat img2=normalizar(imagen,true);
			img.imagen=img2;
		} else
			img.imagen=imagen;
		img.temporal=temporal;
		imagenes.push_back(img);
	}*/
	void Agregar(std::string descripcion,cv::Mat imagen,bool temporal) {
		InfoImagen img;
		img.descripcion=descripcion;
		if (imagen.type()!=CV_8UC3) {
			img.imagen=normalizar(imagen,true);
		} else {
			img.imagen=imagen;
		}
		img.temporal=temporal;
		imagenes.push_back(img);
	}
	void Add(std::string descripcion, cv::Mat imagen) {
		Agregar(descripcion,imagen,false);
	}
	/*void Agregar(std::string descripcion,cv::Mat imagen) {
		Agregar(descripcion,imagen,false);
	}*/
	void mostrar(std::string title,int mode) {
		for (unsigned int i=0;i<imagenes.size();i++) {
			std::string desc=title+" - "+imagenes[i].descripcion;
			cv::namedWindow(desc,mode);
			cv::imshow(desc,imagenes[i].imagen);
			imagenes[i].mostrada=true;
		}
	}
	void guardar(std::string title) {
		for (unsigned int i=0;i<imagenes.size();i++) {
			std::string desc=title+" - "+imagenes[i].descripcion+".png";
			cv::Mat mt=imagenes[i].imagen;
			if (imagenes[i].imagen.type()==CV_32FC3) {
				imagenes[i].imagen.convertTo(mt,CV_8UC3,128,128);
			}
			if (imagenes[i].imagen.type()==CV_32FC1) {
				imagenes[i].imagen.convertTo(mt,CV_8UC1,128,128);
			}
			cv::imwrite(desc,mt);
		}
	}
	virtual ~ListaImagenes() {
		for (unsigned int i=0;i<imagenes.size();i++) {
			if (imagenes[i].mostrada&&imagenes[i].temporal) {
				cv::destroyWindow(imagenes[i].descripcion);
			}
		}
	}
};
		}
	}
}