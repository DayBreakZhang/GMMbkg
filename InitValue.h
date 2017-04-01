#pragma once

#include"InfoRetrieval.h"
#include"CmGMM.h"

class InitValue
{
public:
	InfoRetrieval m_info;

	std::vector<int> borderIdx, innerIdx;

	CmGMM _bGMM, _fGMM; // Background and foreground GMM
	Mat _bGMMidx1i, _fGMMidx1i;	// Background and foreground GMM components, supply memory for GMM, not used for Grabcut 

public:
	InitValue()
		:_bGMM(4), _fGMM(4){}
	void GetBgvalue(cv::Mat& unaryMap, const std::string& pic);

	void getIdxs();

	void clusterBorder(cv::Mat& borderlabels, std::vector<cv::Vec3f>& border);
	int removeCluster(double sumclus[3], cv::Mat& borderlabels, std::vector<cv::Vec3f>& border);
	void getSalFromClusteredBorder(cv::Mat& unaryMap, bool illustrate = false);

	void getSalFromGmmBorder(cv::Mat& unaryMap, const std::string& pic);
};

class Covariance
{
public:
	cv::Mat cov;

	cv::Mat inverseCovs;
	//double inverseCovs[componentsCount][3][3]; //Э����������  
	double mean[3];
	double covDeterms;  //Э���������ʽ  

	double sums[3];
	double prods[3][3];
	int sampleCounts;
	//int totalSampleCount;

public:
	void initLearning()
	{
		sums[0] = sums[1] = sums[2] = 0;
		prods[0][0] = prods[0][1] = prods[0][2] = 0;
		prods[1][0] = prods[1][1] = prods[1][2] = 0;
		prods[2][0] = prods[2][1] = prods[2][2] = 0;
		sampleCounts = 0;
	}

	void addSample(const cv::Vec3d color)
	{
		sums[0] += color[0]; sums[1] += color[1]; sums[2] += color[2];
		prods[0][0] += color[0] * color[0]; prods[0][1] += color[0] * color[1]; prods[0][2] += color[0] * color[2];
		prods[1][0] += color[1] * color[0]; prods[1][1] += color[1] * color[1]; prods[1][2] += color[1] * color[2];
		prods[2][0] += color[2] * color[0]; prods[2][1] += color[2] * color[1]; prods[2][2] += color[2] * color[2];
		sampleCounts++;
	}

	void calcInverseCovAndDeterm(double* c)
	{
		double dtrm =
			covDeterms = c[0] * (c[4] * c[8] - c[5] * c[7]) - c[1] * (c[3] * c[8] - c[5] * c[6])
			+ c[2] * (c[3] * c[7] - c[4] * c[6]);

		//��C++�У�ÿһ�����õ��������Ͷ�ӵ�в�ͬ������, ʹ��<limits>����Ի�  
		//����Щ�����������͵���ֵ���ԡ���Ϊ�����㷨�Ľضϣ�����ʹ�ã���a=2��  
		//b=3ʱ 10*a/b == 20/b������������ô���أ�  
		//���С������epsilon�����������ˣ�С����ͨ��Ϊ���ø����������͵�  
		//����1����Сֵ��1֮������ʾ����dtrm���������С��������ô������Ϊ�㡣  
		//������ʽ��֤dtrm>0��������ʽ�ļ�����ȷ��Э����Գ�������������ʽ����0����  
		CV_Assert(dtrm > std::numeric_limits<double>::epsilon());
		//���׷��������  
		inverseCovs = cv::Mat_<double>(3, 3);
		inverseCovs.at<double>(0,0) = (c[4] * c[8] - c[5] * c[7]) / dtrm;
		inverseCovs.at<double>(1, 0) = -(c[3] * c[8] - c[5] * c[6]) / dtrm;
		inverseCovs.at<double>(2, 0) = (c[3] * c[7] - c[4] * c[6]) / dtrm;
		inverseCovs.at<double>(0, 1) = -(c[1] * c[8] - c[2] * c[7]) / dtrm;
		inverseCovs.at<double>(1, 1) = (c[0] * c[8] - c[2] * c[6]) / dtrm;
		inverseCovs.at<double>(2, 1) = -(c[0] * c[7] - c[1] * c[6]) / dtrm;
		inverseCovs.at<double>(0, 2) = (c[1] * c[5] - c[2] * c[4]) / dtrm;
		inverseCovs.at<double>(1, 2) = -(c[0] * c[5] - c[2] * c[3]) / dtrm;
		inverseCovs.at<double>(2, 2) = (c[0] * c[4] - c[1] * c[3]) / dtrm;
	}

	void endLearning()
	{
		int n = sampleCounts;
		const double variance = 0.01;
		mean[0] = sums[0] / n; mean[1] = sums[1] / n; mean[2] = sums[2] / n;

		//�����ci����˹ģ�͵�Э����  
		cov = cv::Mat_<double>(3, 3);
		double c[9];
		
		c[0] = cov.at<double>(0, 0) = prods[0][0] / n - mean[0] * mean[0];
		c[1] = cov.at<double>(0, 1) = prods[0][1] / n - mean[0] * mean[1];
		c[2] = cov.at<double>(0, 2) = prods[0][2] / n - mean[0] * mean[2];
		c[3] = cov.at<double>(1, 0) = prods[1][0] / n - mean[1] * mean[0];
		c[4] = cov.at<double>(1, 1) = prods[1][1] / n - mean[1] * mean[1];
		c[5] = cov.at<double>(1, 2) = prods[1][2] / n - mean[1] * mean[2];
		c[6] = cov.at<double>(2, 0) = prods[2][0] / n - mean[2] * mean[0];
		c[7] = cov.at<double>(2, 1) = prods[2][1] / n - mean[2] * mean[1];
		c[8] = cov.at<double>(2, 2) = prods[2][2] / n - mean[2] * mean[2];

		//�����ci����˹ģ�͵�Э���������ʽ  
		double dtrm = c[0] * (c[4] * c[8] - c[5] * c[7]) - c[1] * (c[3] * c[8] - c[5] * c[6]) + c[2] * (c[3] * c[7] - c[4] * c[6]);
		if (dtrm <= std::numeric_limits<double>::epsilon())
		{
			//�൱���������ʽС�ڵ���0�����Խ���Ԫ�أ����Ӱ��������������  
			//Ϊ�˻������ȣ�Э������󣨲���������󣬵�����ļ�����Ҫ��������󣩡�  
			// Adds the white noise to avoid singular covariance matrix.
			c[0] += variance;
			c[4] += variance;
			c[8] += variance;
			cov.at<double>(0, 0) += variance;
			cov.at<double>(1, 1) += variance;
			cov.at<double>(2, 2) += variance;
		}

		//�����ci����˹ģ�͵�Э�������Inverse������ʽDeterminant  
		calcInverseCovAndDeterm(c);
	}

};