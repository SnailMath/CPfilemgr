#include <appdef.hpp>

#include <sdk/os/string.hpp>
#include <sdk/os/file.hpp>
#include <sdk/os/lcd.hpp>
#include <sdk/os/debug.hpp>
#include <sdk/os/mem.hpp>
#include <sdk/os/input.hpp>

APP_NAME("Filemgr")
APP_DESCRIPTION("A simple file manager")
APP_AUTHOR("SnailMath")
APP_VERSION("1.0.0")

typedef unsigned int size_t;
int abs(int a);
void* realloc_sized(void* ptr, size_t oldsz, size_t newsz);
int  stbi_my_read(void *user,char *data,int size); 
void stbi_my_skip(void *user,int n);
int  stbi_my_eof (void *user);

#define STBI_MALLOC(sz)		malloc(sz)
#define STBI_FREE(p)		free(p)
#define STBI_REALLOC_SIZED(p,oldsz,newsz) realloc_sized(p,oldsz,newsz)
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ASSERT
#define STBI_NO_FAILURE_STRINGS
//#define STBI_ONLY_JPEG
//#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "calc_image.h"

#define EXIT_PROGRAM	-1
#define GO_ROOT		-2
#define GO_SD		-3
#define GO_UP		-4

int getCommandInput();

struct dirEntry{
	char fileName[100];
	char type;
};
const int maxFiles = 41;
const int maxFilesDisplay = 41;
int dirFiles = 0;
struct dirEntry directory[64];

char g_path[400];
wchar_t g_wpath[400];

void fileClick(char* fileName);
void showImage(char* fileName);
void show565(char* fileName);
void showText(char* fileName);
void showHex(char* fileName);


#define to_ch(x) numToAscii[addr[x]]
unsigned char numToAscii[257]= "................................ !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~.................................................................................................................................";

extern "C"
void main() {
	LCD_VRAMBackup();
	//initialize g_path
	memset(g_path,0,sizeof(g_path));
	strcpy (g_path, "\\fls0\\");

	while(true){
		memset(g_wpath,0,sizeof(g_wpath));
	
		//convert from char to wchar
		for(int i=0; g_path[i]!=0; i++){
			wchar_t ch = g_path[i];
			g_wpath[i] = ch;
		}
		
		//add the * to the file path 
		{
			int i=0;
			while(g_wpath[i]!=0)i++; //seek to the end of the string
			g_wpath[i++]='*'; //add an *
			g_wpath[i  ]= 0 ; //add the 0
		}
	
		LCD_ClearScreen();
		Debug_Printf(0,0,false,0,"%s",g_path);
		Debug_Printf(0,1,false,0,".. D (go up one directory)");
		LCD_Refresh();
	
		int dirFiles = 0;

		int findHandle;
		wchar_t fileName[100];
		struct findInfo findInfoBuf;
		int ret = findFirst(g_wpath, &findHandle, fileName, &findInfoBuf);
		while (ret>=0 && dirFiles<maxFiles ){
			//create dirEntry structure
			struct dirEntry thisfile;
			memset(&thisfile, 0, sizeof(thisfile));
			//copy file name
			for (int i=0; fileName[i]!=0; i++){
				wchar_t ch = fileName[i];
				thisfile.fileName[i] = ch;
			}
			//copy file type
			thisfile.type=findInfoBuf.type==findInfoBuf.EntryTypeDirectory?'D':'F';
			//display this
			if(dirFiles<maxFilesDisplay){
				Debug_Printf(0,dirFiles+2,false,0,"%2d %c %s",dirFiles,thisfile.type,thisfile.fileName);
				LCD_Refresh();
			}
			//Debug_Printf(15,43,false,0,"%2d %s",dirFiles,thisfile.fileName);
			//LCD_Refresh();
			//save this dirEntry to directory
			directory[dirFiles++] = thisfile;
			
			//serch the next
			ret = findNext(findHandle, fileName, &findInfoBuf);
		}
		findClose(findHandle);
	
		int go = getCommandInput();

		if (go>=0 && go<dirFiles){
			struct dirEntry thisfile = directory[go];
			if(thisfile.type=='D'){ //type = folder
				strcat(g_path,thisfile.fileName);
				strcat(g_path,"\\");
			}else{ //type = file
				char thisFileName[400];
				memset(thisFileName,0,sizeof(thisFileName));
				strcat(thisFileName,g_path);
				strcat(thisFileName,thisfile.fileName);
				//Debug_Printf(0,0,true,0,"open:'%s'",thisFileName);LCD_Refresh();Debug_WaitKey();
				fileClick(thisFileName);
			}
		}
		if (go==GO_UP){
			//Debug_Printf(0,0,true,0,"entering ..!");
			//seek from the back and remove the last two backslashs
			int i=399;
			while(g_path[i]!='\\' && i>0)i--; 
			g_path[i]=0;
			while(g_path[i]!='\\' && i>0)i--; 
			g_path[i]='\\'; //keep this a bs, only needed when no file name
			g_path[++i]=0;
		}
		if (go==GO_ROOT){
			memset(g_path,0,sizeof(g_path));
			strcpy (g_path, "\\fls0\\");
		}
		if (go==GO_SD){
			memset(g_path,0,sizeof(g_path));
			strcpy (g_path, "\\drv0\\"); //not tested yet...
		}

		/*//free the memory used.
		while(dirFiles){
			dirEntry* thisfile = directory[--dirFiles];
			free(thisfile);
		}*/

		if (go==EXIT_PROGRAM) break;
		//LCD_Refresh();
		//Debug_WaitKey();



	}

	//Debug_Printf(0,0,true,0,"closed!");
	//LCD_Refresh();
	//Debug_WaitKey();


	LCD_VRAMRestore();
	LCD_Refresh();
}
	
int getCommandInput(){
	char cmd[13]; //the command line
	int pos = 1; //the position of the cursor in cmd
	strcpy (cmd,">_          ");

	struct InputEvent event;
	while (true){
		Debug_Printf(0,43,false,0,"%s",cmd); LCD_Refresh();
		memset(&event,0,sizeof(event));
		GetInput(&event,0xFFFFFFFF,0x10);
		if(event.type==EVENT_KEY){
			if(event.data.key.direction==KEY_PRESSED){
				if(pos<=10){//If we have have less than 11 chars ('>' + 10 digits)
					switch (event.data.key.keyCode){
						case KEYCODE_0: cmd[pos++]='0';cmd[pos]='_';break;
						case KEYCODE_1: cmd[pos++]='1';cmd[pos]='_';break;
						case KEYCODE_2: cmd[pos++]='2';cmd[pos]='_';break;
						case KEYCODE_3: cmd[pos++]='3';cmd[pos]='_';break;
						case KEYCODE_4: cmd[pos++]='4';cmd[pos]='_';break;
						case KEYCODE_5: cmd[pos++]='5';cmd[pos]='_';break;
						case KEYCODE_6: cmd[pos++]='6';cmd[pos]='_';break;
						case KEYCODE_7: cmd[pos++]='7';cmd[pos]='_';break;
						case KEYCODE_8: cmd[pos++]='8';cmd[pos]='_';break;
						case KEYCODE_9: cmd[pos++]='9';cmd[pos]='_';break;
						case KEYCODE_DOT: 
							if (pos==1){
								if(cmd[1]=='.'){
									return GO_UP;
								}else{
									cmd[1]='.';
								}
							}break;
						case KEYCODE_KEYBOARD: return GO_ROOT;
						case KEYCODE_SHIFT: return GO_SD;
					}
				}//pos!<=8
				if (pos>1){ //If there are chars you can delete one.
					if(event.data.key.keyCode==KEYCODE_BACKSPACE){
						cmd[pos--]=' ';cmd[pos]='_';
					}//keycode!=Backspace
				}//pos!>1
				if(event.data.key.keyCode==KEYCODE_POWER_CLEAR){
					return EXIT_PROGRAM;
				}//!Clear
				if(event.data.key.keyCode==KEYCODE_EXE){
					while(cmd[1]!='_'&&cmd[1]!='.'){
						int ret = cmd[1]-'0';
						int i=2;
						while(cmd[i]!='_'){
							ret = (ret*10) + (cmd[i]-'0');
							i++;
						}//cmd[i]!='_'
						return ret;
					}//cmd[1]!='_'
				}//!EXE
			}//direction!Pressed
		}//type!=key
		else if(event.type==EVENT_TOUCH){
			if(event.data.touch_single.direction==TOUCH_UP){
				long y = event.data.touch_single.p1_y;
				y = y*341 >>12; //multiply by 341 and divide by 4096 to divide by 12
				if (y==1) return 99;
				else return (y-2);

			}//OUCH_UP
		}//EVENT_TOUCH
	}//while (true)
}

int abs(int a){
	return a<0?-a:a;
}
void* realloc_sized(void* oldptr, size_t oldsz, size_t newsz){
	void* newptr = malloc(newsz);
	memcpy(newptr,oldptr,oldsz);
	free(oldptr);
	return newptr;
}
static stbi_io_callbacks stbi__my_callbacks = {
	stbi_my_read,
	stbi_my_skip,
	stbi_my_eof,
};
int  stbi_my_read(void *user,char *data,int size){
	int ret = (int) read((int)user, data, size);
	return ret;
}
void stbi_my_skip(void *user,int n){
	lseek((int) user, n, SEEK_CUR);
}
int  stbi_my_eof (void *user){
return 0;
}

//Open a file and display it when it was clicked.
void fileClick(char* fileName){
	LCD_ClearScreen();
	Debug_Printf(3,2,false,0,"Open file '%s'",fileName);
	Debug_Printf(3,3,false,0,"1 - as binary file");
	Debug_Printf(3,4,false,0,"2 - as text file");
	Debug_Printf(3,5,false,0,"3 - as image file (png/jpg/gif)");
	Debug_Printf(3,6,false,0,"4 - as image file (.565)");
	LCD_Refresh();

	int selection = getCommandInput();
	if       (selection==1){
		showHex(fileName);
	}else if (selection==2){
		showText(fileName);
	}else if (selection==3){
		showImage(fileName);
	}else if (selection==4){
		show565(fileName);
	}
}
void showImage(char* fileName){
	uint16_t* VRAM = LCD_GetVRAMAddress();
	int W,H; LCD_GetSize(&W,&H);
	int w=0;
	int h=0;
	int c;
	int fp;
	for(long i=0;i<W*H;i++)VRAM[i]=0;
	Debug_Printf(20,43,true,0,"Please Wait...");
	LCD_Refresh();
	for(long i=W*(H-12);i<W*H;i++)VRAM[i]=0;
	//fp = open("\\fls0\\finn.png", OPEN_READ);
	fp = open(fileName, OPEN_READ);
	stbi__context s;
	stbi__start_callbacks(&s, &stbi__my_callbacks, (void *) fp);
	unsigned char *img = stbi__load_and_postprocess_8bit(&s, &w, &h, &c, 3);
	close(fp);

	uint32_t pos = 0;
	for(int y=h-1;y>=0;y--){
		for(int x=0;x<w;x++){
			unsigned char r = img[pos++];
			unsigned char g = img[pos++];
			unsigned char b = img[pos++];
			VRAM[x+y*W]=((r<<8)&0b1111100000000000) | ((g<<3)&0b0000011111100000) | ((b>>3)&0b0000000000011111);
		}
	}
	LCD_Refresh();
	if(w>0)
		Debug_WaitKey();
	stbi_image_free(img);
}

void showHex(char* fileName){
	int fp;
	uint8_t* addr;
	fp = open(fileName, OPEN_READ);
	getAddr(fp,0,(const void**)&addr);
	close(fp);

	LCD_ClearScreen();
	Debug_Printf(0,0,false,0,"Viewing file %s",fileName);
	/*int i=0;
	for(int y=1;y<44;y++){
		Debug_Printf(1,y,false,0,"%02X",i>>4);
		for(int x=0;x<8;x++){
			Debug_Printf(4+(x*5),y,false,0,"%02X%02X",(unsigned char)addr[i++],(unsigned char)addr[i++]);
		}

	}*/
	Debug_Printf(0,4,false, 0,"   0  1  2  3   4  5  6  7");
	for(int i=0; i<16;i++){
		Debug_Printf(1,5+(2*i),false, 0,"%X %02X %02X %02X %02X  %02X %02X %02X %02X  %c%c%c%c %c%c%c%c", i,
			addr [0+(16*i)],addr [1+(16*i)],addr [ 2+(16*i)],addr [ 3+(16*i)],addr [ 4+(16*i)],addr [ 5+(16*i)],addr [ 6+(16*i)],addr [ 7+(16*i)],
			to_ch(0+(16*i)),to_ch(1+(16*i)),to_ch( 2+(16*i)),to_ch( 3+(16*i)),to_ch( 4+(16*i)),to_ch( 5+(16*i)),to_ch( 6+(16*i)),to_ch( 7+(16*i)));
		Debug_Printf(1,6+(2*i),false, 0, "  %02X %02X %02X %02X  %02X %02X %02X %02X  %c%c%c%c %c%c%c%c",
			addr [8+(16*i)],addr [9+(16*i)],addr [10+(16*i)],addr [11+(16*i)],addr [12+(16*i)],addr [13+(16*i)],addr [14+(16*i)],addr [15+(16*i)],
			to_ch(8+(16*i)),to_ch(9+(16*i)),to_ch(10+(16*i)),to_ch(11+(16*i)),to_ch(12+(16*i)),to_ch(13+(16*i)),to_ch(14+(16*i)),to_ch(15+(16*i)));
	}

	LCD_Refresh();
	Debug_WaitKey();
}

//Debug_Printf(0,4,false, 0,"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"); //44 lines, 53 chars each
void showText(char* fileName){
	int fp;
	uint8_t* addr;
	fp = open(fileName, OPEN_READ);
	getAddr(fp,0,(const void**)&addr);
	close(fp);

	uint32_t page = 0; //the currently displeyed page
	int32_t skip = 1; //number of frames that should be advanced
	uint32_t i = 0;
	bool end = false;
	while(true){
		LCD_ClearScreen();
		Debug_Printf(0,0,false,0,"Viewing file %s",fileName);
		while(skip>0){
			int      j = 0;
			char line[54];
			for(int y=1;y<43;y++){
				j=0;
				while(true){
					unsigned char ch = addr[i+j];
					if(ch==0){
						end = true;
						line[j]=0;
						break;
					}
					if(ch=='\n'||ch=='\r'){
						line[j]=0;
						j++;
						break;
					}
					if(j==53){
						line[j]=0;
						int j2 = j;
						while(j2>0){
							if(line[j2]==' '){
								line[j2]=0;
								j=j2+1;
								break;
							}
							j2--;
						}
						break;
					}
					line[j]=numToAscii[(int)ch];
					j++;
				}
				if(skip==1) //don't print pages if you skip them
					Debug_Printf(0,y,false,0,"%s",line);
				i+=j;
				if(end){
					break;
				}
			}
			skip--; //turning pages done
			page++; //we are now on the next page.
			if(end){
				break;
			}
		}
		Debug_Printf(13,43,false,0,"To skip x pages type x+100 (page %d)",page);
		LCD_Refresh();
		skip = getCommandInput();
		if(skip==EXIT_PROGRAM||end)
			break;
		skip-=100;
		if(skip<=0)
			skip=1;
	}
}

void show565(char* fileName){
	uint16_t* VRAM = LCD_GetVRAMAddress();
	int W,H; LCD_GetSize(&W,&H);

	int fp;
	uint16_t* addr;
	fp = open(fileName, OPEN_READ);
	getAddr(fp,0,(const void**)&addr);
	close(fp);

	Debug_Printf(0,42,false,0,"Please type in the width (usually 160)");
	//the getCommandInput subroutine will call LCD_Refresh, so I don't have to do so here...
	long width = getCommandInput();
	if(width<1)
		width=160;
	uint32_t pos = 0;
	for(int y=0;y<528;y++){
		for(int x=0;x<width;x++){
			uint16_t color= addr[pos++];
			VRAM[x+y*W]=color;
		}
	}
	LCD_Refresh();
	Debug_WaitKey();
}

