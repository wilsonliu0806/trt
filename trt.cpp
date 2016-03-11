#include "trt.h"
#include "mdbcust/shmcust_pub.h"
#include "CAgentProgDealLog.h"
#include "hbcommon/hbconfig.h"
#include "pluginpacket/PlugInExternProperty.h"
#include "pricing/InitAccumulation.h"
bool gbRunState = true;

using namespace hbcommon;
//文件锁，防止重复起多个进程


class TRatingCompOffer
{
public:

    bool operator()(const TOfferInfo &lh, const TOfferInfo &rh)
    {
        //ur 193425 【湖南省份集团需求】4G流量包重复订购批价优先级判断 by liu.wei

        //1.按销售品优先级排序；       
        if (lh.lOfferPriority != rh.lOfferPriority)
        {
            return lh.lOfferPriority < rh.lOfferPriority;
        }
        //2.如果销售品优先级一样，则按销售品实例生效时间，生效时间早，优先级高；
        if (lh.tmPOIEffDate != rh.tmPOIEffDate)
        {
            return lh.tmPOIEffDate < rh.tmPOIEffDate;
        }
        //3.如果售品实例生效时间一样的，按失效时间排序，失效时间早的，优先级高；
        if (lh.tmExpDate != rh.tmExpDate)
        {
            return lh.tmExpDate < rh.tmExpDate;
        }
        //4.如果失效时间一样的，则按销售品实例ID为优先级，ID大的优先级高；
        if (lh.lOfferInstanceID != rh.lOfferInstanceID)
        {
            return lh.lOfferInstanceID > rh.lOfferInstanceID;
        }

        return false;
    }
};;
CTrtApp::CTrtApp()
{

    //初始化程序
    string AppFileName  = APP_DIR;
    AppFileName        += APP_CFG;
    
    int iRet = CAppComponent::Init(AppFileName.c_str());
    if(iRet != 0)
    {
        printf("CAppComponent::Init failed.\n");
        throw;
    }

    m_pTbInfoManager = NULL;
    m_pclRateRefValPreCom=NULL;
    m_pclAccumSeq=NULL;
    m_pclInitAccumulation=NULL;
}
/*
 *类    名:CTrtApp
 *函 数 名:PrintUsage
 *功    能:批价帮助信息
 *输入参数:none
 *输出参数:none
 *返 回 值:none
 *修改记录:
  修改日期      修改人            修改内容
  ----------------------------------------
  2005-09-14    xxxxxx            方法生成
 */
void CTrtApp::PrintUsage()
{
    cout << "命令参考                           " << endl;
    cout << " 1.定价类型查找 参数为事件类型"<<endl;
	cout << "  trt -i 1" << endl;
    cout << " 2.CONVERT_RATING_RESULT测试，无参数"<<endl;
	cout << "  trt -c" << endl;
    cout << " 3.外部属性测试，无参数"<<endl;
    cout << "  trt -e" << endl;
    cout << " 4.定价计划测试，无参数"<<endl;
    cout << "  trt -p" << endl;
    cout << " 5 订购销售品 , 无参数"<<endl;
    cout << "  trt -b" << endl;
    cout << " 6 话单批价 , 无参数"<<endl;
    cout << "  trt -r" << endl;
    cout << endl;
    cout << "End" << endl;

    return;
}

/*
 *类    名:CTrtApp
 *函 数 名:InitParam
 *功    能:初始化参数
 *输入参数:1 - TCdrRateExecInfo &tCdrRateExecInfo
 *输出参数:none
 *返 回 值:1 - true 取运行参数成功
           2 - false 取运行参数失败
 *修改记录:
  修改日期      修改人            修改内容
  ----------------------------------------
  2005-09-14    xxxxxx            方法生成
 */
bool CTrtApp::InitParam(int argc, char *argv[])
{
    
    if(argc==1)
    {
        //无任何参数： 打印命令行帮助信息
        return false;
    }

    int opt;
    while((opt=getopt(argc,argv,"pbhrcei:"))!=EOF)
    {
        switch(opt)
        {
        case 'i':
            {
                // 因为有好多地方用到这个标志，用这个转换的类设置这个标志。
                m_iEventTypeId = atoi(optarg);
                m_iMod = TRT_MOD_SERCH_PRICING_DATA;
                break;
            }
        case 'c':
            {
                m_iMod = TRT_MOD_CONVERT_RATING_RESULT;
                break;
            }
        case 'e':
            {
                m_iMod = TRT_MOD_EXTERN_PROERTY;
                break;
            }
        case 'p':
            {
                m_iMod = TRT_MOD_PRICING_PLAN;
                break;
            }
        case 'b':
            {
                m_iMod = TRT_MOD_BUY_OFFER;
                break;
            }
        case 'h':
            {
                return false;
            }
        case 'r':
            {
                m_iMod = TRT_MOD_CDR_RATING;
                break;
            }
        default:
            {
                return false;
            }
        }
    }
    return true;
}



int CTrtApp::Run()
{

    try 
    {
        cout << "//////////////////////////////////////////////////////////////////////////" << endl;
        cout << "//                                                                      //" << endl;
        cout << "//                     Prodigy CdrRating Program Start                  //" << endl;
        cout << "//                                                                      //" << endl;
        cout << "//////////////////////////////////////////////////////////////////////////" << endl;

        int iRet = 0;
        //初始化程序
        string AppFileName  = APP_DIR;
        AppFileName        += APP_CFG;

        CDBConfig tDBConfig(AppFileName.c_str() , "COMMON");

        //连接数据库

        ConnectDB(tDBConfig.szProdUserName,tDBConfig.szProdPassWord,tDBConfig.szProdDsName,DBPROD);
        //ConnectDB(tDBConfig.szAcctUserName,tDBConfig.szAcctPassWord,tDBConfig.szAcctDsName,DBACCT);
        ConnectDB(tDBConfig.szBillUserName,tDBConfig.szBillPassWord,tDBConfig.szBillDsName,DBBILL);
        ConnectDB(tDBConfig.szCustUserName,tDBConfig.szCustPassWord,tDBConfig.szCustDsName,DBCUST);
		///< begin mod by sxm for 112405
        //ConnectDB(tDBConfig.szCashBillUserName,tDBConfig.szCashBillPassWord,tDBConfig.szCashBillDsName,DBCASH_BILL);
        ///< end mod by sxm for 112405
#ifdef __RATING_TTSUPPORT
        ConnectDB(tDBConfig.szTTUserName,tDBConfig.szTTPassWord,tDBConfig.szTTDsName,DBTT);
#elif defined __RATING_MDBSUPPORT 
        ConnectDB(tDBConfig.szMdbDataUserName,tDBConfig.szMdbDataPassWord,tDBConfig.szMdbDataDsName,DBMDBDATA);
#endif  
        //初始化共享内存连接

        iRet = CShmcustManager::Init(this);
        if (0 != iRet)
        {
            return ERROR_SHMTABLE;
        }

        if (0 != CShmdataManager::Init(this))
        {
            return ERROR_SHMTABLE;
        }             

        //初始化pricing内存
        m_pTbInfoManager = new TTbInfoManager(this);

        if(m_pTbInfoManager->LoadTbInfo() == false)
        {
            cout << "clTbInfoMan.LoadTbInfo() Error !" << endl;
            return ERROR_PRICETABLE;
        }
        else
        {
            cout << "clTbInfoMan.LoadTbInfo() Message : Init ...... OK"  << endl;
        }

        InitTestData();

        switch(m_iMod)
        {
        case TRT_MOD_SERCH_PRICING_DATA:
            RunSearchUnitTest(m_iEventTypeId);
            break;
        case TRT_MOD_CONVERT_RATING_RESULT:
            RunOfferFee();
            break;
        case TRT_MOD_EXTERN_PROERTY:
            RunExternPorperty();
            break;
        case TRT_MOD_PRICING_PLAN:
            RunPricingPlan();
            break;
        case TRT_MOD_BUY_OFFER:
            RunBuyProductOffer();
            break;
        case TRT_MOD_CDR_RATING:
            TestCdrRating();
            break;
        default:
            RunRatableResourceUpdate();
        }
              
    }
    catch (TDBException &ex)
    {
        cout << "[App message:] DB exception ! #error_msg:[" << ex.GetErrMsg() << "]" << "#error_sql:[" \
            << ex.GetErrSql() << "]" << endl;
    }
    catch (TException &ex)
    {
        cout << "[App message:] App exception ! #error_msg:[" << ex.GetErrMsg() << "]" << endl;
    }
    catch (std::exception &e)
    {
        cout << "[App message:] std exception ! #error_msg:[" << e.what() << "]" << endl;
    }
    catch (...)
    {
        cout<<"Unkonw Exception"<<endl;
    }

    cout << "//////////////////////////////////////////////////////////////////////////" << endl;
    cout << "//                                                                      //" << endl;
    cout << "//                     Prodigy CdrRating Program End                    //" << endl;
    cout << "//                                                                      //" << endl;
    cout << "//////////////////////////////////////////////////////////////////////////" << endl;

    return 0;
}

int CTrtApp::RunSearchUnitTest( int iEventTypeId )
{
    int iRet = 0;
    ResultPointer<TEventPricingStrategy> pEventPricingStrategy;
    ResultVector<TPricingCombine*> vPricingCombine;
    set<long> stPlan;
    set<long>::iterator pPlanD=stPlan.begin();

    int iNullProductOffer = 0;
    int iNullInstance = 0;
    int iNullObjectInstance = 0;
    int iGetRecordCount = 0;

    m_pTbInfoManager->pEventPricingStrategyInfo->Scan();
    
    while (true)
    {
        pEventPricingStrategy = m_pTbInfoManager->pEventPricingStrategyInfo->Next();
        if (!pEventPricingStrategy)
        {
            NG_LOG(LOG_NORMAL,"Scan Event Pricing Strategy End!");
            break;
        }
        static int tmpCount = 0;

        tmpCount++;

        if (tmpCount%10000 == 0)
        {
            NG_LOG(LOG_NORMAL,"Scan %d Event Pricing Strategy records!\n",tmpCount);
        }
        if (strcmp(pEventPricingStrategy->state,"00A")!=0)
        {
            continue;
        }

        if (pEventPricingStrategy->event_type_id!=iEventTypeId)
        {
            continue;
        }
        
        m_pTbInfoManager->pPricingCombineInfo->FindByEventPricingStrategyID(pEventPricingStrategy->event_pricing_strategy_id,vPricingCombine);
        if(vPricingCombine.empty())
        {
            continue;
        }

        for(vector<TPricingCombine *>::iterator itor_comb=vPricingCombine.begin(); itor_comb!=vPricingCombine.end(); itor_comb++)
        {
            ResultPointer<TPricingPlan> pPricingPlan = m_pTbInfoManager->pPricingPlanInfo->FindByIDTime((*itor_comb)->pricing_plan_id, time(NULL), time(NULL));
            if(pPricingPlan == NULL)
            {
                //return ERR_IMPORT_MAN_FIND_PRICING_PLAN;
                continue;
            }
            //如果是子定价计划，需要找到父定价计划.
            if (strcmp(pPricingPlan->type_type, PRICING_PLAN_TYPE_PARENT) != 0)
            {
                //如果是父定价计划--父子定价定价计划上都可能存在定价组合,父定价计划上可能不存在.
                ResultVector<TPricingPlanRelation *> vResult;
                // 根据父定价计划ID查找子定价计划
                m_pTbInfoManager->pPricingPlanRelationInfo->FindByZIDTime(pPricingPlan->pricing_plan_id,
                    time(NULL),
                    vResult);
                if (!vResult.empty())
                {
                    // 对结果按照优先级排序
                    vector<TPricingPlanRelation *>::iterator iter_tmp;
                    for (iter_tmp=vResult.begin(); iter_tmp!=vResult.end(); ++iter_tmp)
                    {           
                        //过滤非10C和版本号不同的
                        if(strcmp((*iter_tmp)->relation_type_id,PRICING_PLAN_RELATION_TYPE_PACKAGE)!=0)
                        {
                            continue;
                        }

                        if((*iter_tmp)->plan_z_version_id != pPricingPlan->version_id)
                        {
                            continue;
                        }

                        ResultPointer<TPricingPlan> ptZPlan = m_pTbInfoManager->pPricingPlanInfo->FindByIDTime((*iter_tmp)->plan_a_id, time(NULL));
                        if (ptZPlan == NULL)
                        {
                            // 如果不在则忽略.
                            continue;
                        }
                        else
                        {
                            // 直接从父定价计划复制,然后替换定价计划ID,此容器的循序是先按子定价计划，后按父定价计划

                            //记录子定价计划,并查找到 offer_object_id 
                            vector<TOfferPricingObjectRelation> vOfferPricingObjectRelation;
                            m_pTbInfoManager->pOfferPricingObjectRelationInfo->FindRelationByPlan(pPricingPlan->pricing_plan_id,
                                pPricingPlan->version_id,
                                ptZPlan->pricing_plan_id,
                                vOfferPricingObjectRelation);
                            vector<TOfferPricingObjectRelation>::iterator pRelationCycle=vOfferPricingObjectRelation.begin();

                            pPlanD=stPlan.find(pPricingPlan->pricing_plan_id);
                            if(pPlanD==stPlan.end())
                            {
                                //sPlanResult.insert(tPlanResult);
                                stPlan.insert(pPricingPlan->pricing_plan_id);
                                if (stPlan.size()%100 == 0)
                                {
                                    NG_LOG(LOG_DEBUG,"get pricing_plan %d\n",stPlan.size());
                                }
                            }
                        }
                    }//for (iter_tmp=vResult.begin(); iter_tmp!=vResult.end(); ++iter_tmp)
                }   //if (!vResult.empty())   
                else
                {
                    //没找到副定价计划.(新疆对于这种情况过滤)
                    //TPlanResult tPlanResult;
                    //tPlanResult.pPricingPlan      = pPricingPlan;
                    //tPlanResult.pPricingRefObject = itor->pPricingRefObject;
                    //tPlanResult.vBillingCycleId   = itor->vBillingCycleId;
                    //tPlanResult.iParentPlanID     = -1;
                    //
                    //pPlanD=stPlan.find(pPricingPlan->pricing_plan_id);
                    //if(pPlanD==stPlan.end())
                    //{
                    //    sPlanResult.insert(tPlanResult);
                    //    stPlan.insert(pPricingPlan->pricing_plan_id);
                    //}
                }                                                                   
            }//if (strcmp(pPricingPlan->type_type, PRICING_PLAN_TYPE_PARENT) == 0)
            else
            { 
                //是父定价计划.
                //记录子定价计划,并查找到 offer_object_id 
                vector<TOfferPricingObjectRelation> vOfferPricingObjectRelation;
                m_pTbInfoManager->pOfferPricingObjectRelationInfo->FindRelationByPlan(pPricingPlan->pricing_plan_id,
                    pPricingPlan->version_id,
                    pPricingPlan->pricing_plan_id,
                    vOfferPricingObjectRelation);
                vector<TOfferPricingObjectRelation>::iterator pRelationCycle=vOfferPricingObjectRelation.begin();
                for(;pRelationCycle!=vOfferPricingObjectRelation.end();pRelationCycle++)
                {
                    cout<<"fu定价计划找到的offer_object："<<pRelationCycle->offer_object_id<<endl;

                }   

                stPlan.insert(pPricingPlan->pricing_plan_id);
                if (stPlan.size()%100 == 0)
                {
                    NG_LOG(LOG_DEBUG,"get pricing_plan %d\n",stPlan.size());
                }

            }
        }

        pEventPricingStrategy = m_pTbInfoManager->pEventPricingStrategyInfo->Next();
    }

    //debug
    NG_LOG(LOG_NORMAL,"get pricing_plan:size=%d\n",stPlan.size());
    //根据定价计划回找
    bool bFound = false;
    ResultVector<TProductOffer*> vProductOffer;
    for (set<long>::iterator itr = stPlan.begin();itr!=stPlan.end()
        ;itr++)
    {
        static int i = 0;
        bFound = false;
        i++;
        NG_LOG(LOG_NORMAL,"Scan pricing plan %d / %d-----id = [%lld]",i,stPlan.size(),*itr);
        vProductOffer.clear();
        m_pTbInfoManager->pProductOfferInfo->FindByPricingPlanID(*itr,vProductOffer);
        if (vProductOffer.empty())
        {
            iNullProductOffer++;
            continue;
        }

        //
        for (vector<TProductOffer*>::iterator itrPO = vProductOffer.begin();
            itrPO!=vProductOffer.end();itrPO++)
        {
            int iRet = m_pTbInfoManager->pProductOfferInstanceInfo->FindByProductOfferId((*itrPO)->offer_id);
            if (iRet!=0)
            {
                iNullInstance++;
                continue;
            }


            while(!m_pTbInfoManager->pProductOfferInstanceInfo->Next())
            {
                iRet = m_pTbInfoManager->pProductOfferObjectInstanceInfo->FindByProductOfferInstanceId(m_pTbInfoManager->pProductOfferInstanceInfo->pData->product_offer_instance_id);          
                if (iRet!=0)
                {
                    iNullObjectInstance++;
                    continue;
                }

                if(!m_pTbInfoManager->pProductOfferObjectInstanceInfo->Next())
                {
                    NG_LOG(LOG_NORMAL,"Product Offer Object Instance [%lld] product_offer [%lld]\n",m_pTbInfoManager->pProductOfferObjectInstanceInfo->pData->product_offer_instance_id,
                        (*itrPO)->offer_id);
                    bFound = true;
                    break;
                }
            }

        }

    }

    NG_LOG(LOG_NORMAL,"\n%lld no product_offer\n%lld no product_offer_instance\n%lld no object_instance\n %lld records",iNullProductOffer,iNullInstance,
        iNullObjectInstance,iGetRecordCount);
    return iRet;
}

int CTrtApp::RunRatableResourceUpdate()
{

    int iRet = 0;
#if 0
    m_pBillLog->Log(LOG_NORMAL, 0, "Application start  ......\n");   
    //开始测试
    TRatableResourceIncrementInfo tInfo;
    tInfo.tRatableResourceAccumulator.new_rec = true;
    tInfo.tRatableResourceAccumulator.cust_category = 999;
    tInfo.tRatableResourceAccumulator.ratable_resource_id = 1748;
    tInfo.tRatableResourceAccumulator.if_suite_resource[0] = '1'; 
    tInfo.tRatableResourceAccumulator.ratable_resource_accum_id = 100000000002;
    tInfo.tRatableResourceAccumulator.product_offer_instance_id = 600392606108;
    strcpy(tInfo.tRatableResourceAccumulator.owner_type,"80C");
    tInfo.tRatableResourceAccumulator.billing_cycle_id = 191312;
    //TRatableResourceOperator::Instance(this,RR_OP_DB_DB)->Query(100000000001,tInfo.tRatableResourceAccumulator); 
    TRatableResourceOperator::Instance(this,RR_OP_DB_DB)->Update(tInfo.tRatableResourceAccumulator);
    TRatableResourceOperator::Instance()->SetBillingCycleId(191312);
    TRatableResourceOperator::Instance()->SetTbInfoManager(m_pTbInfoManager);
    TRatableResourceOperator::Instance()->Commit(NULL,true);
    this->m_pDBMdbData->Commit();
    m_pBillLog->Log(LOG_NORMAL, 0, "Application end ......\n"); 
#endif
    return iRet;
}

int CTrtApp::RunOfferFee()
{
    int iRet = 0;
    TCallDetailRecord clCdrRecord;
    TCallDetailResultRecord clCallDetailResultRecord;
    TCdrEvent clCdrEvent(this, &clCallDetailResultRecord, &clCdrRecord, m_pTbInfoManager);
    vector<TRateResultRecordOfEps> VcAllRateResult;
    iRet = SetEventAttr(clCdrEvent);
    if (iRet!=0)
    {
        NG_ERROR(WARNING_FATAL,iRet,"SetEventAttr Failed！\n");
        return iRet;
    }
    iRet = SetOwnerPlus(clCdrEvent);
    if (iRet!=0)
    {
        NG_ERROR(WARNING_FATAL,iRet,"SetOwnerPlus Failed！\n");
        return iRet;
    }
    iRet = FillEpsData(VcAllRateResult);
    if (iRet!=0)
    {
        NG_ERROR(WARNING_FATAL,iRet,"SetEventAttr Failed！\n");
        return iRet;
    }
    clCdrEvent.ConvertResultRecord(VcAllRateResult);
    return iRet;
}

int CTrtApp::SetEventAttr( TCdrEvent &clCdrEvent )
{
    int iRet = 0;

    char szcdr_content[1024]={0};
    char szcdr_head[1024]={0};
    m_pReadIni->ReadString("RATING","CDR_HEAD",szcdr_head,"");
    NG_LOG(LOG_NORMAL,"--cdr_head[%s]\n",szcdr_head);
    m_pReadIni->ReadString("RATING","CDR_CONTENT",szcdr_content,"");
    NG_LOG(LOG_NORMAL,"--cdr_content[%s]\n",szcdr_content);

    vector<string> tmpvec_idstring;
    tmpvec_idstring.clear();
    GetStringBySeparater('|',szcdr_head,tmpvec_idstring);

    vector<string> tmpvec_valuestring;
    tmpvec_valuestring.clear();
    GetStringBySeparater('|',szcdr_content,tmpvec_valuestring);

    vector<string>::iterator itr_id = tmpvec_idstring.begin(),itr_value = tmpvec_valuestring.begin();
    for (;itr_id!=tmpvec_idstring.end()
        ;itr_id++,itr_value++)
    {
        NG_LOG(LOG_DEBUG,"Get Config Attr [%s=%s]",(*itr_id).c_str(),(*itr_value).c_str());
        clCdrEvent.SetAttr(atoi((*itr_id).c_str()),(*itr_value).c_str());
    }

    return iRet;
}

int CTrtApp::GetStringBySeparater( const char cSeparater,const char * pszString,vector<string>& vecString )
{
    int iRet = 0;
    NG_LOG(LOG_NORMAL,"ParamString %s\n",pszString);
    string tInputString = pszString;
    std::string::size_type PrePos = 0;
    std::string::size_type Pos = 0;

    if ((Pos = tInputString.find(cSeparater,Pos))== std::string::npos)
    {
        vecString.push_back(tInputString);
    }
    else
    {
        while ((Pos = tInputString.find(cSeparater,Pos))!= std::string::npos)
        {
            string tstrSub = tInputString.substr(PrePos,Pos-PrePos);
            NG_LOG(LOG_NORMAL,"ParamString:SubString %s\n",tstrSub.c_str());
            vecString.push_back(tstrSub);
            Pos++;
            PrePos = Pos;
        }

        string tstrSub = tInputString.substr(PrePos,Pos-PrePos);
        NG_LOG(LOG_NORMAL,"Last ParamString:SubString %s\n",tstrSub.c_str());
        vecString.push_back(tstrSub);
    }

    return iRet;
}

int CTrtApp::FillEpsData( vector<TRateResultRecordOfEps> &VcAllRateResult )
{
    int iRet = 0;
    TAvailableEventPrcStategy tAEPS;
    iRet = FillAEPS(tAEPS);
    if (iRet!=0)
    {
        NG_ERROR(WARNING_FATAL,iRet,"FillAEPS Failed!\n");
        return iRet;
    }

    TRateResultRecordOfEps tmpResult;
    iRet = FillEPSRlt(tAEPS,tmpResult);
    if (iRet!=0)
    {
        NG_ERROR(WARNING_FATAL,iRet,"FillEPSRlt Failed!\n");
        return iRet;
    }

    VcAllRateResult.push_back(tmpResult);
    return iRet;
}

int CTrtApp::FillEPSRlt( TAvailableEventPrcStategy& tAEPS, TRateResultRecordOfEps& tmpResult )
{  
    int iRet = 0;
    char szresource_id[32]={0};
    char szaction_id[32]={0};
    char szvalue[32]={0};
    NG_LOG(LOG_NORMAL,"Read Resource Info From TRT.ini\n");
    m_pReadIni->ReadString("RESOURCE","ID",szresource_id,"");
    NG_LOG(LOG_NORMAL,"--RESOURCE id[%s]\n",szresource_id);
    m_pReadIni->ReadString("RESOURCE","ACTION_ID",szaction_id,"");
    NG_LOG(LOG_NORMAL,"--RESOURCE action_id[%s]\n",szaction_id);
    m_pReadIni->ReadString("RESOURCE","VALUE",szvalue,"");
    NG_LOG(LOG_NORMAL,"--RESOURCE value[%s]\n",szvalue);
    NG_LOG(LOG_NORMAL,"Read Resource Info From TRT.ini End\n");
    if (strcmp(szresource_id,"")==0)
    {
        NG_ERROR(WARNING_FATAL,-1,"No Resource Config in trt.ini,please check!");
        return -1;
    }
    TRateResourceRes tResource;
#ifdef __GANSU__
    tResource.iaction_id = atoi(szaction_id) ;
#endif
    tResource.lResourceID = atol(szresource_id);
    tResource.dResourceValue = atoi(szvalue);

    tmpResult.vResource.push_back(tResource);
    return iRet;
}

int CTrtApp::FillAEPS( TAvailableEventPrcStategy &tAEPS )
{
    int iRet = 0;
    NG_LOG(LOG_NORMAL,"CTrtApp::FillAEPS NO THING!\n");
    return iRet;
}

int CTrtApp::InitTestData()
{
    int iRet = 0;
    char szIniPath[1024]={0};
    strncpy(szIniPath,getenv("HBCONFIGDIR")==NULL?"":getenv("HBCONFIGDIR"),sizeof(szIniPath));
    if (getenv("HBCONFIGDIR")==NULL)
    {
        strncpy(szIniPath,getenv("HOME"),sizeof(szIniPath));
        strcat(szIniPath,"/src/cpp/billing/application/app_trt/trt.ini");
    }
    else
    {
        strcat(szIniPath,"/trt.ini");
    }
    NG_LOG(LOG_NORMAL,"INIT ini cfg path[%s]\n",szIniPath);

    m_pReadIni = new CReadIni(szIniPath);
    if (m_pReadIni==NULL)
    {
        NG_ERROR(WARNING_FATAL,-1,"trt.ini not exists,please check!\n");
        return -1;
    }
    return iRet;
}

int CTrtApp::RunExternPorperty()
{
    int iRet = 0;
    TCallDetailRecord clCdrRecord;
    TCallDetailResultRecord clCallDetailResultRecord;
    TCdrEvent clCdrEvent(this, &clCallDetailResultRecord, &clCdrRecord, m_pTbInfoManager);
    vector<TRateResultRecordOfEps> VcAllRateResult;
    iRet = SetEventAttr(clCdrEvent);
    if (iRet!=0)
    {
        NG_ERROR(WARNING_FATAL,iRet,"SetEventAttr Failed！\n");
        return iRet;
    }
    vector<long long> vInstanceId;
    char tmpstr_product_offer_instance_id[16]={0};
    m_pReadIni->ReadString("OFFER","PRODUCT_OFFER_INSTANCE_ID",tmpstr_product_offer_instance_id,"");
    NG_LOG(LOG_NORMAL,"Get PRODUCT_OFFER_INSTANCE_ID %s\n",tmpstr_product_offer_instance_id);
    vInstanceId.push_back(atol(tmpstr_product_offer_instance_id));
    CExternPropertyHelper *pExtern = new CExternPropertyHelper(m_pTbInfoManager,&clCdrEvent,TYPE_PROD,&vInstanceId);
    TAttrPlus tAttrPlug;
    pExtern->execute(421,NBR_PARTY_NONE,&tAttrPlug,false,1,true);
    NG_LOG(LOG_NORMAL,"result %s\n",pExtern->m_bReturn?"TRUE":"FALSE");
    if (pExtern->m_bReturn)
    {
        NG_LOG(LOG_NORMAL,"result %f\n",tAttrPlug.num_val);
    }
    return iRet;
}

TCdrEvent * CTrtApp::CreateCdrEventIns()
{
    if (m_pCdrEvent != NULL)
    {
        TCallDetailRecord clCdrRecord;
        TCallDetailResultRecord clCallDetailResultRecord;
        m_pCdrEvent = new TCdrEvent(this, &clCallDetailResultRecord, &clCdrRecord, m_pTbInfoManager);
        if (m_pCdrEvent == NULL)
        {
            NG_ERROR(WARNING_FATAL,-1,"new TCdrEvent Failed!\n");
            return NULL;
        }

        SetEventAttr(*m_pCdrEvent);
    }

    return m_pCdrEvent;
}

int CTrtApp::RunPricingPlan()
{
    int iRet = 0;
#if 0
    /*
    TCallDetailRecord clCdrRecord;
    TCallDetailResultRecord clCallDetailResultRecord;
    TCdrEvent clCdrEvent(this, &clCallDetailResultRecord, &clCdrRecord, m_pTbInfoManager);
    vector<TRateResultRecordOfEps> VcAllRateResult;
    iRet = SetEventAttr(clCdrEvent);
    if (iRet!=0)
    {
        NG_ERROR(WARNING_FATAL,iRet,"SetEventAttr Failed！\n");
        return iRet;
    }

    TPricingPlanHelper *pPricingPlanHelper = new TRatePricingPlanHelper(m_pTbInfoManager, &clCdrEvent);
    vector<TAvailablePricingPlan> vAvailablePricingPlan;
    pPricingPlanHelper->execute(vAvailablePricingPlan);
    vector<TAvailablePricingPlan>::iterator itr = vAvailablePricingPlan.begin();
    for (;itr!=vAvailablePricingPlan.end()
        ;itr++)
    {
        NG_LOG(LOG_DEBUG,"Pricing plan ",itr->pricing_plan_id,itr->product_offer_instance_priority,
            itr->product_offer_instance_eff_date,itr->product_offer_instance_exp_date);
    }

    */
    vector<TOfferInfo> vOfferInfo;
    FillOfferInfoVec(vOfferInfo);
    sort(vOfferInfo.begin(), vOfferInfo.end(), TRatingCompOffer());
    OutputOfferInfoVec(vOfferInfo);
#endif
    return iRet;
}

int CTrtApp::FillOfferInfoVec( vector<TOfferInfo> &vOfferInfo )
{
    int iRet = 0;
    /**/
    int iOfferNum = 0;
    iOfferNum=m_pReadIni->ReadInteger("OFFER","TEST_NUM",0);
    for (int i = 1;i<=iOfferNum;i++)
    {
        TOfferInfo tOfferInfo;
        char szOffer[16]={0};
        snprintf(szOffer,sizeof(szOffer),"OFFER%d",i);
        //OfferID
        char szValue[256]={0};
        m_pReadIni->ReadString(szOffer,"OFFER_ID",szValue,"");
        tOfferInfo.lOfferID = atol(szValue);
        //OfferPriority
        memset(&szValue,0x00,sizeof(szValue));
        m_pReadIni->ReadString(szOffer,"OFFER_PRI",szValue,"");
        tOfferInfo.lOfferPriority=atol(szValue);
        //POIEffDate
        memset(&szValue,0x00,sizeof(szValue));
        m_pReadIni->ReadString(szOffer,"EFF_DATE",szValue,"");
        tOfferInfo.tmPOIEffDate=atol(szValue);
        //ExpDate
        memset(&szValue,0x00,sizeof(szValue));
        m_pReadIni->ReadString(szOffer,"EXP_DATE",szValue,"");
        tOfferInfo.tmExpDate=atol(szValue);
        //OfferInstanceID
        memset(&szValue,0x00,sizeof(szValue));
        m_pReadIni->ReadString(szOffer,"PRODUCT_OFFER_INSTANCE_ID",szValue,"");
        tOfferInfo.lOfferInstanceID=atol(szValue);

        vOfferInfo.push_back(tOfferInfo);
    }

    NG_LOG(LOG_NORMAL,"Get Cfg Offer %d",vOfferInfo.size());
    return iRet;
}

int CTrtApp::OutputOfferInfoVec( vector<TOfferInfo> &vOfferInfo )
{
    int iRet = 0;
    vector<TOfferInfo>::iterator itr = vOfferInfo.begin();
    for (;itr!=vOfferInfo.end();itr++)
    {
        NG_LOG(LOG_DEBUG,"Offer %lld[Pri %lld eff %lld exp %lld instid %lld]",itr->lOfferID,itr->lOfferPriority,itr->tmPOIEffDate,itr->tmExpDate,itr->lOfferInstanceID);
    }
    return iRet;
}

int CTrtApp::SetOwnerPlus( TCdrEvent &clCdrEvent )
{
    int iRet = 0;
    static TOwnerPlus tOwnerPlus;
    tOwnerPlus.plan_owner_obj_type = TYPE_OFFER;
    char tmpstr_product_offer_instance_id[16]={0};
    char tmpstr_pricing_plan_id[16]={0};
    //PRODUCT_OFFER_INSTANCE_ID
    m_pReadIni->ReadString("OFFER","PRODUCT_OFFER_INSTANCE_ID",tmpstr_product_offer_instance_id,"");
    NG_LOG(LOG_NORMAL,"Get PRODUCT_OFFER_INSTANCE_ID %s\n",tmpstr_product_offer_instance_id);
    tOwnerPlus.plan_owner_ins_id = atol(tmpstr_product_offer_instance_id);
    //PRICING_PLAN_ID
    m_pReadIni->ReadString("OFFER","PRICING_PLAN_ID",tmpstr_pricing_plan_id,"");
    NG_LOG(LOG_NORMAL,"Get PRICING_PLAN_ID %s\n",tmpstr_pricing_plan_id);
    tOwnerPlus.pricing_plan_id = atoi(tmpstr_pricing_plan_id);

    clCdrEvent.SetCurOwnerPlus(&tOwnerPlus);

    TRatingToolsPackage::Instance(m_pTbInfoManager)->Clear();
    TRatingToolsPackage::Instance(m_pTbInfoManager)->SetCurRatableEvent(&clCdrEvent);

    return iRet;
}

int CTrtApp::RunViewPricingPlan( int iPricingPlan )
{
    int iRet = 0;
#if 0
    m_pTbInfoManager->pPricingPlanInfo->FindByIDTime(iPricingPlan);
#endif
    return iRet;
}
#define SEL_SERV_HISTORY "select latn_id from serv_history where serv_id = :serv_id"
#define INS_PRODUCT_OFFER_INSTANCE_FIELD "PRODUCT_OFFER_INSTANCE_ID,"\
    "CUST_AGREEMENT_ID,"\
    "CUST_ID,"\
    "PRODUCT_OFFER_ID,"\
    "EFF_DATE,"\
    "EXP_DATE,"\
    "STATE,"\
    "STATE_DATE,"\
    "PRIORITY_VALUE,"\
    "CREATE_DATE,"\
    "LATN_ID,"\
    "OCS_USER_FLAG,"\
    "SEGMENT_ID"

#define INS_PRODUCT_OFFER_INSTANCE_VALUE ":PRODUCT_OFFER_INSTANCE_ID,"\
    ":CUST_AGREEMENT_ID,"\
    ":CUST_ID,"\
    ":PRODUCT_OFFER_ID,"\
    ":EFF_DATE,"\
    ":EXP_DATE,"\
    ":STATE,"\
    ":STATE_DATE,"\
    ":PRIORITY_VALUE,"\
    ":CREATE_DATE,"\
    ":LATN_ID,"\
    ":OCS_USER_FLAG,"\
    ":SEGMENT_ID"
#define INS_PRODUCT_OFFER_INSTANCE "INSERT INTO PRODUCT_OFFER_INSTANCE ("INS_PRODUCT_OFFER_INSTANCE_FIELD") VALUES ("INS_PRODUCT_OFFER_INSTANCE_VALUE")"
#define INS_PRODUCT_OFFER_INSTANCE_MDB_FIELD "PRODUCT_OFFER_INSTANCE_ID,"\
    "CUST_ID,"\
    "PRODUCT_OFFER_ID,"\
    "EFF_DATE,"\
    "EXP_DATE,"\
    "STATE,"\
    "STATE_DATE,"\
    "PRIORITY_VALUE,"\
    "CREATE_DATE,"\
    "LATN_ID,"\
    "SEGMENT_ID"
#define INS_PRODUCT_OFFER_INSTANCE_MDB_VALUE ":PRODUCT_OFFER_INSTANCE_ID,"\
    ":CUST_ID,"\
    ":PRODUCT_OFFER_ID,"\
    ":EFF_DATE,"\
    ":EXP_DATE,"\
    ":STATE,"\
    ":STATE_DATE,"\
    ":PRIORITY_VALUE,"\
    ":CREATE_DATE,"\
    ":LATN_ID,"\
    ":SEGMENT_ID"
#define INS_PRODUCT_OFFER_INSTANCE_MDB "INSERT INTO PRODUCT_OFFER_INSTANCE ("INS_PRODUCT_OFFER_INSTANCE_MDB_FIELD") VALUES ("INS_PRODUCT_OFFER_INSTANCE_MDB_VALUE")"
                           
#define INS_PRODUCT_OFFER_OBJECT_INSTANCE_FIELD "OFFER_OBJECT_INSTANCE_ID,"\
    "PRODUCT_OFFER_INSTANCE_ID,"\
    "OBJECT_ID,"\
    "EFF_DATE,"\
    "EXP_DATE,"\
    "OFFER_OBJECT_ID,"\
    "SEQ_NBR,"\
    "OBJECT_TYPE,"\
    "STATE,"\
    "LATN_ID,"\
    "OCS_USER_FLAG,"\
    "AGREEMENT_ID,"\
    "SEGMENT_ID"
#define INS_PRODUCT_OFFER_OBJECT_INSTANCE_VALUE ":OFFER_OBJECT_INSTANCE_ID,"\
    ":PRODUCT_OFFER_INSTANCE_ID,"\
    ":OBJECT_ID,"\
    ":EFF_DATE,"\
    ":EXP_DATE,"\
    ":OFFER_OBJECT_ID,"\
    ":SEQ_NBR,"\
    ":OBJECT_TYPE,"\
    ":STATE,"\
    ":LATN_ID,"\
    ":OCS_USER_FLAG,"\
    ":AGREEMENT_ID,"\
    ":SEGMENT_ID"
#define INS_PRODUCT_OFFER_OBJECT_INSTANCE "INSERT INTO PRODUCT_OFFER_OBJECT_INSTANCE("INS_PRODUCT_OFFER_OBJECT_INSTANCE_FIELD") VALUES ("INS_PRODUCT_OFFER_OBJECT_INSTANCE_VALUE")"
#define INS_PRODUCT_OFFER_OBJECT_INSTANCE_FIELD_MDB "OFFER_OBJECT_INSTANCE_ID,"\
    "PRODUCT_OFFER_INSTANCE_ID,"\
    "OBJECT_ID,"\
    "EFF_DATE,"\
    "EXP_DATE,"\
    "OFFER_OBJECT_ID,"\
    "SEQ_NBR,"\
    "OBJECT_TYPE,"\
    "STATE,"\
    "NEW_FLAG"
#define INS_PRODUCT_OFFER_OBJECT_INSTANCE_VALUE_MDB ":OFFER_OBJECT_INSTANCE_ID,"\
    ":PRODUCT_OFFER_INSTANCE_ID,"\
    ":OBJECT_ID,"\
    ":EFF_DATE,"\
    ":EXP_DATE,"\
    ":OFFER_OBJECT_ID,"\
    ":SEQ_NBR,"\
    ":OBJECT_TYPE,"\
    ":STATE,"\
    ":NEW_FLAG"
#define INS_PRODUCT_OFFER_OBJECT_INSTANCE_MDB "INSERT INTO PRODUCT_OFFER_OBJECT_INSTANCE("INS_PRODUCT_OFFER_OBJECT_INSTANCE_FIELD_MDB") VALUES ("INS_PRODUCT_OFFER_OBJECT_INSTANCE_VALUE_MDB")"

int g_iLatnid = 0;
int g_iProductOfferId = 0;
int g_iOcsUserFlag = 0;
int g_iProductOfferInstanceId = 0;
int g_iSegmentID=0;
int t_iCustId = 0;
long long  g_llServId = 0;
int g_iOfferObjectId = 0;
string g_strEffDate="";
string g_strExpDate="";
int CTrtApp::RunBuyProductOffer(  )
{
    int iRet = 0;
    string t_strServId = "";
    NG_LOG(LOG_NORMAL,"RunBuyProductOffer \n");

    ReadParamFromIni("SERV","PRODUCT_OFFER_ID",g_iProductOfferId);
    ReadParamFromIni("SERV","LATN_ID",g_iLatnid);
    ReadParamFromIni("SERV","OCS_USER_FLAG",g_iOcsUserFlag);
    ReadParamFromIni("SERV","EFF_DATE",g_strEffDate);
    ReadParamFromIni("SERV","EXP_DATE",g_strExpDate);
    ReadParamFromIni("SERV","PRODUCT_OFFER_INSTANCE_ID",g_iProductOfferInstanceId);
    ReadParamFromIni("SERV","SEGMENT_ID",g_iSegmentID);
    ReadParamFromIni("SERV","SERV_ID",t_strServId);
    g_llServId = atoll(t_strServId.c_str());
    ReadParamFromIni("SERV","OFFER_OBJECT_ID",g_iOfferObjectId);

    TDBQuery tDBCust(this->m_pDBCust);
    TMdbQuery tMDBCust(this->m_pDBMdbCust);

    InsertTableProductOfferInstance(&tDBCust);

    InsertTableProductOfferInstance(&tMDBCust);

    InsertTableProductOfferObjectInstance(&tDBCust);

    InsertTableProductOfferObjectInstance(&tMDBCust);

    m_pDBCust->Commit();
    m_pDBMdbCust->Commit();

    return iRet;
}

int CTrtApp::ReadParamFromIni(const char *pSecName,const char*pParamName,int&iParamValue )
{
    iParamValue = m_pReadIni->ReadInteger(pSecName,pParamName,0);
    NG_LOG(LOG_NORMAL,"ReadParamFromIni %s %s %d\n",pSecName,pParamName,iParamValue);
}

int CTrtApp::ReadParamFromIni( const char *pSecName,const char*pParamName,string &strParamValue )
{
    char szTmpValue[512]={0};
    m_pReadIni->ReadString(pSecName,pParamName,szTmpValue,"");
    NG_LOG(LOG_NORMAL,"ReadParamFromIni %s %s %s\n",pSecName,pParamName,szTmpValue);
    strParamValue = szTmpValue;
}

int CTrtApp::InsertTableProductOfferInstance( TDBQuery *pQuery )
{

    pQuery->Close();

    pQuery->SetSQL(INS_PRODUCT_OFFER_INSTANCE);
    pQuery->SetParameter("PRODUCT_OFFER_INSTANCE_ID",g_iProductOfferInstanceId);
    pQuery->SetParameter("CUST_AGREEMENT_ID",1);
    pQuery->SetParameter("CUST_ID",t_iCustId);
    pQuery->SetParameter("PRODUCT_OFFER_ID",g_iProductOfferId);
    pQuery->SetParameter("EFF_DATE",g_strEffDate.c_str());
    pQuery->SetParameter("EXP_DATE",g_strExpDate.c_str());
    pQuery->SetParameter("STATE","00A");
    pQuery->SetParameter("STATE_DATE",TDateTimeFunc::GetCurrentTimeStr());
    pQuery->SetParameter("PRIORITY_VALUE",0);
    pQuery->SetParameter("CREATE_DATE",TDateTimeFunc::GetCurrentTimeStr());
    pQuery->SetParameter("LATN_ID",g_iLatnid);
    pQuery->SetParameter("OCS_USER_FLAG",g_iOcsUserFlag);
    pQuery->SetParameter("SEGMENT_ID",g_iSegmentID);

    if (pQuery->Execute())
    {
        NG_LOG(LOG_NORMAL,"InsertTableProductOfferInstance Success!\n");
    }
    else
    {
        NG_ERROR(WARNING_FATAL,1,"InsertTableProductOfferInstance Failed!\n");
        return -1;
    }
    
    return 0;
}

int CTrtApp::InsertTableProductOfferObjectInstance( TDBQuery *pQuery )
{
    pQuery->Close();

    pQuery->SetSQL(INS_PRODUCT_OFFER_OBJECT_INSTANCE);
    pQuery->SetParameter("OFFER_OBJECT_INSTANCE_ID",g_iProductOfferInstanceId);
    pQuery->SetParameter("PRODUCT_OFFER_INSTANCE_ID",g_iProductOfferInstanceId);
    pQuery->SetParameter("OBJECT_ID",g_llServId);
    pQuery->SetParameter("OBJECT_TYPE","80A");
    pQuery->SetParameter("OFFER_OBJECT_ID",g_iOfferObjectId);
    pQuery->SetParameter("SEQ_NBR",0);
    pQuery->SetParameter("EFF_DATE",g_strEffDate.c_str());
    pQuery->SetParameter("EXP_DATE",g_strExpDate.c_str());
    pQuery->SetParameter("STATE","00A");
    pQuery->SetParameter("LATN_ID",g_iLatnid);
    pQuery->SetParameter("OCS_USER_FLAG",g_iOcsUserFlag); 
    pQuery->SetParameter("AGREEMENT_ID",1);
    pQuery->SetParameter("SEGMENT_ID",g_iSegmentID);

    if (pQuery->Execute())
    {
        NG_LOG(LOG_NORMAL,"InsertTableProductOfferObjectInstance Success!\n");
    }
    else
    {
        NG_ERROR(WARNING_FATAL,1,"InsertTableProductOfferObjectInstance Failed!\n");
        return -1;
    }

    return 0;
}
//后续用模板技术进行修改
int CTrtApp::InsertTableProductOfferObjectInstance( TMdbQuery *pQuery )
{
    pQuery->Close();

    pQuery->SetSQL(INS_PRODUCT_OFFER_OBJECT_INSTANCE_MDB);
    pQuery->SetParameter("OFFER_OBJECT_INSTANCE_ID",g_iProductOfferInstanceId);
    pQuery->SetParameter("PRODUCT_OFFER_INSTANCE_ID",g_iProductOfferInstanceId);
    pQuery->SetParameter("OBJECT_ID",g_llServId);
    pQuery->SetParameter("OBJECT_TYPE","80A");
    pQuery->SetParameter("OFFER_OBJECT_ID",g_iOfferObjectId);
    pQuery->SetParameter("SEQ_NBR",0);
    pQuery->SetParameter("EFF_DATE",g_strEffDate.c_str());
    pQuery->SetParameter("EXP_DATE",g_strExpDate.c_str());
    pQuery->SetParameter("STATE","00A");
    pQuery->SetParameter("NEW_FLAG","0");

    if (pQuery->Execute())
    {
        NG_LOG(LOG_NORMAL,"InsertTableProductOfferObjectInstance Success!\n");
    }
    else
    {
        NG_ERROR(WARNING_FATAL,1,"InsertTableProductOfferObjectInstance Failed!\n");
        return -1;
    }

    return 0;
}

int CTrtApp::InsertTableProductOfferInstance( TMdbQuery *pQuery )
{
    pQuery->Close();
    NG_LOG(LOG_NORMAL,"MDB Insert[%s]",INS_PRODUCT_OFFER_INSTANCE_MDB);
    pQuery->SetSQL(INS_PRODUCT_OFFER_INSTANCE_MDB);
    pQuery->SetParameter("PRODUCT_OFFER_INSTANCE_ID",g_iProductOfferInstanceId);
    pQuery->SetParameter("CUST_ID",t_iCustId);
    pQuery->SetParameter("PRODUCT_OFFER_ID",g_iProductOfferId);
    pQuery->SetParameter("EFF_DATE",g_strEffDate.c_str());
    pQuery->SetParameter("EXP_DATE",g_strExpDate.c_str());
    pQuery->SetParameter("STATE","00A");
    pQuery->SetParameter("STATE_DATE",TDateTimeFunc::GetCurrentTimeStr());
    pQuery->SetParameter("PRIORITY_VALUE",0);
    pQuery->SetParameter("CREATE_DATE",TDateTimeFunc::GetCurrentTimeStr());
    pQuery->SetParameter("LATN_ID",g_iLatnid);
    pQuery->SetParameter("SEGMENT_ID",g_iSegmentID);

    if (pQuery->Execute())
    {
        NG_LOG(LOG_NORMAL,"InsertTableProductOfferInstance Success!\n");
    }
    else
    {
        NG_ERROR(WARNING_FATAL,1,"InsertTableProductOfferInstance Failed!\n");
        return -1;
    }
    
    return 0;
}

int CTrtApp::TestCdrRating()
{
    int iRet = 0;
    //构造RatableEvent 模拟预处理
    TCdrEvent *pCdrEvent = CreateCdrEventIns();
    if (pCdrEvent == NULL)
    {
        NG_ERROR(WARNING_FATAL,-1,"CreateCdrEventIns Failed!\n");
        return -1;
    }
    LSRatableResourceAccumulator tlistRatableResult;
    tlistRatableResult.clear();
    //预处理完毕，进行批价
    iRet = RunCdrRating(pCdrEvent,&tlistRatableResult);
    if (iRet!=0)
    {
        NG_ERROR(WARNING_FATAL,-1,"RunCdrRating Failed!\n");
        return -1;
    }

    return iRet;
}

int CTrtApp::RunCdrRating( TRatableEvent *pclRatableEvent,LSRatableResourceAccumulator *plsCalcResult )
{
    int iRet = ZX_SYS_NO_ERROR;
    // 批价事件信息设置
    pclRatableEvent->GetErrCode();
    pclRatableEvent->ClearResult();
    pclRatableEvent->ResetEpsAnalysisResult();
    // 情况累积量信息
    TRatableResourceOperator::Instance()->Rollback();
    //////////////////////////////////////////////////////////////////////////
    if (!m_pclAccumSeq)
    {
        m_pclAccumSeq = new (nothrow)TUniqueID(this, "RATABLE_RESOURCE_ACCUMULATOR", "RATABLE_RESOURCE_ACCUM_ID", 1000);///<modified by zhangfengli 11/12/2012 for 109816
        if (!m_pclAccumSeq)
        {
            iRet = -1;
            this->m_pBillLog->Warning(WARNING_NORMAL, iRet,
                "TRatingEngineOcsIF::Init() : Create TUniqueID instance fail : [FILE=%s, Line=%d] \n",
                __FILE__, __LINE__);
            return iRet;            
        }
    }

    // 实例化累积量初始化组件
    if (!m_pclInitAccumulation)
    {
        m_pclInitAccumulation = new (nothrow)TInitAccumulation(m_pTbInfoManager, -1);
        if (!m_pclInitAccumulation)
        {
            iRet = -1;
            this->m_pBillLog->Warning(WARNING_NORMAL, iRet, 
                "TRatingEngineOcsIF::Init() : Create Accumulator init com fail : [FILE=%s, Line=%d] \n",
                __FILE__, __LINE__);            
            return iRet;
        }
    }

    // 实例化TRateRefValuePreCombine
    if (!m_pclRateRefValPreCom)
    {
        m_pclRateRefValPreCom = new (nothrow)TRateRefValuePreCombine(m_pTbInfoManager);
        if (!m_pclRateRefValPreCom)
        {
            iRet = -1;
            this->m_pBillLog->Warning(WARNING_NORMAL, iRet, 
                "TRatingEngineOcsIF::Init() : Create TRateRefValuePreCombine instance fail : [FILE=%s, Line=%d] \n",
                __FILE__, __LINE__);
            return iRet;
        }
    }
    //////////////////////////////////////////////////////////////////////////
    // 事件批价
    ((TCdrEvent *)pclRatableEvent)->SetUnique(m_pclAccumSeq);
    ((TCdrEvent *)pclRatableEvent)->SetInitAccumu(m_pclInitAccumulation);
    m_pclRateRefValPreCom->SetCdrEvent((TCdrEvent *)pclRatableEvent);
    TRatePricingManager clRatePricingManager(m_pTbInfoManager, pclRatableEvent, m_pclRateRefValPreCom);
    if (!clRatePricingManager.execute())
    {
        iRet = -1;
        this->m_pBillLog->Warning(WARNING_NORMAL, iRet,
            "TRatingEngineOcsIF::CdrRating() : TRatePricingManager::execute error : [err_msg=%d] : [FILE=%s, Line=%d] \n",
            pclRatableEvent->GetErrCode(), __FILE__, __LINE__);
        return iRet;
    } 

    TRatableResourceOperator::Instance()->Commit(plsCalcResult);

    NG_LOG(LOG_NORMAL,"%s",PrintCdrResult(m_pRatableEvent->GetRateResult()).c_str());
}

string CTrtApp::PrintCdrResult( const TCallDetailResultRecord &tCallDetailResultRecord )
{
    int iVsize = 0;
    ostringstream sAddText;

    sAddText << "=================TCallDetailResultRecord===================" << endl;
    sAddText << " total_charge         = " << tCallDetailResultRecord.total_charge << endl;
    sAddText << " total_old_charge     = " << tCallDetailResultRecord.total_old_charge << endl;
    sAddText << " event_inst_id        = " << tCallDetailResultRecord.event_inst_id << endl;
    sAddText << " billing_cycle_id     = " << tCallDetailResultRecord.billing_cycle_id << endl;
    sAddText << " region_id            = " << tCallDetailResultRecord.region_id << endl;
    sAddText << " file_id              = " << tCallDetailResultRecord.file_id << endl;
    sAddText << " duration             = " << tCallDetailResultRecord.duration << endl;
    sAddText << " llCalcRemainDuration = " << tCallDetailResultRecord.llCalcRemainDuration << endl;
    //sAddText<<" pricing_plan_id      = "<<tCallDetailResultRecord.pricing_plan_id<<endl;
    sAddText << " measure_method_type  = " << tCallDetailResultRecord.measure_method_type << endl;
    sAddText << " created_date         = " << tCallDetailResultRecord.created_date << endl;
    sAddText << " dealed_date          = " << tCallDetailResultRecord.dealed_date << endl;

    iVsize = tCallDetailResultRecord.vFeeDetail.size();
    sAddText << " vFeeDetail.size      = " << iVsize << endl;
    for ( int i = 0; i < iVsize; i++ )
    {
        sAddText << "     --vFeeDetail[" << i << "]" << endl;
        const TFeeDetail *po = &( tCallDetailResultRecord.vFeeDetail[i] );

        sAddText << "     old_charge                = " << po->llold_charge << endl;
        sAddText << "     charge                    = " << po->llcharge << endl;
        sAddText << "     tariff                    = " << po->tariff << endl;
        sAddText << "     tariff_id                 = " << po->tariff_id << endl;
        sAddText << "     rate_duration             = " << po->rate_duration << endl;
        sAddText << "     event_pricing_strategy_id = " << po->event_pricing_strategy_id << endl;
        sAddText << "     pricing_plan_id           = " << po->pricing_plan_id << endl;
        sAddText << "     acct_item_type_id         = " << po->acct_item_type_id << endl;
        sAddText << "     measure_method_type       = " << po->measure_method_type << endl;
        sAddText << "     bHaveRemain               = " << ( po->bHaveRemain ? "TRUE" : "FALSE" ) << endl;
        //sAddText<<"     bGetMin                   = "<<po->bGetMin<<endl;
        sAddText << "     plan_owner_inst_id         = " << po->plan_owner_inst_id << endl;
    }

    iVsize = 0;
    iVsize = tCallDetailResultRecord.vResourceDetail.size();
    sAddText << " vResourceDetail.size      = " << iVsize << endl;
    for ( int i = 0; i < iVsize; i++ )
    {
        sAddText << "     --vResourceDetail[" << i << "]" << endl;
        const TResourceDetail *po = &( tCallDetailResultRecord.vResourceDetail[i] );

        sAddText << "     ratable_resource_accumulator_id        = " << po->ratable_resource_accumulator_id << endl;
        sAddText << "     ratable_resource_id                    = " << po->ratable_resource_id << endl;
        sAddText << "     update_value                           = " << po->update_value << endl;
    }

    return sAddText.str();
}

static void signal_func(int signo)
{
    printf("\nrating received signal:%d.\n",signo);
    signal(SIGINT,  signal_func);
    signal(SIGTERM, signal_func);

    gbRunState = false;
    return;
}

/*
 *类    名:CTrtApp
 *函 数 名:main
 *功    能:批价main函数
 *输入参数:
 *输出参数:
 *返 回 值:
 *修改记录:
  修改日期      修改人            修改内容
  ----------------------------------------
  2005-09-14    xxxxxx            方法生成
 */
int main(int argc, char *argv[])
{
    int iRet = 0;
    signal(SIGINT,  signal_func);
    signal(SIGTERM, signal_func);
    try
    {
        __AUTO_NGLOG__(LOG_TYPE_APP, NULL, NULL);

        CTrtApp tCdrRatingApp;

        if (tCdrRatingApp.InitParam(argc, argv) == false)
        {
            tCdrRatingApp.PrintUsage();
            return ERROR_PARAMS;
        }

        iRet = tCdrRatingApp.Run();
        if(iRet != 0 )
        {
            printf(__WHEREFORMAT__ "tCdrRatingApp.Run() failed\n",__WHERE__);
            return iRet;
        }
    }
    catch (TDBException &ex)
    {
        cout << "[App message:] DB exception ! #error_msg:[" << ex.GetErrMsg() << "]" << "#error_sql:[" \
            << ex.GetErrSql() << "]" << endl;
    }
    catch (TException &ex)
    {
        cout << "[App message:] App exception ! #error_msg:[" << ex.GetErrMsg() << "]" << endl;
    }
    catch (std::exception &e)
    {
        cout << "[App message:] std exception ! #error_msg:[" << e.what() << "]" << endl;
    }
    catch (...)
    {
        cout<<"Unkonw Exception"<<endl;
    }
    
}
