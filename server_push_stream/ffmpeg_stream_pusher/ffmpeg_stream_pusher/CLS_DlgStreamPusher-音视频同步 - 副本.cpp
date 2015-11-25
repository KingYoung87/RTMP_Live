
// ffmpeg_stream_pusherDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CLS_StreamPusher.h"
#include "CLS_DlgStreamPusher.h"
#include ".\common\Public_Function.h"
//#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WRITE_FILE 1
#define USE_H264	1
#define USE_AAC	1
#define ENCODE_FPS 60//30.000030

FILE *pFile = NULL;
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;
int tempsize = 4608;
char *tempcontenx = (char*)malloc(4608);//4096~4608
int  tempcontenxlen;
static int64_t audio_callback_time;
static double a = 0.8;//缩小比例

static char *dup_wchar_to_utf8(wchar_t *w)
{
	char *s = NULL;
	int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
	s = (char *)av_malloc(l);
	if (s)
		WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, 0, 0);
	return s;
}

static inline int compute_mod(int a, int b)
{
	return a < 0 ? a%b + b : a%b;
}

static int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index){
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
		return 0;
	while (1) {
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame){
			ret = 0;
			break;
		}
		TRACE("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}

// CLS_DlgStreamPusher 对话框
CLS_DlgStreamPusher::CLS_DlgStreamPusher(CWnd* pParent /*=NULL*/)
	: CDialog(CLS_DlgStreamPusher::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_cstrFilePath = "";
	m_blUrl = FALSE;
	m_bkBrush = NULL;
	m_pStreamInfo = NULL;
	m_mapDeviceInfo.clear();
	m_blVideoShow = FALSE;
	m_blAudioShow = FALSE;
	m_blPushStream = FALSE;
	m_blOut = FALSE;
	m_cstrPushAddr = "";
	m_pFmtVideoCtx = NULL;
	m_pFmtAudioCtx = NULL;
	m_pFmtRtmpCtx = NULL;
	m_pCodecVideoCtx = NULL;
	m_pPictureBuf = NULL;
	m_pPushThrid = NULL;
	m_iPictureSize = -1;
	m_iVideoIndex = -1;
	m_iVideoOutIndex = -1;
	m_iAudioOutIndex = -1;
}

void CLS_DlgStreamPusher::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHK_SRC_TYPE, m_chkSrcType);
	DDX_Control(pDX, IDC_EDT_LOACL_FILE_PATH, m_edtLocalFilePath);
	DDX_Control(pDX, IDC_BTN_OPEN_LOCAL_FILE, m_btnOpenLocalFile);
	DDX_Control(pDX, IDC_EDT_PUSHER_ADDR, m_edtPusherAddr);
	DDX_Control(pDX, IDC_STC_PREVIEW, m_stcPreview);
	DDX_Control(pDX, IDC_COB_DEVICE_VIDEO, m_cboDeviceVideo);
	DDX_Control(pDX, IDC_COB_DEVICE_AUDIO, m_cboDeviceAudio);
	DDX_Control(pDX, IDC_CHK_SHOW_VIDEO, m_chkShowVideo);
	DDX_Control(pDX, IDC_COB_RESOLUTION, m_cboResolution);
	DDX_Control(pDX, IDC_CHK_WRITE_FILE, m_chkWriteFile);
}

BEGIN_MESSAGE_MAP(CLS_DlgStreamPusher, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CLS_DlgStreamPusher::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHK_SRC_TYPE, &CLS_DlgStreamPusher::OnBnClickedChkSrcType)
	ON_BN_CLICKED(IDC_BTN_OPEN_LOCAL_FILE, &CLS_DlgStreamPusher::OnBnClickedBtnOpenLocalFile)
	ON_BN_CLICKED(IDC_BTN_PREVIEW, &CLS_DlgStreamPusher::OnBnClickedBtnPreview)
	ON_BN_CLICKED(IDCANCEL, &CLS_DlgStreamPusher::OnBnClickedCancel)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTN_DEVICE_VIDEO_TEST, &CLS_DlgStreamPusher::OnBnClickedBtnDeviceVideoTest)
	ON_BN_CLICKED(IDC_BTN_DEVICE_AUDIO_TEST, &CLS_DlgStreamPusher::OnBnClickedBtnDeviceAudioTest)
	ON_BN_CLICKED(IDC_BTN_DEVICE_AUDIO_TEST_STOP, &CLS_DlgStreamPusher::OnBnClickedBtnDeviceAudioTestStop)
	ON_BN_CLICKED(IDC_BTN_DEVICE_VIDEO_TEST_STOP, &CLS_DlgStreamPusher::OnBnClickedBtnDeviceVideoTestStop)
	ON_BN_CLICKED(IDC_CHK_SHOW_VIDEO, &CLS_DlgStreamPusher::OnBnClickedChkShowVideo)
	ON_BN_CLICKED(ID_BTN_PUSH_CODE, &CLS_DlgStreamPusher::OnBnClickedBtnPushCode)
	ON_BN_CLICKED(IDC_CHK_WRITE_FILE, &CLS_DlgStreamPusher::OnBnClickedChkWriteFile)
END_MESSAGE_MAP()


// CLS_DlgStreamPusher 消息处理程序

BOOL CLS_DlgStreamPusher::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//数据结构初始化
	InitData();

	//界面信息初始化
	InitDlgItem();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CLS_DlgStreamPusher::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CLS_DlgStreamPusher::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLS_DlgStreamPusher::OnBnClickedChkSrcType()
{
	//选择推送源类型
	if (m_chkSrcType.GetCheck()){
		m_blUrl = TRUE;
		m_edtLocalFilePath.EnableWindow(FALSE);
		m_btnOpenLocalFile.EnableWindow(FALSE);
	}
	else{
		m_blUrl = FALSE;
		m_edtLocalFilePath.EnableWindow(TRUE);
		m_btnOpenLocalFile.EnableWindow(TRUE);
	}
}


void CLS_DlgStreamPusher::OnBnClickedBtnOpenLocalFile()
{
	//打开本地文件
	CString szFilter = _T("All Files (*.*)|*.*|avi Files (*.avi)|*.avi|rmvb Files (*.rmvb)|*.rmvb|3gp Files (*.3gp)|*.3gp|mp3 Files (*.mp3)|*.mp3|mp4 Files (*.mp4)|*.mp4|mpeg Files (*.ts)|*.ts|flv Files (*.flv)|*.flv|mov Files (*.mov)|*.mov||");
	CFileDialog dlg(TRUE, NULL, NULL, OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, szFilter, NULL);
	if (IDOK == dlg.DoModal())
	{
		m_cstrFilePath = dlg.GetPathName();
	}
	else
	{
		MessageBox(_T("获取文件名称失败 请重新加载"), NULL, MB_OK);
		m_cstrFilePath = "";
		return;
	}
}


void CLS_DlgStreamPusher::OnBnClickedBtnPreview()
{
	//源视频流预览
	if (m_cstrFilePath == ""){
		MessageBox("请选择进行推送的文件!");
		return;
	}

	//进行文件解析进行显示
	if (m_blUrl){
		if (UrlStreamColParse() < 0){
			goto END;
		}
	}
	else{
		if (LocalFileParse() < 0){
			goto END;
		}
	}

END:
	MessageBox("解析失败，请确认网络连接无误!");
	return;
}


void CLS_DlgStreamPusher::OnBnClickedCancel()
{
	//先释放相关资源
	UnInitInfo();

	CoUninitialize();

	//退出程序
	CDialog::OnCancel();
}
static int audio_refresh_thread(void *opaque)
{
	struct_stream_info *pstrct_stream = (struct_stream_info *)opaque;
	if (NULL == pstrct_stream){
		TRACE("NULL == pstrct_stream\n");
		return -1;
	}
	while (!pstrct_stream->m_abort_request) {
		SDL_Event event;
		event.type = FF_AUDIO_REFRESH_EVENT;
		event.user.data1 = opaque;
		if (!pstrct_stream->m_refresh) {
			pstrct_stream->m_refresh = 1;
			SDL_PushEvent(&event);
		}
		//FIXME ideally we should wait the correct time but SDLs event passing is so slow it would be silly
		av_usleep(pstrct_stream->m_pAudioStream && pstrct_stream->m_show_mode != SHOW_MODE_VIDEO ? 20 * 1000 : 5000);
	}
	return 0;
}

static int video_refresh_thread(void *opaque)
{
	struct_stream_info* pstrct_streaminfo = (struct_stream_info*)opaque;
	if (NULL == pstrct_streaminfo){
		TRACE("NULL == pstrct_streaminfo");
		return -1;
	}
	while (!pstrct_streaminfo->m_abort_request) {
		SDL_Event event;
		event.type = FF_VIDEO_REFRESH_EVENT;
		SDL_PushEvent(&event);
	}
	SDL_Delay(40);
	pstrct_streaminfo->m_abort_request = 0;
	SDL_Event event;
	event.type = FF_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

UINT Thread_Event(LPVOID lpParam){
	CLS_DlgStreamPusher *dlg = (CLS_DlgStreamPusher *)lpParam;
	if (dlg != NULL){
		dlg->event_loop(dlg->m_pStreamInfo);
	}
	return 0;
}

void CLS_DlgStreamPusher::InitDlgItem()
{
	//进行界面信息的初始化操作
	//默认是网络流推送
	m_chkSrcType.SetCheck(1);

	m_blUrl = TRUE;
	m_edtLocalFilePath.EnableWindow(FALSE);
	m_btnOpenLocalFile.EnableWindow(FALSE);
	m_chkWriteFile.SetCheck(TRUE);
	m_chkShowVideo.SetCheck(TRUE);

	if (NULL == m_bkBrush)
	{
		m_bkBrush = CreateSolidBrush(COLOR_BLACK);
	}

	//初始化网络库
	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	//初始化directshow库
	if (FAILED(CoInitialize(NULL))){
		TRACE("CoInitialize Failed!\r\n");
		return;
	}

	//获取当前连接的视频设备
	int iRet= GetDeviceInfo(n_Video);
	if (iRet < 0){
		TRACE("获取视频设备失败！");
		return;
	}

	//获取当前连接输入的音频设备
	iRet = GetDeviceInfo(n_Audio);
	if (iRet < 0){
		TRACE("获取音频设备失败！");
		return;
	}

	//将获取到的设备信息插入下拉框中
	std::map<int, std::vector<std::string>>::iterator iter = m_mapDeviceInfo.begin();
	for (; iter != m_mapDeviceInfo.end(); iter ++){
		if (n_Video == iter->first){
			//视频设备
			for (int i = 0; i < iter->second.size(); i ++){
				int iCount = m_cboDeviceVideo.GetCount();
				m_cboDeviceVideo.InsertString(iCount, iter->second[i].c_str());
			}
			m_cboDeviceVideo.SetCurSel(0);
		}
		else if (n_Audio == iter->first){
			//音频设备
			for (int i = 0; i < iter->second.size(); i ++){
				int iCount = m_cboDeviceAudio.GetCount();
				m_cboDeviceAudio.InsertString(iCount, iter->second[i].c_str());
			}
			m_cboDeviceAudio.SetCurSel(0);
		}
	}

	m_cboResolution.SetCurSel(0);

	//SDL初始化
	int flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
	int  sdlinit = SDL_Init(flags);
	if (sdlinit)
	{
		char * sss = (char*)SDL_GetError();
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		fprintf(stderr, "(Did you set the DISPLAY variable?)\n");
		return;
	}

	//将CSTATIC控件和sdl显示窗口关联 
	HWND hWnd = this->GetDlgItem(IDC_STC_PREVIEW)->GetSafeHwnd();
	if (hWnd != NULL)
	{
		if (m_pStreamInfo != NULL){
			m_pStreamInfo->m_show_screen = SDL_CreateWindowFrom((void*)hWnd);
			if (m_pStreamInfo->m_show_screen == NULL){
				TRACE("SDL_CreateWindowFrom ERR(%d)\n", SDL_GetError());
				return;
			}

			RECT rectDisPlay;
			this->GetDlgItem(IDC_STC_PREVIEW)->GetWindowRect(&rectDisPlay);
			m_pStreamInfo->m_xleft = rectDisPlay.left;
			m_pStreamInfo->m_ytop = rectDisPlay.top;
			m_pStreamInfo->m_width = rectDisPlay.right - rectDisPlay.left;
			m_pStreamInfo->m_height = rectDisPlay.bottom - rectDisPlay.top;

			//处理显示区域
			FillDisplayRect();

			m_pStreamInfo->m_sdlRenderer = SDL_CreateRenderer(m_pStreamInfo->m_show_screen, -1, 0);
			if (m_pStreamInfo->m_sdlRenderer == NULL){
				TRACE("SDL_CreateRenderer--sdlRenderer == NULL err(%d)\n", SDL_GetError());
				return;
			}
		} 
	}

	//设置SDL事件状态
	SDL_EventState(SDL_WINDOWEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	if (m_pThreadEvent == NULL){
		m_pThreadEvent = AfxBeginThread(Thread_Event, this);//开启线程
	}

	m_edtPusherAddr.SetWindowText("rtmp://dlpub.live.hupucdn.com/prod/ca093f98f0696a56fc2d42bfd309b903");// ("rtmp://123.59.87.24:1935/hls/live_stream");("rtmp://xxg2c3.publish.z1.pili.qiniup.com/dongqiudi/live3?key=bc21bda2fe27c512");// 

	return;
}

void CLS_DlgStreamPusher::UnInitInfo()
{
	m_blOut = TRUE;
	//释放相关资源
	if (m_cstrFilePath != ""){
		m_cstrFilePath.ReleaseBuffer();
		m_cstrFilePath = "";
	}
	if (NULL != m_bkBrush)
	{
		DeleteObject(m_bkBrush);
		m_bkBrush = NULL;
	}

	m_mapDeviceInfo.clear();

	if (NULL != m_pPushThrid){
		SDL_WaitThread(m_pPushThrid, NULL);
		m_pPushThrid = NULL;
	}
	if (NULL != m_pStreamInfo){
		if (m_pStreamInfo->m_video_tid){
			SDL_WaitThread(m_pStreamInfo->m_video_tid, NULL);
			m_pStreamInfo->m_video_tid = NULL;
		}
		if (m_pStreamInfo->m_audio_tid){
			SDL_WaitThread(m_pStreamInfo->m_audio_tid, NULL);
			m_pStreamInfo->m_audio_tid = NULL;
		}
		if (m_pStreamInfo->m_video_sws_ctx){
			sws_freeContext(m_pStreamInfo->m_video_sws_ctx);
		}
		if (m_pStreamInfo->m_pVideoFrameShowYUV){
			av_frame_free(&m_pStreamInfo->m_pVideoFrameShowYUV);
		}
		if (m_pStreamInfo->m_pVideoFramePushYUV){
			av_frame_free(&m_pStreamInfo->m_pVideoFramePushYUV);
		}
		if (m_pStreamInfo->m_pVideoFrame){
			av_frame_free(&m_pStreamInfo->m_pVideoFrame);
		}
		if (m_pStreamInfo->m_pAudioMutex){
			SDL_DestroyMutex(m_pStreamInfo->m_pAudioMutex);
		}
		if (m_pStreamInfo->m_pVideoMutex){
			SDL_DestroyMutex(m_pStreamInfo->m_pVideoMutex);
		}

	}

	if (NULL != m_pFmtVideoCtx){
		avformat_close_input(&m_pFmtVideoCtx);
	}

	if (NULL != m_pFmtAudioCtx){
		avformat_close_input(&m_pFmtAudioCtx);
	}

	if (NULL != m_pFmtRtmpCtx){
		if (!(m_pFmtRtmpCtx->flags & AVFMT_NOFILE)){
			avio_close(m_pFmtRtmpCtx->pb);
		}
		avformat_free_context(m_pFmtRtmpCtx);
	}

	if (NULL != m_pPictureBuf){
		av_free(m_pPictureBuf);
	}
	return;
}

HBRUSH CLS_DlgStreamPusher::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	switch (nCtlColor){
	case CTLCOLOR_STATIC:
		if (pWnd->GetDlgCtrlID() == IDC_STC_PREVIEW){
			pDC->SetBkMode(TRANSPARENT);
			pDC->SetBkColor(COLOR_BLACK);
			return m_bkBrush;
		}
	}
	return   hbr;
}

int CLS_DlgStreamPusher::UrlStreamColParse()
{
	int iRet = -1;

	return iRet;
}

int CLS_DlgStreamPusher::LocalFileParse()
{
	int iRet = -1;

	return iRet;
}

int CLS_DlgStreamPusher::GetDeviceInfo(int _iDeviceType)
{
	int iRet = -1;
	ICreateDevEnum *pDevEnum;
	IEnumMoniker   *pEnumMon;
	IMoniker	   *pMoniker;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (LPVOID*)&pDevEnum);
	if (SUCCEEDED(hr))
	{
		if (_iDeviceType == n_Video){
			hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMon, 0);
		}
		else{
			hr = pDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEnumMon, 0);
		}
		if (hr == S_FALSE)
		{
			TRACE("没有找到合适的音视频设备！");
			hr = VFW_E_NOT_FOUND;
			return hr;
		}
		pEnumMon->Reset();
		ULONG cFetched;
		while (hr = pEnumMon->Next(1, &pMoniker, &cFetched), hr == S_OK)
		{
			IPropertyBag *pProBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (LPVOID*)&pProBag);
			if (SUCCEEDED(hr))
			{
				VARIANT varTemp;
				varTemp.vt = VT_BSTR;
				hr = pProBag->Read(L"FriendlyName", &varTemp, NULL);
				if (SUCCEEDED(hr))
				{
					iRet = 0;
					_bstr_t bstr_t(varTemp.bstrVal);
					std::string strDeviceName = bstr_t;

					//将取到的设备名称存入容器中
					std::vector<std::string> vecDeviceName;
					vecDeviceName.clear();
					vecDeviceName.push_back(strDeviceName);
					SysFreeString(varTemp.bstrVal);

					//如果容器相应的键值有值就量进行相应的添加处理，没值的话整体进行添加处理
					std::map<int, std::vector<std::string>>::iterator iterInfo = m_mapDeviceInfo.find(_iDeviceType);
					if (iterInfo == m_mapDeviceInfo.end()){
						m_mapDeviceInfo.insert(map<int, std::vector<std::string>>::value_type(_iDeviceType, vecDeviceName));
					}
					else{
						m_mapDeviceInfo[_iDeviceType].push_back(strDeviceName);
					}
				}
				pProBag->Release();
			}
			pMoniker->Release();
		}
		pEnumMon->Release();
	}

	return iRet;
}

int video_thr(LPVOID lpParam)
{
	int iRet = -1;
	struct_stream_info	*	strct_streaminfo	= NULL;
	int						iVideoPic			= 0;
	int64_t					start_time			= 0;
	AVPacket		pkt;

	CLS_DlgStreamPusher* pThis = (CLS_DlgStreamPusher*)lpParam;
	if (pThis == NULL){
		TRACE("video_thr--pThis == NULL\n");
		return iRet;
	}
	if (NULL == pThis->m_pFmtVideoCtx || NULL == pThis->m_pCodecVideoCtx){
		TRACE("NULL == pThis->m_pFmtVideoCtx || NULL == pThis->m_pCodecVideoCtx\n");
		return iRet;
	}

	strct_streaminfo = pThis->m_pStreamInfo;
	if(NULL == strct_streaminfo){
		TRACE("NULL == strct_streaminfo\n");
		return iRet;
	}

#if WRITE_FILE
	//FILE *fp_yuv = fopen("output.yuv", "wb+");
#endif

	start_time = av_gettime();

	AVFrame	* pFrame;
	pFrame = avcodec_alloc_frame();
	AVFrame *picture = avcodec_alloc_frame();
	int size = avpicture_get_size(pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->pix_fmt,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->width, pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->height);
	uint8_t *picture_buf = (uint8_t*)av_malloc(size);

	avpicture_fill((AVPicture *)picture, picture_buf,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->pix_fmt,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->width,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->height);

	int height = pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->height;
	int width = pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->width;
	int y_size = height*width;

	//从摄像头获取数据
	while(1){
		if (pThis->m_blOut){
			break;
		}
		pkt.data = NULL;
		pkt.size = 0;
		if (av_read_frame(pThis->m_pFmtVideoCtx, &pkt) >= 0){
			//解码显示
			if (pkt.stream_index != 0){
				av_free_packet(&pkt);
				continue;
			}
			iRet = avcodec_decode_video2(pThis->m_pCodecVideoCtx, pFrame, &iVideoPic, &pkt);
			if (iRet < 0){
				TRACE("Decode Error.\n");
				goto END;
			}
			av_free_packet(&pkt);
			if (iVideoPic <= 0){
				TRACE("iVideoPic <= 0");
				goto END;
			}

			if (sws_scale(strct_streaminfo->m_video_sws_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->height,
				picture->data, picture->linesize) < 0){
				TRACE("sws_scale < 0");
				goto END;
			}

			int iVideoFifoSize = av_fifo_space(strct_streaminfo->m_pVideoFifo);
			//TRACE("----video_thr----iVideoFifoSize is[%ld] and size is[%d]----\n", iVideoFifoSize, size);
			if (iVideoFifoSize >= size){
				SDL_mutexP(strct_streaminfo->m_pVideoMutex);
				av_fifo_generic_write(strct_streaminfo->m_pVideoFifo, picture->data[0], y_size, NULL);
				av_fifo_generic_write(strct_streaminfo->m_pVideoFifo, picture->data[1], y_size / 4, NULL);
				av_fifo_generic_write(strct_streaminfo->m_pVideoFifo, picture->data[2], y_size / 4, NULL);
				SDL_mutexV(strct_streaminfo->m_pVideoMutex);
			}
		}
	}

	iRet = 1;
END:
#if WRITE_FILE
	//fclose(fp_yuv);
#endif
	av_frame_free(&picture);
	av_frame_free(&pFrame);

	return iRet;
}

void CLS_DlgStreamPusher::OnBnClickedBtnDeviceVideoTest()
{
	m_pStreamInfo->m_itest_start = 1;
}

void UpdateSampleDisplay(struct_stream_info *_pstrct_streaminfo, short *samples, int samples_size)//CLS_DlgStreamPusher::
{
	int size, len;

	size = samples_size / sizeof(short);
	while (size > 0) {
		len = SAMPLE_ARRAY_SIZE - _pstrct_streaminfo->m_sample_array_index;
		if (len > size)
			len = size;
		memcpy(_pstrct_streaminfo->m_sample_array + _pstrct_streaminfo->m_sample_array_index, samples, len * sizeof(short));
		samples += len;
		_pstrct_streaminfo->m_sample_array_index += len;
		if (_pstrct_streaminfo->m_sample_array_index >= SAMPLE_ARRAY_SIZE)
			_pstrct_streaminfo->m_sample_array_index = 0;
		size -= len;
	}
}

void  fill_audio(void *udata, Uint8 *stream, int len)//CLS_DlgStreamPusher::
{
	struct_stream_info* struct_stream = (struct_stream_info*)udata;
	if (struct_stream == NULL){
		TRACE("struct_stream == NULL \n");
		return;
	}
	audio_callback_time = av_gettime();

	int frame_size = av_samples_get_buffer_size(NULL, struct_stream->m_audio_tgt.channels, 1, struct_stream->m_audio_tgt.fmt, 1);
	int len1 = 0;

	if (audio_len == 0)	/*  Only  play  if  we  have  data  left  */
		return;
	len = (len>audio_len ? audio_len : len);	/*  Mix as  much  data  as  possible  */

	while (len > 0 && audio_len > 0){
		if (struct_stream->m_audio_buf_index >= struct_stream->m_audio_buf_size){
			if (struct_stream->m_aduio_pkt_size < 0){
				struct_stream->m_audio_buf = struct_stream->m_silence_buf;
				struct_stream->m_audio_buf_size = sizeof(struct_stream->m_silence_buf) / frame_size * frame_size;
			}
			else{
				if (struct_stream->m_show_mode == SHOW_MODE_WAVES){
					UpdateSampleDisplay(struct_stream, (int16_t *)struct_stream->m_audio_buf, struct_stream->m_aduio_pkt_size);
				}
				struct_stream->m_audio_buf_size = struct_stream->m_aduio_pkt_size;
			}
		}

		len1 = struct_stream->m_audio_buf_size - struct_stream->m_audio_buf_index;
		if (len1 > len)
			len1 = len;
		memcpy(stream, (uint8_t *)struct_stream->m_audio_buf + struct_stream->m_audio_buf_index, len1);
		audio_len -= len;
		len -= len1;
		stream += len1;
		struct_stream->m_audio_buf_index += len1;
	}
	audio_len = 0;

	struct_stream->m_audio_write_buf_size = struct_stream->m_audio_buf_size - struct_stream->m_audio_buf_index;
}

int audio_thr(LPVOID lpParam)
{
	//采集音频，进行储存
	int iRet = -1;
	AVPacket		pkt;
	AVFrame*		pFrame = NULL;
	pFrame = av_frame_alloc();
	if (NULL == pFrame){
		TRACE("NULL == pFrame!\n");
		return iRet;
	}

	CLS_DlgStreamPusher* pThis = (CLS_DlgStreamPusher*)lpParam;
	if (pThis == NULL){
		TRACE("audio_thr--pThis == NULL\n");
		return iRet;
	}
	if (NULL == pThis->m_pFmtAudioCtx){
		TRACE("NULL == pThis->m_pFmtAudioCtx\n");
		return iRet;
	}
	AVCodecContext* pCodec = pThis->m_pFmtAudioCtx->streams[0]->codec;
	if (NULL == pCodec){
		TRACE("NULL == pCodec");
		return iRet;
	}

	struct_stream_info* pStrctStreamInfo = pThis->m_pStreamInfo;
	if (NULL == pStrctStreamInfo){
		TRACE("NULL == pStrctStreamInfo\n");
		return iRet;
	}

	while (1){
		if (pThis->m_blOut){
			break;
		}
		pkt.data = NULL;
		pkt.size = 0;
		if (av_read_frame(pThis->m_pFmtAudioCtx, &pkt) < 0 || _kbhit() != 0){
			continue;
		}

		if (!pFrame) {
			if (!(pFrame = avcodec_alloc_frame())){
				TRACE("!(strct_stream_info->m_pAudioFrame = avcodec_alloc_frame())\n");
				goto END;
			}
		}
		else{
			avcodec_get_frame_defaults(pFrame);
		}

		int gotframe = -1;
		if (avcodec_decode_audio4(pThis->m_pFmtAudioCtx->streams[0]->codec, pFrame, &gotframe, &pkt) < 0){
			av_frame_free(&pFrame);
			TRACE("can not decoder a frame\n");
			break;
		}
		av_free_packet(&pkt);

		if (!gotframe){
			//没有获取到数据，继续下一次
			TRACE("avcodec_decode_audio4 gotframe is 0!\n");
			continue;
		}

		SDL_mutexP(pStrctStreamInfo->m_pAudioMutex);
		if (NULL == pStrctStreamInfo->m_pAudioFifo){
			pStrctStreamInfo->m_pAudioFifo = av_audio_fifo_alloc(pThis->m_pFmtAudioCtx->streams[0]->codec->sample_fmt,
				pThis->m_pFmtAudioCtx->streams[0]->codec->channels, 3 * pFrame->nb_samples);
		}

		int buf_space = av_audio_fifo_space(pStrctStreamInfo->m_pAudioFifo);
		//TRACE("~~~audio_thr~~~buf_space is [%d] and pFrame->nb_samples is [%d]\n", buf_space, pFrame->nb_samples);
		if (av_audio_fifo_space(pStrctStreamInfo->m_pAudioFifo) >= pFrame->nb_samples){
			av_audio_fifo_write(pStrctStreamInfo->m_pAudioFifo, (void **)pFrame->data, pFrame->nb_samples);
		}
		else if (buf_space > 0){
			av_audio_fifo_write(pStrctStreamInfo->m_pAudioFifo, (void **)pFrame->data, buf_space);
		}
		SDL_mutexV(pStrctStreamInfo->m_pAudioMutex);
	}
	iRet = 1;

END:
	av_frame_free(&pFrame);
	return iRet;
}
void CLS_DlgStreamPusher::OnBnClickedBtnDeviceAudioTest()
{
	m_blAudioShow = TRUE;
	m_pStreamInfo->m_itest_start = 1;

	//创建纹理
	m_pStreamInfo->m_sdlTexture = SDL_CreateTexture(m_pStreamInfo->m_sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_pStreamInfo->m_width, m_pStreamInfo->m_height);

	//开启audio测试线程
	m_pStreamInfo->m_show_mode = SHOW_MODE_WAVES;
}

void CLS_DlgStreamPusher::InitData()
{
	m_pStreamInfo = (struct_stream_info *)calloc(1, sizeof(struct_stream_info));
	if (NULL == m_pStreamInfo)
	{
		TRACE("m_streamstate is NULL!\n");
		return;
	}

	//初始化流媒体
	m_pStreamInfo->m_pFormatCtx = avformat_alloc_context();
	if (NULL == m_pStreamInfo->m_pFormatCtx){
		TRACE("NULL == m_pStreamInfo->m_pFormatCtx\n");
		return;
	}

	m_pStreamInfo->m_audio_tid = NULL;
	m_pStreamInfo->m_video_tid = NULL;
	m_pStreamInfo->m_show_screen = NULL;
	m_pStreamInfo->m_screen_surface = NULL;
	m_pStreamInfo->m_xleft = 0;
	m_pStreamInfo->m_ytop = 0;
	m_pStreamInfo->m_width = 0;
	m_pStreamInfo->m_height = 0;
	m_pStreamInfo->m_pAudioStream = NULL;
	m_pStreamInfo->m_pAudioFifo = NULL;
	m_pStreamInfo->m_pVideoFifo = NULL;
	m_pStreamInfo->m_pAudioMutex = SDL_CreateMutex();
	m_pStreamInfo->m_pVideoMutex = SDL_CreateMutex();
	m_pStreamInfo->m_pVideoStream = NULL;
	m_pStreamInfo->m_pAudioFrame = NULL;
	m_pStreamInfo->m_pVideoFrame = NULL;
	m_pStreamInfo->m_audio_swr_ctx = NULL;
	m_pStreamInfo->m_audio_buf = NULL;
	m_pStreamInfo->m_audio_buf_size = 0;
	m_pStreamInfo->m_audio_buf_index = 0;
	m_pStreamInfo->m_aduio_pkt_size = 0;
	m_pStreamInfo->m_sample_array_index = 0;
	m_pStreamInfo->m_abort_request = 0;
	m_pStreamInfo->m_refresh = 0;
	m_pStreamInfo->m_show_mode = SHOW_MODE_NONE;
	m_pStreamInfo->m_audio_refresh_tid = NULL;
	m_pStreamInfo->m_video_refresh_tid = NULL;
	m_pStreamInfo->m_itest_start = 0;

	m_pThreadEvent = NULL;
}

char* CLS_DlgStreamPusher::GetDeviceName(int _iDeviceType)
{
	char* pDeviceName = NULL;
	CString cstrDevName = "";
	CString cstrOutDevName = "";
	if (_iDeviceType == n_Audio){
		m_cboDeviceAudio.GetWindowText(cstrDevName);
	}
	else if (_iDeviceType == n_Video){
		m_cboDeviceVideo.GetWindowText(cstrDevName);
	}

	AnsiToUTF8(cstrDevName.GetBuffer(), cstrOutDevName);
	if (_iDeviceType == n_Audio){
		cstrOutDevName = "audio=" + cstrDevName;
	}
	else if (_iDeviceType == n_Video){
		cstrOutDevName = "video=" + cstrDevName;
	}

	wchar_t* wstring = string_to_wstring(cstrOutDevName.GetBuffer());
	pDeviceName = dup_wchar_to_utf8(wstring);

	return pDeviceName;
}

struct_stream_info* CLS_DlgStreamPusher::GetStreamStrcInfo()
{
	return m_pStreamInfo;
}

void CLS_DlgStreamPusher::FillDisplayRect()
{
	struct_stream_info* streamInfo = GetStreamStrcInfo();
	if (NULL == streamInfo){
		TRACE("NULL == streamInfo");
		return;
	}

	streamInfo->m_screen_surface = SDL_GetWindowSurface(streamInfo->m_show_screen);
	if (NULL == streamInfo->m_screen_surface){
		TRACE("NULL == streamInfo->m_screen_surface");
		return;
	}

	//背景色
	int bgcolor = SDL_MapRGB(streamInfo->m_screen_surface->format, 0x00, 0x00, 0x00);
	fill_rec(streamInfo->m_screen_surface, m_pStreamInfo->m_xleft, m_pStreamInfo->m_ytop, m_pStreamInfo->m_width, m_pStreamInfo->m_height,bgcolor);

	if (SDL_UpdateWindowSurface(streamInfo->m_show_screen) != 0){
		TRACE("SDL_UpdateWindowSurface ERR");
	}
}

int CLS_DlgStreamPusher::SynAudio(struct_stream_info* _pstrct_streaminfo, int _inb_samples)
{
	int iRet = -1;
	if (NULL == _pstrct_streaminfo){
		TRACE("NULL == _pstrct_streaminfo\n");
		return iRet;
	}
	int wanted_nb_samples = _inb_samples;

	//if (((_pstrct_streaminfo->av_sync_type == AV_SYNC_VIDEO_MASTER && _pstrct_streaminfo->video_st) ||
	//	_pstrct_streaminfo->av_sync_type == AV_SYNC_EXTERNAL_CLOCK)) {
	//	double diff, avg_diff;
	//	int min_nb_samples, max_nb_samples;

	//	diff = get_audio_clock(is) - get_master_clock(is);

	//	if (diff < AV_NOSYNC_THRESHOLD) {
	//		is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
	//		if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
	//			/* not enough measures to have a correct estimate */
	//			is->audio_diff_avg_count++;
	//		}
	//		else {
	//			/* estimate the A-V difference */
	//			avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

	//			if (fabs(avg_diff) >= is->audio_diff_threshold) {
	//				wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
	//				min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
	//				max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
	//				wanted_nb_samples = FFMIN(FFMAX(wanted_nb_samples, min_nb_samples), max_nb_samples);
	//			}
	//			av_dlog(NULL, "diff=%f adiff=%f sample_diff=%d apts=%0.3f vpts=%0.3f %f\n",
	//				diff, avg_diff, wanted_nb_samples - nb_samples,
	//				is->audio_clock, is->video_clock, is->audio_diff_threshold);
	//		}
	//	}
	//	else {
	//		/* too big difference : may be initial PTS errors, so
	//		reset A-V filter */
	//		is->audio_diff_avg_count = 0;
	//		is->audio_diff_cum = 0;
	//	}
	//}

	return wanted_nb_samples;
}

void CLS_DlgStreamPusher::event_loop(struct_stream_info *_pstrct_streaminfo)
{
	if (NULL == _pstrct_streaminfo){
		TRACE("NULL == _pstrct_streaminfo");
		return;
	}
	
	SDL_Event event;
	double incr, pos, frac;
	for (;;) {

		break;
		double x;
		//判断退出
		if (_pstrct_streaminfo->m_abort_request){
			break;
		}

		SDL_WaitEvent(&event);
		switch (event.type) {
		case FF_AUDIO_REFRESH_EVENT:
			screen_refresh(event.user.data1);
			_pstrct_streaminfo->m_refresh = 0;
			break;
		case FF_VIDEO_REFRESH_EVENT:
			break;
		case FF_BREAK_EVENT:
			break;
		case FF_QUIT_EVENT:
			stream_stop(event.user.data1);
			break;
		default:
			break;
		}
	}
}

void CLS_DlgStreamPusher::screen_refresh(void *opaque)
{
	struct_stream_info* strct_streaminfo = (struct_stream_info*)opaque;
	if (NULL == strct_streaminfo){
		TRACE("NULL == strct_streaminfo");
		return;
	}

	screen_display(strct_streaminfo);
}

void CLS_DlgStreamPusher::screen_display(struct_stream_info *_pstrct_streaminfo)
{
	if (_pstrct_streaminfo->m_pAudioFrame && _pstrct_streaminfo->m_show_mode != SHOW_MODE_VIDEO){
		audio_display(_pstrct_streaminfo);
	}
	else{
		video_display(_pstrct_streaminfo);
	}
}

void CLS_DlgStreamPusher::audio_display(struct_stream_info *_pstrct_streaminfo)
{
	if (audio_callback_time == 0){
		TRACE("audio_callback_time == 0");
		return;
	}
	int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
	int ch, channels, h, h2, bgcolor, fgcolor;
	int16_t time_diff;
	int rdft_bits, nb_freq;

	for (rdft_bits = 1; (1 << rdft_bits) < 2 * _pstrct_streaminfo->m_height; rdft_bits++)
		;
	nb_freq = 1 << (rdft_bits - 1);

	/* compute display index : center on currently output samples */
	channels = _pstrct_streaminfo->m_audio_tgt.channels;
	nb_display_channels = channels;
	if (!_pstrct_streaminfo->m_paused) {
		int data_used = _pstrct_streaminfo->m_show_mode == SHOW_MODE_WAVES ? _pstrct_streaminfo->m_width : (2 * nb_freq);
		n = 2 * channels;
		delay = _pstrct_streaminfo->m_audio_write_buf_size;
		delay /= n;

		/* to be more precise, we take into account the time spent since
		the last buffer computation */
		if (audio_callback_time) {
			time_diff = av_gettime() - audio_callback_time;
			delay -= (time_diff * _pstrct_streaminfo->m_audio_tgt.freq) / 1000000;
		}

		delay += 2 * data_used;
		if (delay < data_used)
			delay = data_used;

		i_start = x = compute_mod(_pstrct_streaminfo->m_sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
		if (_pstrct_streaminfo->m_show_mode == SHOW_MODE_WAVES) {
			h = INT_MIN;
			for (i = 0; i < 1000; i += channels) {
				int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
				int a = _pstrct_streaminfo->m_sample_array[idx];
				int b = _pstrct_streaminfo->m_sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
				int c = _pstrct_streaminfo->m_sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
				int d = _pstrct_streaminfo->m_sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
				int score = a - d;
				if (h < score && (b ^ c) < 0) {
					h = score;
					i_start = idx;
				}
			}
		}

		_pstrct_streaminfo->m_audio_last_i_start = i_start;
	}


	//背景色
	bgcolor = SDL_MapRGB(_pstrct_streaminfo->m_screen_surface->format, 0x00, 0x00, 0x00);
	if (_pstrct_streaminfo->m_show_mode == SHOW_MODE_WAVES) {
		SDL_Rect sdl_rect;
		_pstrct_streaminfo->m_xleft = 0;
		_pstrct_streaminfo->m_ytop = 0;
		sdl_rect.x = _pstrct_streaminfo->m_xleft;
		sdl_rect.y = _pstrct_streaminfo->m_ytop;
		sdl_rect.w = _pstrct_streaminfo->m_width;
		sdl_rect.h = _pstrct_streaminfo->m_height;

		fill_rec(_pstrct_streaminfo->m_screen_surface, _pstrct_streaminfo->m_xleft, _pstrct_streaminfo->m_ytop, _pstrct_streaminfo->m_width, _pstrct_streaminfo->m_height,bgcolor);

		fgcolor = SDL_MapRGB(_pstrct_streaminfo->m_screen_surface->format, 0xff, 0xff, 0xff);

		/* total height for one channel */
		h = _pstrct_streaminfo->m_height / nb_display_channels;
		/* graph height / 2 */
		h2 = (h * 9) / 20;
		for (ch = 0; ch < nb_display_channels; ch++) {
			i = i_start + ch;
			y1 = _pstrct_streaminfo->m_ytop + ch * h + (h / 2); /* position of center line */
			for (x = 0; x < _pstrct_streaminfo->m_width; x++) {
				y = (_pstrct_streaminfo->m_sample_array[i] * h2) >> 15;
				if (y < 0) {
					y = -y;
					ys = y1 - y;
				}
				else {
					ys = y1;
				}

				fill_rec(_pstrct_streaminfo->m_screen_surface, _pstrct_streaminfo->m_xleft + x, ys, 1, y, fgcolor);

				i += channels;
				if (i >= SAMPLE_ARRAY_SIZE)
					i -= SAMPLE_ARRAY_SIZE;
			}
		}

		fgcolor = SDL_MapRGB(_pstrct_streaminfo->m_screen_surface->format, 0x00, 0x00, 0xff);

		for (ch = 1; ch < nb_display_channels; ch++) {
			y = _pstrct_streaminfo->m_ytop + ch * h;

			fill_rec(_pstrct_streaminfo->m_screen_surface, _pstrct_streaminfo->m_xleft, y, _pstrct_streaminfo->m_width, 1, fgcolor);
		}
		if (SDL_UpdateWindowSurface(_pstrct_streaminfo->m_show_screen) != 0){
			TRACE("SDL_UpdateWindowSurface ERR");
		}
		//SDL_CreateRGBSurface();
		SDL_UpdateTexture(_pstrct_streaminfo->m_sdlTexture, NULL, _pstrct_streaminfo->m_screen_surface->pixels, _pstrct_streaminfo->m_screen_surface->pitch);
		SDL_RenderClear(_pstrct_streaminfo->m_sdlRenderer);
		SDL_RenderCopy(_pstrct_streaminfo->m_sdlRenderer, _pstrct_streaminfo->m_sdlTexture, &sdl_rect, &sdl_rect);
		SDL_RenderPresent(_pstrct_streaminfo->m_sdlRenderer);
	}
}

void CLS_DlgStreamPusher::video_display(struct_stream_info *_pstrct_streaminfo)
{

}

void CLS_DlgStreamPusher::fill_rec(SDL_Surface *screen,
	int x, int y, int w, int h, int color)
{
	if (NULL == screen){
		TRACE("NULL == screen");
		return;
	}

	//区域填充
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(screen, &rect, color);
}

void CLS_DlgStreamPusher::OnBnClickedBtnDeviceAudioTestStop()
{
	//停止音频测试
	if (!m_blAudioShow){
		TRACE("!m_blAudioShow");
		return;
	}
	if (NULL == m_pStreamInfo){
		TRACE("NULL == m_pStreamInfo");
		return;
	}
	m_blAudioShow = FALSE;
	stop_test();
	SDL_CloseAudio();

	m_pStreamInfo->m_audio_tid = NULL;
}

void CLS_DlgStreamPusher::OnBnClickedBtnDeviceVideoTestStop()
{
	//停止视频测试
	if (!m_blVideoShow){
		TRACE("!m_blVideoShow");
		return;
	}
	if (NULL == m_pStreamInfo){
		TRACE("NULL == m_pStreamInfo");
		return;
	}
	m_blVideoShow = FALSE;
}

void CLS_DlgStreamPusher::stop_test()
{
	SDL_Event event;
	event.type = FF_QUIT_EVENT;
	event.user.data1 = m_pStreamInfo;
	{
		SDL_PushEvent(&event);
	}
}

void CLS_DlgStreamPusher::stream_stop(void *opaque)
{
	struct_stream_info* pstrct_streaminfo = (struct_stream_info*)opaque;
	if (NULL == pstrct_streaminfo){
		TRACE("NULL == pstrct_streaminfo");
		return;
	}

	//做停止流推送的操作
	pstrct_streaminfo->m_abort_request = 1;
	SDL_WaitThread(pstrct_streaminfo->m_audio_tid, NULL);
	SDL_WaitThread(pstrct_streaminfo->m_audio_refresh_tid, NULL);
	SDL_WaitThread(pstrct_streaminfo->m_video_tid, NULL);
	SDL_WaitThread(pstrct_streaminfo->m_video_refresh_tid, NULL);

	pstrct_streaminfo->m_audio_tid = NULL;
	pstrct_streaminfo->m_video_tid = NULL;
	pstrct_streaminfo->m_audio_refresh_tid = NULL;
	pstrct_streaminfo->m_video_refresh_tid = NULL;
}

void CLS_DlgStreamPusher::OnBnClickedChkShowVideo()
{
	//控制是否显示视频
	if (m_chkShowVideo.GetCheck()){
		m_blVideoShow = TRUE;
	}
	else{
		m_blVideoShow = FALSE;
	}
}

void CLS_DlgStreamPusher::OnBnClickedBtnPushCode()
{
	//通过命令行的方式进行推送
	//ffmpeg -f dshow -s 320x240 -r 30 -i video="FaceTime HD Camera" -vcodec libx264 -preset:v ultrafast -tune:v zerolatency -f flv "rtmp://5380.lsspublish.aodianyun.com/Young/stream"
	CString cstrRtmpAddr = "";
	m_edtPusherAddr.GetWindowText(cstrRtmpAddr);
	if (cstrRtmpAddr == ""){
		MessageBox(_T("请输入合法的推流地址！/n"));
		return;
	}

	CString cstrVideoDev = "";
	m_cboDeviceVideo.GetWindowText(cstrVideoDev);
	if (cstrVideoDev == ""){
		MessageBox(_T("请选择合适的视频输入设备！/n"));
		return;
	}

	CString cstrAudioDev = "";
	m_cboDeviceAudio.GetWindowText(cstrAudioDev);
	if (cstrAudioDev == ""){
		MessageBox(_T("请选择合适的音频输入设备！/n"));
		return;
	}

	string strResolution = get_resolution();
	if (strResolution == ""){
		MessageBox("请选择合适的分辨率进行操作！/n");
		return;
	}

	//获取程序运行路径
	char exeFullPath[MAX_PATH]; // Full path
	string strPath = "";
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	strPath = (string)exeFullPath;    // Get full path of the file
	int pos = strPath.find_last_of('\\', strPath.length());
	strPath.substr(0, pos);

	//组合执行命令
}

string CLS_DlgStreamPusher::get_resolution()
{
	string strResolution = "";
	int iIndex = m_cboResolution.GetCurSel();
	if (iIndex < 0){
		TRACE("iIndex < 0");
		return strResolution;
	}

	switch (iIndex)
	{
	case 0:
		strResolution = "1920x1080";
		break;
	case 1:
		strResolution = "1280x720";
		break;
	case 2:
		strResolution = "720x576";
		break;
	case 3:
		strResolution = "320x240";
		break;
	default:
		strResolution = "1280x720";
		break;
	}

	return strResolution;
}

void CLS_DlgStreamPusher::OnBnClickedChkWriteFile()
{
}

int push_thr(LPVOID lpParam)
{
	int iRet = -1;
	int64_t cur_pts_v = 0, cur_pts_a = 0;
	int	frame_video_index = 0;
	int frame_audio_index = 0;

	CLS_DlgStreamPusher *pThis = (CLS_DlgStreamPusher*)lpParam;
	if (NULL == pThis){
		TRACE("NULL == pThis\n");
		return iRet;
	}

	struct_stream_info* pStrctStreamInfo = pThis->m_pStreamInfo;
	if (NULL == pStrctStreamInfo){
		TRACE("NULL == pStrctStreamInfo\n");
		return iRet;
	}

	//开启视频采集线程
	/*if (pStrctStreamInfo->m_video_tid == NULL){
		pStrctStreamInfo->m_video_tid = SDL_CreateThread(video_thr, NULL, (void*)pThis);
		if (pStrctStreamInfo->m_video_tid == NULL){
			TRACE("创建视频测试线程失败！\n");
			return iRet;
		}
	}

	if (pStrctStreamInfo->m_audio_tid == NULL){
		pStrctStreamInfo->m_audio_tid = SDL_CreateThread(audio_thr, NULL, (void*)pThis);
		if (pStrctStreamInfo->m_audio_tid == NULL){
			TRACE("创建音频测试线程失败！\n");
			return iRet;
		}
	}*/

	AVFrame *picture = av_frame_alloc();
	int size = avpicture_get_size(pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->pix_fmt,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->width, pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->height);
	uint8_t *out_buffer = (uint8_t*)av_malloc(size);

	avpicture_fill((AVPicture *)picture, out_buffer,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->pix_fmt,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->width,
		pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoIndex]->codec->height);

	AVPacket pkt;//视频包
	AVPacket pkt_out;//声音包
	AVFrame *frame;//声音帧
	while (1){
		if (pThis->m_blOut){
			break;
		}
		if (av_compare_ts(cur_pts_v, pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->time_base,
			cur_pts_a, pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->time_base) <= 0){
			//视频处理
			if (av_fifo_size(pStrctStreamInfo->m_pVideoFifo) >= size){
				SDL_mutexP(pStrctStreamInfo->m_pVideoMutex);
				av_fifo_generic_read(pStrctStreamInfo->m_pVideoFifo, out_buffer, size, NULL);
				SDL_mutexV(pStrctStreamInfo->m_pVideoMutex);

				avpicture_fill((AVPicture *)picture, out_buffer,
					pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->pix_fmt,
					pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->width,
					pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec->height);

				//pts = n * (（1 / timbase）/ fps);
				picture->pts = frame_video_index * ((pThis->m_pFmtVideoCtx->streams[0]->time_base.den / pThis->m_pFmtVideoCtx->streams[0]->time_base.num) / ENCODE_FPS);

				int got_picture = 0;
				av_init_packet(&pkt);
				pkt.data = NULL;
				pkt.size = 0;
				iRet = avcodec_encode_video2(pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->codec, &pkt, picture, &got_picture);
				if (iRet < 0){
					//编码错误,不理会此帧
					av_free_packet(&pkt);
					continue;
				}

				if (got_picture == 1){
					pkt.stream_index = pThis->m_iVideoOutIndex;
					pkt.pts = av_rescale_q_rnd(picture->pts, pThis->m_pFmtVideoCtx->streams[0]->time_base,
						pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
					pkt.dts = av_rescale_q_rnd(pkt.dts, pThis->m_pFmtVideoCtx->streams[0]->time_base,
						pThis->m_pFmtRtmpCtx->streams[pThis->m_iVideoOutIndex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

					pkt.pos = -1;
					TRACE("------video pts-------is [%d]\n", pkt.pts);
					//TRACE("~~~~~~video dts~~~~~~~is [%d]\n", pkt.dts);
					cur_pts_v = pkt.pts;

					if (av_interleaved_write_frame(pThis->m_pFmtRtmpCtx, &pkt) < 0 ){
						TRACE("av_interleaved_write_frame video err!\n");
					}
					av_free_packet(&pkt);
					frame_video_index++;
				}
			}
		}
		else{

			//Audio
			if (NULL == pStrctStreamInfo->m_pAudioFifo){
				continue;//还未初始化fifo
			}
			if (av_audio_fifo_size(pStrctStreamInfo->m_pAudioFifo) >=
				(pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size > 0 ? pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size : 1024))
			{
				frame = av_frame_alloc();
				frame->nb_samples = pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size > 0 ? pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size : 1024;
				frame->channel_layout = pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->channel_layout;
				frame->format = pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->sample_fmt;
				frame->sample_rate = pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->sample_rate;
				av_frame_get_buffer(frame, 0);

				SDL_mutexP(pStrctStreamInfo->m_pAudioMutex);
				av_audio_fifo_read(pStrctStreamInfo->m_pAudioFifo, (void **)frame->data,
					(pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size > 0 ? pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size : 1024));
				SDL_mutexV(pStrctStreamInfo->m_pAudioMutex);

				//if (pThis->m_pFmtRtmpCtx->streams[0]->codec->sample_fmt != pThis->m_pFmtAudioCtx->streams[pThis->m_iAudioOutIndex]->codec->sample_fmt
				//	|| pThis->m_pFmtRtmpCtx->streams[0]->codec->channels != pThis->m_pFmtAudioCtx->streams[pThis->m_iAudioOutIndex]->codec->channels
				//	|| pThis->m_pFmtRtmpCtx->streams[0]->codec->sample_rate != pThis->m_pFmtAudioCtx->streams[pThis->m_iAudioOutIndex]->codec->sample_rate){
				//	//如果输入和输出的音频格式不一样 需要重采样，这里是一样的就没做
				//}


				av_init_packet(&pkt_out);
				int got_picture = -1;
				pkt_out.data = NULL;
				pkt_out.size = 0;

				frame->pts = frame_audio_index * pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec->frame_size;
				if (avcodec_encode_audio2(pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->codec, &pkt_out, frame, &got_picture) < 0){
					TRACE("can not decoder a frame");
					av_free_packet(&pkt_out);
					continue;
				}
				if (NULL == pkt_out.data){
					TRACE("NULL == pkt_out.data");
					av_free_packet(&pkt_out);
					continue;
				}
				if (got_picture == 1){
					pkt_out.stream_index = pThis->m_iAudioOutIndex;

					pkt_out.pts = av_rescale_q_rnd(pkt_out.pts, pThis->m_pFmtAudioCtx->streams[0]->time_base,
						pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
					pkt_out.dts = av_rescale_q_rnd(pkt_out.dts, pThis->m_pFmtAudioCtx->streams[0]->time_base,
						pThis->m_pFmtRtmpCtx->streams[pThis->m_iAudioOutIndex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
					//pkt_out.duration = pkt_out.dts - cur_pts_a;
					pkt_out.pos = -1;
					TRACE("~~~~audio pts~~~~~is [%d]\n", pkt_out.pts);
					//TRACE("~~~~audio dts~~~~~is [%d]\n", pkt_out.pts);

					cur_pts_a = pkt_out.pts;
					if (av_interleaved_write_frame(pThis->m_pFmtRtmpCtx, &pkt_out) < 0){
						TRACE("av_interleaved_write_frame audio err!\n");
					}
					av_free_packet(&pkt_out);
					frame_audio_index++;
				}
				av_frame_free(&frame);
			}
		}
	}

	//推送结束
	av_write_trailer(pThis->m_pFmtRtmpCtx);
	av_frame_free(&picture);

	return iRet;
}
void CLS_DlgStreamPusher::OnBnClickedOk()
{
	//进行推流操作
	m_edtPusherAddr.GetWindowText(m_cstrPushAddr);
	if (m_cstrPushAddr == ""){
		MessageBox(_T("请输入正确的推流地址！\n"));
		return;
	}
	//ffmpeg - r 25 - f dshow - s 640×480 - i video = ”video source name” : audio = ”audio source name” - vcodec libx264 - b 600k - vpre slow - acodec libfaac - ab 128k - f flv rtmp ://server/application/stream_name
	if (OpenCamera() < 0){
		TRACE("OpenCamera failed!/n");
		goto END;
	}

	if (OpenAduio() < 0){
		TRACE("OpenAduio failed!/n");
		goto END;
	}

	if (OpenRtmpUrl() < 0){
		TRACE("OpenRtmpUrl failed!/n");
		goto END;
	}

	if (m_pPushThrid == NULL){
		m_pPushThrid = SDL_CreateThread(push_thr, NULL, (void*)this);
		if (NULL == m_pPushThrid){
			MessageBox("创建推送线程失败！");
			goto END;
		}
	}
	//开启视频采集线程
	if (m_pStreamInfo->m_video_tid == NULL){
		m_pStreamInfo->m_video_tid = SDL_CreateThread(video_thr, NULL, (void*)this);
		if (m_pStreamInfo->m_video_tid == NULL){
			TRACE("创建视频测试线程失败！\n");
			goto END;
		}
	}

	if (m_pStreamInfo->m_audio_tid == NULL){
		m_pStreamInfo->m_audio_tid = SDL_CreateThread(audio_thr, NULL, (void*)this);
		if (m_pStreamInfo->m_audio_tid == NULL){
			TRACE("创建音频测试线程失败！\n");
			goto END;
		}
	}

	//设置开始推流标记量
	m_blPushStream = TRUE;

END:
	/*if (NULL != m_pStreamInfo){
		if (m_pStreamInfo->m_video_sws_ctx){
			sws_freeContext(m_pStreamInfo->m_video_sws_ctx);
		}
		if (m_pStreamInfo->m_pVideoFrameShowYUV){
			av_frame_free(&m_pStreamInfo->m_pVideoFrameShowYUV);
		}
		if (m_pStreamInfo->m_pVideoFramePushYUV){
			av_frame_free(&m_pStreamInfo->m_pVideoFramePushYUV);
		}
		if (m_pStreamInfo->m_pVideoFrame){
			av_frame_free(&m_pStreamInfo->m_pVideoFrame);
		}
		if (m_pStreamInfo->m_pAudioMutex){
			SDL_DestroyMutex(m_pStreamInfo->m_pAudioMutex);
		}
		if (m_pStreamInfo->m_pVideoMutex){
			SDL_DestroyMutex(m_pStreamInfo->m_pVideoMutex);
		}

	}

	if (NULL != m_pFmtVideoCtx){
		avformat_close_input(&m_pFmtVideoCtx);
	}

	if (NULL != m_pFmtAudioCtx){
		avformat_close_input(&m_pFmtAudioCtx);
	}

	if (NULL != m_pFmtRtmpCtx){
		if (!(m_pFmtRtmpCtx->flags & AVFMT_NOFILE)){
			avio_close(m_pFmtRtmpCtx->pb);
		}
		avformat_free_context(m_pFmtRtmpCtx);
	}

	if (NULL != m_pPictureBuf){
		av_free(m_pPictureBuf);
	}*/

	return;
}

int CLS_DlgStreamPusher::OpenCamera()
{
	int iRet = -1;
	AVInputFormat  *pVideoInputFmt = NULL;
	AVCodec		  *pCodec = NULL;

	if (NULL == m_pStreamInfo){
		TRACE("NULL == m_pStreamInfo\n");
		return iRet;
	}

	//创建纹理
	m_pStreamInfo->m_sdlTexture = SDL_CreateTexture(m_pStreamInfo->m_sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, m_pStreamInfo->m_width, m_pStreamInfo->m_height);

	//设置SDL显示模式
	m_pStreamInfo->m_show_mode = SHOW_MODE_VIDEO;

	//根据设备名称打开设备
	char* psDevName = GetDeviceName(n_Video);
	if (psDevName == NULL){
		TRACE("video_thr--psDevName == NULL");
		return iRet;
	}
	pVideoInputFmt = av_find_input_format("dshow");
	if (pVideoInputFmt == NULL){
		TRACE("pVideoInputFmt == NULL\n");
		return iRet;
	}

	//打开摄像头
	m_pFmtVideoCtx = avformat_alloc_context();
	if (avformat_open_input(&m_pFmtVideoCtx, psDevName, pVideoInputFmt, NULL) != 0){
		TRACE("avformat_open_input err!\n");
		return iRet;
	}
	if (avformat_find_stream_info(m_pFmtVideoCtx, NULL) < 0){
		TRACE("avformat_find_stream_info(m_pFmtVideoCtx, NULL) < 0\n");
		return iRet;
	}

	for (int i = 0; i < m_pFmtVideoCtx->nb_streams; i++){
		if (m_pFmtVideoCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
			m_iVideoIndex = i;
			break;
		}
	}

	if (m_iVideoIndex < 0){
		TRACE("m_iVideoIndex < 0\n");
		return iRet;
	}

	m_pCodecVideoCtx = m_pFmtVideoCtx->streams[m_iVideoIndex]->codec;
	if (NULL == m_pCodecVideoCtx){
		TRACE("NULL == m_pCodecVideoCtx");
		return iRet;
	}

	//获取视频的宽与高
	m_iVideoHeight = m_pCodecVideoCtx->height;
	m_iVideoWidth = m_pCodecVideoCtx->width;
	TRACE("video_thr--video_height[%d],video_width[%d]", m_iVideoHeight
		, m_iVideoWidth);

	m_pStreamInfo->m_pVideoFrame = av_frame_alloc();
	m_pStreamInfo->m_pVideoFrameShowYUV = av_frame_alloc();
	m_pStreamInfo->m_pVideoFramePushYUV = av_frame_alloc();
	m_pStreamInfo->m_video_sws_ctx = sws_getContext(m_iVideoWidth, m_iVideoHeight, m_pCodecVideoCtx->pix_fmt,
		m_iVideoWidth, m_iVideoHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	if (NULL == m_pStreamInfo->m_video_sws_ctx){
		TRACE("NULL == strct_streaminfo->m_video_sws_ctx\n");
		return iRet;
	}

	m_pStreamInfo->m_video_refresh_tid = SDL_CreateThread(video_refresh_thread, NULL, m_pStreamInfo);

	//需要将采集信息写文件，打开解码器
#if WRITE_FILE
	pCodec = avcodec_find_decoder(m_pCodecVideoCtx->codec_id);
	if (pCodec == NULL){
		TRACE("avcodec_find_decoder<0");
		return iRet;
	}
	if (avcodec_open2(m_pCodecVideoCtx, pCodec, NULL)<0){
		TRACE("avcodec_open2<0");
		return iRet;
	}
#endif

	iRet = 0;
	return iRet;
}

int CLS_DlgStreamPusher::OpenAduio()
{
	int iRet = -1;
	AVInputFormat  *pAudioInputFmt = NULL;
	AVCodec		  *pCodec = NULL;

	//根据设备名称打开设备
	char* psDevName = GetDeviceName(n_Audio);
	if (psDevName == NULL){
		TRACE("OpenAduio--psDevName == NULL");
		return iRet;
	}
	pAudioInputFmt = av_find_input_format("dshow");
	if (pAudioInputFmt == NULL){
		TRACE("pAudioInputFmt == NULL\n");
		return iRet;
	}

	//打开麦克风
	m_pFmtAudioCtx = avformat_alloc_context();
	if (avformat_open_input(&m_pFmtAudioCtx, psDevName, pAudioInputFmt, NULL) != 0){
		TRACE("avformat_open_input err!\n");
		return iRet;
	}
	if (avformat_find_stream_info(m_pFmtAudioCtx, NULL) < 0){
		TRACE("avformat_find_stream_info(m_pFmtAudioCtx, NULL) < 0\n");
		return iRet;
	}

	for (int i = 0; i < m_pFmtAudioCtx->nb_streams; i++){
		if (m_pFmtAudioCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
			m_iAudioIndex = i;
			break;
		}
	}

	if (m_iAudioIndex < 0){
		TRACE("m_iAudioIndex < 0\n");
		return iRet;
	}

	m_pCodecAudioCtx = m_pFmtAudioCtx->streams[m_iAudioIndex]->codec;
	if (NULL == m_pCodecAudioCtx){
		TRACE("NULL == m_pCodecAudioCtx");
		return iRet;
	}

	//需要将采集信息写文件，打开解码器
	pCodec = avcodec_find_decoder(m_pCodecAudioCtx->codec_id);
	if (pCodec == NULL){
		TRACE("avcodec_find_decoder<0");
		return iRet;
	}
	if (avcodec_open2(m_pCodecAudioCtx, pCodec, NULL)<0){
		TRACE("avcodec_open2<0");
		return iRet;
	}

	iRet = 0;
	return iRet;
}

int CLS_DlgStreamPusher::OpenRtmpUrl()
{
	int iRet = -1;
	AVOutputFormat		*pStreamOutfmt = NULL;
	AVStream				*pVideoStream = NULL;
	AVStream				*pAudioStream = NULL;
	if (NULL == m_pStreamInfo || NULL == m_pCodecVideoCtx){
		TRACE("NULL == m_pStreamInfo || NULL == m_pCodecVideoCtx");
		return iRet;
	}
	//根据推流地址获取到AVFormatContext
	//m_cstrPushAddr = "test.flv";
	avformat_alloc_output_context2(&m_pFmtRtmpCtx, NULL, "flv", m_cstrPushAddr);
	if (NULL == m_pFmtRtmpCtx){
		TRACE("NULL == m_pFmtRtmpCtx\n");
		return iRet;
	}

	pStreamOutfmt = m_pFmtRtmpCtx->oformat;
	if (NULL == pStreamOutfmt){
		TRACE("NULL == pStreamOutfmt\n");
		return iRet;
	}

	//视频推流信息
	if (NULL != m_pFmtVideoCtx){
		pVideoStream = avformat_new_stream(m_pFmtRtmpCtx, NULL);
		if (!pVideoStream) {
			TRACE("Failed allocating output stream\n");
			return iRet;
		}
		pVideoStream->codec->codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		pVideoStream->codec->codec_tag = 0;
		pVideoStream->codec->height = m_pCodecVideoCtx->height;
		pVideoStream->codec->width = m_pCodecVideoCtx->width;
		pVideoStream->codec->time_base = m_pCodecVideoCtx->time_base;
		pVideoStream->codec->sample_aspect_ratio = m_pCodecVideoCtx->sample_aspect_ratio;
		pVideoStream->codec->pix_fmt = AV_PIX_FMT_YUV420P;
		// take first format from list of supported formats
		pVideoStream->codec->bit_rate = 40000;
		pVideoStream->codec->gop_size = m_pCodecVideoCtx->gop_size;//300;
		pVideoStream->codec->qmin = 5;
		pVideoStream->codec->qmax = 51;
		pVideoStream->codec->max_b_frames = m_pCodecVideoCtx->max_b_frames;
		pVideoStream->r_frame_rate = m_pFmtVideoCtx->streams[m_iVideoIndex]->r_frame_rate;

		m_iVideoOutIndex = pVideoStream->index;

		if (m_pFmtRtmpCtx->oformat->flags & AVFMT_GLOBALHEADER)
			pVideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

		//打开视频编码器
		if ((avcodec_open2(pVideoStream->codec, pVideoStream->codec->codec, NULL)) < 0){
			TRACE("can not open the encoder\n");
			return iRet;
		}

		//推流帧
		m_iPictureSize = avpicture_get_size(pVideoStream->codec->pix_fmt, pVideoStream->codec->width, pVideoStream->codec->height);
		m_pPictureBuf = (uint8_t *)av_malloc(m_iPictureSize);
		avpicture_fill((AVPicture *)m_pStreamInfo->m_pVideoFrameShowYUV, m_pPictureBuf, pVideoStream->codec->pix_fmt, pVideoStream->codec->width, pVideoStream->codec->height);
		avpicture_fill((AVPicture *)m_pStreamInfo->m_pVideoFramePushYUV, m_pPictureBuf, pVideoStream->codec->pix_fmt, pVideoStream->codec->width, pVideoStream->codec->height);

		m_pStreamInfo->m_pVideoFifo = av_fifo_alloc( 30 * avpicture_get_size(AV_PIX_FMT_YUV420P, pVideoStream->codec->width, pVideoStream->codec->height));
	}

	//音频推流信息
	if (NULL != m_pFmtAudioCtx){
		pAudioStream = avformat_new_stream(m_pFmtRtmpCtx,NULL);
		if (NULL == pAudioStream){
			TRACE("NULL == pAudioStream");
			return iRet;
		}
		if (NULL == m_pFmtRtmpCtx->streams[m_iAudioIndex]){
			TRACE("NULL == m_pFmtRtmpCtx->streams[m_iAudioIndex]\n");
			return iRet;
		}
		pAudioStream->codec->codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		pAudioStream->codec->sample_rate = m_pCodecAudioCtx->sample_rate;
		pAudioStream->codec->channel_layout = m_pFmtRtmpCtx->streams[m_iAudioIndex]->codec->channel_layout;
		pAudioStream->codec->channels = av_get_channel_layout_nb_channels(pAudioStream->codec->channel_layout);
		if (pAudioStream->codec->channel_layout == 0)
		{
			pAudioStream->codec->channel_layout = AV_CH_LAYOUT_STEREO;
			pAudioStream->codec->channels = av_get_channel_layout_nb_channels(pAudioStream->codec->channel_layout);
		}
		pAudioStream->codec->codec_type = AVMEDIA_TYPE_AUDIO;
		//pAudioStream->codec->bit_rate = 64000;
		pAudioStream->codec->sample_fmt = pAudioStream->codec->codec->sample_fmts[0];
		AVRational time_base = { 1, pAudioStream->codec->sample_rate };
		pAudioStream->time_base = time_base;
		m_pFmtAudioCtx->streams[0]->time_base = time_base;
		m_iAudioOutIndex = pAudioStream->index;

		pAudioStream->codec->codec_tag = 0;
		if (m_pFmtRtmpCtx->oformat->flags & AVFMT_GLOBALHEADER)
			pAudioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

		//打开音频编码器
		if (avcodec_open2(pAudioStream->codec, pAudioStream->codec->codec, 0) < 0)
		{
			TRACE("avcodec_open2 failed !\n");
			return iRet;
		}
	}

	//写头
	if (!(pStreamOutfmt->flags & AVFMT_NOFILE)) {
		iRet = avio_open(&m_pFmtRtmpCtx->pb, m_cstrPushAddr, AVIO_FLAG_WRITE);
		if (iRet < 0) {
			TRACE("Could not open output URL '%s'", m_cstrPushAddr);
			return iRet;
		}
	}

	iRet = avformat_write_header(m_pFmtRtmpCtx, NULL);
	if (iRet < 0) {
		TRACE("Error occurred when opening output URL\n");
		return iRet;
	}

	return iRet;
}