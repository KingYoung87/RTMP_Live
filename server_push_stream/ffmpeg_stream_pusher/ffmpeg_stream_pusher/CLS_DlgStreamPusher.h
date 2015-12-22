/*************************************************************
//module_name��	������
//module_func��	�������ļ�/ʵʱ����Ƶ�����������URL��ַ��
				ͨ��rtmp����������
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
#include "dshow.h"  //����ICreateDevEnum

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
	n_Video = 0,		//��Ƶ
	n_Audio = 1		//��Ƶ
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
	SDL_Window			*m_pShowScreen;			//����Ƶ��ʾSDL����
	SDL_Surface			*m_pScreenSurface;		//��screen�󶨵ı���
	int					 m_xLeft;				//��ʾ��������꼰��С
	int					 m_yTop;
	int					 m_width;
	int					 m_height;
	int					 m_iAbortRequest;		//�˳����
	int					 m_iRefresh;				//ˢ�±��
	int					 m_iShowMode;			//��ʾģʽ
	int					 m_iPaused;				//��ͣ���
	SDL_Renderer			*m_pSdlRender;
	SDL_Texture			*m_pSdlTexture;
	/************************��Ƶ��ز���-start*********************/
	SDL_Thread			*m_pAudioThr;		//��Ƶ�����߳�
	SDL_Thread			*m_pAudioRefreshThr;	//��Ƶˢ���߳̾��
	AVStream				*m_pAudioStream;		//��Ƶ��
	AVFrame				*m_pAudioFrame;		//��Ƶ֡
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
	/************************��Ƶ��ز���-end***********************/

	/************************��Ƶ��ز���-satrt*********************/
	SDL_Thread			*m_pVideoThr;			//��Ƶ�����߳�
	SDL_Thread			*m_pVideoRefreshThr;		//��Ƶˢ���߳̾��
	AVFifoBuffer			*m_pVideoFifo;
	SDL_mutex			*m_pVideoMutex;
	AVStream				*m_pVideoStream;			//��Ƶ��
	uint8_t				*m_pPushPicSize;			//����Pic��С
	SwsContext			*m_pVideoSwsCtx;			//��Ƶ�仯ctx
	
	/************************��Ƶ��ز���-end***********************/
}struct_stream_info;

typedef struct PUSH_FILE_STREAM{
	CString			m_cstrFilePath;	//�ļ�����
	AVFormatContext *m_pFmtFileCtx;	//�ļ���������
}STRCT_PUSH_FILE;

// CLS_DlgStreamPusher �Ի���
class CLS_DlgStreamPusher : public CDialog
{
// ����
public:
	CLS_DlgStreamPusher(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_FFMPEG_STREAM_PUSHER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedChkSrcType();
	afx_msg void OnBnClickedBtnOpenLocalFile();
	afx_msg void OnBnClickedBtnPreview();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedChkShowVideo();
	afx_msg void OnBnClickedChkWriteFile();
	afx_msg void OnCbnSelchangeCobDeviceVideo();
	afx_msg void OnCbnSelchangeCobResolution();
	afx_msg void OnBnClickedBtnRefreshvideo();
	afx_msg void OnBnClickedBtnOpenLocalFiles();
	afx_msg void OnCbnSelchangeCboPushAddr();
	afx_msg void OnNMDblclkListLocalFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickListLocalFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	//�ؼ�����
	CEdit	m_edtFrameRate;
	CButton m_chkSrcType;
	CButton m_chkShowVideo;
	CButton m_chkWriteFile;
	CButton m_btnOpenLocalFile;
	CButton m_btnOpenFiles;
	CStatic m_stcSelFile;
	CStatic m_stcPreview;
	CStatic m_stcPushStatus;
	CComboBox m_cboDeviceVideo;
	CComboBox m_cboDeviceAudio;
	CComboBox m_cboResolution;
	CComboBox m_cboPushAddr;
	CListCtrl m_lstLocalFiles;

	CMenu	m_Rmenu;

private:
	/**********************
	method: ������Ϣ��ʼ��
	return: void
	**********************/
	void InitDlgItem();

	/**********************
	method: ������Ϣ��ʼ��
	return: void
	**********************/
	void InitData();

	/**********************
	method: SDL��ʼ��
	return: void
	**********************/
	void InitSdl();

	/**********************
	method: ������Ϣ����ʼ��
	return: void
	**********************/
	void UnInitInfo();
	
	/**********************
	method: ��ȡ�豸��Ϣ
	return: void
	**********************/
	void GetDevice();

	/**********************
	method: ��ȡ���ӵ���Ƶ����Ƶ�豸
	param :	_iDeviceType:�豸����
	return: >=0:�ɹ�
			<0:ʧ��
	**********************/
	int  GetDeviceInfo(int _iDeviceType);

	/**********************
	method: ��ȡ�ֱ���
	param
	return: �ֱ�����Ϣ
	**********************/
	void GetResolution(int _iVideoIndex);

	/**********************
	method: ���˲���
	param
	return: true:�ɹ� false:ʧ��
	**********************/
	bool BindFilter(int iDeviceID, IBaseFilter **pOutFilter, DeviceType deviceType);

	/**********************
	method: ��ȡ����Ƶ��Ϣ�ṹ��
	param :	
	return: ����Ƶ��Ϣ�ṹ��ָ��
	**********************/
	struct_stream_info* GetStreamStrcInfo();

	/**********************
	method: ����ˢ��
	param : opaque:��������Ϣ
	return:
	**********************/
	void RefreshScreen(void *opaque);

	/**********************
	method: ������ʾ
	param : _pstrct_streaminfo:��������Ϣ
	return:
	**********************/
	void DisplayScreen(struct_stream_info *_pstrct_streaminfo);

	/**********************
	method: ������ʾ��Ƶ����
	param : _pstrct_streaminfo:��������Ϣ
	return:
	**********************/
	void DisplayAudio(struct_stream_info *_pstrct_streaminfo);

	/**********************
	method: ������ʾ��Ƶ
	param : _pstrct_streaminfo:��������Ϣ
	return:
	**********************/
	void DisplayVideo(struct_stream_info *_pstrct_streaminfo);

	/**********************
	method: �����������
	param : screen:����
			x:���Ͻǵ������
			y:���Ͻǵ�������
			w:�����
			h:�����
			color:��ɫֵ
	return:
	**********************/
	static void FillRec(SDL_Surface *screen,
		int x, int y, int w, int h, int color);

	/**********************
	method: ֹͣ����
	return:
	**********************/
	void StopTest();

	/**********************
	method: ֹͣ������
	param:	opaque:����Ϣ
	return:
	**********************/
	void StopStream(void *opaque);

	/**********************
	method: ������ͷ
	param:
	return: <0����ʧ��
			=0���򿪳ɹ�
	**********************/
	int OpenCamera();

	/**********************
	method: ����˷�
	param:
	return: <0����ʧ��
			=0���򿪳ɹ�
	**********************/
	int OpenAduio();

	/**********************
	method: ���ļ���Ϣ�����б�
	param:	_cstrFilePath: �ļ�·��
	return: <0������ʧ��
			=0������ɹ�
	**********************/
	int InsertFileList(CString _cstrFilePath);

	/**********************
	method: �����ļ��ڴ�
	param:	
	return: 
	**********************/
	void ClearFileMem();

	/**********************
	method: ��������ַ
	param:
	return: <0����ʧ��
			=0���򿪳ɹ�
	**********************/
	int OpenRtmpAddr();

	/**********************
	method: ��URL������ַ
	param:
	return: <0����ʧ��
			=0���򿪳ɹ�
	**********************/
	int OpenRtmpUrl();

	/**********************
	method: ���ļ�������ַ
	param:
	return: <0����ʧ��
			=0���򿪳ɹ�
	**********************/
	int OpenRtmpFile();

	/**********************
	method: ������Ƶ���Ŵ���
	param:
	return: <0������ʧ��
			=0�������ɹ�
	**********************/
	int CreateVideoWindow();

	/**********************
	method: ��ʼ����Ƶ���Ŵ���
	param:
	return: <0����ʼ��ʧ��
			=0����ʼ���ɹ�
	**********************/
	int InitVideoWindow();

	/**********************
	method: ��ȡ��ǰ�ļ����µ������ļ�
	param:
	return: 
	**********************/
	void GetFiles(CString _cstrFilePath);

	/**********************
	method: ���������ʾ�ؼ�
	param:
	return:
	**********************/
	void ShowControls(BOOL _blShow);

	/**********************
	method: ɾ���ļ�
	param:
	return:
	**********************/
	void OnDeleteItem();

	/**********************
	method: ����ļ�
	param:
	return:
	**********************/
	void OnClearItems();

	/**********************
	method: ����sdl�������Ϣ
	param:
	return: < 0: ����ʧ��
			= 0: ���³ɹ�
	**********************/
	int UpdateSdlInfo();

public:
	/**********************
	method: ��ȡ��ǰѡ�������Ƶ�豸����
	param :	_iDeviceType:�豸����
	return: �豸����
	**********************/
	char*  GetDeviceName(int _iDeviceType);

	/**********************
	method: �¼�������
	param : _pstrct_streaminfo:��������Ϣ
	return:
	**********************/
	void EventLoop(struct_stream_info *_pstrct_streaminfo);

	/**********************
	method: �������ļ�
	param:	_cstrFilePath: �ļ�·��
	return: AVFormatContext* �ļ���Ϣ
	**********************/
	AVFormatContext* OpenFile(CString _cstrFilePath);

	HBRUSH									m_bkBrush;		//����ˢ
	struct_stream_info*						m_pStreamInfo;	//����Ƶȫ�ֽṹ��
	BOOL										m_blVideoShow;	//�Ƿ���ʾ��Ƶ
	BOOL										m_blAudioShow;	//�Ƿ���ʾ��Ƶ
	BOOL										m_blCreateVideoWin;//�Ƿ񴴽���Ƶ����
	BOOL										m_blUrl;			//�Ƿ�����������
	BOOL										m_blPushReady;	//�Ƿ�����OK
	BOOL										m_blPushStart;	//�Ƿ�ʼ����	
	BOOL										m_blPreview;		//�Ƿ������ƵԤ��
	BOOL										m_blPushSuccess;	//�Ƿ����ͳɹ�
	BOOL										m_blPushSingle;	//�Ƿ����͵��ļ�
	BOOL										m_blPushCircle;	//�Ƿ�ѭ�������ļ�

	CString									m_cstrPushAddr;	//������ַ
	CString									m_cstrFilePath;	//�����ļ�·��
	AVFormatContext						   *m_pFmtVideoCtx;	//��Ƶ�ɼ�format
	AVFormatContext						   *m_pFmtAudioCtx;	//��Ƶ�ɼ�format
	AVFormatContext						   *m_pFmtRtmpCtx;	//rtmp����format
	AVCodecContext						   *m_pCodecVideoCtx;//��Ƶ�ɼ���������Ϣ
	AVCodecContext						   *m_pCodecAudioCtx;//��Ƶ�ɼ���������Ϣ
	int										m_iVideoIndex;	//��Ƶ�ɼ�����������
	int										m_iAudioIndex;	//��Ƶ�ɼ�����������
	int										m_iVideoOutIndex;//������Ƶ����������
	int										m_iAudioOutIndex;//������Ƶ����������
	int										m_iSrcVideoHeight;//Դ��Ƶ��
	int										m_iSrcVideoWidth;//Դ��Ƶ��
	int										m_iDstVideoHeight;//�����Ƶ��
	int										m_iDstVideoWidth;//�����Ƶ��
	int										m_iFrameRate;	//֡��
	int										m_iLstSelIndex;	//�б�ѡ����
	int										m_iWindowWidth;	//������Ŀ�
	int										m_iWiddowHeight;	//������ĸ�
	SDL_Thread							   *m_pPushStreamThrid;//�����߳�
	SDL_Thread							   *m_pPushFileThrid;//���ļ��߳�
	map<int, std::vector<std::string>>		m_mapDeviceInfo;	//�豸��Ϣ����
	map<int, map<int, int>>					m_mapResolution;	//�ֱ�������
	map<int, STRCT_PUSH_FILE>					m_mapPushFile;	//�����ļ�����

	STRCT_PUSH_FILE							m_strctPushFile;	//�����ļ��ṹ��
};
