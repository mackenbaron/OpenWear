#include "string"
#include <Windows.h>
#include "mropencv.h"
#include "mrgl.h"
#include "Glasses.h"
using namespace std;
const double scale = 1.5;
String cascadeName = "haarcascade_frontalface_alt2.xml";
String nestedCascadeName = "haarcascade_eye.xml";
CascadeClassifier cascade, nestedCascade;
Mat frame;
CGlasses m_glasses;
GLfloat glasses_x=0, glasses_y=0, glasses_scale=1.0f;
void detectAndDraw(Mat& img,CascadeClassifier& cascade, CascadeClassifier& nestedCascade,double scale);
void initCV()
{
	if (!cascade.load(cascadeName))
	{
		cerr << "ERROR: Could not load classifier cascade" << endl;
		return ;
	}

	if (!nestedCascade.load(nestedCascadeName))
	{
		cerr << "WARNING: Could not load classifier cascade for nested objects" << endl;
		return ;
	}
}
void initGL()
{
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0f, 1.0f, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GLuint		texture[1];
	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	GLfloat light_pos[4];
	light_pos[0] = 0;
	light_pos[1] = 0;
	light_pos[2] = -2.0;
	light_pos[3] = 0;
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glEnable(GL_LIGHT0);
	m_glasses.ReadData();
}
void DrawVideo()
{
	glTranslatef(0.0f, 0.0f, -1.0f);
	flip(frame, frame,0);
	cvtColor(frame, frame, CV_RGB2BGR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,frame.cols, frame.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); 
	glVertex3f(-1.0f, -1.0f,0.0f);
	glTexCoord2f(1.0f, 0.0f); 
	glVertex3f(1.0f, -1.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); 
	glVertex3f(1.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); 
	glVertex3f(-1.0f, 1.0f, 0.0f);
	glEnd();
}
void onDraw(void* param)
{  
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	glPushMatrix();
//	cout << glasses_x<< "," << glasses_y << endl;
	glTranslatef((glasses_x - 320)/320.0f, (280-glasses_y)/240.0f, -2.5f+glasses_scale*0.01f);
//	cout << glasses_scale<< endl;
//	glTranslatef((glasses_x - 320) / 320.0f, (280 - glasses_y) / 240.0f, -1.6f-glasses_scale/128.0);
//	glTranslatef(0.0f, 0.0f, -1.6f);
//	glRotatef(30, 0, 1, 0);
	glScalef(0.3, 0.3, 0.3);
	m_glasses.Draw();
	glPopMatrix();
	glPushMatrix();
	DrawVideo();
	glPopMatrix();
}

int main(void)
{
	string openGLWindowName = "OpenWear";
	namedWindow(openGLWindowName, WINDOW_OPENGL);
	resizeWindow(openGLWindowName, 640, 480);
	initGL();
	initCV();
	setOpenGlContext(openGLWindowName);
	setOpenGlDrawCallback(openGLWindowName, onDraw, NULL);	
	VideoCapture capture(0);
	while (1)
	{
		capture >> frame;
		if (frame.empty())
			break;
		detectAndDraw(frame.clone(), cascade, nestedCascade, scale);
		updateWindow(openGLWindowName);
		waitKey(1);
	}
	return 0;
}

void detectAndDraw(Mat& img,CascadeClassifier& cascade, CascadeClassifier& nestedCascade,double scale)
{
	int i = 0;
	double t = 0;
	vector<Rect> faces;
	const static Scalar colors[] = { CV_RGB(0, 0, 255),
		CV_RGB(0, 128, 255),
		CV_RGB(0, 255, 255),
		CV_RGB(0, 255, 0),
		CV_RGB(255, 128, 0),
		CV_RGB(255, 255, 0),
		CV_RGB(255, 0, 0),
		CV_RGB(255, 0, 255) };//�ò�ͬ����ɫ��ʾ��ͬ������

	Mat gray, smallImg(cvRound(img.rows / scale), cvRound(img.cols / scale), CV_8UC1);//��ͼƬ��С���ӿ����ٶ�
	cvtColor(img, gray, CV_BGR2GRAY);//��Ϊ�õ�����haar���������Զ��ǻ��ڻҶ�ͼ��ģ�����Ҫת���ɻҶ�ͼ��
	resize(gray, smallImg, smallImg.size(), 0, 0, INTER_LINEAR);//���ߴ���С��1/scale,�����Բ�ֵ
	equalizeHist(smallImg, smallImg);//ֱ��ͼ����
	t = (double)cvGetTickCount();//���������㷨ִ��ʱ��
	//�������
	//detectMultiScale������smallImg��ʾ����Ҫ��������ͼ��ΪsmallImg��faces��ʾ��⵽������Ŀ�����У�1.1��ʾ
	//ÿ��ͼ��ߴ��С�ı���Ϊ1.1��2��ʾÿһ��Ŀ������Ҫ����⵽3�β��������Ŀ��(��Ϊ��Χ�����غͲ�ͬ�Ĵ��ڴ�
	//С�����Լ�⵽����),CV_HAAR_SCALE_IMAGE��ʾ�������ŷ���������⣬��������ͼ��Size(30, 30)ΪĿ���
	//��С���ߴ�
	cascade.detectMultiScale(smallImg, faces,
		1.1, 2, 0
		//|CV_HAAR_FIND_BIGGEST_OBJECT
		//|CV_HAAR_DO_ROUGH_SEARCH
		| CV_HAAR_SCALE_IMAGE
		,
		Size(30, 30));
	t = (double)cvGetTickCount() - t;//���Ϊ�㷨ִ�е�ʱ��
//	printf("detection time = %g ms\n", t / ((double)cvGetTickFrequency()*1000.));
	for (auto r = faces.begin(); r != faces.end(); r++, i++)
	{
		Mat smallImgROI;
		vector<Rect> nestedObjects;
		Point center;
		Scalar color = colors[i % 8];
		int radius;
		center.x = cvRound((r->x + r->width*0.5)*scale);//��ԭ��ԭ���Ĵ�С
		center.y = cvRound((r->y + r->height*0.5)*scale);
		radius = cvRound((r->width + r->height)*0.25*scale);
		cout << radius << endl;
		glasses_x = center.x;
		glasses_y = center.y;
		glasses_scale = radius;
		circle(img, center, radius, color, 3, 8, 0);
		//������ۣ���ÿ������ͼ�ϻ�������
		if (nestedCascade.empty())
			continue;
		smallImgROI = smallImg(*r);
		//������ĺ�������һ��
		nestedCascade.detectMultiScale(smallImgROI, nestedObjects,
			1.1, 2, 0
			//|CV_HAAR_FIND_BIGGEST_OBJECT
			//|CV_HAAR_DO_ROUGH_SEARCH
			//|CV_HAAR_DO_CANNY_PRUNING
			| CV_HAAR_SCALE_IMAGE
			,
			Size(30, 30));
		for (auto nr = nestedObjects.begin(); nr != nestedObjects.end(); nr++)
		{
			center.x = cvRound((r->x + nr->x + nr->width*0.5)*scale);
			center.y = cvRound((r->y + nr->y + nr->height*0.5)*scale);
			radius = cvRound((nr->width + nr->height)*0.25*scale);
			circle(img, center, radius, color, 3, 8, 0);//���۾�Ҳ���������Ͷ�Ӧ������ͼ����һ����
		}
	}
	cv::imshow("detected eyes", img);
}