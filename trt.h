/*
*��    ��:none
*�� �� ��:none
*��    ��:���۳���
*�������:none
*�������:none
*�� �� ֵ:none
*�޸ļ�¼:
�޸�����      �޸���            �޸�����
----------------------------------------
2005-09-14    xxxxxx            ��������
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
    //���Է�ʽ
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
    // ����������ݷ���
    int FillEpsData(vector<TRateResultRecordOfEps> &VcAllRateResult);
    TCdrEvent * CreateCdrEventIns();
    int SetEventAttr(TCdrEvent &clCdrEvent);
    int SetOwnerPlus(TCdrEvent &clCdrEvent);
    int InitTestData();
    int FillOfferInfoVec(vector<TOfferInfo> &vOfferInfo);
    int OutputOfferInfoVec(vector<TOfferInfo> &vOfferInfo);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //���߷���
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
    //CdrEvent��Ҫֱ��ʹ�ã����ù���ģʽ����
    TCdrEvent *m_pCdrEvent;
    //����ʹ�õ����
    // �ۻ���ID
    TUniqueID *m_pclAccumSeq; 
    // �ۻ�����ʼ��
    TInitAccumulation *m_pclInitAccumulation;
    // ��������Ԥ����
    TRateRefValuePreCombine *m_pclRateRefValPreCom;    
};

#endif
