#include "multiperson.h"

using namespace std;

#define WIDTH 800 //������
#define HEIGHT 600 //����߶�
#define NUM 5 //�������
#define HandsOrNot 1 //�Ƿ����ֲ�

//�����ӵ���Ϣ
double Viewx[NUM];
double Viewy[NUM];
double Viewz[NUM];
float Angle1[NUM];
float Angle2[NUM];
double View_dist = 1.2;
float multi_nearz = 0.5f;
float multi_farz = 6.0f;

//�����ӵ�任����
glm::mat4 Model[NUM];
glm::mat4 View[NUM];
glm::mat4 Projection[NUM];

//����ƽ������
glm::vec3 trans[NUM];

//ģ����Ϣ�洢��
vector<glm::vec3> mesh_vertex[NUM], mesh_color[NUM];
vector<glm::ivec3> mesh_edge[NUM];
vector<glm::vec3> sample_vertex[NUM];
vector<glm::ivec3> sample_edge[NUM];
vector<glm::vec3> sample_UV[NUM];
vector<bool> HandJudger;

//�����ֲ��ж�����ȥ���ֲ���Ϣ
void LoadHand(vector<bool>& HandJudger, string HandFilename)
{
	ifstream fin(HandFilename);
	bool buffer;
	HandJudger.clear();
	while (fin >> buffer)
	{
		HandJudger.push_back(buffer);
	}
	if (HandJudger.size() != 6890)cout << "vertex number error! vertex number: "<<HandJudger.size();
	fin.close();
}

//������ģ�ͳ�ʼ��
void MultiInit(Renderer& mesh, int num, const char* vs_name, const char* fs_name, string mesh_filename, string sample_filename, string HandFilename)
{
	mesh.Init(vs_name, fs_name);
	ObjReader(mesh_vertex[num], mesh_edge[num], mesh_color[num], mesh_filename, 2);
	ObjReader(sample_vertex[num], sample_edge[num], sample_filename);
	LoadHand(HandJudger, HandFilename);
	mesh.SetData(mesh_vertex[num], mesh_edge[num]);
	mesh.SetColor(mesh_color[num]);
}

//�������λ�ú���
void GenPosition(float& angle1, float& angle2, glm::vec3& trans, glm::vec3& transold, int seed)
{
	float MINdist = 1.2f;
	srand(seed + 100 * time(0));
	angle1 = (float)(rand() % 45 - 90);

	trans.x = (rand() % 24) / 10 - 1.2f;
	trans.y = (rand() % 24) / 10 - 1.2f;
	trans.z = (rand() % 32) / 8 + 0.5f;

	float distant = ((float)trans.x - (float)transold.x)*((float)trans.x - (float)transold.x) + ((float)trans.y - (float)transold.y)*((float)trans.y - (float)transold.y) + ((float)trans.z - (float)transold.z)*((float)trans.z - (float)transold.z);

	if (distant <= MINdist)
	{
		trans.z = trans.z + 0.8f;
	}
}

void GenPosition(float& angle1, float& angle2, glm::vec3& trans, int seed)
{
	srand(seed + 100 * time(0));
	angle1 = (float)(rand() % 45 - 90);

	trans.x = (rand() % 24) / 10 - 1.2f;
	trans.y = (rand() % 24) / 10 - 1.2f;
	trans.z = (rand() % 16) / 8 + 0.5f;

}

//mvp��ʼ��
void MultiSetUniform(Renderer& mesh, int num, const int RenderDepth)
{
	glm::vec3 center(1, 1.5, 0.75);
	float aspect = (float)HEIGHT / (float)WIDTH;

	if (RenderDepth == 0 && num >= 1)
		GenPosition(Angle1[num], Angle2[num], trans[num], trans[num - 1], num * 2000);
	else if (RenderDepth == 0 && num == 0)
		GenPosition(Angle1[num], Angle2[num], trans[num], num * 2000);

	//����mvp
	Model[num] = glm::mat4(1.0);//Ϊ�����ۻ���������Ҫ����
	Model[num] = glm::mat4(glm::translate(Model[num], trans[num]));
	Model[num] = glm::translate(Model[num], center);
	Model[num] = glm::rotate(Model[num], Angle1[num], glm::vec3(0.0f, 1.0f, 0.0f));
	Model[num] = glm::translate(Model[num], -center);

	View[num] = glm::mat4(glm::lookAt(center - glm::vec3(0.0f, 0.0f, 1.3f), center, glm::vec3(0.0f, -1.0f, 0.0f)));
	Projection[num] = glm::mat4(glm::perspective(glm::radians(90.0f), 1.0f / aspect, multi_nearz, multi_farz));

	mesh.SetUniforms(Model[num], View[num], Projection[num], RenderDepth);
	mesh.viewPosition = center - glm::vec3(Viewx[num], Viewy[num], Viewz[num]);
	mesh.SetLight();
}

//��ȡBoundingBox
void GetBoundingBox(int num, string BoundingBox_filename)
{
	vector<float> Bound_X;
	vector<float> Bound_Y;
	glm::vec4 CoorBuffer;
	for (int i = 0; i < mesh_vertex[num].size(); i++)
	{
		CoorBuffer = Projection[num] * View[num] * Model[num] * glm::vec4(mesh_vertex[num][i], 1.0f);
		CoorBuffer = CoorBuffer * (1.0f / CoorBuffer.w);
		Bound_X.push_back(WIDTH*(CoorBuffer.x / 2 + 0.5f));
		Bound_Y.push_back(HEIGHT*(-CoorBuffer.y / 2 + 0.5f));
	}
	sort(Bound_X.begin(), Bound_X.end());
	sort(Bound_Y.begin(), Bound_Y.end());

	ofstream fout(BoundingBox_filename);
	fout << Bound_X[0] << " " << Bound_X[Bound_X.size() - 1] << endl;
	fout << Bound_Y[0] << " " << Bound_Y[Bound_Y.size() - 1] << endl;
	fout.close();
}

//��ȡsample��ά������ͶӰ
void GetProjection(int num)
{
	glm::vec3 UVbuffer;
	glm::vec4 CoorBuffer;
	sample_UV[num].clear();
	for (int i = 0; i < sample_vertex[num].size(); i++)
	{
		CoorBuffer = Projection[num] * View[num] * Model[num] * glm::vec4(sample_vertex[num][i], 1.0f);
		CoorBuffer = CoorBuffer * (1.0f / CoorBuffer.w);
		UVbuffer.x = WIDTH * (CoorBuffer.x / 2 + 0.5f);
		UVbuffer.y = HEIGHT * (-CoorBuffer.y / 2 + 0.5f);
		UVbuffer.z = 255 * (CoorBuffer.z * 0.5f + 0.5f);
		sample_UV[num].push_back(UVbuffer);
	}
}

//��ʾ����
void MultiDisplay(background& text, GLFWwindow* window, const char* vs_name, const char* fs_name)
{
	static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };//���ñ���ɫ
	Renderer mesh[NUM];
	cv::Mat render_results(HEIGHT, WIDTH, CV_8UC3);//���ڴ洢��Ⱦ�����ͼƬ��

	//��Ҫ��ʱ��Դ
	for (int i = 0; i < NUM; i++)
	{
		string mesh_filename = "../data/mesh" + to_string(i+1) + ".obj";
		string sample_filename = "../data/sampled_mesh" + to_string(i+1) + ".txt";
		string Hand_filename = "../data/eliminationFlagFile.txt";
		MultiInit(mesh[i], i, vs_name, fs_name, mesh_filename, sample_filename, Hand_filename);
	}

	//��Ⱦ��ɫͼ��
	glfwPollEvents();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for(int i=0;i<NUM;i++)
		MultiSetUniform(mesh[i], i, 0);

	text.Drawtexture();//ʹ�ñ�����ͼ

	for(int i=0;i<NUM;i++)
		mesh[i].Draw();

	//�洢��ɫͼ��
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_BGR, GL_UNSIGNED_BYTE, render_results.data);
	cv::flip(render_results, render_results, 0);
	cv::imwrite("../output/color.png", render_results);

	//��Ⱦ���ͼ��
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearBufferfv(GL_COLOR, 0, black);

	for(int i=0;i<NUM;i++)
		MultiSetUniform(mesh[i], i, 1);

	for (int i = 0; i<NUM; i++)
		mesh[i].Draw();

	//�洢���ͼ��
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_BGR, GL_UNSIGNED_BYTE, render_results.data);
	cv::flip(render_results, render_results, 0);
	cv::imwrite("../output/depth.png", render_results);

	//��ȡBoundingBox
	for (int i = 0; i < NUM; i++)
	{
		string bbox_filename = "../output/BoundingBox" + to_string(i+1) + ".txt";
		GetBoundingBox(i, bbox_filename);
	}

	//��ȡ�����front�Լ������������꣨int��
	ofstream front_fout[NUM];
	ofstream coor_fout[NUM];

	for (int j = 0; j < NUM; j++)
	{
		string front_filename = "../output/front" + to_string(j+1) + ".obj";
		string coor_filename = "../output/coor" + to_string(j+1) + ".txt";
		front_fout[j].open(front_filename);
		coor_fout[j].open(coor_filename);
		GetProjection(j);
		for (int i = 0; i < sample_UV[j].size(); i++)
		{
			if (HandJudger[i] == 0 && HandsOrNot == 1)continue;
			int u = (int)sample_UV[j][i].x;
			int v = (int)sample_UV[j][i].y;
			float depth_color = sample_UV[j][i].z;
			front_fout[j] << "v " << sample_vertex[j][i].x << " " << sample_vertex[j][i].y << " " << sample_vertex[j][i].z;
			if (v >= 0 && v < HEIGHT && u >= 0 && u < WIDTH &&
				abs(depth_color - (float)render_results.at<cv::Vec3b>(v, u)[0]) < 1)//ʵ����������ͼ�ϵ������Ϣ��Ƚ�
			{
				front_fout[j] << " 0.0 1.0 0.0" << endl;
				coor_fout[j] << i << " " << u << " " << v << endl;
			}
			else
			{
				front_fout[j] << " 1.0 0.0 0.0" << endl;
			}
		}
		front_fout[j].close();
		coor_fout[j].close();
	}
	
	//��ȡjoints����
	for (int i = 0; i < NUM; i++)
	{
		string Joints_filename = "../output/joints_coor" + to_string(i+1) + ".txt";
		string dataJoints_filename = "../data/joints" + to_string(i+1) + ".txt";
		Cal_Joints(dataJoints_filename, Joints_filename, Model[i], View[i], Projection[i]);
	}

	//��ȡsegmentationͼ��
	smpl segment[NUM];
	for (int i = 0; i < NUM; i++)
	{
		string segment_filename = "../output/segment" + to_string(i + 1) + ".png";
		string smpl_filename = "../data/smpl" + to_string(i + 1) + ".obj";
		string sample_filename = "../data/sampled_mesh" + to_string(i + 1) + ".txt";
		segment[i].Init("../glsl/vs_smpl.glsl", "../glsl/fs_smpl.glsl", smpl_filename, "../data/from_SMPL6890_to_IUV_norm.txt", sample_filename, HandJudger);//�ָ�ģ�ͳ�ʼ��
		segment[i].display(window, Model[i], View[i], Projection[i], segment_filename);//�ָ�ģ�ͷֱ𵥶���ʾ
	}

	//��������ָ�ͼ��
	glfwPollEvents();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearBufferfv(GL_COLOR, 0, black);

	cv::Mat segment_results(HEIGHT, WIDTH, CV_8UC3);

	for(int i=0;i<NUM;i++)
		segment[i].MultiSegDraw(window, Model[i], View[i], Projection[i]);

	glReadPixels(0, 0, WIDTH, HEIGHT, GL_BGR, GL_UNSIGNED_BYTE, segment_results.data);
	cv::flip(segment_results, segment_results, 0);
	cv::imwrite("../output/complete_segment.png", segment_results);//���˷ָ�ģ��д��

	//��������
}

//����ϣ�����ı�view����������ģ�Ͷ���ͬһ��view�£�ֻ����model������ת��ƽ�ƣ�����һ�����Է�ֹ���䣬����������

