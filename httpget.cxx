#include <stdio.h>
#include <windows.h>
#include "httpget.h"
#include "resource.h"

void CALLBACK GetEventCal::WinHttpStatusCallback(HINTERNET hRequest, DWORD_PTR dwContext,
                                                 DWORD dwInternetStatus,
                                                 LPVOID lpvStatusInfo, DWORD dwStatusInfoLen)
{
  GetEventCal *gec=(GetEventCal *)dwContext;

  switch(dwInternetStatus){
  case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
    {
      WinHttpReceiveResponse(hRequest, NULL);
    }
    break;

  case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
    {
      DWORD dwStatusCode=0;
      DWORD dwSize=sizeof(DWORD);
      WinHttpQueryHeaders(hRequest,
                          WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                          WINHTTP_HEADER_NAME_BY_INDEX,
                          &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

      gec->statuscode=dwStatusCode;
      if(dwStatusCode==HTTP_STATUS_OK){
        WinHttpQueryDataAvailable(hRequest, NULL);
      }
    }
    break;

  case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
    {
      DWORD dwSize=*((LPDWORD)lpvStatusInfo);
      if(dwSize==0){
        gec->read_incomplete=false;
        //gec->ld.strcpy((char *)gec->httpbuf);
        //gec->ld.writeWnd(false);
        MessageBox(NULL, (LPCSTR)gec->lpData, NULL, MB_OK);
      }else{
        gec->dwTotalSizePrev=gec->dwTotalSize;
        gec->dwTotalSize+=dwSize;
        gec->lpData=(unsigned char *)malloc(gec->dwTotalSize);
        if(gec->lpPrev!=NULL){
          memcpy(gec->lpData, gec->lpPrev, gec->dwTotalSizePrev);
          free(gec->lpPrev);
        }
        WinHttpReadData(hRequest, gec->lpData+gec->dwTotalSizePrev, dwSize, NULL);
        gec->lpPrev=gec->lpData;
      }
    }
    break;
    
  case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
    {
      if(lpvStatusInfo&&dwStatusInfoLen){
        WinHttpQueryDataAvailable(hRequest, NULL);
      }
      /*
      FILE *fp;
      fp=fopen("C:\\temp\\test.log", "wb");
      fwrite(gec->httpbuf, 1024, 1, fp);
      fclose(fp);
      */
    }
    break;

    //  case WINHTTP_CALLBACK_STATUS_REDIRECT:
    //  case WINHTTP_CALLBACK_INTERMEDIATE_RESPONCE:
  case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
    {
      LPWINHTTP_ASYNC_RESULT lpAsyncResult=(LPWINHTTP_ASYNC_RESULT)lpvStatusInfo;
      gec->resultcode=lpAsyncResult->dwResult;
      gec->errorcode=lpAsyncResult->dwError;
    }
    break;
  default:
    break;
  }
}

bool GetEventCal::GetRawHTTPData(char *url)
{
  wchar_t szURL[256];
  memset(szURL, 0, sizeof(wchar_t)*256);
  MultiByteToWideChar(CP_ACP, 0, url, strlen(url), szURL, 256);

  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieProxyConfig;
  memset(&ieProxyConfig, 0, sizeof(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG));
  BOOL ret=WinHttpGetIEProxyConfigForCurrentUser(&ieProxyConfig);

  if(ret==FALSE){
    hSession=WinHttpOpen(L"SampleApplication/1.0",
                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                         WINHTTP_FLAG_ASYNC);
  }else{
    if(ieProxyConfig.lpszProxy){
      hSession=WinHttpOpen(L"SampleApplication/1.0",
                           WINHTTP_ACCESS_TYPE_NAMED_PROXY,
                           ieProxyConfig.lpszProxy,
                           ieProxyConfig.lpszProxyBypass
                           ? ieProxyConfig.lpszProxyBypass
                           :WINHTTP_NO_PROXY_BYPASS,
                           WINHTTP_FLAG_ASYNC);
      
    }else{
      hSession=WinHttpOpen(L"SampleApplication/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                           WINHTTP_FLAG_ASYNC);
    }
  }
    
  if(hSession==NULL) return false;

  URL_COMPONENTS urlComponents;
  memset(&urlComponents, 0, sizeof(URL_COMPONENTS));
  urlComponents.dwStructSize     = sizeof(URL_COMPONENTS);

  wchar_t szHostName[256], szUrlPath[2048];
  urlComponents.lpszHostName     = szHostName;
  urlComponents.dwHostNameLength = sizeof(szHostName)/sizeof(wchar_t);
  urlComponents.lpszUrlPath      = szUrlPath;
  urlComponents.dwUrlPathLength  = sizeof(szUrlPath)/sizeof(wchar_t);
  if(!WinHttpCrackUrl(szURL, lstrlenW(szURL), 0, &urlComponents)){
    WinHttpCloseHandle(hSession); hSession=NULL;
    return false;
  }

  hConnect=WinHttpConnect(hSession, szHostName, INTERNET_DEFAULT_PORT, 0);
  if(hConnect==NULL){
    WinHttpCloseHandle(hSession); hSession=NULL;
    return false;
  }

  DWORD dwFlags=0;
  if(urlComponents.nScheme==INTERNET_SCHEME_HTTPS) dwFlags=WINHTTP_FLAG_SECURE;
  hRequest=WinHttpOpenRequest(hConnect, L"GET", szUrlPath, NULL,
                              WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);
  if(hRequest==NULL){
    WinHttpCloseHandle(hConnect); hConnect=NULL;
    WinHttpCloseHandle(hSession); hSession=NULL;
    return false;
  }

  WinHttpSetStatusCallback(hRequest, WinHttpStatusCallback,
  //                           WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS, 0);
                           WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS|
                           WINHTTP_CALLBACK_FLAG_REDIRECT,
                           0);

  lpData=lpPrev=NULL;
  dwTotalSize=dwTotalSizePrev=0;
  if(!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                         WINHTTP_NO_REQUEST_DATA, 0, 0, (DWORD)this)){
    WinHttpCloseHandle(hRequest); hRequest=NULL;
    WinHttpCloseHandle(hConnect); hConnect=NULL;
    WinHttpCloseHandle(hSession); hSession=NULL;
    return false;
  }
  return true;
}

void GetEventCal::DispFetchedData(char *url, HWND hwnd)
{
  m_hdlg=CreateDialogParam(m_hAppInstance, MAKEINTRESOURCE(IDD_DIALOG1), hwnd,
                           (DLGPROC)dlgProc, (LONG_PTR)this);
  ld.setOutputWnd(GetDlgItem(m_hdlg, IDC_RICHEDIT1));
  read_incomplete=GetRawHTTPData(url);
}

LRESULT CALLBACK GetEventCal::dlgProc(HWND hdlg, UINT msg, WPARAM /*wparam*/, LPARAM lparam)
{
  GetEventCal *_this=(GetEventCal *)GetWindowLongPtr(hdlg, GWLP_USERDATA);
  switch(msg){
  case WM_CLOSE:
    if(_this->read_incomplete==true){
      SetWindowLongPtr(hdlg, DWL_MSGRESULT, FALSE);
    }else{
      DestroyWindow(hdlg);
      SetWindowLongPtr(hdlg, DWL_MSGRESULT, TRUE);
    }
    break;

  case WM_DESTROY:
    _this->m_hdlg=NULL;
    break;

  case WM_INITDIALOG:
    {
      _this=(GetEventCal *)lparam;
      SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)_this);
      return TRUE;
    }
    break;
  }
  return FALSE;
}
