#include "malpha.h"

MWorkMat::MWorkMat(JMat* pic,JMat* msk,const int* boxs){
    m_boxx = boxs[0];
    m_boxy=boxs[1];
    m_boxwidth=boxs[2]-m_boxx;
    m_boxheight=boxs[3]-m_boxy;
    //printf("x %d y %d w %d h %d \n",m_boxx,m_boxy,m_boxwidth,m_boxheight);
    m_pic = pic;
    m_msk = msk;

    pic_real160 = new JMat(160,160,3,0,1);
    pic_mask160 = new JMat(160,160,3,0,1);
    //pic_crop160 = new JMat(160,160,3,0,1);

    msk_real160 = new JMat(160,160,1,0,1);

    //msk_mask160 = new JMat(160,160,3,0,1);

}

MWorkMat::~MWorkMat(){
    matpic_org168.release();
    matpic_roirst.release();
    delete pic_real160;
    delete pic_mask160;
    delete msk_real160;
    if(pic_clone160) delete pic_clone160;
}

int MWorkMat::munet(JMat** ppic,JMat** pmsk){
    *ppic = pic_real160;
    *pmsk = pic_mask160;
    return 0;
}

int MWorkMat::premunet(){
    matpic_roisrc = cv::Mat(m_pic->cvmat(),cv::Rect(m_boxx,m_boxy,m_boxwidth,m_boxheight));
    cv::resize(matpic_roisrc , matpic_org168, cv::Size(168, 168), cv::INTER_AREA);
    //vtacc
    matpic_roi160 = cv::Mat(matpic_org168,cv::Rect(4,4,160,160));
    cv::Mat cvmask = pic_mask160->cvmat();
    cv::Mat cvreal = pic_real160->cvmat();
    matpic_roi160.copyTo(cvmask);
    matpic_roi160.copyTo(cvreal);
    //cv::rectangle(cvmask,cv::Rect(5,5,150,150),cv::Scalar(0,0,0),-1);//,cv::LineTypes::FILLED);
    cv::rectangle(cvmask,cv::Rect(5,5,150,145),cv::Scalar(0,0,0),-1);//,cv::LineTypes::FILLED);
    //cv::rectangle(cvmask,cv::Rect(4,4,152,152),cv::Scalar(0,0,0),-1);//,cv::LineTypes::FILLED);
    //cv::imwrite("cvmask.bmp",cvmask);
    //cv::waitKey(0);
    pic_clone160 = pic_real160->refclone(0);
    return 0;
}

int MWorkMat::finmunet(JMat* fgpic){
    cv::Mat cvreal = pic_real160->cvmat();

        //for(int k=0;k<16;k++){
            //cv::line(cvreal,cv::Point(0,k*10),cv::Point(160,k*10),cv::Scalar(0,255,0));
        //}
        //for(int k=0;k<16;k++){
            //cv::line(cvreal,cv::Point(k*10,0),cv::Point(k*10,160),cv::Scalar(0,255,0));
        //}
    cvreal.copyTo(matpic_roi160);
    //cv::imwrite("accpre.bmp",matpic_org168);
    if(m_msk) vtacc((uint8_t*)matpic_org168.data,168*168);
    //cv::imwrite("accend.bmp",matpic_org168);
    if(fgpic&&(fgpic->width()==168)){
      std::vector<cv::Mat> list;
      cv::split(matpic_org168,list);
      matmsk_roisrc = cv::Mat(m_msk->cvmat(),cv::Rect(m_boxx,m_boxy,m_boxwidth,m_boxheight));
      cv::resize(matmsk_roisrc , matmsk_org168, cv::Size(168, 168), cv::INTER_AREA);
      cv::Mat rrr(168,168,CV_8UC1);
      cv::cvtColor(matmsk_org168,rrr,cv::COLOR_RGB2GRAY);
      list.push_back(rrr);
      cv::merge(list,fgpic->cvmat());
    }else{
      cv::resize(matpic_org168, matpic_roirst, cv::Size(m_boxwidth, m_boxheight), cv::INTER_AREA);
      if(fgpic){
        matpic_roisrc = cv::Mat(fgpic->cvmat(),cv::Rect(m_boxx,m_boxy,m_boxwidth,m_boxheight));
        matpic_roirst.copyTo(matpic_roisrc);
      }else{
        matpic_roirst.copyTo(matpic_roisrc);
      }
    }
    return 0;
}

int MWorkMat::alpha(JMat** preal,JMat** pimg,JMat** pmsk){
    *preal = pic_clone160;
    *pimg =  pic_real160;
    *pmsk =  msk_real160;
    return 0;
}

int MWorkMat::prealpha(){
    printf("x %d y %d w %d h %d \n",m_boxx,m_boxy,m_boxwidth,m_boxheight);
    //m_msk->show("cba");
    //cv::waitKey(0);
    matmsk_roisrc = cv::Mat(m_msk->cvmat(),cv::Rect(m_boxx,m_boxy,m_boxwidth,m_boxheight));
    cv::resize(matmsk_roisrc , matmsk_org168, cv::Size(168, 168), cv::INTER_AREA);

    matmsk_roi160 = cv::Mat(matmsk_org168,cv::Rect(4,4,160,160));
    cv::Mat cvmask = msk_real160->cvmat();
    cv::cvtColor(matmsk_roi160,cvmask,cv::COLOR_RGB2GRAY);

    //BlendGramAlphaRev(pic_clone160->udata(),msk_real160->udata(),pic_crop160->udata(),160,160);
    //pic_crop160->show("aaa");
    //cv::waitKey(0);
    //pic_crop160
    //
    return 0;
}

int MWorkMat::finalpha(){
    cv::Mat cvmask = msk_real160->cvmat();
    cv::cvtColor(cvmask,matmsk_roi160,cv::COLOR_GRAY2RGB);
    //
    cv::resize(matmsk_org168, matmsk_roirst, cv::Size(m_boxwidth, m_boxheight), cv::INTER_AREA);
    matmsk_roirst.copyTo(matmsk_roisrc);
    return 0;
}

int MWorkMat::vtacc(uint8_t* buf,int count){
    /*
    int avgr = 0;
    int avgb = 0;
    int avgg = 0;
    if(1){
        uint8_t* pb = m_pic->udata();
        for(int k=0;k<10;k++){
            avgr += *pb++;
            avgg += *pb++;
            avgb += *pb++;
        }
        avgr =avgr/10 +10;
        avgg =avgg/10 -20;
        if(avgg<0)avgg=0;
        avgb =avgb/10 + 10;
    }
    */
    uint8_t* pb = buf;
    for(int k=0;k<count;k++){
        int sum  = (pb[0]+ pb[2])/2.0f;
        if(pb[1]>=sum){
            pb[1]=sum;
            //pb[0]=0;
            //pb[2]=0;
            // }else if((pb[0]<avgr)&&(pb[1]>avgg)&&(pb[2]<avgb)){
            //pb[1]=0;
            //pb[0]=0;
            //pb[2]=0;
        }
        pb+=3;
    }
    /*
    long sum = 0l;
    float  mean = sum*0.5f/count;
    uint8_t maxg = (mean>255.f)?255:mean;
    //printf("sum %ld mean %f maxg %d\n",sum,mean,maxg);
    //getchar();
    pb = buf +1;
    for(int k=0;k<count;k++){
        if(*pb>maxg){
            *pb = maxg;
        }
        pb+=3;
    }
    */
    return 0;
}

