/*
*类    名:none
*函 数 名:none
*功    能:批价程序
*输入参数:none
*输出参数:none
*返 回 值:none
*修改记录:
修改日期      修改人            修改内容
----------------------------------------
2005-09-14    xxxxxx            方法生成
*/
#ifndef _CDR_RATING_H_
#define _CDR_RATING_H_

#include <iostream>
#include <exception>

#include "AppComponent.h"
#include "string/Split.h"
#include "HBMsgPlug.h"
#include "PetriInfo.h"

#include "Component.h"
#include "common/debug_new.h"
#include "mdbdata/mdbdata_pub.h"
#include "pricing/TbInfoManager.h"
#include "pricing/EventAdapter.h"
#include "file/ReadIni.h"
#include "pricing/RatingManager.h"
#ifndef APP_CFG
#define APP_CFG                 "App.config"
#endif

#ifdef _WIN32
#include <process.h>
#include "getopt.h"
#define APP_DIR                 ".\\"
#else 
#include <unistd.h>
#define APP_DIR                 "./"
#endif  //_UNIX

using namespace std;
enum TRT_MOD
{
    TRT_MOD_UPDATE_RATABLE_RESOURCE,
    TRT_MOD_SERCH_PRICING_DATA,
    TRT_MOD_CONVERT_RATING_RESULT,
    TRT_MOD_EXTERN_PROERTY,
    TRT_MOD_PRICING_PLAN,
    TRT_MOD_BUY_OFFER,
    TRT_MOD_CDR_RATING,
    TRT_MOD_MAX
};


class CTrtApp :public CAppComponent
{
public:
    CTrtApp();
    ~CTrtApp(){}

    int Run();

public:
    void PrintUsage();

    bool InitParam(int argc, char *argv[]);

private:
    //////////////////////////////////////////////////////////////////////////
    //测试方式
    int RunRatableResourceUpdate();
    int RunSearchUnitTest(int iEventTypeId);
    int RunOfferFee();
    int RunExternPorperty();
    int RunPricingPlan();
    int RunViewPricingPlan(int iPricingPlan);
    int RunBuyProductOffer();
    int TestCdrRating();
    //////////////////////////////////////////////////////////////////////////
    int RunCdrRating(TRatableEvent *pclRatableEvent,LSRatableResourceAccumulator *plsCalcResult);
    //////////////////////////////////////////////////////////////////////////
    // 构造测试数据方法
    int FillEpsData(vector<TRateResultRecordOfEps> &VcAllRateResult);
    TCdrEvent * CreateCdrEventIns();
    int SetEventAttr(TCdrEvent &clCdrEvent);
    int SetOwnerPlus(TCdrEvent &clCdrEvent);
    int InitTestData();
    int FillOfferInfoVec(vector<TOfferInfo> &vOfferInfo);
    int OutputOfferInfoVec(vector<TOfferInfo> &vOfferInfo);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //工具方法
    int InsertTableProductOfferInstance(TDBQuery *pQuery);
    int InsertTableProductOfferInstance(TMdbQuery *pQuery);
    int InsertTableProductOfferObjectInstance(TDBQuery *pQuery);
    int InsertTableProductOfferObjectInstance(TMdbQuery *pQuery);
    int GetStringBySeparater( const char cSeparater,const char * pszString,vector<string>& vecString );
    int FillEPSRlt( TAvailableEventPrcStategy& tAEPS, TRateResultRecordOfEps& tmpResult );
    int FillAEPS( TAvailableEventPrcStategy& tAEPS );
    int ReadParamFromIni(const char *pSecName,const char*pParamName,int&iParamValue);
    int ReadParamFromIni(const char *pSecName,const char*pParamName,string &strParamValue);
    string PrintCdrResult(const TCallDetailResultRecord &tCallDetailResultRecord);
    //////////////////////////////////////////////////////////////////////////
    TRT_MOD m_iMod;
    int m_iEventTypeId;
    TTbInfoManager *m_pTbInfoManager;
    CReadIni *m_pReadIni;
    //CdrEvent不要直接使用，采用工厂模式来用
    TCdrEvent *m_pCdrEvent;
    //批价使用的组件
    // 累积量ID
    TUniqueID *m_pclAccumSeq; 
    // 累积量初始化
    TInitAccumulation *m_pclInitAccumulation;
    // 批价数据预处理
    TRateRefValuePreCombine *m_pclRateRefValPreCom;    
};

#endif
