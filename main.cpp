#include <appdef.hpp>

#include <sdk/os/string.hpp>
#include <sdk/os/file.hpp>
#include <sdk/os/lcd.hpp>
#include <sdk/os/debug.hpp>
#include <sdk/os/mem.hpp>
#include <sdk/os/input.hpp>

/*
 * Fill this section in with some information about your app.
 * All fields are optional - so if you don't need one, take it out.
 */
APP_NAME("Filemgr")
APP_DESCRIPTION("A simple file manager")
APP_AUTHOR("SnailMath")
APP_VERSION("1.0.0")

#define EXIT_PROGRAM	-1
#define GO_ROOT		-2
#define GO_SD		-3

int getCommandInput();

struct dirEntry{
	char fileName[100];
	char type;
};
const int maxFiles = 41;
int dirFiles = 0;
struct dirEntry directory[64];

char g_path[400];
wchar_t g_wpath[400];



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
		Debug_Printf(0,1,false,0,"99 D .. (go up one directory)");
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
			Debug_Printf(0,dirFiles+2,false,0,"%2d %c %s",dirFiles,thisfile.type,thisfile.fileName);
			LCD_Refresh();
			//save this dirEntry to directory
			directory[dirFiles++] = thisfile;
			
			//serch the next
			ret = findNext(findHandle, fileName, &findInfoBuf);
		}
		findClose(findHandle);
	
		int go = getCommandInput();
		//Debug_Printf(10,39,true,0,"go=%2x",go); 
		if (go>=0 && go<dirFiles){
			//Debug_Printf(0,0,true,0,"entering nr.%d!",go);
			struct dirEntry thisfile = directory[go];
			if(thisfile.type=='D'){
				strcat(g_path,thisfile.fileName);
				strcat(g_path,"\\");
			}
		}
		if (go==99){
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
	char cmd[5]; //the command line
	int pos = 1; //the position of the cursor in cmd
	strcpy (cmd,">_  ");

	struct InputEvent event;
	while (true){
		Debug_Printf(0,43,false,0,"%s",cmd); LCD_Refresh();
		memset(&event,0,sizeof(event));
		GetInput(&event,0xFFFFFFFF,0x10);
		if(event.type==EVENT_KEY){
			if(event.data.key.direction==KEY_PRESSED){
				if(pos<=2){//If we have have less than 3 chars ('>' + 2 digits)
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
									return 99;
								}else{
									cmd[1]='.';
								}
							}break;
						case KEYCODE_KEYBOARD: return GO_ROOT;
						case KEYCODE_SHIFT: return GO_SD;
					}
				}//pos!<=2
				if (pos>1){ //If there are chars you can delete one.
					if(event.data.key.keyCode==KEYCODE_BACKSPACE){
						cmd[pos--]=' ';cmd[pos]='_';
					}//keycode!=Backspace
				}//pos!>1
				if(event.data.key.keyCode==KEYCODE_POWER_CLEAR){
					return EXIT_PROGRAM;
				}//!Clear
				if(event.data.key.keyCode==KEYCODE_EXE){
					if(cmd[1]!='_'&&cmd[1]!='.'){
						int ret = cmd[1]-'0';
						if(cmd[2]!='_'){
							ret = (ret*10) + (cmd[2]-'0');
						}//cmd[2]!='_'
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
