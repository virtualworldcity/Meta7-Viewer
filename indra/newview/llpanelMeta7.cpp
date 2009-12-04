
/** 
 * @file LLPanelMeta7.cpp
 * @brief General preferences panel in preferences floater
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanelMeta7.h"
#include "lggBeamMapFloater.h"
// linden library includes
#include "llradiogroup.h"
#include "llbutton.h"
#include "lluictrlfactory.h"

#include "llcombobox.h"
#include "llslider.h"
#include "lltexturectrl.h"

#include "lggBeamMaps.h"

// project includes
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsdserialize.h"

#include "lltabcontainer.h"

#include "llinventorymodel.h"
#include "llfilepicker.h"
#include "llstartup.h"

#include "lltexteditor.h"

#include "llagent.h"

#include "lldirpicker.h"

#include "llweb.h" // [$PLOTR$/]
#include "lggBeamColorMapFloater.h"
#include "llsliderctrl.h"
#include "mfdKeywordFloater.h"

////////begin drop utility/////////////
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class JCInvDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//
// altered to support a callback so i can slap it in things and it just return the item to a func of my choice
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class JCInvDropTarget : public LLView
{
public:
	JCInvDropTarget(const std::string& name, const LLRect& rect, void (*callback)(LLViewerInventoryItem*));
	~JCInvDropTarget();

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
protected:
	void	(*mDownCallback)(LLViewerInventoryItem*);
};


JCInvDropTarget::JCInvDropTarget(const std::string& name, const LLRect& rect,
						  void (*callback)(LLViewerInventoryItem*)) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mDownCallback(callback)
{
}

JCInvDropTarget::~JCInvDropTarget()
{
}

void JCInvDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "JCInvDropTarget::doDrop()" << llendl;
}

BOOL JCInvDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	if(getParent())
	{
		handled = TRUE;
		// check the type
		//switch(cargo_type)
		//{
		//case DAD_ANIMATION:
		//{
			LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
			if(gInventory.getItem(inv_item->getUUID()))
			{
				*accept = ACCEPT_YES_COPY_SINGLE;
				if(drop)
				{
					//printchat("accepted");
					mDownCallback(inv_item);
				}
			}
			else
			{
				*accept = ACCEPT_NO;
			}
		//	break;
		//}
		//default:
		//	*accept = ACCEPT_NO;
		//	break;
		//}
	}
	return handled;
}
////////end drop utility///////////////

LLPanelMeta7* LLPanelMeta7::sInstance;

JCInvDropTarget * LLPanelMeta7::mObjectDropTarget;

LLPanelMeta7::LLPanelMeta7()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_Meta7.xml");
	if(sInstance)delete sInstance;
	sInstance = this;
}

LLPanelMeta7::~LLPanelMeta7()
{
	sInstance = NULL;
	delete mObjectDropTarget;
	mObjectDropTarget = NULL;
}

void LLPanelMeta7::initHelpBtn(const std::string& name, const std::string& xml_alert)
{
	childSetAction(name, onClickHelp, new std::string(xml_alert));
}

void LLPanelMeta7::onClickHelp(void* data)
{
	std::string* xml_alert = (std::string*)data;
	LLNotifications::instance().add(*xml_alert);
}

BOOL LLPanelMeta7::postBuild()
{
	refresh();
	getChild<LLComboBox>("material")->setSimple(gSavedSettings.getString("Meta7BuildPrefs_Material"));
	getChild<LLComboBox>("combobox shininess")->setSimple(gSavedSettings.getString("Meta7BuildPrefs_Shiny"));
	

	LLSliderCtrl* mShapeScaleSlider = getChild<LLSliderCtrl>("Meta7BeamShapeScale",TRUE,FALSE);
	mShapeScaleSlider->setCommitCallback(&LLPanelMeta7::beamUpdateCall);
	mShapeScaleSlider->setCallbackUserData(this);

	LLSliderCtrl* mBeamsPerSecondSlider = getChild<LLSliderCtrl>("Meta7MaxBeamsPerSecond",TRUE,FALSE);
	mBeamsPerSecondSlider->setCommitCallback(&LLPanelMeta7::beamUpdateCall);
	mBeamsPerSecondSlider->setCallbackUserData(this);

	getChild<LLComboBox>("material")->setCommitCallback(onComboBoxCommit);
	getChild<LLComboBox>("combobox shininess")->setCommitCallback(onComboBoxCommit);
	getChild<LLComboBox>("Meta7BeamShape_combo")->setCommitCallback(onComboBoxCommit);
	getChild<LLComboBox>("BeamColor_combo")->setCommitCallback(onComboBoxCommit);
	getChild<LLTextureCtrl>("texture control")->setDefaultImageAssetID(LLUUID("89556747-24cb-43ed-920b-47caed15465f"));
	getChild<LLTextureCtrl>("texture control")->setCommitCallback(onTexturePickerCommit);

		
	//childSetCommitCallback("material",onComboBoxCommit);
	//childSetCommitCallback("combobox shininess",onComboBoxCommit);
	getChild<LLButton>("Meta7Prefs_Stealth")->setClickedCallback(onStealth, this);
	getChild<LLButton>("Meta7Prefs_FullFeatures")->setClickedCallback(onNoStealth, this);


	getChild<LLButton>("keyword_allert")->setClickedCallback(onKeywordAllertButton,this);
	

	getChild<LLButton>("BeamColor_new")->setClickedCallback(onCustomBeamColor, this);
	getChild<LLButton>("BeamColor_refresh")->setClickedCallback(onRefresh,this);
	getChild<LLButton>("BeamColor_delete")->setClickedCallback(onBeamColorDelete,this);
			
	getChild<LLButton>("custom_beam_btn")->setClickedCallback(onCustomBeam, this);
	getChild<LLButton>("refresh_beams")->setClickedCallback(onRefresh,this);
	getChild<LLButton>("delete_beam")->setClickedCallback(onBeamDelete,this);

	getChild<LLButton>("revert_production_voice_btn")->setClickedCallback(onClickVoiceRevertProd, this);
	getChild<LLButton>("revert_debug_voice_btn")->setClickedCallback(onClickVoiceRevertDebug, this);

	getChild<LLButton>("Meta7BreastReset")->setClickedCallback(onClickBoobReset, this);
	

	childSetCommitCallback("production_voice_field", onCommitApplyControl);//onCommitVoiceProductionServerName);
	childSetCommitCallback("debug_voice_field", onCommitApplyControl);//onCommitVoiceDebugServerName);

	childSetCommitCallback("Meta7CmdLinePos", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdLineGround", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdLineHeight", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdLineTeleportHome", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdLineRezPlatform", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdLineMapTo", onCommitApplyControl);	
	childSetCommitCallback("Meta7CmdLineCalc", onCommitApplyControl);

	childSetCommitCallback("Meta7CmdLineDrawDistance", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdTeleportToCam", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdLineKeyToName", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdLineOfferTp", onCommitApplyControl);
	childSetCommitCallback("Meta7CmdLineAO", onCommitApplyControl);

	childSetCommitCallback("X Modifier", onCommitSendAppearance);
	childSetCommitCallback("Y Modifier", onCommitSendAppearance);
	childSetCommitCallback("Z Modifier", onCommitSendAppearance);
	childSetValue("Meta7DoubleClickTeleportMode", gSavedSettings.getBOOL("Meta7DoubleClickTeleportMode"));
	childSetValue("Meta7UseOTR", LLSD((S32)gSavedSettings.getU32("Meta7UseOTR"))); // [$PLOTR$]
	getChild<LLButton>("otr_help_btn")->setClickedCallback(onClickOtrHelp, this);      // [/$PLOTR$]

	initHelpBtn("Meta7Help_TeleportLogin",	"Meta7Help_TeleportLogin");
	initHelpBtn("Meta7Help_Voice",			"Meta7Help_Voice");
	initHelpBtn("Meta7Help_Shields",			"Meta7Help_Shields");
	initHelpBtn("Meta7Help_IM",				"Meta7Help_IM");
	initHelpBtn("Meta7Help_Chat",				"Meta7Help_Chat");
	initHelpBtn("Meta7Help_Misc",				"Meta7Help_Misc");
	initHelpBtn("Meta7Help_CmdLine",			"Meta7Help_CmdLine");
	initHelpBtn("Meta7Help_Avatar",			"Meta7Help_Avatar");
	initHelpBtn("Meta7Help_Build",			"Meta7Help_Build");
	initHelpBtn("Meta7Help_IRC",				"Meta7Help_IRC");
	initHelpBtn("Meta7Help_UtilityStream",	"Meta7Help_UtilityStream");
	initHelpBtn("Meta7Help_Inventory",		"Meta7Help_Inventory");
	initHelpBtn("Meta7Help_Effects",			"Meta7Help_Effects");

	LLView *target_view = getChild<LLView>("im_give_drop_target_rect");
	if(target_view)
	{
		if (mObjectDropTarget)//shouldn't happen
		{
			delete mObjectDropTarget;
		}
		mObjectDropTarget = new JCInvDropTarget("drop target", target_view->getRect(), IMAutoResponseItemDrop);//, mAvatarID);
		addChild(mObjectDropTarget);
	}

	if(LLStartUp::getStartupState() == STATE_STARTED)
	{
		LLUUID itemid = (LLUUID)gSavedPerAccountSettings.getString("Meta7InstantMessageResponseItemData");
		LLViewerInventoryItem* item = gInventory.getItem(itemid);
		if(item)
		{
			childSetValue("im_give_disp_rect_txt","Currently set to: "+item->getName());
		}else if(itemid.isNull())
		{
			childSetValue("im_give_disp_rect_txt","Currently not set");
		}else
		{
			childSetValue("im_give_disp_rect_txt","Currently set to a item not on this account");
		}
	}else
	{
		childSetValue("im_give_disp_rect_txt","Not logged in");
	}

	LLWString auto_response = utf8str_to_wstring( gSavedPerAccountSettings.getString("Meta7InstantMessageResponse") );
	LLWStringUtil::replaceChar(auto_response, '^', '\n');
	LLWStringUtil::replaceChar(auto_response, '%', ' ');
	childSetText("im_response", wstring_to_utf8str(auto_response));
	childSetValue("Meta7InstantMessageResponseFriends", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageResponseFriends"));
	childSetValue("Meta7InstantMessageResponseMuted", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageResponseMuted"));
	childSetValue("Meta7InstantMessageResponseAnyone", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageResponseAnyone"));
	childSetValue("Meta7InstantMessageShowResponded", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageShowResponded"));
	childSetValue("Meta7InstantMessageShowOnTyping", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageShowOnTyping"));
	childSetValue("Meta7InstantMessageResponseRepeat", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageResponseRepeat" ));
	childSetValue("Meta7InstantMessageResponseItem", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageResponseItem"));
	childSetValue("Meta7InstantMessageAnnounceIncoming", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageAnnounceIncoming"));
	childSetValue("Meta7InstantMessageAnnounceStealFocus", gSavedPerAccountSettings.getBOOL("Meta7InstantMessageAnnounceStealFocus"));
	childSetValue("Meta7ShadowsON", gSavedSettings.getBOOL("Meta7ShadowsToggle"));

	childSetAction("set_mirror", onClickSetMirror, this);
	childSetCommitCallback("mirror_location", onCommitApplyControl);

	getChild<LLCheckBoxCtrl>("telerequest_toggle")->setCommitCallback(onConditionalPreferencesChanged);
	getChild<LLCheckBoxCtrl>("mldct_toggle")->setCommitCallback(onConditionalPreferencesChanged);

	refresh();
	return TRUE;
}

void LLPanelMeta7::refresh()
{
	LLComboBox* comboBox = getChild<LLComboBox>("Meta7BeamShape_combo");
	

	if(comboBox != NULL) 
	{
		comboBox->removeall();
		comboBox->add("===OFF===");
		std::vector<std::string> names = gLggBeamMaps.getFileNames();
		for(int i=0; i<(int)names.size(); i++) 
		{
			comboBox->add(names[i]);
		}
		comboBox->setSimple(gSavedSettings.getString("Meta7BeamShape"));
	}

	comboBox = getChild<LLComboBox>("BeamColor_combo");
	if(comboBox != NULL) 
	{
		comboBox->removeall();
		comboBox->add("===OFF===");
		std::vector<std::string> names = gLggBeamMaps.getColorsFileNames();
		for(int i=0; i<(int)names.size(); i++) 
		{
			comboBox->add(names[i]);
		}
		comboBox->setSimple(gSavedSettings.getString("Meta7BeamColorFile"));
	}

	//epic hax (TODO: make this less hax)
	onConditionalPreferencesChanged(getChild<LLCheckBoxCtrl>("telerequest_toggle"), NULL);

	//mSkin = gSavedSettings.getString("SkinCurrent");
	//getChild<LLRadioGroup>("skin_selection")->setValue(mSkin);
}

void LLPanelMeta7::IMAutoResponseItemDrop(LLViewerInventoryItem* item)
{
	gSavedPerAccountSettings.setString("Meta7InstantMessageResponseItemData", item->getUUID().asString());
	sInstance->childSetValue("im_give_disp_rect_txt","Currently set to: "+item->getName());
}

void LLPanelMeta7::apply()
{
	LLTextEditor* im = sInstance->getChild<LLTextEditor>("im_response");
	LLWString im_response;
	if (im) im_response = im->getWText(); 
	LLWStringUtil::replaceTabsWithSpaces(im_response, 4);
	LLWStringUtil::replaceChar(im_response, '\n', '^');
	LLWStringUtil::replaceChar(im_response, ' ', '%');
	gSavedPerAccountSettings.setString("Meta7InstantMessageResponse", std::string(wstring_to_utf8str(im_response)));

	//gSavedPerAccountSettings.setString(
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageResponseMuted", childGetValue("Meta7InstantMessageResponseMuted").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageResponseFriends", childGetValue("Meta7InstantMessageResponseFriends").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageResponseMuted", childGetValue("Meta7InstantMessageResponseMuted").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageResponseAnyone", childGetValue("Meta7InstantMessageResponseAnyone").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageShowResponded", childGetValue("Meta7InstantMessageShowResponded").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageShowOnTyping", childGetValue("Meta7InstantMessageShowOnTyping").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageResponseRepeat", childGetValue("Meta7InstantMessageResponseRepeat").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageResponseItem", childGetValue("Meta7InstantMessageResponseItem").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageAnnounceIncoming", childGetValue("Meta7InstantMessageAnnounceIncoming").asBoolean());
	gSavedPerAccountSettings.setBOOL("Meta7InstantMessageAnnounceStealFocus", childGetValue("Meta7InstantMessageAnnounceStealFocus").asBoolean());
	gSavedSettings.setBOOL("Meta7DoubleClickTeleportMode", childGetValue("Meta7DoubleClickTeleportMode").asBoolean());
	if(((gSavedSettings.getU32("RenderQualityPerformance")>=3) && gSavedSettings.getBOOL("WindLightUseAtmosShaders") && gSavedSettings.getBOOL("VertexShaderEnable")) && childGetValue("Meta7ShadowsON").asBoolean())
	{
		gSavedSettings.setBOOL("RenderUseFBO", childGetValue("Meta7ShadowsON").asBoolean());
		gSavedSettings.setBOOL("RenderDeferred", childGetValue("Meta7ShadowsON").asBoolean());
	}
	else if(!childGetValue("Meta7ShadowsON").asBoolean())
	{
		if(gSavedSettings.getBOOL("RenderDeferred"))
		{
			gSavedSettings.setBOOL("RenderDeferred", childGetValue("Meta7ShadowsON").asBoolean());
			gSavedSettings.setBOOL("RenderUseFBO", childGetValue("Meta7ShadowsON").asBoolean());
		}
	}
	else if(((gSavedSettings.getU32("RenderQualityPerformance")<3) && !gSavedSettings.getBOOL("WindLightUseAtmosShaders") && !gSavedSettings.getBOOL("VertexShaderEnable")) && childGetValue("Meta7ShadowsON").asBoolean())
	{
		childSetValue("Meta7ShadowsON",false);
		LLNotifications::instance().add("NoShadows");
		llwarns<<"Attempt to enable shadow rendering while graphics settings not on ultra!"<<llendl;
	}
	gSavedSettings.setBOOL("Meta7ShadowsToggle", childGetValue("Meta7ShadowsON").asBoolean());
	gSavedSettings.setU32("Meta7UseOTR", (U32)childGetValue("Meta7UseOTR").asReal());
	gLggBeamMaps.forceUpdate();
}

void LLPanelMeta7::cancel()
{
}
void LLPanelMeta7::onClickVoiceRevertProd(void* data)
{
	LLPanelMeta7* self = (LLPanelMeta7*)data;
	gSavedSettings.setString("vivoxProductionServerName", "bhr.vivox.com");
	self->getChild<LLLineEditor>("production_voice_field")->setValue("bhr.vivox.com");
}

void LLPanelMeta7::onClickBoobReset(void* data)
{
	LLPanelMeta7* self = (LLPanelMeta7*)data;
	LLControlVariable *var;

	var = self->findControl("Meta7BoobMass");
	self->getChild<LLSliderCtrl>("Meta7BoobMass")->setValue(var->getDefault());
	var->resetToDefault();

	var = self->findControl("Meta7BoobHardness");
	self->getChild<LLSliderCtrl>("Meta7BoobHardness")->setValue(var->getDefault());
	var->resetToDefault();

	var = self->findControl("Meta7BoobVelMax");
	self->getChild<LLSliderCtrl>("Meta7BoobVelMax")->setValue(var->getDefault());
	var->resetToDefault();

	var = self->findControl("Meta7BoobFriction");
	self->getChild<LLSliderCtrl>("Meta7BoobFriction")->setValue(var->getDefault());
	var->resetToDefault();

	var = self->findControl("Meta7BoobVelMin");
	self->getChild<LLSliderCtrl>("Meta7BoobVelMin")->setValue(var->getDefault());
	var->resetToDefault();
}

void LLPanelMeta7::onCustomBeam(void* data)
{
	//LLPanelMeta7* self =(LLPanelMeta7*)data;
	LggBeamMap::show(true, data);

}
void LLPanelMeta7::onKeywordAllertButton(void * data)
{
	MfdKeywordFloaterStart::show(true,data);
}
void LLPanelMeta7::onCustomBeamColor(void* data)
{
	LggBeamColorMap::show(true,data);
}
void LLPanelMeta7::onStealth(void* data)
{
	//LLPanelMeta7* self =(LLPanelMeta7*)data;
	LLNotifications::instance().add("Meta7Stealth", LLSD(),LLSD(), callbackMeta7Stealth);
	

}
void LLPanelMeta7::callbackMeta7Stealth(const LLSD &notification, const LLSD &response)
{
	//gSavedSettings.setWarning("Meta7OTR", FALSE);
	S32 option = LLNotification::getSelectedOption(notification, response);
	if ( option == 0 )
	{
		gSavedSettings.setU32("Meta7UseOTR",(U32)0);
		gSavedSettings.setBOOL("Meta7RainbowBeam",false);
		gSavedSettings.setString("Meta7BeamShape","===OFF===");
		gSavedSettings.setBOOL("Meta7CryoDetection",false);
		gSavedSettings.setBOOL("Meta7GUSEnabled",false);
		gSavedSettings.setBOOL("Meta7ClothingLayerProtection",false);
		gSavedSettings.setBOOL("Meta7ParticleChat",false);
		gSavedSettings.setBOOL("Meta7RadarChatKeys",false);
		gSavedSettings.setBOOL("Meta7UseBridgeOnline",false);
		gSavedSettings.setBOOL("Meta7UseBridgeRadar",false);
		gSavedSettings.setBOOL("Meta7MoveLockDCT",false);
	}
}
void LLPanelMeta7::onNoStealth(void* data)
{
	//LLPanelMeta7* self =(LLPanelMeta7*)data;
	
	LLNotifications::instance().add("Meta7NoStealth", LLSD(),LLSD(), callbackMeta7NoStealth);
	

}

void LLPanelMeta7::callbackMeta7NoStealth(const LLSD &notification, const LLSD &response)
{
	//gSavedSettings.setWarning("Meta7OTR", FALSE);
	S32 option = LLNotification::getSelectedOption(notification, response);
	if ( option == 0 )
	{
		gSavedSettings.setU32("Meta7UseOTR",(U32)2);
		gSavedSettings.setBOOL("Meta7RainbowBeam",true);
		gSavedSettings.setString("Meta7BeamShape","Meta7");
		//gSavedSettings.setBOOL("Meta7CryoDetect",true);
		//gSavedSettings.setBOOL("Meta7GUSEnabled",true);
		gSavedSettings.setBOOL("Meta7ClothingLayerProtection",true);
		gSavedSettings.setBOOL("Meta7BuildBridge",true);
		gSavedSettings.setBOOL("Meta7UseBridgeOnline",true);
		gSavedSettings.setBOOL("Meta7UseBridgeRadar",true);		
		gSavedSettings.setBOOL("Meta7MoveLockDCT",true);

	}
}
void LLPanelMeta7::beamUpdateCall(LLUICtrl* crtl, void* userdata)
{
	gLggBeamMaps.forceUpdate();
}
void LLPanelMeta7::onComboBoxCommit(LLUICtrl* ctrl, void* userdata)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if(box)
	{
		gSavedSettings.setString(box->getControlName(),box->getValue().asString());
	}
	
}
void LLPanelMeta7::onTexturePickerCommit(LLUICtrl* ctrl, void* userdata)
{
	LLTextureCtrl*	image_ctrl = (LLTextureCtrl*)ctrl;
	if(image_ctrl)
	{
		gSavedSettings.setString("Meta7BuildPrefs_Texture", image_ctrl->getImageAssetID().asString());
	}
}

void LLPanelMeta7::onClickOtrHelp(void* data)           // [$PLOTR$]
{
    LLWeb::loadURL("http://www.cypherpunks.ca/otr/");
}                                                         // [/$PLOTR$]

void LLPanelMeta7::onRefresh(void* data)
{
	LLPanelMeta7* self = (LLPanelMeta7*)data;
	self->refresh();
	
	
}
void LLPanelMeta7::onClickVoiceRevertDebug(void* data)
{
	LLPanelMeta7* self = (LLPanelMeta7*)data;
	gSavedSettings.setString("vivoxDebugServerName", "bhd.vivox.com");
	self->getChild<LLLineEditor>("debug_voice_field")->setValue("bhd.vivox.com");

 
 
}
void LLPanelMeta7::onBeamDelete(void* data)
{
	LLPanelMeta7* self = (LLPanelMeta7*)data;
	
	LLComboBox* comboBox = self->getChild<LLComboBox>("Meta7BeamShape_combo");

	if(comboBox != NULL) 
	{
		std::string filename = comboBox->getValue().asString()+".xml";
		std::string path_name1(gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS , "beams", filename));
		std::string path_name2(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "beams", filename));
		
		if(gDirUtilp->fileExists(path_name1))
		{
			LLFile::remove(path_name1);
			gSavedSettings.setString("Meta7BeamShape","===OFF===");
		}
		if(gDirUtilp->fileExists(path_name2))
		{
			LLFile::remove(path_name2);
			gSavedSettings.setString("Meta7BeamShape","===OFF===");
		}
	}
	self->refresh();
}
void LLPanelMeta7::onBeamColorDelete(void* data)
{
	LLPanelMeta7* self = (LLPanelMeta7*)data;

	LLComboBox* comboBox = self->getChild<LLComboBox>("BeamColor_combo");

	if(comboBox != NULL) 
	{
		std::string filename = comboBox->getValue().asString()+".xml";
		std::string path_name1(gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS , "beamsColors", filename));
		std::string path_name2(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "beamsColors", filename));

		if(gDirUtilp->fileExists(path_name1))
		{
			LLFile::remove(path_name1);
			gSavedSettings.setString("Meta7BeamColorFile","===OFF===");
		}
		if(gDirUtilp->fileExists(path_name2))
		{
			LLFile::remove(path_name2);
			gSavedSettings.setString("Meta7BeamColorFile","===OFF===");
		}
	}
	self->refresh();
}



//workaround for lineeditor dumbness in regards to control_name
void LLPanelMeta7::onCommitApplyControl(LLUICtrl* caller, void* user_data)
{
	LLLineEditor* line = (LLLineEditor*)caller;
	if(line)
	{
		LLControlVariable *var = line->findControl(line->getControlName());
		if(var)var->setValue(line->getValue());
	}
}

void LLPanelMeta7::onCommitSendAppearance(LLUICtrl* ctrl, void* userdata)
{
	gAgent.sendAgentSetAppearance();
	//llinfos << llformat("%d,%d,%d",gSavedSettings.getF32("Meta7AvatarXModifier"),gSavedSettings.getF32("Meta7AvatarYModifier"),gSavedSettings.getF32("Meta7AvatarZModifier")) << llendl;
}

void LLPanelMeta7::onClickSetMirror(void* user_data)
{
	LLPanelMeta7* self = (LLPanelMeta7*)user_data;

	std::string cur_name(gSavedSettings.getString("Meta7InvMirrorLocation"));
	std::string proposed_name(cur_name);
	
	LLDirPicker& picker = LLDirPicker::instance();
	if (! picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}

	std::string dir_name = picker.getDirName();
	if (!dir_name.empty() && dir_name != cur_name)
	{
		self->childSetText("mirror_location", dir_name);
		//LLNotifications::instance().add("CacheWillBeMoved");
		gSavedSettings.setString("Meta7InvMirrorLocation", dir_name);
	}
	else
	{
		std::string cache_location = gDirUtilp->getCacheDir();
		self->childSetText("mirror_location", cache_location);
	}
}

void LLPanelMeta7::onConditionalPreferencesChanged(LLUICtrl* ctrl, void* userdata)
{
	LLPanelMeta7* self = (LLPanelMeta7*)ctrl->getParent();
	if(!self)return;
	LLCheckBoxCtrl* teleport = self->getChild<LLCheckBoxCtrl>("telerequest_toggle");
	LLCheckBoxCtrl* movelock = self->getChild<LLCheckBoxCtrl>("mldct_toggle");
	if(!(teleport && movelock))return;
	//bool teep = (bool)teleport->getValue().asBoolean();
	bool moov = (bool)movelock->getValue().asBoolean();
	if(moov)
	{
		teleport->setEnabled(true);
	}
	else
	{
		teleport->setValue(LLSD(true));
		gSavedSettings.setBOOL("Meta7RequestLocalTeleports", true);
		teleport->setEnabled(false);
	}
}

/*
//static 
void LLPanelMeta7::onClickSilver(void* data)
{
	LLPanelMeta7* self = (LLPanelMeta7*)data;
	gSavedSettings.setString("SkinCurrent", "silver");
	self->getChild<LLRadioGroup>("skin_selection")->setValue("silver");
}*/
