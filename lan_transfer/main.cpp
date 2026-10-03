#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <orbis/NetCtl.h>
#include <orbis/Net.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include <sstream>
#include <string>
#include <vector>
#include <atomic>
#include "graphics.h"
#include "log.h"

std::stringstream debugLogStream;
static const int FTP_PORT = 2122;
static Scene2D *scene;
static FT_Face font;
static Color bg = {12, 19, 31}, white = {238, 244, 250}, cyan = {73, 205, 220}, muted = {151, 171, 190};
static int frameId = 0;
static char ip[16] = "Not connected";
static bool showHelp = true;
static const size_t TRANSFER_BUFFER_SIZE = 256 * 1024;
static const int MAX_FTP_CLIENTS = 4;
static std::atomic<int> activeFtpClients(0);

template <typename Signature> struct PthreadStartArgument;
template <typename Result, typename A1, typename A2, typename A3, typename A4, typename A5>
struct PthreadStartArgument<Result (*)(A1, A2, A3, A4, A5)> { typedef A3 Type; };
static int createPthread(OrbisPthread *thread, const OrbisPthreadAttr *attr,
                         void *(*entry)(void *), void *arg, const char *name) {
    typedef typename PthreadStartArgument<decltype(&scePthreadCreate)>::Type StartArgument;
    return scePthreadCreate(thread, attr, reinterpret_cast<StartArgument>(entry), arg, name);
}

static bool recvLine(int fd, char *out, size_t cap) {
    size_t n=0; char c;
    while (n+1<cap) { int r=(int)recv(fd,&c,1,0); if(r<=0) return false; if(c=='\n') break; if(c!='\r') out[n++]=c; }
    out[n]=0; return true;
}
static void reply(int fd,const char *s) { send(fd,s,strlen(s),0); }
static bool safePath(const char *cwd,const char *arg,char *out,size_t cap) {
    char tmp[1024]; int written;
    out[0]=0;
    if (!arg || !*arg) written=snprintf(tmp,sizeof(tmp),"%s",cwd);
    else if (arg[0]=='/') written=snprintf(tmp,sizeof(tmp),"%s",arg);
    else written=snprintf(tmp,sizeof(tmp),"%s/%s",cwd,arg);
    if(written<0||(size_t)written>=sizeof(tmp))return false;
    std::vector<std::string> parts; char *save=0; char *tok=strtok_r(tmp,"/",&save);
    while(tok) { if(!strcmp(tok,"..")) { if(!parts.empty()) parts.pop_back(); } else if(strcmp(tok,".")) parts.push_back(tok); tok=strtok_r(0,"/",&save); }
    size_t used=0; out[used++]='/'; out[1]=0;
    for(size_t i=0;i<parts.size();i++) { size_t len=parts[i].size(); if(used+len+2>cap)return false; if(used>1)out[used++]='/'; memcpy(out+used,parts[i].c_str(),len); used+=len; out[used]=0; }
    return true;
}
static void sendListing(int data,const char *path,bool namesOnly,bool machine=false) {
    struct stat st;
    if(!stat(path,&st)&&S_ISREG(st.st_mode)) { char row[1200]; const char *name=strrchr(path,'/');name=name?name+1:path;if(machine)snprintf(row,sizeof(row),"type=file;size=%lld;modify=20200101000000; %s\r\n",(long long)st.st_size,name);else if(namesOnly)snprintf(row,sizeof(row),"%s\r\n",name);else snprintf(row,sizeof(row),"-rw-r--r-- 1 user user %lld Jan 01 00:00 %s\r\n",(long long)st.st_size,name);send(data,row,strlen(row),0); return; }
    DIR *d=opendir(path); if(!d)return; struct dirent *e;
    while((e=readdir(d))) { if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue; char full[1200],row[1400]; snprintf(full,sizeof(full),"%s/%s",path,e->d_name);stat(full,&st);if(machine)snprintf(row,sizeof(row),"type=%s;size=%lld;modify=20200101000000; %s\r\n",S_ISDIR(st.st_mode)?"dir":"file",(long long)st.st_size,e->d_name);else if(namesOnly)snprintf(row,sizeof(row),"%s\r\n",e->d_name);else snprintf(row,sizeof(row),"%crw-r--r-- 1 user user %lld Jan 01 00:00 %s\r\n",S_ISDIR(st.st_mode)?'d':'-',(long long)st.st_size,e->d_name);send(data,row,strlen(row),0); }
    closedir(d);
}
static void transferClient(int client) {
    char *transferBuffer=(char*)malloc(TRANSFER_BUFFER_SIZE);
    if(!transferBuffer){reply(client,"421 Server out of memory\r\n");close(client);return;}
    reply(client,"220 SFTP (SimpleFTP) ready\r\n"); char line[2048],cwd[1024]="/",arg[1500]; int dataListen=-1;bool activeMode=false;sockaddr_in activeTarget={};
    while(recvLine(client,line,sizeof(line))) {
        char *sp=strchr(line,' '); if(sp){*sp++=0;snprintf(arg,sizeof(arg),"%s",sp);}else arg[0]=0;
        if(!strcmp(line,"USER"))reply(client,"331 Anonymous login; send any password\r\n");
        else if(!strcmp(line,"PASS"))reply(client,"230 Login successful\r\n");
        else if(!strcmp(line,"SYST"))reply(client,"215 UNIX Type: L8\r\n");
        else if(!strcmp(line,"FEAT"))reply(client,"211-Features\r\n PASV\r\n EPSV\r\n SIZE\r\n211 End\r\n");
        else if(!strcmp(line,"TYPE"))reply(client,"200 Type set\r\n");
        else if(!strcmp(line,"OPTS"))reply(client,"200 Options accepted\r\n");
        else if(!strcmp(line,"CLNT"))reply(client,"200 Client noted\r\n");
        else if(!strcmp(line,"NOOP"))reply(client,"200 OK\r\n");
        else if(!strcmp(line,"PWD")||!strcmp(line,"XPWD")){char r[1100];snprintf(r,sizeof(r),"257 \"%s\"\r\n",cwd);reply(client,r);}
        else if(!strcmp(line,"CWD")||!strcmp(line,"CDUP")){char p[1200];bool ok;if(!strcmp(line,"CDUP"))ok=safePath(cwd,"..",p,sizeof(p));else ok=safePath(cwd,arg,p,sizeof(p));struct stat st;if(ok&&!stat(p,&st)&&S_ISDIR(st.st_mode)){snprintf(cwd,sizeof(cwd),"%s",p);reply(client,"250 Directory changed\r\n");}else reply(client,"550 Directory unavailable\r\n");}
        else if(!strcmp(line,"PASV")||!strcmp(line,"EPSV")){if(dataListen>=0)close(dataListen);dataListen=socket(AF_INET,SOCK_STREAM,0);int yes=1;setsockopt(dataListen,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));struct sockaddr_in a={};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=0;struct sockaddr_in bound={};socklen_t boundLen=sizeof(bound);if(dataListen<0||bind(dataListen,(sockaddr*)&a,sizeof(a))||listen(dataListen,1)||getsockname(dataListen,(sockaddr*)&bound,&boundLen)){reply(client,"425 Cannot open passive port\r\n");if(dataListen>=0)close(dataListen);dataListen=-1;}else {uint16_t port=ntohs(bound.sin_port);if(!strcmp(line,"EPSV")){char r[96];snprintf(r,sizeof(r),"229 Entering Extended Passive Mode (|||%u|)\r\n",port);reply(client,r);}else {sockaddr_in local={};socklen_t localLen=sizeof(local);getsockname(client,(sockaddr*)&local,&localLen);uint32_t host=ntohl(local.sin_addr.s_addr);char r[128];snprintf(r,sizeof(r),"227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n",(host>>24)&255,(host>>16)&255,(host>>8)&255,host&255,port>>8,port&255);reply(client,r);}}}
        else if(!strcmp(line,"PORT")){unsigned h1,h2,h3,h4,p1,p2;if(sscanf(arg,"%u,%u,%u,%u,%u,%u",&h1,&h2,&h3,&h4,&p1,&p2)!=6||h1>255||h2>255||h3>255||h4>255||p1>255||p2>255){reply(client,"501 Invalid PORT address\r\n");continue;}sockaddr_in peer={};socklen_t peerLen=sizeof(peer);if(getpeername(client,(sockaddr*)&peer,&peerLen)||peer.sin_addr.s_addr!=htonl((h1<<24)|(h2<<16)|(h3<<8)|h4)){reply(client,"501 Active address must match control connection\r\n");continue;}if(dataListen>=0){close(dataListen);dataListen=-1;}activeTarget={};activeTarget.sin_family=AF_INET;activeTarget.sin_addr.s_addr=peer.sin_addr.s_addr;activeTarget.sin_port=htons((p1<<8)|p2);activeMode=true;reply(client,"200 PORT command successful\r\n");}
        else if(!strcmp(line,"LIST")||!strcmp(line,"NLST")||!strcmp(line,"MLSD")||!strcmp(line,"RETR")||!strcmp(line,"STOR")){if(dataListen<0&&!activeMode){reply(client,"425 Use PASV or PORT first\r\n");continue;} char path[1200];if(!safePath(cwd,arg,path,sizeof(path))){reply(client,"550 Path too long\r\n");continue;}reply(client,"150 Opening data connection\r\n");int data=-1;if(activeMode){data=socket(AF_INET,SOCK_STREAM,0);if(data>=0&&connect(data,(sockaddr*)&activeTarget,sizeof(activeTarget))<0){close(data);data=-1;}activeMode=false;}else {sockaddr_in peer;socklen_t plen=sizeof(peer);data=accept(dataListen,(sockaddr*)&peer,&plen);close(dataListen);dataListen=-1;}if(data<0){reply(client,"425 Data connection failed\r\n");continue;}
            if(!strcmp(line,"LIST")||!strcmp(line,"NLST")||!strcmp(line,"MLSD"))sendListing(data,path,!strcmp(line,"NLST"),!strcmp(line,"MLSD"));else if(!strcmp(line,"RETR")){FILE*f=fopen(path,"rb");if(f){size_t n;while((n=fread(transferBuffer,1,TRANSFER_BUFFER_SIZE,f))>0){size_t off=0;while(off<n){int w=(int)send(data,transferBuffer+off,n-off,0);if(w<=0)break;off+=w;}if(off<n)break;}fclose(f);}}
            else {FILE*f=fopen(path,"wb");if(f){int n;while((n=(int)recv(data,transferBuffer,TRANSFER_BUFFER_SIZE,0))>0)if(fwrite(transferBuffer,1,n,f)!=(size_t)n)break;fclose(f);}}
            close(data);reply(client,"226 Transfer complete\r\n");}
        else if(!strcmp(line,"SIZE")){char p[1200],r[128];struct stat st;if(safePath(cwd,arg,p,sizeof(p))&&!stat(p,&st)){snprintf(r,sizeof(r),"213 %lld\r\n",(long long)st.st_size);reply(client,r);}else reply(client,"550 Not found\r\n");}
        else if(!strcmp(line,"QUIT")){reply(client,"221 Goodbye\r\n");break;}
        else reply(client,"502 Command not supported\r\n");
    }
    if(dataListen>=0)close(dataListen);close(client);free(transferBuffer);
}
static void *ftpClientThread(void *arg) {
    int client=(int)(intptr_t)arg;
    transferClient(client);
    activeFtpClients.fetch_sub(1);
    return 0;
}
static void ftpServer() {
    int s=socket(AF_INET,SOCK_STREAM,0);int yes=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));sockaddr_in a={};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=htons(FTP_PORT);
    while(bind(s,(sockaddr*)&a,sizeof(a))<0)sceKernelUsleep(1000000);listen(s,4);
    for(;;){sockaddr_in c;socklen_t n=sizeof(c);int fd=accept(s,(sockaddr*)&c,&n);if(fd<0)continue;int count=activeFtpClients.fetch_add(1);if(count>=MAX_FTP_CLIENTS){activeFtpClients.fetch_sub(1);reply(fd,"421 Too many FTP connections\r\n");close(fd);continue;}OrbisPthread clientThread;int rc=createPthread(&clientThread,0,ftpClientThread,(void*)(intptr_t)fd,"ftp-client");if(rc!=0){activeFtpClients.fetch_sub(1);reply(fd,"421 Could not start FTP session\r\n");close(fd);continue;}scePthreadDetach(clientThread);}
}
static void *ftpServerThread(void *) { ftpServer(); return 0; }
static void getAddress(){OrbisNetCtlInfo info;if(sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS,&info)>=0&&info.ip_address[0]&&strcmp(info.ip_address,"0.0.0.0"))snprintf(ip,sizeof(ip),"%s",info.ip_address);else snprintf(ip,sizeof(ip),"Not connected");}
int main(){
    setvbuf(stdout,0,_IONBF,0);
    sceNetInit();sceNetCtlInit();getAddress();scePadInit();OrbisUserServiceInitializeParams userParams={ORBIS_KERNEL_PRIO_FIFO_LOWEST};sceUserServiceInitialize(&userParams);int userId=0;sceUserServiceGetInitialUser(&userId);int pad=scePadOpen(userId,ORBIS_PAD_PORT_TYPE_STANDARD,0,0);OrbisPadData state={};uint32_t prev=0;
    scene=new Scene2D(1920,1080,4);if(!scene->Init(0xC000000,2))return -1;
    if(!scene->InitFont(&font,"/app0/assets/fonts/Gontserrat-Regular.ttf",44))return -2;
    OrbisPthread thread; createPthread(&thread,0,ftpServerThread,0,"ftp-server");
    for(;;){uint32_t buttons=0;if(pad>=0&&scePadReadState(pad,&state)>=0)buttons=state.buttons;
        const uint32_t confirm=ORBIS_PAD_BUTTON_CROSS|ORBIS_PAD_BUTTON_CIRCLE;
        bool confirmPressed=(buttons&confirm)&&!(prev&confirm);prev=buttons;
        if(confirmPressed)showHelp=!showHelp;
        scene->FrameBufferFill(bg);
        scene->DrawText((char*)"SFTP (SimpleFTP)",font,130,150,bg,cyan);
        scene->DrawText((char*)"FILE TRANSFER ON YOUR LOCAL NETWORK",font,130,235,bg,white);
        char url[128];snprintf(url,sizeof(url),"ftp://%s:%d/",ip,FTP_PORT);scene->DrawText(url,font,130,355,bg,cyan);
        scene->DrawText((char*)"FileZilla: PS4 IP  |  Port 2122  |  User anonymous",font,130,465,bg,white);
        scene->DrawText((char*)"X / O / remote OK: setup instructions",font,130,900,bg,muted);
        if(showHelp){scene->DrawRectangle(90,80,1740,840,(Color){20,31,48});scene->DrawText((char*)"CONNECT THE PS4 AND PC TO THE SAME NETWORK",font,145,180,(Color){20,31,48},cyan);scene->DrawText((char*)"PS4 address:",font,145,275,(Color){20,31,48},white);scene->DrawText(url,font,460,275,(Color){20,31,48},cyan);scene->DrawText((char*)"In FileZilla: enter the PS4 IP, port 2122, user anonymous.",font,145,375,(Color){20,31,48},white);scene->DrawText((char*)"Use FTP. Passive or active transfer mode will work.",font,145,465,(Color){20,31,48},white);scene->DrawText((char*)"Drag files between the PC and PS4 to upload or download.",font,145,555,(Color){20,31,48},white);scene->DrawText((char*)"Press X, O, or remote OK to continue",font,145,700,(Color){20,31,48},cyan);}
        scene->SubmitFlip(frameId);scene->FrameWait(frameId);scene->FrameBufferSwap();frameId++;sceKernelUsleep(16000);
        if(!strcmp(ip,"Not connected"))getAddress();
    }
}
