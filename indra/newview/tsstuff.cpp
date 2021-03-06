// Copyright (c)2009 Thomas Shikami
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "llviewerprecompiledheaders.h"

#include "tsstuff.h"

#include "lluictrlfactory.h"
#include "llviewertexteditor.h"
#include "llcallbacklist.h"
#include "llviewercontrol.h"
#include "llkeyboard.h"

TSStuff* TSStuff::sInstance = NULL;

void TSLuaAction::run()
{
	// empty
}

TSLuaAction::~TSLuaAction()
{
	// do nothing
}

class TSLuaActionPrint : public TSLuaAction {
public:
	TSLuaActionPrint(const char *s, BOOL isError = FALSE);
	~TSLuaActionPrint();

	void run();
private:
	char *mString;
	BOOL mError;
};

TSLuaActionPrint::TSLuaActionPrint(const char *s, BOOL isError)
{
	mString = strdup(s);
	mError = isError;
}

TSLuaActionPrint::~TSLuaActionPrint()
{
	free(mString);
}

void TSLuaActionPrint::run()
{
	LLFloaterLuaConsole *console = LLFloaterLuaConsole::getInstance();
	LLViewerTextEditor *editor = console->getChild<LLViewerTextEditor>("Lua Output Editor");
	LLColor4 text_color;

	if(mError) {
		text_color = gSavedSettings.getColor4("ScriptErrorColor");
		editor->appendColoredText(std::string(mString) + "\n", false, false, text_color);
	} else {
		text_color = gSavedSettings.getColor4("SystemChatColor");
		editor->appendColoredText(std::string(mString), false, false, text_color);
	}
}

class TSStuffLua : public TSLua {
public:
	TSStuffLua(TSLuaThread *);

public:
	void output(const char *s);
	void error(const char *s);
	bool abortCondition();

private:
	TSLuaThread *mThread;
};

TSStuffLua::TSStuffLua(TSLuaThread *thread)
{
	mThread = thread;
}

void TSStuffLua::output(const char *s)
{
	mThread->addAction(new TSLuaActionPrint(s));
}

void TSStuffLua::error(const char *s)
{
	mThread->addAction(new TSLuaActionPrint(s, TRUE));
}

bool TSStuffLua::abortCondition()
{
	return (mThread->isQuitting() || mThread->isAborted());
}

TSStuff::TSStuff()
{
}

TSStuff::~TSStuff()
{
	sInstance = NULL;
}

void TSStuff::init()
{
	if(!sInstance) {
		sInstance = new TSStuff();
	}
	sInstance->doInit();
	gIdleCallbacks.addFunction(idle, sInstance);
}

void TSStuff::doInit()
{
	mThread = new TSLuaThread();
	mThread->start();
}

void TSStuff::cleanupClass()
{
	sInstance->doCleanup();
	delete sInstance;
}

void TSStuff::doCleanup()
{
	delete mThread;
}

TSStuff* TSStuff::getInstance()
{
	return sInstance;
}

void TSStuff::resetLua()
{
	delete mThread;
	mThread = new TSLuaThread();
	mThread->start();
}

void TSStuff::idle(void *data)
{
	getInstance()->getThread()->runActions();
}

TSLuaThread::TSLuaThread() : LLThread(std::string("LuaThread"))
{
	mSyncAction = NULL;
	mExecuting = false;
	mAborted = false;
}

TSLuaThread::~TSLuaThread()
{
	delete mSyncAction;
}

bool TSLuaThread::runCondition()
{
	while(!mActionDeleteQueue.empty()) {
		delete mActionDeleteQueue.front();
		mActionDeleteQueue.pop();
	}
	if(mSyncAction) {
		return false;
	}
	return (!mCommandQueue.empty() || mExecuting) && mActionQueue.size() < 10;
}

void TSLuaThread::run()
{
	char *s;
	mLua = new TSStuffLua(this);

	while(1)
	{
		checkPause();

		if(isQuitting())
			break;

		lockData();
		if(mCommandQueue.empty()) {
			s = NULL;
		} else {
			s = mCommandQueue.front();
			mCommandQueue.pop();
		}
		unlockData();

		if(s) {
			mExecuting = true;
			mAborted = false;
			mLua->execute(s);
			mExecuting = false;
			free(s);
		}

		lockData();
		if(mAborted) {
			while(!mCommandQueue.empty()) {
				free(mCommandQueue.front());
				mCommandQueue.pop();
			}
		}
		mAborted = false;
		unlockData();
	}
	delete mLua;
}

void TSLuaThread::abort()
{
	mAborted = true;
}

// Main Thread
void TSLuaThread::addCommand(const char *s)
{
	lockData();
	mCommandQueue.push(strdup(s));
	wakeLocked();
	unlockData();
}

// Lua Thread
void TSLuaThread::addAction(TSLuaAction *action)
{
	checkPause();

	lockData();
	mActionQueue.push(action);
	unlockData();
}

// Lua Thread
TSLuaAction *TSLuaThread::getActionDelete()
{
	TSLuaAction *action = NULL;

	lockData();
	if(!mActionDeleteQueue.empty()) {
		action = mActionDeleteQueue.front();
		mActionDeleteQueue.pop();
	}
	unlockData();

	return action;
}

// Lua Thread
void TSLuaThread::syncAction(TSLuaAction *action)
{
	lockData();
	mSyncAction = action;
	unlockData();

	checkPause();

	lockData();
	mSyncAction = NULL;
	unlockData();
}

// Main Thread
void TSLuaThread::runActions()
{
	lockData();
	while(!mActionQueue.empty()) {
		mActionQueue.front()->run();
		mActionDeleteQueue.push(mActionQueue.front());
		mActionQueue.pop();
	}
	if(mSyncAction) {
		mSyncAction->run();
		mSyncAction = NULL;
	}
	wakeLocked();
	unlockData();
}

LLFloaterLuaConsole* LLFloaterLuaConsole::sInstance = NULL;

LLFloaterLuaConsole::LLFloaterLuaConsole() :
LLFloater(std::string("lua console"))
{
}

LLFloaterLuaConsole::~LLFloaterLuaConsole()
{
	sInstance = NULL;
}

LLFloaterLuaConsole* LLFloaterLuaConsole::getInstance()
{
	if(!sInstance) {
		sInstance = new LLFloaterLuaConsole();
		LLUICtrlFactory::getInstance()->buildFloater(sInstance, "floater_lua_console.xml");
	}
	return sInstance;
}

void LLFloaterLuaConsole::show(void*)
{
	getInstance()->open();
}

void LLFloaterLuaConsole::toggle(void*)
{
	if(isVisible())
	{
		sInstance->setVisible(FALSE);
	}
	else
	{
		show(NULL);
	}
}

void LLFloaterLuaConsole::onClose(bool app_quitting)
{
	setVisible(FALSE);
}

BOOL LLFloaterLuaConsole::postBuild()
{
	childSetAction("Send", onClickSend, this);
	childSetAction("Clear", onClickClear, this);
	childSetAction("Abort", onClickAbort, this);
	childSetAction("Reset", onClickReset, this);
	
	//childDisable("Send");
	LLButton * sendp = getChild<LLButton>("Send");
	LLPanel * luap = getChild<LLPanel>("lua_panel");
	if(sendp && luap)
	{
		luap->setDefaultBtn(sendp);
	}

	LLLineEditor * editorp = getChild<LLLineEditor>("Lua Editor", TRUE);
	if(editorp)
	{
		editorp->setCommitOnFocusLost(FALSE);
		editorp->setRevertOnEsc(FALSE);
		editorp->setEnableLineHistory(TRUE);
	}

	return TRUE;
}

void LLFloaterLuaConsole::draw()
{
	LLFloater::draw();
}

void LLFloaterLuaConsole::onClickSend(void *data)
{
	LLFloaterLuaConsole *self = (LLFloaterLuaConsole *)data;
	TSStuff *stuff = TSStuff::getInstance();
	TSLuaThread *thread = stuff->getThread();
	LLLineEditor *editor = self->getChild<LLLineEditor>("Lua Editor", TRUE);

	if(editor->getLength()) {
		thread->addCommand(editor->getText().c_str());
		editor->updateHistory();
		editor->clear();
	}
}

void LLFloaterLuaConsole::onClickClear(void *data)
{
	LLFloaterLuaConsole *self = (LLFloaterLuaConsole *)data;
	LLViewerTextEditor *editor = self->getChild<LLViewerTextEditor>("Lua Output Editor");
	editor->removeTextFromEnd(editor->getWText().length());
	editor->makePristine();
}

void LLFloaterLuaConsole::onClickAbort(void *data)
{
	TSStuff *stuff = TSStuff::getInstance();
	stuff->getThread()->abort();
}

void LLFloaterLuaConsole::onClickReset(void *data)
{
	TSStuff *stuff = TSStuff::getInstance();
	stuff->resetLua();
}
