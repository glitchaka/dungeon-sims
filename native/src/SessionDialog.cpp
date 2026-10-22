#define NOMINMAX
#include <windows.h>
#include "SessionDialog.h"
#include "Multiplayer.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <random>
#include <vector>
namespace {
std::wstring wide(const std::string& text){int n=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);std::wstring s(n,0);MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),s.data(),n);return s;}
std::string value(HWND w,int id){int n=GetWindowTextLengthW(GetDlgItem(w,id));std::wstring s(n+1,0);GetWindowTextW(GetDlgItem(w,id),s.data(),n+1);n=WideCharToMultiByte(CP_UTF8,0,s.data(),n,nullptr,0,nullptr,nullptr);std::string out(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),static_cast<int>(wcslen(s.c_str())),out.data(),n,nullptr,nullptr);return out;}
struct Form {SessionOptions* options;bool accepted{};};
LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){auto* f=reinterpret_cast<Form*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    if(message==WM_NCCREATE){f=static_cast<Form*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(f));}
    if(message==WM_COMMAND&&LOWORD(wp)==1){auto& o=*f->options;std::string name=value(w,101),profile=value(w,102);if(name.empty()||name.size()>32||profile.empty()||profile.size()>32||profile.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")!=std::string::npos){MessageBoxW(w,L"Escribe un nombre y un perfil de hasta 32 caracteres. El perfil admite letras, numeros, - y _.",L"Datos de la isla",MB_OK);return 0;}
        int mode=static_cast<int>(SendDlgItemMessageW(w,103,CB_GETCURSEL,0,0));o.name=name;o.profile=profile;o.host=mode==1;o.ip=mode==2?value(w,104):"";if(mode==2&&o.ip.empty()){MessageBoxW(w,L"Introduce la IPv4 de la otra isla.",L"Conexion",MB_OK);return 0;}
        if(o.profile=="default"&&mode!=0)o.profile=mode==1?"island-host":"island-guest";
        try{auto seed=value(w,105);o.seed=seed.empty()?std::random_device{}():static_cast<unsigned>(std::stoul(seed));}catch(...){MessageBoxW(w,L"La semilla debe ser un numero positivo.",L"Semilla",MB_OK);return 0;}
        o.fresh=SendDlgItemMessageW(w,106,BM_GETCHECK,0,0)==BST_CHECKED;f->accepted=true;DestroyWindow(w);return 0;
    }
    if(message==WM_COMMAND&&LOWORD(wp)==2){auto text=wide(isla::localIPv4());if(OpenClipboard(w)){EmptyClipboard();auto data=GlobalAlloc(GMEM_MOVEABLE,(text.size()+1)*sizeof(wchar_t));if(data){auto memory=GlobalLock(data);memcpy(memory,text.c_str(),(text.size()+1)*sizeof(wchar_t));GlobalUnlock(data);SetClipboardData(CF_UNICODETEXT,data);}CloseClipboard();}return 0;}
    if(message==WM_CLOSE){DestroyWindow(w);return 0;}return DefWindowProcW(w,message,wp,lp);
}
}
bool chooseSession(SessionOptions& o){
    auto instance=GetModuleHandleW(nullptr);WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=instance;cls.lpszClassName=L"IslaVivaSession";cls.hCursor=LoadCursor(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&cls);
    Form form{&o};HWND w=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,L"Isla Viva - Tu nombre y tu isla",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU, CW_USEDEFAULT,CW_USEDEFAULT,560,385,nullptr,nullptr,instance,&form);if(!w)return false;
    auto add=[&](const wchar_t* type,const wchar_t* text,int x,int y,int width,int height,int id,DWORD style){HWND c=CreateWindowExW(type==std::wstring(L"EDIT")?WS_EX_CLIENTEDGE:0,type,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);};
    add(L"STATIC",L"Tu nombre",24,22,160,20,0,0);add(L"EDIT",wide(o.name).c_str(),190,18,320,26,101,WS_TABSTOP|ES_AUTOHSCROLL);
    add(L"STATIC",L"Perfil de guardado",24,62,160,20,0,0);add(L"EDIT",wide(o.profile).c_str(),190,58,320,26,102,WS_TABSTOP|ES_AUTOHSCROLL);
    add(L"STATIC",L"Modo",24,102,160,20,0,0);add(L"COMBOBOX",L"",190,98,320,120,103,WS_TABSTOP|CBS_DROPDOWNLIST);
    for(const auto* text:{L"Mi isla individual",L"Abrir mi isla a otro jugador",L"Conectar a otra isla"})SendDlgItemMessageW(w,103,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));SendDlgItemMessageW(w,103,CB_SETCURSEL,o.host?1:o.ip.empty()?0:2,0);
    add(L"STATIC",L"IP de la otra isla",24,142,160,20,0,0);add(L"EDIT",wide(o.ip.empty()?"127.0.0.1":o.ip).c_str(),190,138,320,26,104,WS_TABSTOP|ES_AUTOHSCROLL);
    add(L"STATIC",L"Semilla de nueva isla",24,182,160,20,0,0);add(L"EDIT",o.seed?wide(std::to_string(o.seed)).c_str():L"",190,178,320,26,105,WS_TABSTOP|ES_NUMBER);
    add(L"BUTTON",L"Crear una nueva isla aleatoria (conserva copia de la partida anterior)",24,220,490,28,106,WS_TABSTOP|BS_AUTOCHECKBOX);if(o.fresh)SendDlgItemMessageW(w,106,BM_SETCHECK,BST_CHECKED,0);
    add(L"STATIC",wide("Tu direccion local: "+isla::localIPv4()+":"+std::to_string(o.port)).c_str(),24,258,490,22,0,0);
    add(L"BUTTON",L"Copiar mi direccion",24,292,180,32,2,WS_TABSTOP|BS_PUSHBUTTON);
    add(L"BUTTON",L"Entrar a mi isla",330,292,180,32,1,WS_TABSTOP|BS_DEFPUSHBUTTON);
    ShowWindow(w,SW_SHOW);SetFocus(GetDlgItem(w,101));MSG m{};while(IsWindow(w)&&GetMessageW(&m,nullptr,0,0)>0){if(!IsDialogMessageW(w,&m)){TranslateMessage(&m);DispatchMessageW(&m);}}
    return form.accepted;
}

namespace {
struct ConnectionForm{std::string* ip;unsigned short* port;bool accepted{};};
LRESULT CALLBACK connectProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){auto* f=reinterpret_cast<ConnectionForm*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(msg==WM_NCCREATE){f=static_cast<ConnectionForm*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(f));}
    if(msg==WM_COMMAND&&LOWORD(wp)==1){try{auto ip=value(w,101);int port=std::stoi(value(w,102));if(ip.empty()||port<1024||port>65535)throw std::runtime_error("Invalid address");*f->ip=ip;*f->port=static_cast<unsigned short>(port);f->accepted=true;DestroyWindow(w);}catch(...){MessageBoxW(w,L"Escribe una direccion IPv4 y un puerto entre 1024 y 65535.",L"Conexion",MB_OK);}return 0;}if(msg==WM_CLOSE){DestroyWindow(w);return 0;}return DefWindowProcW(w,msg,wp,lp);}
}
bool chooseConnection(std::string& ip,unsigned short& port){auto instance=GetModuleHandleW(nullptr);WNDCLASSW cls{};cls.lpfnWndProc=connectProc;cls.hInstance=instance;cls.lpszClassName=L"IslaVivaConnect";cls.hCursor=LoadCursor(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&cls);ConnectionForm form{&ip,&port};auto w=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,L"Conectarse a otra isla",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,460,220,nullptr,nullptr,instance,&form);
    auto add=[&](const wchar_t* type,const std::wstring& text,int x,int y,int width,int height,int id,DWORD style){auto c=CreateWindowExW(0,type,text.c_str(),WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);};
    add(L"STATIC",L"Direccion IPv4 del anfitrion",20,18,300,20,0,0);add(L"EDIT",wide(ip.empty()?"127.0.0.1":ip),20,42,300,26,101,WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL);add(L"STATIC",L"Puerto",330,18,90,20,0,0);add(L"EDIT",wide(std::to_string(port)),330,42,90,26,102,WS_BORDER|WS_TABSTOP|ES_NUMBER);add(L"STATIC",L"Pide al anfitrion la direccion que muestra al abrir su isla.",20,84,410,32,0,0);add(L"BUTTON",L"Conectar",280,126,140,30,1,WS_TABSTOP|BS_DEFPUSHBUTTON);
    ShowWindow(w,SW_SHOW);SetFocus(GetDlgItem(w,101));MSG m{};while(IsWindow(w)&&GetMessageW(&m,nullptr,0,0)>0)if(!IsDialogMessageW(w,&m)){TranslateMessage(&m);DispatchMessageW(&m);}return form.accepted;
}
