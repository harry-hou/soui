// dui-demo.cpp : main source file
//

#include "stdafx.h"

#include <unknown/com-loader.hpp>

#include "MainDlg.h"

//#define RENDER_GDI      //打开RENDER_GDI时使用render-gdi模块来渲染，否则采用render-skia渲染

#define SUPPORT_LANG    //打开SUPPORT_LANG时，演示多语言支持

#define RES_TYPE 0   //从文件中加载资源
// #define RES_TYPE 1   //从PE资源中加载UI资源

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{
    HRESULT hRes = OleInitialize(NULL);
    ASSERT(SUCCEEDED(hRes));
    
    int nRet = 0; 

    SComLoader imgDecLoader;
    SComLoader renderLoader;
    SComLoader transLoader;
        
    //将程序的运行路径修改到demo所在的目录
    TCHAR szCurrentDir[MAX_PATH]={0};
    GetModuleFileName( NULL, szCurrentDir, sizeof(szCurrentDir) );
    LPTSTR lpInsertPos = _tcsrchr( szCurrentDir, _T('\\') );
    _tcscpy(lpInsertPos+1,_T(".."));
    SetCurrentDirectory(szCurrentDir);
    
    TCHAR szComDir[MAX_PATH];
    GetEnvironmentVariable(_T("SOUIPATH"),szComDir,MAX_PATH);
    #ifdef _DEBUG
    _tcscat(szComDir,_T("\\bin\\debug"));
    #else
    _tcscat(szComDir,_T("\\bin\\release"));
    #endif
    SetDllDirectory(szComDir);
    
    {

        CAutoRefPtr<SOUI::IImgDecoderFactory> pImgDecoderFactory;
        CAutoRefPtr<SOUI::IRenderFactory> pRenderFactory;
        imgDecLoader.CreateInstance(_T("imgdecoder-wic.dll"),(IObjRef**)&pImgDecoderFactory);
#ifdef RENDER_GDI
        renderLoader.CreateInstance(_T("render-gdi.dll"),(IObjRef**)&pRenderFactory);
#else
        renderLoader.CreateInstance(_T("render-skia.dll"),(IObjRef**)&pRenderFactory);
#endif

        pRenderFactory->SetImgDecoderFactory(pImgDecoderFactory);

        SApplication *theApp=new SApplication(pRenderFactory,hInstance);

        CAutoRefPtr<ITranslatorMgr> trans;
        transLoader.CreateInstance(_T("translator.dll"),(IObjRef**)&trans);
        if(trans)
        {
            theApp->SetTranslator(trans);
            pugi::xml_document xmlLang;
            if(xmlLang.load_file(L"translator/lang_cn.xml"))
            {
                CAutoRefPtr<ITranslator> langCN;
                trans->CreateTranslator(&langCN);
                langCN->Load(&xmlLang.child(L"language"),1);//1=LD_XML
                trans->InstallTranslator(langCN);
            }
        }

#if (RES_TYPE == 0)
        SResProviderFiles *pResProvider=new SResProviderFiles;
        if(!pResProvider->Init(_T("skin")))
        {
            ASSERT(0);
            return 1;
        }
#else //(RES_TYPE==1)
        SResProviderPE *pResProvider = new SResProviderPE(hInstance);
#endif

        theApp->AddResProvider(pResProvider);
        
        HMODULE hSysSkins=LoadLibrary(_T("soui-sys-skin.dll"));
        if(hSysSkins)
        {
            SResProviderPE resPE(hSysSkins);
            SSkinPool::getSingleton().LoadBuiltinSkins(&resPE,_T("SYS_SKIN"),_T("XML"));
        }

        theApp->Init(_T("XML_INIT")); 
        theApp->SetMsgBoxTemplate(_T("XML_MSGBOX"),_T("LAYOUT"));

        // BLOCK: Run application
        {
            CMainDlg dlgMain;  
            dlgMain.Create(GetActiveWindow(),0,0,800,600);
            dlgMain.SendMessage(WM_INITDIALOG);
            dlgMain.CenterWindow(dlgMain.m_hWnd);
            dlgMain.ShowWindow(SW_SHOWNORMAL);
            nRet=theApp->Run(dlgMain.m_hWnd);
        }

        delete theApp;
        delete pResProvider;
    }

    OleUninitialize();
    return nRet;
}
