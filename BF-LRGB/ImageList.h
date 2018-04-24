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
#include "ImageList.h"
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {


struct ImageInformation {
	std::string description;
	cv::Mat image;
	bool temporary;
	bool shown;
	ImageInformation() {
		temporary=false;
		shown=false;
	}
	ImageInformation(const ImageInformation &c):description(c.description) {
		this->image=c.image;
		this->temporary=c.temporary;
		this->shown=c.shown;
	}
	~ImageInformation() {
		/*IImagen *img=imagen.Puntero();
		imagen.Detach();
		if (img!=NULL) {
			delete img;
		}*/
	}
	virtual const char *GetDescription() {
		return description.c_str();
	}
	virtual cv::Mat GetImage() {
		return image;
	}
};
class ImageList {	
	std::vector<ImageInformation> images;
public:
	ImageList() {
		images.reserve(100);//el multithreading puede causar errores si es que el tamaño del vector cambia y se obtiene una referencia a una posicion invalida de la memoria
	}
	virtual void Clear() {
		images.clear();
	}
	virtual int GetSize() {
		return (int)images.size();
	}
	virtual cv::Mat GetImage(int posicion) {
		return images[posicion].image;
	}
	virtual const char *GetDescription(int posicion) {
		return images[posicion].description.c_str();
	}
	virtual cv::Mat GetImage(const char *descripcion) {
		for (unsigned i=0;i<images.size();i++) {
			if (strncmp(images[i].GetDescription(),descripcion,1000)==0)
				return images[i].GetImage();
		}
		return cv::Mat();
	}
	cv::Mat normalize(cv::Mat res,bool clonar) {
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
		ImageInformation img;
		img.description=descripcion;
		cv::Mat conv;
		if (valmin==valmax)
			valmax++;
		float centro=(valmax+valmin)/2;
		float rango=(valmax-valmin);
		cv::Mat res=(mat-valmin)/rango;
		res=normalize(res,false);
		img.image=res;
		images.push_back(img);
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
	void Add(std::string descripcion,cv::Mat imagen,bool temporal) {
		ImageInformation img;
		img.description=descripcion;
		if (imagen.type()!=CV_8UC3) {
			img.image=normalize(imagen,true);
		} else {
			img.image=imagen;
		}
		img.temporary=temporal;
		images.push_back(img);
	}
	void Add(std::string descripcion, cv::Mat imagen) {
		Add(descripcion,imagen,false);
	}
	/*void Agregar(std::string descripcion,cv::Mat imagen) {
		Agregar(descripcion,imagen,false);
	}*/
	void show(std::string title,int mode) {
		for (unsigned int i=0;i<images.size();i++) {
			std::string desc=title+" - "+images[i].description;
			cv::namedWindow(desc,mode);
			cv::imshow(desc,images[i].image);
			images[i].shown=true;
		}
	}
	void store(std::string title) {
		for (unsigned int i=0;i<images.size();i++) {
			std::string desc=title+" - "+images[i].description+".png";
			cv::Mat mt=images[i].image;
			if (images[i].image.type()==CV_32FC3) {
				images[i].image.convertTo(mt,CV_8UC3,128,128);
			}
			if (images[i].image.type()==CV_32FC1) {
				images[i].image.convertTo(mt,CV_8UC1,128,128);
			}
			cv::imwrite(desc,mt);
		}
	}
	virtual ~ImageList() {
		for (unsigned int i=0;i<images.size();i++) {
			if (images[i].shown&&images[i].temporary) {
				cv::destroyWindow(images[i].description);
			}
		}
	}
};
		}
	}
}