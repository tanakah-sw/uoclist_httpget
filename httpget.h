#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include "logdisp.h"

class GetEventCal
{
public:
  GetEventCal():
    m_hAppInstance(0), m_hdlg(0), read_incomplete(false),
    hSession(NULL), hConnect(NULL), hRequest(NULL), lpData(NULL), lpPrev(NULL)
  {};
  ~GetEventCal(){};

  void Init(HINSTANCE hinstance)
  {
    m_hAppInstance=hinstance;
  }
  void DispFetchedData(char *url, HWND hwnd);
  void SessionClear()
  {
    if(lpData!=NULL)   free(lpData);
    if(hRequest!=NULL) WinHttpCloseHandle(hRequest);
    if(hConnect!=NULL) WinHttpCloseHandle(hConnect);
    if(hSession!=NULL) WinHttpCloseHandle(hSession);
    lpData  =NULL;
    hRequest=NULL;
    hConnect=NULL;
    hSession=NULL;
  }
  void Close()
  {
    if(m_hdlg!=NULL) SendMessage(m_hdlg, WM_CLOSE, 0, 0);
    SessionClear();
  };
  
private:
  static LRESULT CALLBACK dlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam);
  static void CALLBACK WinHttpStatusCallback(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);

  bool GetRawHTTPData(char *url);

  HINSTANCE m_hAppInstance;
  HWND m_hdlg;

  LogDisp ld;

  bool read_incomplete;

  HINTERNET hSession, hConnect, hRequest;

  DWORD statuscode;
  DWORD resultcode;
  DWORD errorcode;

  unsigned char *lpData, *lpPrev;
  DWORD dwTotalSize, dwTotalSizePrev;
};

