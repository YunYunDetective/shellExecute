#include <windows.h>
#include <tchar.h>
#include <process.h>
#include "ncbind/ncbind.hpp"

#define WM_SHELLEXECUTED (WM_APP+1)

/**
 * ���\�b�h�ǉ��p�N���X
 */
class WindowShell {

protected:
	iTJSDispatch2 *objthis; //< �I�u�W�F�N�g���̎Q��

	// �C�x���g����
	static bool __stdcall shellExecuted(void *userdata, tTVPWindowMessage *Message) {
		if (Message->Msg == WM_SHELLEXECUTED) {
			iTJSDispatch2 *obj = (iTJSDispatch2*)userdata;
			tTJSVariant process = (tjs_int)Message->WParam;
			tTJSVariant endCode = (tjs_int)Message->LParam;
			tTJSVariant *p[] = {&process, &endCode};
			obj->FuncCall(0, L"onShellExecuted", NULL, NULL, 2, p, obj);
			return true;
		}
		return false;
	}
	
	// ���[�U���b�Z�[�W���V�[�o�̓o�^/����
	void setReceiver(tTVPWindowMessageReceiver receiver, bool enable) {
		tTJSVariant mode     = enable ? (tTVInteger)(tjs_int)wrmRegister : (tTVInteger)(tjs_int)wrmUnregister;
		tTJSVariant proc     = (tTVInteger)(tjs_int)receiver;
		tTJSVariant userdata = (tTVInteger)(tjs_int)objthis;
		tTJSVariant *p[] = {&mode, &proc, &userdata};
		if (objthis->FuncCall(0, L"registerMessageReceiver", NULL, NULL, 4, p, objthis) != TJS_S_OK) {
			TVPThrowExceptionMessage(L"can't regist user message receiver");
		}
	}
	
public:
	// �R���X�g���N�^
	WindowShell(iTJSDispatch2 *objthis) : objthis(objthis) {
		setReceiver(shellExecuted, true);
	}

	// �f�X�g���N�^
	~WindowShell() {
		setReceiver(shellExecuted, false);
	}

public:
	/**
	 * ���s���
	 */
	struct ExecuteInfo {
		HANDLE process;         // �҂��Ώۃv���Z�X
		iTJSDispatch2 *objthis; // this �ێ��p
		ExecuteInfo(HANDLE process, iTJSDispatch2 *objthis) : process(process), objthis(objthis) {};
	};
	
	/**
	 * �I���҂��X���b�h����
	 * @param data ���[�U(ExecuteInfo)
	 */
	static void waitProcess(void *data) {
		// �p�����[�^�����p��
		HANDLE process         = ((ExecuteInfo*)data)->process;
		iTJSDispatch2 *objthis = ((ExecuteInfo*)data)->objthis;
		delete data;

		// �v���Z�X�҂�
		WaitForSingleObject(process, INFINITE);
		DWORD dt;
		GetExitCodeProcess(process, &dt); // ���ʎ擾
		CloseHandle(process);

		// ���M
		tTJSVariant val;
		objthis->PropGet(0, TJS_W("HWND"), NULL, &val, objthis);
		PostMessage(reinterpret_cast<HWND>((tjs_int)(val)), WM_SHELLEXECUTED, (WPARAM)process, (LPARAM)dt);
	}
	
	/**
	 * �v���Z�X�̒�~
	 * @param keyName ���ʃL�[
	 * @param endCode �I���R�[�h
	 */
	void terminateProcess(int process, int endCode) {
		TerminateProcess((HANDLE)process, endCode);
	}

	/**
	 * �v���Z�X�̎��s
	 * @param keyName ���ʃL�[
	 * @param target �^�[�Q�b�g
	 * @praam param �p�����[�^
	 */
	int shellExecute(LPCTSTR target, LPCTSTR param) {
		SHELLEXECUTEINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cbSize = sizeof(si);
		si.lpVerb = _T("open");
		si.lpFile = target;
		si.lpParameters = param;
		si.nShow = SW_SHOWNORMAL;
		si.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
		if (ShellExecuteEx(&si)) {
			_beginthread(waitProcess, 0, new ExecuteInfo(si.hProcess, objthis));
			return (int)si.hProcess;
		}
		return (int)INVALID_HANDLE_VALUE;
	}

};

// �C���X�^���X�Q�b�^
NCB_GET_INSTANCE_HOOK(WindowShell)
{
	NCB_INSTANCE_GETTER(objthis) { // objthis �� iTJSDispatch2* �^�̈����Ƃ���
		ClassT* obj = GetNativeInstance(objthis);	// �l�C�e�B�u�C���X�^���X�|�C���^�擾
		if (!obj) {
			obj = new ClassT(objthis);				// �Ȃ��ꍇ�͐�������
			SetNativeInstance(objthis, obj);		// objthis �� obj ���l�C�e�B�u�C���X�^���X�Ƃ��ēo�^����
		}
		return obj;
	}
};

// �t�b�N���A�^�b�`
NCB_ATTACH_CLASS_WITH_HOOK(WindowShell, Window) {
	Method(L"shellExecute", &WindowShell::shellExecute);
	Method(L"terminateProcess", &WindowShell::terminateProcess);
}
