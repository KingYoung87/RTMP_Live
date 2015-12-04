/*************************************************************
//module_name：	推流器
//module_func：	将本地文件/实时音视频流解码输出到URL地址，
				通过rtmp进行流推送
//module_owner:	Young
//module_time:	2015-08-17 15:43
**************************************************************/
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <math.h>
#include <string>
#include <map>
#include <vector>
using namespace std;

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/mathematics.h"
#include "libswresample/swresample.h" 
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"  
#include "libavutil/time.h"  
#include "libavdevice/avdevice.h" 
#include "inttypes.h"
#include "SDL.h"
#include "SDL_thread.h"
};
#include "afxcmn.h"
#include "afxwin.h"
#undef main

//DirectShow
#include "dshow.h"  //包含ICreateDevEnum

//opengl
#include "freeglut.h"

#define COLOR_BLACK	 RGB(0, 0, 0)
#define AUDIO_BUF_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define FF_ALLOC_EVENT   (SDL_USEREVENT)
#define FF_AUDIO_REFRESH_EVENT (SDL_USEREVENT + 1)
#define FF_VIDEO_REFRESH_EVENT (SDL_USEREVENT + 2)
#define FF_BREAK_EVENT (SDL_USEREVENT + 3)
#define FF_QUIT_EVENT    (SDL_USEREVENT + 10)

#define MAX_FILE_PATH  320

enum DeviceType{
	n_Video = 0,		//视频
	n_Audio = 1		//音频
};
typedef struct AudioParams {
	int freq;
	int channels;
	int channel_layout;
	enum AVSampleFormat fmt;
} AudioParams;

enum ShowMode {
	SHOW_MODE_NONE = -1, 
	SHOW_MODE_VIDEO = 0, 
	SHOW_MODE_WAVES = 1, 
	SHOW_MODE_RDFT = 2, 
	SHOW_MODE_NB = 3
};
typedef struct stream_info{
	AVFormatContext		*m_pFormatCtx;
	SDL_Window			*m_pShowScreen;			//音视频显示SDL窗口
	SDL_Surface			*m_pScreenSurface;		//与screen绑定的变量
	int					 m_xLeft;				//显示窗体的坐标及大小
	int					 m_yTop;
	int					 m_width;
	int					 m_height;
	int					 m_iAbortRequest;		//退出标记
	int					 m_iRefresh;				//刷新标记
	int					 m_iShowMode;			//显示模式
	int					 m_iPaused;				//暂停标记
	SDL_Renderer			*m_pSdlRender;
	SDL_Texture			*m_pSdlTexture;
	/************************音频相关参数-start*********************/
	SDL_Thread			*m_pAudioThr;		//音频测试线程
	SDL_Thread			*m_pAudioRefreshThr;	//音频刷新线程句柄
	AVStream				*m_pAudioStream;		//音频流
	AVFrame				*m_pAudioFrame;		//音频帧
	AVAudioFifo			*m_pAudioFifo;
	SDL_mutex			*m_pAudioMutex;
	AudioParams			 m_AudioPrm;
	uint8_t				*m_pAudioBuf;
	int					 m_iAudioBufSize;
	int					 m_iAudioBufIndex;
	int					 m_iAduioPktSize;
	int					 m_iAudioWriteBufSize;
	int					 m_iAudioLastStart;
	uint8_t				 m_uSilenceBuf[AUDIO_BUF_SIZE];
	int16_t				 m_iSampleArray[SAMPLE_ARRAY_SIZE];
	int					 m_iSampleArrayIndex;
	/************************音频相关参数-end***********************/

	/************************视频相关参数-satrt*********************/
	SDL_Thread			*m_pVideoThr;			//视频测试线程
	SDL_Thread			*m_pVideoRefreshThr;		//视频刷新线程句柄
	AVFifoBuffer			*m_pVideoFifo;
	SDL_mutex			*m_pVideoMutex;
	AVStream				*m_pVideoStream;			//视频流
	uint8_t				*m_pVideoDecPicSize;		//视频解码Pic大小
	uint8_t				*m_pPushPicSize;			//推送Pic大小
	SwsContext			*m_pVideoSwsCtx;
	
	/************************视频相关参数-end***********************/
}struct_stream_info;

// CLS_DlgStreamPusher 对话框
class CLS_DlgStreamPusher : public CDialog
{
// 构造
public:
	CLS_DlgStreamPusher(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_FFMPEG_STREAM_PUSHER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	CWinThread *m_pThreadEvent;					//事件处理线程

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedChkSrcType();
	afx_msg void OnBnClickedBtnOpenLocalFile();
	afx_msg void OnBnClickedBtnPreview();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedChkShowVideo();
	afx_msg void OnBnClickedBtnDeviceAudioTestStop();
	afx_msg void OnBnClickedBtnDeviceVideoTestStop();
	afx_msg void OnBnClickedChkWriteFile();
	afx_msg void OnCbnSelchangeCobDeviceVideo();
	afx_msg void OnCbnSelchangeCobResolution();
	afx_msg void OnBnClickedBtnRefreshvideo();

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	//控件定义
	CEdit	m_edtLocalFilePath;
	CEdit	m_edtPusherAddr;
	CButton m_chkSrcType;
	CButton m_chkShowVideo;
	CButton m_chkWriteFile;
	CButton m_btnOpenLocalFile;
	CStatic m_stcPreview;
	CComboBox m_cboDeviceVideo;
	CComboBox m_cboDeviceAudio;
	CComboBox m_cboResolution;

private:
	/**********************
	method: 界面信息初始化
	return: void
	**********************/
	void InitDlgItem();

	/**********************
	method: 数据信息初始化
	return: void
	**********************/
	void InitData();

	/**********************
	method: SDL初始化
	return: void
	**********************/
	void InitSdl();

	/**********************
	method: 界面信息反初始化
	return: void
	**********************/
	void UnInitInfo();
	
	/**********************
	method: 获取设备信息
	return: void
	**********************/
	void GetDevice();

	/**********************
	method: 获取连接的视频与音频设备
	param :	_iDeviceType:设备类型
	return: >=0:成功
			<0:失败
	**********************/
	int  GetDeviceInfo(int _iDeviceType);

	/**********************
	method: 获取分辨率
	param
	return: 分辨率信息
	**********************/
	void GetResolution(int _iVideoIndex);

	/**********************
	method: 绑定滤波器
	param
	return: true:成功 false:失败
	**********************/
	bool BindFilter(int iDeviceID, IBaseFilter **pOutFilter, DeviceType deviceType);

	/**********************
	method: 获取音视频信息结构体
	param :	
	return: 音视频信息结构体指针
	**********************/
	struct_stream_info* GetStreamStrcInfo();

	/**********************
	method: 处理显示区域
	param : 
	return: 
	**********************/
	void	 FillDisplayRect();

	/**********************
	method: 窗体刷新
	param : opaque:流参数信息
	return:
	**********************/
	void RefreshScreen(void *opaque);

	/**********************
	method: 窗体显示
	param : _pstrct_streaminfo:流参数信息
	return:
	**********************/
	void DisplayScreen(struct_stream_info *_pstrct_streaminfo);

	/**********************
	method: 窗体显示音频波形
	param : _pstrct_streaminfo:流参数信息
	return:
	**********************/
	void DisplayAudio(struct_stream_info *_pstrct_streaminfo);

	/**********************
	method: 窗体显示视频
	param : _pstrct_streaminfo:流参数信息
	return:
	**********************/
	void DisplayVideo(struct_stream_info *_pstrct_streaminfo);

	/**********************
	method: 窗体区域填充
	param : screen:窗体
			x:左上角点横坐标
			y:左上角点纵坐标
			w:窗体宽
			h:窗体高
			color:颜色值
	return:
	**********************/
	static void FillRec(SDL_Surface *screen,
		int x, int y, int w, int h, int color);

	/**********************
	method: 停止测试
	return:
	**********************/
	void StopTest();

	/**********************
	method: 停止流推送
	param:	opaque:流信息
	return:
	**********************/
	void StopStream(void *opaque);

	/**********************
	method: 打开摄像头
	param:
	return: <0：打开失败
			=0：打开成功
	**********************/
	int OpenCamera();

	/**********************
	method: 打开麦克风
	param:
	return: <0：打开失败
			=0：打开成功
	**********************/
	int OpenAduio();

	/**********************
	method: 打开推流地址
	param:
	return: <0：打开失败
			=0：打开成功
	**********************/
	int OpenRtmpUrl();

	CString									m_cstrFilePath;	//推送文件路径
	HBRUSH									m_bkBrush;		//背景刷
	BOOL										m_blUrl;			//是否网络流推送

	std::map<int, std::vector<std::string>>	m_mapDeviceInfo;	//设备信息容器

public:
	/**********************
	method: 获取当前选择的音视频设备名臣
	param :	_iDeviceType:设备类型
	return: 设备名称
	**********************/
	char*  GetDeviceName(int _iDeviceType);

	/**********************
	method: 事件处理器
	param : _pstrct_streaminfo:流参数信息
	return:
	**********************/
	void EventLoop(struct_stream_info *_pstrct_streaminfo);

	struct_stream_info*						m_pStreamInfo;	//音视频全局结构体
	BOOL										m_blVideoShow;	//是否显示视频
	BOOL										m_blAudioShow;	//是否显示音频
	BOOL										m_blPushStream;	//是否进行推流
	CString									m_cstrPushAddr;	//推流地址
	AVFormatContext						   *m_pFmtVideoCtx;	//视频采集format
	AVFormatContext						   *m_pFmtAudioCtx;	//音频采集format
	AVFormatContext						   *m_pFmtRtmpCtx;	//rtmp推送format
	AVCodecContext						   *m_pCodecVideoCtx;//视频采集解码器信息
	AVCodecContext						   *m_pCodecAudioCtx;//音频采集解码器信息
	int										m_iVideoIndex;	//视频采集解码器索引
	int										m_iAudioIndex;	//音频采集解码器索引
	int										m_iVideoOutIndex;//推送视频解码器索引
	int										m_iAudioOutIndex;//推送音频解码器索引
	int										m_iSrcVideoHeight;//源视频高
	int										m_iSrcVideoWidth;//源视频宽
	int										m_iDstVideoHeight;//输出视频高
	int										m_iDstVideoWidth;//输出视频宽
	SDL_Thread							   *m_pPushThrid;	//推流线程
	map<int, map<int, int>>					m_mapResolution;	//分辨率容器
};
