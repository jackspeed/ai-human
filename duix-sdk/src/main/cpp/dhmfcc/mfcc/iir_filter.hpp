#pragma once

#ifndef SERVICESUPERVISOR_IIR_FILTER_H
#define SERVICESUPERVISOR_IIR_FILTER_H

//E(t,f) is computed using a first-order in-finite impulse response (IIR) filter
#define UES_IIR_I
//#define UES_IIR_II

#ifdef UES_IIR_I

class IIR_I
{
private:
    double *m_pNum;
    double *m_pDen;
    double *m_px;
    double *m_py;
    int m_num_order;
    int m_den_order;
public:
    IIR_I();
    ~IIR_I();
    void reset();
    void setPara(double num[], int num_order, double den[], int den_order);
    void resp(double data_in[], int m, double data_out[], int n);
    void filter(double data_in[], double data_out[], int len);
};

#endif

#ifdef UES_IIR_II
class IIR_II
{
public:
    IIR_II();
    void reset();
    void setPara(double num[], int num_order, double den[], int den_order);
    void resp(double data_in[], int m, double data_out[], int n);
    double filter(double data);
    void filter(double data[], int len);
    void filter(double data_in[], double data_out[], int len);
protected:
private:
    double *m_pNum;
    double *m_pDen;
    double *m_pW;
    int m_num_order;
    int m_den_order;
    int m_N;
};

class IIR_BODE
{
private:
    double *m_pNum;
    double *m_pDen;
    int m_num_order;
    int m_den_order;
    std::complex<double> poly_val(double p[], int order, double omega);
public:
    IIR_BODE();
    void setPara(double num[], int num_order, double den[], int den_order);
    std::complex<double> bode(double omega);
    void bode(double omega[], int n, std::complex<double> resp[]);
};
#endif

#endif
