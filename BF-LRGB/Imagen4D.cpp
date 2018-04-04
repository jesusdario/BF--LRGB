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
#include "Imagen4D.h"
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {

			cv::Mat EscalarBrilloYColor(cv::Mat origen, float escalabrillo, float escalacolor) {
				Imagen4D imagen(origen);
				imagen.ScaleLightness(escalabrillo, 128.0f);
				imagen.ScaleColor(escalacolor);
				cv::Mat res = imagen.GetImage();
				return res;
			}
			cv::Mat EscalarBrillo(cv::Mat origen, float escala) {
				Imagen4D imagen(origen);
				imagen.ScaleLightness(escala, 128.0f);
				cv::Mat res = imagen.GetImage();
				return res;
			};
			cv::Mat EscalarColor(cv::Mat origen, float escala) {
				Imagen4D imagen(origen);
				imagen.ScaleColor(escala);
				cv::Mat res = imagen.GetImage();
				return res;
			};

		}
	}
}