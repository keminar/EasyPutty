#pragma once

#include "framework.h"
#include "apputils.h"


#define MAX_COMMAND_LEN 8190 //������󳤶�

// �洢������Ϣ�Ľṹ��

typedef struct {
	wchar_t name[256];
	wchar_t path[MAX_PATH];
	wchar_t params[MAX_COMMAND_LEN];
	wchar_t tags[256];
} ProgramInfo;

typedef struct {
	wchar_t name[256];
	wchar_t hostName[256];
	int port;
	wchar_t connectType[64];
	wchar_t credential[256];
	wchar_t otherParams[256];
	wchar_t tags[256];
} SessionInfo;

typedef struct {
	wchar_t name[256];
	wchar_t userName[256];
	wchar_t password[128];
	wchar_t privateKey[256];
} CredentialInfo;

// ��ϣ��ڵ�
typedef struct HashNode {
	wchar_t key[256];
	CredentialInfo value;
	struct HashNode* next;
} HashNode;

// ��ϣ��
typedef struct {
	HashNode** buckets;
	int count;//ʹ�ô�С
	int capacity;//�ܴ�С
} ConfigMap;

// ���ݵ�����
typedef struct {
	wchar_t name[256];
	wchar_t command[MAX_COMMAND_LEN];
} NameCommand;

LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitOverview(HINSTANCE hInstance, struct TabWindowsInfo *tabWindowsInfo, HWND hostWindow, HWND searchEdit);
void SetListViewData(HWND hListView);
void InitializeListViewColumns(HWND hWndListView);
void AddListViewItem(HWND hWndListView, int nItem, const wchar_t* name, const wchar_t* type, const wchar_t* command, const wchar_t* tags, const wchar_t* credential, const wchar_t* input);
void execCommand(HWND hwnd, HWND hListView, int selectedItem);
void filezillaCommand(HWND hwnd, HWND hListView, int selectedItem);
void winscpCommand(HWND hwnd, HWND hListView, int selectedItem);
void psftpCommand(HWND hwnd, HWND hListView, int selectedItem);
wchar_t** ListIniFiles(const wchar_t* directoryPath, int* fileCount);
void FreeFileList(wchar_t** fileList, int fileCount);
void ReadSessionFromIni(const wchar_t* filepath, SessionInfo* config);
void ReadCredentialFromIni(const wchar_t* filepath, CredentialInfo* config);
void ReadProgramFromIni(const wchar_t* filepath, ProgramInfo* config);

unsigned int hash(const wchar_t* str, int capacity);
// ��ʼ������ӳ���
ConfigMap* initConfigMap(int capacity);

// ���������
int addConfig(ConfigMap* map, const wchar_t* name, const wchar_t* username, const wchar_t* password, const wchar_t* privateKey);

// ����Name��������
CredentialInfo* findConfigByName(ConfigMap* map, const wchar_t* name);

// �ͷ�����ӳ����ڴ�
void FreeConfigMap(ConfigMap* map);