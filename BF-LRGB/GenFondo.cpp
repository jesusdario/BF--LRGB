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
#include "Preprocessor.h"
#include "BackgroundGenerator.h"
#include "ForegroundDetector.h"//Res. actuales
//#include "BuscadorObjetos22.h"
//#define BuscadorObjetos21 BuscadorObjetos22

#include "LRGBImage.h"
//#include "GeneradorFondo23.h"
//#define GeneradorFondo22 GeneradorFondo23

bool consuavizado = true;
double thetaThresholdProbabilityTrue = 0.9;
double etaThresholdProbabilityFalse = 0.3;
int xiThresholdNumberOfTrueEvents = 25;
int rhoThresholdNumberOfFalseEvents = 25;

double gammaValue = 0.9;
double psivalue = 0.9;
double betavalue = 1000;

double chiIntercoefficientDistance = 0.25;
double zetaValue = 0.8;
double tauSqrt3Value = 1.0;
bool deteccionsombras = false;
bool withoperationclose = false;

float deltabackground = 0.05;
float limdetforeground = 0.09;

bool agregarimagenes = true;
bool removeIlluminationArtifacts = true;

/*
int trackingvelestimada = 10;
int trackingveltotal = 12;*/
bool detectSuddenChangesInBackground = true;
double thresholdSuddenChanges = 0.10;
int numberOfAveragedFrames = 25;
bool connormalizacionLRGB = false;


//NUEVO PARAMETRO
int nummaxfondos = 3;



#include "VideoSource.h"
#include <fstream>
#include <string>
#include "ListaImagenes.h"
using namespace DR::SoporteBase::Imagenes;


template<class T,class Info> 
class ColaSinc {
	CRITICAL_SECTION hCrit;
	std::vector<T> mats;
public:
	Info info;
	bool salir;
	ColaSinc() {
		ZeroMemory(&hCrit, sizeof(hCrit));
		InitializeCriticalSection(&hCrit);
		salir = false;
	}
	~ColaSinc() {
		DeleteCriticalSection(&hCrit);
	}
	int ObtenerTam() {
		return (int)mats.size();
	}
	void Agregar(T elem) {
		EnterCriticalSection(&hCrit);
		mats.push_back(elem);
		LeaveCriticalSection(&hCrit);
	}
	T Obtener() {
		while (mats.size() == 0)
			Sleep(1);
		EnterCriticalSection(&hCrit);
		T m = mats[0];
		mats.erase(mats.begin());
		LeaveCriticalSection(&hCrit);
		return m;
	}
};
struct InfoMat {
	cv::Mat imagen;
	cv::Mat gt;
	bool gtfin;
	int index;
	int gtindexarchivo;
	int gtindex;
	InfoMat(cv::Mat img, cv::Mat gt,bool gtfin,int index,int gtindex,int gtindexarchivo) {
		this->imagen = img;
		this->gt = gt;
		this->gtfin = gtfin;
		this->index = index;
		this->gtindex = gtindex;
		this->gtindexarchivo = gtindexarchivo;
	}
};

bool terminarengt = false;

DWORD WINAPI Tarea(LPVOID param) {
	ColaSinc <InfoMat, VideoSourceDirectory*> *cl= (ColaSinc<InfoMat, VideoSourceDirectory*>*)param;
	while (true) {
		if (cl->salir)
			break;
		while (cl->ObtenerTam() > 20) {
			Sleep(1);
		}
		cv::Mat image, gt;
		int index,gtindex, gtindexarchivo;
		image = cl->info->GetNextImage(gt,&index,&gtindex, &gtindexarchivo);
		bool gtfin = cl->info->GroundTruthFinished();
		gtfin = terminarengt ? gtfin:false;//false;//Considerar todos los marcos (para training)
		cl->Agregar(InfoMat(image, gt,gtfin,index,gtindex,gtindexarchivo));
		if (image.empty()||gtfin)
			break;		
	}
	return 0;
}
HANDLE CrearTareaLectura(ColaSinc<InfoMat, VideoSourceDirectory*>*cl) {
	HANDLE hTareaLectura = INVALID_HANDLE_VALUE;
	DWORD id = 0;
	hTareaLectura = CreateThread(NULL, 1000000, &Tarea, cl, 0, &id);
	return hTareaLectura;
}
std::string ObtenerNumero(int numero, int numceros) {
	char buf2[100];
	sprintf_s(buf2, "%%0%ii", numceros);
	char buf[1000];
	sprintf_s(buf, buf2, numero);
	return std::string(buf);
}
cv::Mat PreprocesadoGTSABS(cv::Mat gt) {
	std::vector <cv::Mat> vec;
	cv::split(gt, vec);
	return vec[0]>0 | vec[1]>0 | vec[2]>0;
}
bool ProcesarGTSABS(VideoSourceDirectory *s, cv::Mat foreground,cv::Mat &gt,int iniciox,int finx,int inicioy,int finy,int &vtp,int &vtn, int &vfp, int &vfn) {
	gt=PreprocesadoGTSABS(gt);
	cv::Mat mascara = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::Mat erosionada;
	cv::erode(gt, erosionada, mascara);
	int vTP = 0, vTN = 0, vFP = 0, vFN = 0;
	for (int i = inicioy; i <= finy; i++) {
		unsigned char *pforeground = foreground.ptr<unsigned char>(i);
		unsigned char *pgroundtruth = gt.ptr<unsigned char>(i);
		unsigned char *per = erosionada.ptr<unsigned char>(i);
		//ERROR DE CALCULO CORREGIDO:
		pforeground += iniciox;
		pgroundtruth += iniciox;
		per += iniciox;
		for (int o = iniciox; o <= finx; o++) {
			if (*per == *pgroundtruth) {
				vTP += (*pforeground&*pgroundtruth) & 0xff ? 1 : 0;
				vTN += (~*pforeground&~*pgroundtruth) & 0xff ? 1 : 0;
				vFP += (*pforeground&~*pgroundtruth) & 0xff ? 1 : 0;
				vFN += (~*pforeground&*pgroundtruth) & 0xff ? 1 : 0;				
			}
			pforeground++;
			pgroundtruth++;
			per++;
		}
	}
	vtp = vTP;
	vtn = vTN;
	vfp = vFP;
	vfn = vFN;	
	return true;
}
cv::Mat PreprocesarGTChangeDetection(cv::Mat gt) {
	std::vector <cv::Mat> vec;
	cv::split(gt, vec);
	return vec[0];
}
bool ProcesarGTChangeDetection(VideoSourceDirectory *d,cv::Mat foreground, cv::Mat &gt, int iniciox, int finx, int inicioy, int finy, int &vtp, int &vtn, int &vfp, int &vfn) {
	gt = PreprocesarGTChangeDetection(gt);
	cv::Mat roi=d->GetROI();
	//cv::Mat mascara = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	int vTP = 0, vTN = 0, vFP = 0, vFN = 0;
	bool incluida = false;
	if (roi.rows > 0) {
		int deltac = roi.channels();
		for (int i = inicioy; i <= finy; i++) {
			unsigned char *ptrroi = roi.ptr<unsigned char>(i);
			unsigned char *pforeground = foreground.ptr<unsigned char>(i);
			unsigned char *pgroundtruth = gt.ptr<unsigned char>(i);
			ptrroi += iniciox;
			pforeground += iniciox;
			pgroundtruth += iniciox;
			for (int o = iniciox; o <= finx; o++) {
				if (*ptrroi) {
					if (*pgroundtruth == 0 || *pgroundtruth == 255 /*|| *pgroundtruth == 50*/) {
						unsigned char valgt = *pgroundtruth > 0 ? 255 : 0;
						vTP += (*pforeground&valgt) & 0xff ? 1 : 0;
						vTN += (~*pforeground&~valgt) & 0xff ? 1 : 0;
						vFP += (*pforeground&~valgt) & 0xff ? 1 : 0;
						vFN += (~*pforeground&valgt) & 0xff ? 1 : 0;
						incluida = true;
					}
				}
				pforeground++;
				pgroundtruth++;
				ptrroi += deltac;
			}
		}
	}
	else {
		for (int i = inicioy; i <= finy; i++) {
			unsigned char *pforeground = foreground.ptr<unsigned char>(i);
			unsigned char *pgroundtruth = gt.ptr<unsigned char>(i);
			pforeground += iniciox;
			pgroundtruth += iniciox;
			for (int o = iniciox; o <= finx; o++) {
				if (*pgroundtruth == 0 || *pgroundtruth == 255 /*|| *pgroundtruth == 50*/) {
					unsigned char valgt = *pgroundtruth > 0 ? 255 : 0;
					vTP += (*pforeground&valgt) & 0xff ? 1 : 0;
					vTN += (~*pforeground&~valgt) & 0xff ? 1 : 0;
					vFP += (*pforeground&~valgt) & 0xff ? 1 : 0;
					vFN += (~*pforeground&valgt) & 0xff ? 1 : 0;
					incluida = true;
				}
				pforeground++;
				pgroundtruth++;

			}
		}
	}
	vtp = vTP;
	vtn = vTN;
	vfp = vFP;
	vfn = vFN;
	return incluida;
}
bool ProcesarGTWallflower(VideoSourceDirectory *s,cv::Mat foreground, cv::Mat &gt, int iniciox, int finx, int inicioy, int finy, int &vtp, int &vtn, int &vfp, int &vfn) {
	gt = PreprocesarGTChangeDetection(gt);
	int vTP = 0, vTN = 0, vFP = 0, vFN = 0;
	for (int i = inicioy; i <= finy; i++) {
		unsigned char *pforeground = foreground.ptr<unsigned char>(i);
		unsigned char *pgroundtruth = gt.ptr<unsigned char>(i);
		pforeground += iniciox;
		pgroundtruth += iniciox;
		for (int o = iniciox; o <= finx; o++) {
			unsigned char valgt = *pgroundtruth > 0?255:0;
			unsigned char valforeground = (*pforeground) > 0 ? 255 : 0;
			vTP += (valforeground&valgt) &0xff? 1 : 0;
			vTN += (~valforeground&~valgt) & 0xff ? 1 : 0;
			vFP += (valforeground&~valgt) & 0xff ? 1 : 0;
			vFN += (~valforeground&valgt) & 0xff ? 1 : 0;
			pforeground++;
			pgroundtruth++;
		}
	}
	vtp = vTP;
	vtn = vTN;
	vfp = vFP;
	vfn = vFN;
	return true;
}

typedef bool(*FuncProc)(VideoSourceDirectory *dir,cv::Mat foreground, cv::Mat &gt, int iniciox, int finx, int inicioy, int finy, int &vtp, int &vtn, int &vfp, int &vfn);
class InfoVideo {
public:
	std::string descripcion;
	std::string video;
	std::string mascaratraining;
	int iniciox, finx, inicioy, finy;
	int deltagt;
	std::string salida;

	bool AnalizarImagenes(ColaSinc<InfoMat, VideoSourceDirectory*> &cl, DR::SoporteBase::Imagenes::Preprocessor *p, DR::SoporteBase::Imagenes::ForegroundDetector *bb, DR::SoporteBase::Imagenes::BackgroundGenerator * gf, __int64 tp[], __int64 tn[], __int64 fp[], __int64 fn[], ListaImagenes &lista, int *nframes, int nframetotal, FuncProc procgt,bool escribirresultados, std::string prefijo, int numceros,std::string descripcion="",std::string salidamuestra="", bool guardarimagengt=false, int numframeguardar=-1) {
		VideoSourceDirectory &d = *cl.info;
		cv::Mat gt, image;
		InfoMat info = cl.Obtener();
		int index = nframetotal;
		image = info.imagen;
		gt = info.gt;
		bool gtfin = info.gtfin;
		if (image.empty())
			return true;
		int iniciox = this->iniciox;
		int finx = this->finx;
		int inicioy = this->inicioy;
		int finy = this->finy;
		if (iniciox < 0) iniciox = 0;
		if (finx < 0) finx = image.cols - 1;
		if (inicioy < 0) inicioy = 0;
		if (finy < 0) finy = image.rows - 1;
		cv::Mat imgentrada = image;
		bool salir = false;
		cv::Mat img = p->Preprocesar(imgentrada,&salir);
		if (salir)
			return true;
		cv::Mat foreground;

		//cv::Mat input, output, background;
		cv::Mat output, background;
		//bgs[i]->process(image, output, background);//input, output, background);			
		
		Timer t;
		t.Start();
		gf->AddImage(img, agregarimagenes, lista);
		bb->FindForeground(img, gf, agregarimagenes, lista);
		t.Stop();
		double time = t.GetTime();
		if (nframetotal % 100 == 0) {
			std::cout << time << "\r";
		}
		if (bb->GetForeground().rows==0) {
			if (escribirresultados) {
				cv::imwrite(salida + "\\" + prefijo + ObtenerNumero(index, numceros) + ".png", cv::Mat::zeros(image.rows, image.cols, CV_8UC1));
			}
			return false;
		}
		for (int i = 0; i < lista.ObtenerNumImagenes(); i++) {
			cv::imshow(lista.ObtenerDescripcion(i), lista.ObtenerImagen(i));
		}
		
		output = bb->GetForeground();
		background = gf->GetCurrentBackground();
		if (guardarimagengt&&gt.rows != 0) {
			cv::imwrite(salidamuestra + "\\" + descripcion + "(entrada)" + ObtenerNumero(index, numceros) + ".png", imgentrada);
			for (int i = 0; i < lista.ObtenerNumImagenes(); i++) {
				cv::imwrite(salidamuestra + "\\" + descripcion+"("+lista.ObtenerDescripcion(i)+ ")"+ObtenerNumero(index, numceros) + ".png", lista.ObtenerImagen(i));
			}
			if (gt.rows!=0)
				cv::imwrite(salidamuestra + "\\" + descripcion + "(groundtruth)"+ObtenerNumero(index, numceros) + ".png", gt);
			cv::Mat imarch = bb->GetForeground() * 255;
			cv::imwrite(salidamuestra+"\\"+descripcion + ObtenerNumero(index, numceros) + ".png", imarch);
		}
		if (nframetotal == numframeguardar) {
			cv::imwrite(salidamuestra + "\\" + descripcion + "(entrada)" + ObtenerNumero(index, numceros) + ".png", imgentrada);
			for (int i = 0; i < lista.ObtenerNumImagenes(); i++) {
				cv::imwrite(salidamuestra + "\\" + descripcion + "(" + lista.ObtenerDescripcion(i) + ")" + ObtenerNumero(index, numceros) + ".png", lista.ObtenerImagen(i));
			}
			if (gt.rows != 0)
				cv::imwrite(salidamuestra + "\\" + descripcion + "(groundtruth)" + ObtenerNumero(index, numceros) + ".png", gt);
			cv::Mat imarch = bb->GetForeground() * 255;
			cv::imwrite(salidamuestra+"\\"+descripcion + ObtenerNumero(index, numceros) + ".png", imarch);
		}
		lista.Limpiar();
		output *= 255;
		if (escribirresultados) {
			cv::imwrite(salida + "\\" + prefijo + ObtenerNumero(index, numceros) + ".png", output);
		}
		if (agregarimagenes) {
			cv::imshow("Img", image);
			cv::imshow("Foreground", output);
			cv::imshow("Background", background);
		}
		if (!gt.empty()&&agregarimagenes) {
			cv::imshow("GT", gt);
		}
		else {

		}
		if (!output.empty()) {
			if (!gt.empty()) {
				cv::Mat foreground1, foreground;
				cv::threshold(output, foreground1, 250, 255, cv::THRESH_BINARY);
				if (foreground1.channels() == 3) {
					std::vector<cv::Mat> m;
					cv::split(foreground1, m);
					m[0].convertTo(foreground, CV_8UC1);
				}
				else {
					foreground1.convertTo(foreground, CV_8UC1);
				}
				cv::Mat mTP, mTN, mFP, mFN;
				int vTP = 0, vTN = 0, vFP = 0, vFN = 0;
				bool res = procgt(cl.info, foreground, gt, iniciox, finx, inicioy, finy, vTP, vTN, vFP, vFN);
				if (res) {
					if (agregarimagenes) {
						std::vector<cv::Mat> vc;
						cv::split(gt, vc);
						gt = (~(vc[0] == 0))&(vc[0] == 255);
						mTP = foreground&gt;
						//mTN = ~foreground&~gt;
						mFP = foreground&~gt;
						mFN = ~foreground&gt;
						std::vector<cv::Mat> vecmed;
						vecmed.push_back(mTP);
						vecmed.push_back(mFP);
						vecmed.push_back(mFN);
						cv::Mat vis;
						cv::merge(vecmed, vis);
						char buf[1000];
						sprintf_s(buf, "%i", *nframes);
						cv::putText(vis, buf, cv::Point(5, 15), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
						cv::imshow("VIS", vis);
					}
					tp[0] += vTP;//cv::countNonZero(mTP);
					tn[0] += vTN;//cv::countNonZero(mTN);
					fp[0] += vFP;//cv::countNonZero(mFP);
					fn[0] += vFN;//cv::countNonZero(mFN);
					if (nframes != NULL)
						(*nframes)++;
				}
			}
		}
		if (gtfin)
			return true;
		cv::waitKey(1);
		return false;
	}

};
class DatosVideos {
	std::vector<InfoVideo> videos;
public:
	std::string mascaravideo;
	std::string mascaragt;
	std::string salidaprefijo;
	bool utilizarvideostraining;
	bool salidaescribir;
	int salidanumceros;
	int inicionframe;
	bool guardarimagenengt;
	int numframeguardar;
	std::string salidamuestra;
	void AgregarVideo(std::string descripcion, std::string video, int vdeltagt, int iniciox, int finx, int inicioy, int finy,std::string mascaratraining,std::string salida) {
		InfoVideo v;
		v.video = video;
		v.descripcion = descripcion;
		v.deltagt = vdeltagt;
		if (iniciox == -1) {
			v.iniciox = v.finx = v.inicioy = v.finy = -1;
		}
		else {
			v.iniciox = iniciox-1;//Indices en videos comienzan en 1
			v.finx = finx-1;
			v.inicioy = inicioy-1;
			v.finy = finy-1;
		}
		v.salida = salida;
		videos.push_back(v);
	}
	void Configurar(std::string mascaravideo, std::string mascaragt,bool salidaescribir,std::string salidaprefijo,int salidanumceros,int inicionframe,bool utilizarvideostraining,std::string salidamuestra="", bool guardarimagenengt=false, int numframeguardar=-1) {
		this->mascaravideo = mascaravideo;
		this->mascaragt = mascaragt;
		this->salidaescribir = salidaescribir;
		this->salidaprefijo = salidaprefijo;
		this->salidanumceros = salidanumceros;
		this->inicionframe = inicionframe;
		this->utilizarvideostraining = utilizarvideostraining;
		this->salidamuestra = salidamuestra;
		this->guardarimagenengt = guardarimagenengt;
		this->numframeguardar = numframeguardar;
	}
	InfoVideo&ObtenerVideo(int i) {
		return videos[i];
	}
	int ObtenerNumVideos() {
		return (int)videos.size();
	}
	double MedidaF(double TP, double FP, double TN, double FN) {
		//double precision = TP / (TP + FP);
		//double recall = TP / (TP + FN);
		double denominador = (2 * TP + FP + FN);
		if (denominador == 0) {
			return 1;
		}
		double medidaF = 2 * TP / denominador;
		return medidaF;
	}
	void ProcesarVideos(std::fstream& arch, FuncProc procgt) {

		for (int o = 0; o < videos.size(); o++) {
			std::cout << videos[o].descripcion << std::endl;
			ListaImagenes lista;

			std::string video = videos[o].video;
			char buf1[10000];
			char buf2[10000];
			sprintf_s(buf1, mascaravideo.c_str(), video.c_str(), video.c_str(), video.c_str());
			sprintf_s(buf2, mascaragt.c_str(), video.c_str(), video.c_str(), video.c_str());
			VideoSourceDirectory d(buf1, buf2, "", videos[o].deltagt);//"C:\\videosprueba\\falldetectdataset\\wallflower\\" + video + "\\b*.bmp",
																					 //"C:\\videosprueba\\falldetectdataset\\wallflower\\" + video + "\\hand_segmented_*.bmp");
			ColaSinc<InfoMat, VideoSourceDirectory*> cl;
			cl.info = &d;
			HANDLE h = CrearTareaLectura(&cl);
			const int numbgs = 1;
			__int64 tp[numbgs], fp[numbgs], tn[numbgs], fn[numbgs];
			ZeroMemory(tp, sizeof(tp));
			ZeroMemory(fp, sizeof(fp));
			ZeroMemory(tn, sizeof(tn));
			ZeroMemory(fn, sizeof(fn));
			DR::SoporteBase::Imagenes::Preprocessor *p = new Preprocessor();
			DR::SoporteBase::Imagenes::ForegroundDetector *bb = new DR::SoporteBase::Imagenes::ForegroundDetector();
			DR::SoporteBase::Imagenes::BackgroundGenerator * gf = new DR::SoporteBase::Imagenes::BackgroundGenerator();
			int nframes = 0;
			if (utilizarvideostraining) {
				Preprocessor *p2= new Preprocessor();
				VideoSourceDirectory d2(videos[o].mascaratraining, "c:\\ND.png", 0);
				ColaSinc<InfoMat, VideoSourceDirectory *> cl2;
				cl2.info = &d2;
				HANDLE h2 = CrearTareaLectura(&cl2);
				int nframetot = inicionframe;
				while (!videos[o].AnalizarImagenes(cl2, p2, bb, gf, tp, tn, fp, fn, lista, &nframes, nframetot, procgt, false, std::string(), salidanumceros)) {
					nframetot++;
				}
				WaitForSingleObject(h2, INFINITE);
				CloseHandle(h2);
				ZeroMemory(tp, sizeof(tp));
				ZeroMemory(fp, sizeof(fp));
				ZeroMemory(tn, sizeof(tn));
				ZeroMemory(fn, sizeof(fn));
				nframes = 0;
				delete p2;
			}
			int nframetot = inicionframe;
			while (!videos[o].AnalizarImagenes(cl, p, bb, gf, tp, tn, fp, fn, lista, &nframes, nframetot, procgt, salidaescribir, salidaprefijo, salidanumceros, videos[o].descripcion,salidamuestra,guardarimagenengt,numframeguardar)) {				
				nframetot++;
			}
			cl.salir = true;
			arch << videos[o].descripcion << "\t";
			double medidaf = 0;
			for (int i = 0; i < numbgs; i++) {
				__int64 totalerrors = fp[i] + fn[i];
				medidaf = MedidaF(tp[i], fp[i], tn[i], fn[i]);
				arch << tp[i] << "\t" << tn[i] << "\t" << fp[i] << "\t" << fn[i] << "\t" << totalerrors << "\t" << nframes << "\t"<<medidaf<<"\t";
				//delete bgs[i];
				//bgs[i] = NULL;
			}
			arch << std::endl;
			arch.flush();
			std::cout << "MedidaF:"<<medidaf << std::endl;
			WaitForSingleObject(h, INFINITE);
			CloseHandle(h);
			delete p;
			delete gf;
			delete bb;
		}
	}
};


void EvaluarConst2(double difdeltak, double distancia);
std::string aplicar(std::string mascara, std::string video) {
	char buf[10000];
	sprintf_s(buf, mascara.c_str(), video.c_str(), video.c_str());
	return std::string(buf);
}
void ObtenerSubdirectorios(std::string directorio,std::vector<std::string>&dirs) {
	WIN32_FIND_DATAA dat;
	HANDLE hd = FindFirstFileA((directorio+"\\*.*").c_str(), &dat);
	if (hd != INVALID_HANDLE_VALUE) {
		do {
			if (dat.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
				std::string cad = dat.cFileName;
				if (cad == "." || cad == "..")
					continue;
				dirs.push_back(cad);
			}
		} while (FindNextFileA(hd, &dat));
		sort(dirs.begin(), dirs.end());
	}
}
void AgregarParametros(std::fstream &arch) {
	arch << "Parámetros Preprocesado" << std::endl;
	arch << "Con suavizado\t" <<consuavizado<< std::endl;
	arch << "Con normalización LRGB\t" << connormalizacionLRGB << std::endl;
	arch << "Parámetros Algoritmo Fondo" << std::endl;
	arch << "Threshold Coef. Fondo\t" << deltabackground << std::endl;
	arch << "Beta\t" << betavalue << std::endl;
	arch << "Probabilidad True\t" << thetaThresholdProbabilityTrue << std::endl;
	arch << "Probabilidad False\t" << etaThresholdProbabilityFalse << std::endl;
	arch << "Num Eventos True\t" << xiThresholdNumberOfTrueEvents << std::endl;
	arch << "Num Eventos False\t" << rhoThresholdNumberOfFalseEvents << std::endl;
	arch << "Num Imágenes Promedio\t" << numberOfAveragedFrames << std::endl;
	arch << "Gama\t" << gammaValue << std::endl;
	arch << "Fi\t" << psivalue << std::endl;
	arch << "Parámetros Algoritmo Detección Objetos" << std::endl;
	arch << "Threshold Coef. Foreground\t" << limdetforeground << std::endl;
	arch << "Dist. coef. escala\t" << chiIntercoefficientDistance << std::endl;
	arch << "Distancia\t" << zetaValue << std::endl;
	arch << "Proyeccion\t" << tauSqrt3Value << std::endl;
	arch << "Con Deteccion Sombras\t" << deteccionsombras << std::endl;
	arch << "Con Operación Close\t" << withoperationclose << std::endl;
	arch << "Threshold Cambio súbito\t" << thresholdSuddenChanges << std::endl;
	
}
bool iniciacon(const char *cadena, const char *subcadena) {
	int lim = 1000;
	for (int i = 0; i < lim; i++) {		
		if (subcadena[i] == 0)
			return true;
		if (cadena[i] == 0)
			return false;
		if (cadena[i] != subcadena[i])
			return false;
	}
	return false;
}
void RealizarTestChangeDetectionNet(bool condeteccionsombras, bool coperacionclose,bool iniagregarimagenes) {
	/*
	valorProbabilidadTrue = 0.95;//0.9
	valorProbabilidadFalse = 0.1;//0.3
	valorEventosTrue = 50;
	valorEventosFalse = 10;

	valorgama = 0.7;//0.8
	valorpsi = 0.9;//0.8
	valorbeta = 100;//1000

	valorInterCoef = 0.75;//0.65
	valorDistancia = 0.8;
	valorProyec = 1.0;
	deteccionsombras = false;
	conoperacionclose = true;
	quitarartefactosilum = true;

	limdetfondo = 0.04;//0.05;
	limdetforeground = 0.09;

	agregarimagenes = false;

	considerarcambiosubito = true;
	thresholdcambiosubito = 0.10;
	numframespromedio = 4;

	valorInterCoef = 0.35;//0.65;//0.45;
	//valorgama = 0.7;
	//valorpsi = 0.9;
	*/

	/*
	PARAMETROS PAPER
	consuavizado = false;
	valorProbabilidadTrue = 0.90;
	valorProbabilidadFalse = 0.30;
	valorEventosTrue = 50;
	valorEventosFalse = 10;

	valorgama = 0.7;
	valorpsi = 0.9;
	valorbeta = 100;

	valorInterCoef = 0.25;//0.75;
	valorDistancia = 0.8;
	valorProyec = 1.0;
	deteccionsombras = false;
	conoperacionclose = false;

	limdetfondo = 0.05;
	limdetforeground = 0.09;

	agregarimagenes = false;
	quitarartefactosilum = true;

	considerarcambiosubito = true;
	thresholdcambiosubito = 0.10;
	numframespromedio = 6;

	connormalizacionLRGB = true;


	agregarimagenes = iniagregarimagenes;//false;
	deteccionsombras = condeteccionsombras;
	conoperacionclose = coperacionclose;
	
	*/

	consuavizado = false;
	thetaThresholdProbabilityTrue = 0.95;//0.95 OK
	etaThresholdProbabilityFalse = 0.3;//0.3 OK
	xiThresholdNumberOfTrueEvents = 50;
	rhoThresholdNumberOfFalseEvents = 10;

	gammaValue = 0.75;//0.7;
	psivalue = 0.9;
	betavalue = 100;

	chiIntercoefficientDistance = 0.35;//0.75;
	zetaValue = 0.8;
	tauSqrt3Value = 1.0;
	deteccionsombras = false;
	withoperationclose = false;

	deltabackground = 0.05;
	limdetforeground = 0.09;

	agregarimagenes = false;
	removeIlluminationArtifacts = true;

	detectSuddenChangesInBackground = true;
	thresholdSuddenChanges = 0.10;
	numberOfAveragedFrames = 6;

	connormalizacionLRGB = true;


	agregarimagenes = iniagregarimagenes;//false;
	deteccionsombras = condeteccionsombras;
	withoperationclose = coperacionclose;

	std::fstream arch;
	arch.open("d:\\logchangedetectionnet2014.txt", std::ios::out);
	std::string directoriobase = "d:\\dataset2014";
	std::string dataset = directoriobase + "\\dataset";
	std::string resultados = directoriobase + "\\results";
	std::vector<std::string> dirs;
	ObtenerSubdirectorios(dataset, dirs);
	AgregarParametros(arch);
	std::string exclusiones[] = {	
		
		"badWeather",
		//"baseline",
		"cameraJitter",
		"dynamicBackground",
		"intermittentObjectMotion",
		"lowFramerate",
		//nightVideos
		"PTZ",
		//"shadow",
		"thermal",
		"turbulence"
	};
	int numexclusiones = sizeof(exclusiones) / sizeof(std::string);
	for (int i = 0; i < dirs.size(); i++) {
		DatosVideos info;
		std::vector<std::string> dirs2;
		ObtenerSubdirectorios(dataset+"\\"+dirs[i], dirs2);
		bool encontrado = false;
		for (int s = 0; s < numexclusiones; s++) {
			if (iniciacon(dirs[i].c_str(), exclusiones[s].c_str())) {
				encontrado = true;
				break;
			}
		}
		if (encontrado) continue;
		for (int o = 0; o < dirs2.size(); o++) {
			/*if (iniciacon(dirs2[o].c_str(),"PETS2006")||
				iniciacon(dirs2[o].c_str(), "bridgeEntry") ||
				iniciacon(dirs2[o].c_str(), "winterStreet") ||
				iniciacon(dirs2[o].c_str(), "pedestrians") ||
				iniciacon(dirs2[o].c_str(), "copyMachine")) {*/
			info.AgregarVideo(dirs[i] + "\\" + dirs2[o], dirs2[o].c_str(), 0, -1, -1, -1, -1, "", resultados + "\\" + dirs[i] + "\\" + dirs2[o]);
			//}
		}
		info.Configurar((dataset + "\\" + dirs[i] + "\\%s\\input\\in*.*").c_str(), (dataset + "\\" + dirs[i] + "\\%s\\groundtruth\\gt*.*").c_str(),
			false,//true,//true, 
			"bin", 6,1,false);
		info.ProcesarVideos(arch, ProcesarGTChangeDetection);
		arch.flush();
	}	
	arch.close();
}
void TestVariacionWallflower() {
	
	bool coperacionclose = true;
	double ini, fin, inc;

	std::fstream arch;
	arch.open("f:\\wallflowerlogVAR.txt", std::ios::out);
	terminarengt = true;
	for (int parametro = 0; parametro <= 14; parametro++) {
		if (parametro == 1) continue;
		if (parametro == 10) continue;
		thetaThresholdProbabilityTrue = 0.9;//0.9
		etaThresholdProbabilityFalse = 0.45;//0.3
		xiThresholdNumberOfTrueEvents = 9;
		rhoThresholdNumberOfFalseEvents = 4;

		gammaValue = 0.7;//0.8
		psivalue = 0.9;//0.8
		betavalue = 25;//50;//1000

		chiIntercoefficientDistance = 0.25;//0.65
		zetaValue = 0.8;
		tauSqrt3Value = 0.9;//1.0;
		deteccionsombras = false;
		withoperationclose = coperacionclose;
		removeIlluminationArtifacts = true;

		deltabackground = 0.04;//0.05;
		limdetforeground = 0.09;

		agregarimagenes = false;

		detectSuddenChangesInBackground = true;
		thresholdSuddenChanges = 0.10;

		numberOfAveragedFrames = 30;//25;

		if (parametro==0)
			AgregarParametros(arch);
		switch (parametro) {
		case 0://valorbeta
			arch << "Variando Beta" << std::endl;
			ini = 25;
			fin = 250.1;
			inc = 25;
			break;
		case 1://valoreventosfalse=valoreventostrue
			arch << "Variando Eventos True/false" << std::endl;
			ini = 3;
			fin = 10;
			inc = 1;
			break;
		case 2://valorprobabilidadtrue
			arch << "Variando Probabilidad true" << std::endl;
			ini = 0.8;
			fin = 0.951;
			inc = 0.05;
			break;
		case 3://valorprobabilidadfalse
			arch << "Variando Probabilidad False" << std::endl;
			ini = 0.1;
			fin = 0.45;
			inc = 0.05;
			break;
		case 4://valorgama
			arch << "Variando Gamma" << std::endl;
			ini = 0.6;
			fin = 1.01;
			inc = 0.1;
			break;
		case 5://valorpsi
			arch << "Variando Psi" << std::endl;
			ini = 0.6;
			fin = 1.01;
			inc = 0.1;
			break;
		case 6://valorInterCoef
			arch << "Variando ValorInterCoef" << std::endl;
			ini = 0.15;
			fin = 0.851;
			inc = 0.1;
			break;
		case 7://valordistancia
			arch << "Variando Distancia" << std::endl;
			ini = 0.7;
			fin = 1.51;
			inc = 0.1;
			break;
		case 8://valorproyec
			arch << "Variando ValorProyec" << std::endl;
			ini = 0.7;
			fin = 1.51;
			inc = 0.1;
			break;
		case 9://limdetfondo
			arch << "Variando LimDetFondo" << std::endl;
			ini = 0.02;
			fin = 0.111;
			inc = 0.01;
			break;
		case 10://limdetforeground
			arch << "Variando LimDetForeground" << std::endl;
			ini = 0.12;
			fin = 0.211;
			inc = 0.01;
			removeIlluminationArtifacts = false;
			break;
		case 11://thresholdcambiosubito
			arch << "Variando Threshold Cambio S�bito" << std::endl;
			ini = 0.05;
			fin = 0.201;
			inc = 0.05;
			break;
		case 12://numframespromedio
			arch << "Variando Numero Frames Promedio" << std::endl;
			ini = 15;
			fin = 25.1;
			inc = 1;
			//ini = 5;
			//fin = 30;
			//inc = 5;
			break;
		case 13://variando eventos true
			arch << "Variando Eventos True" << std::endl;
			ini = 4;
			fin = 12.1;
			inc = 1;
			break;
		case 14://variando eventos false
			arch << "Variando Eventos False" << std::endl;
			ini = 4;
			fin = 25;
			inc = 2;
			break;

		}

		
		arch.flush();
		for (double v = ini; v <= fin; v += inc) {
			switch (parametro) {
			case 0:
				betavalue = v;
				break;
			case 1:
				rhoThresholdNumberOfFalseEvents = xiThresholdNumberOfTrueEvents = v;
				break;
			case 2:
				thetaThresholdProbabilityTrue = v;
				break;
			case 3:
				etaThresholdProbabilityFalse = v;
				break;
			case 4:
				gammaValue = v;
				break;
			case 5:
				psivalue = v;
				break;
			case 6:
				chiIntercoefficientDistance = v;
				break;
			case 7:
				zetaValue = v;
				break;
			case 8:
				tauSqrt3Value = v;
				break;
			case 9:
				deltabackground = v;
				break;
			case 10:
				limdetforeground = v;
				break;
			case 11:
				thresholdSuddenChanges = v;
				break;
			case 12:
				numberOfAveragedFrames = v;
				break;
			case 13:
				xiThresholdNumberOfTrueEvents = v;
				break;
			case 14:
				rhoThresholdNumberOfFalseEvents = v;
				break;
			}
			arch << "VALOR VARIABLE:" << v<<std::endl;
			DatosVideos info;
			info.AgregarVideo("Moved Object", "MovedObject.zip", 0, -1, -1, -1, -1, "", "");
			info.AgregarVideo("Time Of Day", "TimeOfDay.zip", 0, -1, -1, -1, -1, "", "");
			info.AgregarVideo("Light Switch", "LightSwitch.zip", 0, -1, -1, -1, -1, "", "");
			info.AgregarVideo("Waving Trees", "WavingTrees.zip", 0, -1, -1, -1, -1, "", "");
			info.AgregarVideo("Camouflage", "Camouflage.zip", 0, -1, -1, -1, -1, "", "");
			info.AgregarVideo("Bootstrap", "Bootstrap.zip", 0, -1, -1, -1, -1, "", "");
			info.AgregarVideo("Foreground Aperture", "ForegroundAperture.zip", 0, -1, -1, -1, -1, "", "");

			info.Configurar("C:\\videosprueba\\falldetectdataset\\wallflower\\%s\\b*.bmp", "C:\\videosprueba\\falldetectdataset\\wallflower\\%s\\hand_segmented_*.bmp", false, std::string(), 6, 1, false, "", false, -1);
			info.ProcesarVideos(arch, ProcesarGTWallflower);
			arch.flush();
		}
	}
	arch.close();
	terminarengt = false;
}
void TestVariacionSABS() {

	bool coperacionclose = true;
	double ini, fin, inc;

	std::fstream arch;
	terminarengt = false;
	arch.open("f:\\sabslogVAR.txt", std::ios::out);
	for (int parametro = 0; parametro <= 14; parametro++) {
		if (parametro == 1) continue;
		if (parametro == 10 || parametro == 11) continue;
		/*
		valorProbabilidadTrue = 0.95;//0.9
		valorProbabilidadFalse = 0.1;//0.3
		valorEventosTrue = 25;
		valorEventosFalse = 25;

		valorgama = 0.7;//0.8
		valorpsi = 0.7;//0.8
		valorbeta = 100;//1000

		valorInterCoef = 0.75;//0.65
		valorDistancia = 0.8;
		valorProyec = 1.0;
		deteccionsombras = false;
		conoperacionclose = false;
		quitarartefactosilum = true;

		limdetfondo = 0.06;//0.05;
		limdetforeground = 0.09;

		agregarimagenes = false;

		considerarcambiosubito = true;
		thresholdcambiosubito = 0.10;
		*/
		/*
		valorProbabilidadTrue = 0.95;//0.9
		valorProbabilidadFalse = 0.1;//0.3
		valorEventosTrue = 25;
		valorEventosFalse = 25;

		valorgama = 0.7;//0.8
		valorpsi = 0.7;//0.8
		valorbeta = 100;//1000

		valorInterCoef = 0.65;//0.65
		valorDistancia = 0.8;
		valorProyec = 1.0;
		deteccionsombras = false;
		conoperacionclose = false;
		quitarartefactosilum = true;

		limdetfondo = 0.04;//0.05;
		limdetforeground = 0.09;

		agregarimagenes = false;

		considerarcambiosubito = true;
		thresholdcambiosubito = 0.10;
		*/
		/*
		valorProbabilidadTrue = 0.95;//0.9
		valorProbabilidadFalse = 0.1;//0.3
		valorEventosTrue = 10;
		valorEventosFalse = 10;

		valorgama = 0.7;//0.8
		valorpsi = 0.9;//0.8
		valorbeta = 100;//1000

		valorInterCoef = 0.65;//0.65
		valorDistancia = 0.8;
		valorProyec = 1.0;
		deteccionsombras = false;
		conoperacionclose = false;
		quitarartefactosilum = true;

		limdetfondo = 0.04;//0.05;
		limdetforeground = 0.09;

		agregarimagenes = false;

		considerarcambiosubito = true;
		thresholdcambiosubito = 0.10;
		*/
		/*
		valorProbabilidadTrue = 0.95;//0.9
		valorProbabilidadFalse = 0.1;//0.3
		valorEventosTrue = 10;
		valorEventosFalse = 10;

		valorgama = 0.7;//0.8
		valorpsi = 0.9;//0.8
		valorbeta = 100;//1000

		valorInterCoef = 0.75;//0.65
		valorDistancia = 0.8;
		valorProyec = 1.0;
		deteccionsombras = false;
		conoperacionclose = false;
		quitarartefactosilum = true;

		limdetfondo = 0.04;//0.05;
		limdetforeground = 0.09;

		agregarimagenes = false;

		considerarcambiosubito = true;
		thresholdcambiosubito = 0.10;
		numframespromedio = 4;
		*/
		thetaThresholdProbabilityTrue = 0.95;//0.9
		etaThresholdProbabilityFalse = 0.1;//0.3
		xiThresholdNumberOfTrueEvents = 50;
		rhoThresholdNumberOfFalseEvents = 10;

		gammaValue = 0.7;//0.8
		psivalue = 0.9;//0.8
		betavalue = 100;//1000

		chiIntercoefficientDistance = 0.75;//0.65
		zetaValue = 0.8;
		tauSqrt3Value = 1.0;
		deteccionsombras = false;
		withoperationclose = false;
		removeIlluminationArtifacts = true;

		deltabackground = 0.04;//0.05;
		limdetforeground = 0.09;

		agregarimagenes = false;

		detectSuddenChangesInBackground = true;
		thresholdSuddenChanges = 0.10;
		numberOfAveragedFrames = 4;
		if (parametro == 0)
			AgregarParametros(arch);
		switch (parametro) {
		case 0://valorbeta
			arch << "Variando Beta" << std::endl;
			ini = 100;
			fin = 1200;
			inc = 200;
			break;
		case 1://valoreventosfalse=valoreventostrue
			arch << "Variando Eventos True/false" << std::endl;
			ini = 5;
			fin = 40.1;
			inc = 5;
			break;
		case 2://valorprobabilidadtrue
			arch << "Variando Probabilidad true" << std::endl;
			ini = 0.8;
			fin = 0.951;
			inc = 0.05;
			break;
		case 3://valorprobabilidadfalse
			arch << "Variando Probabilidad False" << std::endl;
			ini = 0.1;
			fin = 0.45;
			inc = 0.05;
			break;
		case 4://valorgama
			arch << "Variando Gamma" << std::endl;
			ini = 0.6;
			fin = 1.01;
			inc = 0.1;
			break;
		case 5://valorpsi
			arch << "Variando Psi" << std::endl;
			ini = 0.6;
			fin = 1.01;
			inc = 0.1;
			break;
		case 6://valorInterCoef
			arch << "Variando ValorInterCoef" << std::endl;
			ini = 0.15;
			fin = 0.851;
			inc = 0.1;
			break;
		case 7://valordistancia
			arch << "Variando Distancia" << std::endl;
			ini = 0.7;
			fin = 1.51;
			inc = 0.1;
			break;
		case 8://valorproyec
			arch << "Variando ValorProyec" << std::endl;
			ini = 0.7;
			fin = 1.51;
			inc = 0.1;
			break;
		case 9://limdetfondo
			arch << "Variando LimDetFondo" << std::endl;
			ini = 0.02;
			fin = 0.111;
			inc = 0.01;
			break;
		case 10://limdetforeground
			arch << "Variando LimDetForeground" << std::endl;
			ini = 0.12;
			fin = 0.211;
			inc = 0.01;
			removeIlluminationArtifacts = false;
			break;
		case 11://thresholdcambiosubito
			arch << "Variando Threshold Cambio S�bito" << std::endl;
			ini = 0.05;
			fin = 0.251;
			inc = 0.05;
			break;
		case 12://numframespromedio
			arch << "Variando Numero Frames Promedio" << std::endl;
			ini = 1;
			fin = 10.1;
			inc = 1;
			break;
		case 13://variando eventos true
			arch << "Variando Eventos True" << std::endl;
			ini = 10;
			fin = 80.1;
			inc = 10;
			break;
		case 14://variando eventos false
			arch << "Variando Eventos False" << std::endl;
			ini = 10;
			fin = 80.1;
			inc = 10;
			break;
		}


		arch.flush();
		for (double v = ini; v <= fin; v += inc) {
			switch (parametro) {
			case 0:
				betavalue = v;
				break;
			case 1:
				rhoThresholdNumberOfFalseEvents = xiThresholdNumberOfTrueEvents = v;
				break;
			case 2:
				thetaThresholdProbabilityTrue = v;
				break;
			case 3:
				etaThresholdProbabilityFalse = v;
				break;
			case 4:
				gammaValue = v;
				break;
			case 5:
				psivalue = v;
				break;
			case 6:
				chiIntercoefficientDistance = v;
				break;
			case 7:
				zetaValue = v;
				break;
			case 8:
				tauSqrt3Value = v;
				break;
			case 9:
				deltabackground = v;
				break;
			case 10:
				limdetforeground = v;
				break;
			case 11:
				thresholdSuddenChanges = v;
				break;
			case 12:
				numberOfAveragedFrames = v;
				break;
			case 13:
				xiThresholdNumberOfTrueEvents = v;
				break;
			case 14:
				rhoThresholdNumberOfFalseEvents = v;
				break;
			}
			arch << "VALOR VARIABLE:" << v << std::endl;
			DatosVideos info;
			info.AgregarVideo("Basic", "Basic", -801, -1, -1, -1, -1, "C:\\videosprueba\\SABS\\Train\\NoForegroundDay\\*.png", std::string());
			info.AgregarVideo("Darkening", "Darkening", -1, -1, -1, -1, -1, "C:\\videosprueba\\SABS\\Train\\NoForegroundDay\\*.png", std::string());
			info.AgregarVideo("Noisy Night", "NoisyNight", -801, -1, -1, -1, -1, "C:\\videosprueba\\SABS\\Train\\NoForegroundNightNoisy\\*.png", std::string());
			std::string directoriomuestra;
			directoriomuestra = "c:\\videosprueba\\Resultados\\SABSNormal";
			info.Configurar("C:\\videosprueba\\SABS\\Test\\%s\\*.png", "C:\\videosprueba\\SABS\\GT\\GT*.png", false, std::string(), 6, 1, false, "", false, -1);
			info.ProcesarVideos(arch, ProcesarGTSABS);
			arch.flush();
		}
	}
	arch.close();
}

void RealizarTestSABS(bool consombras,bool coperacionclose, bool agimagenes) {
	//valorInterCoef = 0.65;//0.45;
	//valorgama = 0.8;
	//valorpsi = 0.8;
	/*Primer paper
	valorProbabilidadTrue = 0.95;//0.9
	valorProbabilidadFalse = 0.1;//0.3
	valorEventosTrue = 50;//10;
	valorEventosFalse = 10;

	valorgama = 0.7;//0.8
	valorpsi = 0.9;//0.8
	valorbeta = 100;//1000

	valorInterCoef = 0.75;//0.65
	valorDistancia = 0.8;
	valorProyec = 1.0;
	deteccionsombras = false;
	conoperacionclose = false;
	quitarartefactosilum = true;

	limdetfondo = 0.04;//0.05;
	limdetforeground = 0.09;

	agregarimagenes = true;

	considerarcambiosubito = true;
	thresholdcambiosubito = 0.10;
	numframespromedio = 4;


	deteccionsombras = consombras;
	conoperacionclose = coperacionclose;
	*/

	consuavizado = true;//ANTES ERA CON SUAVIZADO

	thetaThresholdProbabilityTrue = 0.95;//0.9
	etaThresholdProbabilityFalse = 0.1;//0.3
	xiThresholdNumberOfTrueEvents = 50;//10;
	rhoThresholdNumberOfFalseEvents = 10;

	gammaValue = 0.7;//0.8
	psivalue = 0.9;//0.8
	betavalue = 100;//1000

	chiIntercoefficientDistance = 0.75;//0.65
	zetaValue = 0.8;
	tauSqrt3Value = 1.0;
	deteccionsombras = false;
	withoperationclose = false;
	removeIlluminationArtifacts = true;

	deltabackground = 0.04;//0.05;
	limdetforeground = 0.09;

	agregarimagenes = true;

	detectSuddenChangesInBackground = true;
	thresholdSuddenChanges = 0.10;
	numberOfAveragedFrames = 4;

	connormalizacionLRGB = false;
	deteccionsombras = consombras;
	withoperationclose = coperacionclose;

	/*otros valores
	valorProbabilidadTrue = 0.95;//0.9
	valorProbabilidadFalse = 0.1;//0.3
	valorEventosTrue = 50;//10;
	valorEventosFalse = 10;

	valorgama = 0.65;//0.8
	valorpsi = 0.77;//0.8
	valorbeta = 100;//1000

	valorInterCoef = 0.68;//0.65
	valorDistancia = 0.8;
	valorProyec = 1.0;
	deteccionsombras = false;
	conoperacionclose = false;
	quitarartefactosilum = true;

	limdetfondo = 0.05;
	limdetforeground = 0.09;
	
	//agregarimagenes = true;
	agregarimagenes = agimagenes;

	considerarcambiosubito = true;
	thresholdcambiosubito = 0.10;
	numframespromedio = 4;

	deteccionsombras = consombras;
	conoperacionclose = coperacionclose;
	*/

	std::fstream arch;
	
	if (consombras) {
		arch << "Con Sombras" << std::endl;
		arch.open("d:\\logSABSSombras.txt", std::ios::out);
	}
	else {
		arch << "Sin Sombras" << std::endl;
		arch.open("d:\\logSABSSinSombras.txt", std::ios::out);
	}
	DatosVideos info;
	std::string resultados2 = "d:\\resultadossabs";
	//resultados + "\\" + dirs[i] + "\\" + dirs2[o]
	info.AgregarVideo("Basic", "Basic", -801, -1, -1, -1, -1, "d:\\videosprueba\\SABS\\Train\\NoForegroundDay\\*.png","d:\\resultadossabs\\out01");//Ok
	info.AgregarVideo("Dynamic Background", "Basic", -801, 100, 380, 200, 560, "d:\\videosprueba\\SABS\\Train\\NoForegroundDay\\*.png", "d:\\resultadossabs\\out03");//Ok
	info.AgregarVideo("Bootstrap", "Bootstrap", 0, -1, -1, -1, -1, "d:\\videosprueba\\SABS\\Train\\Bootstrap\\*.png", "d:\\resultadossabs\\out11");//Ok
	info.AgregarVideo("Darkening", "Darkening", -1, -1, -1, -1, -1, "d:\\videosprueba\\SABS\\Train\\NoForegroundDay\\*.png", "d:\\resultadossabs\\out05");//
	info.AgregarVideo("Light Switch", "LightSwitch", -801, -1, -1, -1, -1, "d:\\videosprueba\\SABS\\Train\\NoForegroundNight\\*.png", "d:\\resultadossabs\\out04");//
	info.AgregarVideo("Noisy Night", "NoisyNight", -801, -1, -1, -1, -1, "d:\\videosprueba\\SABS\\Train\\NoForegroundNightNoisy\\*.png", "d:\\resultadossabs\\out09");//Ok?
	//info.AgregarVideo("Shadow", "NoCamouflage", -1, -1, -1, -1, -1, "C:\\videosprueba\\SABS\\Train\\NoForegroundDay\\*.png", std::string());
	info.AgregarVideo("Camouflage", "Camouflage", -1, 400, 600, 150, 500, "d:\\videosprueba\\SABS\\Train\\NoForegroundDay\\*.png", "d:\\resultadossabs\\out07");//Ok
	info.AgregarVideo("NoCamouflage", "NoCamouflage", -1, 400, 600, 150, 500, "d:\\videosprueba\\SABS\\Train\\NoForegroundDay\\*.png", "d:\\resultadossabs\\out08");//Ok
	info.AgregarVideo("MPEG4 40kbps", "MPEG4_40kbps", -801, -1, -1, -1, -1, "d:\\videosprueba\\SABS\\Train\\MPEG4_40kbps\\*.png", "d:\\resultadossabs\\out10");
	AgregarParametros(arch);
	agregarimagenes = true;
	std::string directoriomuestra;
	if (consombras)
		directoriomuestra = "d:\\videosprueba\\Resultados\\SABSSombras";
	else
		directoriomuestra = "d:\\ResultadosSABSNormal";
	info.Configurar("d:\\videosprueba\\SABS\\Test\\%s\\*.png", "d:\\videosprueba\\SABS\\GT\\GT*.png", false, "fg_", 6, 1,false,directoriomuestra,false,531);
	info.ProcesarVideos(arch,ProcesarGTSABS); //el ground truth no corresponde!
	arch.close();
}
void RealizarTestWallflower(bool coperacionclose,bool conagregarimagenes) {
	std::fstream arch;
	arch.open("d:\\logwallflower.txt", std::ios::out);
	deteccionsombras = false;
	withoperationclose = coperacionclose;
	DatosVideos info;
	info.AgregarVideo("Moved Object", "MovedObject", 0, -1, -1, -1, -1, "","");
	info.AgregarVideo("Time Of Day", "TimeOfDay", 0, -1, -1, -1, -1, "", "");
	info.AgregarVideo("Light Switch", "LightSwitch", 0, -1, -1, -1, -1, "", "");
	info.AgregarVideo("Waving Trees", "WavingTrees", 0, -1, -1, -1, -1, "", "");
	info.AgregarVideo("Camouflage", "Camouflage", 0, -1, -1, -1, -1, "", "");
	info.AgregarVideo("Bootstrap", "Bootstrap", 0, -1, -1, -1, -1, "", "");
	info.AgregarVideo("Foreground Aperture", "ForegroundAperture", 0, -1, -1, -1, -1, "", "");
	/*
	valorInterCoef = 0.25;
	valorbeta = std::ceil(valorbeta * 4 / 29.97);
	valorEventosFalse = std::ceil(valorEventosFalse * 4 / 29.97);
	valorEventosTrue = std::ceil(valorEventosTrue * 4 / 29.97);
	valorgama = 0.8;
	valorpsi = 0.8;*/

	thetaThresholdProbabilityTrue = 0.9;//0.9
	etaThresholdProbabilityFalse = 0.45;//0.3
	xiThresholdNumberOfTrueEvents = 9;
	rhoThresholdNumberOfFalseEvents = 4;

	gammaValue = 0.7;//0.8
	psivalue = 0.9;//0.8
	betavalue = 25;//50;//1000

	chiIntercoefficientDistance = 0.25;//0.65
	zetaValue = 0.8;
	tauSqrt3Value = 0.9;//1.0;
	deteccionsombras = false;
	withoperationclose = coperacionclose;
	removeIlluminationArtifacts = true;

	deltabackground = 0.04;//0.05;
	limdetforeground = 0.09;

	agregarimagenes = conagregarimagenes;// true;

	detectSuddenChangesInBackground = true;
	thresholdSuddenChanges = 0.10;

	numberOfAveragedFrames = 30;//25;

	AgregarParametros(arch);
	//agregarimagenes = true;
	info.Configurar("d:\\videosprueba\\falldetectdataset\\wallflower\\%s\\b*.bmp", "d:\\videosprueba\\falldetectdataset\\wallflower\\%s\\hand_segmented_*.bmp", false, std::string(), 6, 1,false,"d:\\ResultadosWallflower",true,-1);
	info.ProcesarVideos(arch,ProcesarGTWallflower);
	arch.close();
}
#include "LRGBImage.h"
cv::Mat Escalar(cv::Mat mat) {
	double valmin, valmax;
	cv::minMaxIdx(mat, &valmin, &valmax);
	cv::Mat res=(mat - valmin) * 255 / (valmax - valmin);
	cv::Mat r2;
	res.convertTo(r2, CV_8UC1);
	return r2;
}
int main()
{	
	RealizarTestWallflower(true, true);
	//RealizarTestSABS(false, true, true);
	//RealizarTestChangeDetectionNet(false, true,true);
	

	//-----------------------------

	//RealizarTestSABS(false, true, true);
	//RealizarTestSABS(true, true);
	//RealizarTestWallflower(true);
	return 0;
}

