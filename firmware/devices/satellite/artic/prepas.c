/*    *******************************************************    */
/*    *******************************************************    */
/*               Copyright (c) 1995 CLS                          */
/*    All rights reserved including the right of reproduction    */
/*    in whole or in part in any form.                           */
/*    *******************************************************    */
/*    *******************************************************    */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "syshal_sat.h"

#include "prepas.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"



/* +-------------------------------------------------------------------+*/
/* +                      variables externes                           +*/
/* +-------------------------------------------------------------------+*/


/* +-------------------------------------------------------------------+*/
/* +                      C O N S T A N T E S                          +*/
/* +-------------------------------------------------------------------+*/

#define SECONDS_IN_DAY (24 * 60 * 60)

const   float   pi  = 3.1415926535;     /* Pi     value     */
const   float   demi_pi = 1.570796327;          /* Pi/2   value     */
const   float   two_pi  = 6.283185307;          /* 2*pi   value     */
const   float   deg_rad = 0.017453292;          /* pi/180 value     */
const   float   rad_deg = 57.29577951;          /* 180/pi value     */

const   int pas = SYSHAL_SAT_MINIMUM_PAS;                   /* en (sec)     */

const   float   rt  = 6378.137;             /* Earth radius     */
const   float   rs  = 7200;         /* Orbit radius     */

const int TIME_CONVERTOR = 631152000; /* Change apoch from 1990 to 1970 */

/*Tables des nombres de jours ecoules avant chaque mois de l'annee  */
/*    ek_quant (1...12,1) pour les annees bissextiles           */
/*    ek_quant (1...12,2) pour les annees non bissextiles       */
/*    ---------------------------------------------------------------   */



void su_distance(long   t1,     /* input */

                 float  x_pf,
                 float  y_pf,
                 float  z_pf,
                 float  ws,
                 float  sin_i,
                 float  cos_i,
                 float  asc_node,
                 float  wt,

                 float  *d2);       /* output */
uint8_t select_closest(pp *pt_pp, int number_sat, uint32_t desired_time);

#ifdef DEBUG_SAT

void print_list(pp * p_pp,  int number_sat);
void print_config(pc    *p_pc);
void print_sat(po *p_po, int number_sat);

#endif

// WARN: number_sat must be greater than 0
uint32_t next_predict(bulletin_data_t *bulletin, float min_elevation,  uint8_t number_sat, float lon, float lat, long current_time)
{
    pc  tab_PC[1];              /* array of configuration parameter */
    po  tab_PO[SYSHAL_SAT_MAX_NUM_SATS];
    pp tab_PP[SYSHAL_SAT_MAX_NUM_SATS];         /* array of result */
    tab_PC[0].pf_lon = lon;
    tab_PC[0].pf_lat = lat;

    tab_PC[0].time_start = current_time;
    tab_PC[0].time_end = current_time + SECONDS_IN_DAY;
    tab_PC[0].s_differe = 0;

    tab_PC[0].site_min_requis = min_elevation;
    tab_PC[0].site_max_requis = 90.0f;

    tab_PC[0].marge_temporelle = 0;
    tab_PC[0].marge_geog_lat = 0;
    tab_PC[0].marge_geog_lon = 0;

    tab_PC[0].Npass_max = 1;



    po  *pt_po;                 /* pointer on tab_po            */
    pc  *pt_pc;                 /* pointer on tab_pc            */
    pp  *pt_pp;                 /* pointer on tab_pp            */

    for (int i = 0; i < number_sat; ++i)
    {
        memcpy(tab_PO[i].sat, bulletin[i].sat, 2);
        tab_PO[i].time_bul = bulletin[i].time_bulletin;
        tab_PO[i].dga = bulletin[i].params[0];
        tab_PO[i].inc = bulletin[i].params[1];
        tab_PO[i].lon_asc = bulletin[i].params[2];
        tab_PO[i].d_noeud = bulletin[i].params[3];
        tab_PO[i].ts = bulletin[i].params[4];
        tab_PO[i].dgap = bulletin[i].params[5];
    }


    pt_pc  = &tab_PC[0];
    pt_po  = &tab_PO[0];
    pt_pp  = &tab_PP[0];
#ifdef DEBUG_SAT
    print_config(pt_pc);
    print_sat(pt_po, number_sat);
#endif
    prepas(pt_pc, pt_po, pt_pp, number_sat);

#ifdef DEBUG_SAT
    print_list(pt_pp, number_sat);
#endif


    uint8_t index = select_closest(pt_pp, number_sat, current_time);


    return (pt_pp[index].tpp + (pt_pp[index].duree / 2));
}
int prepas (    pc  * p_pc,
                po  * p_po,
                pp * p_pp,
                int number_sat
           )
{
    /* -----------------------------------------------------------------
    C
    C   Satellite passes prediction
    C   Circular method taking into account drag coefficient (friction effect)
    C   and J2 term of earth potential
    C
    C       ephemeris calculation
    C
    C
    C   Author  : J.P. Malarde
    C   Company : C.L.S.
    C   Issue   : 1.0   25/09/97    first issue
    C             2.0   10/02/14    Sharc version
    C
    C -----------------------------------------------------------------*/


    long    tpp;
    int duree;
    int duree_passage_MC; /* duree du passage Marges Comprises */

    long    s_deb;      /* beginning of prediction (sec) */
    long    s_fin;      /* end of prediction (sec) */
    long    k;          /* number of revolution */
    long    s_bul;      /* bulletin epoch (sec) */
    long    t0;     /* date (sec) */
    long    t1;     /* date (sec) */
    long    t2;     /* date (sec) */
    long    tmil[8];    /* date milieu du prochain passage/satellite */

    int isat;
    int step = 0;

    int site;       /* min site */
    int passage;
    float   d2;
    float   d2_min;
    float   d2_mem;
    float   d2_mem_mem;
    float   temp;
    float   v;

    int duree_passage;
    long date_milieu_passage;
//long date_debut_passage;
    int site_max_passage;

    int site_max;   /* min site */
    int marge;
    int table_site_max[8];                  /* DB */
    int table_duree[8];                     /* DB */
    float   delta_lon;  /* asc.node drift during one revolution (deg) */
    float   wt;     /* earth rotation */

    float   visi_min;       /* visibility at site min*/
    float   visi_max;       /* visibility at site_max */

    float   ws0;        /* mean anomaly */
    float   d_ws;       /* friction effect */
    float   ws;     /* mean anomaly with friction effect */
    float   sin_i, cos_i;   /* sin, cos of orbit inclination */
    float   asc_node;   /* longitude of ascending node in terrestrial frame */
    float   x_pf;       /* beacon position */
    float   y_pf;
    float   z_pf;

    float   v_lon;
    float   v_lat;
    int     v_differe;
    float   v_site_max_requis;
    float   v_ts;
    float   v_marge_temporelle;
    float   v_marge_geog_lat;
    float   v_marge_geog_lon;
    float   v_dgap;
    int     Npass;      /* nombre de passages */
    int     Npass_max;  /* nombre de passages max par satellite */

    float   v_site_min_requis;

    memset(tmil,        0, sizeof(tmil));
    memset(table_site_max,  0, sizeof(table_site_max));
    memset(table_duree, 0, sizeof(table_duree));

    /* ...  EEPROM --> RAM transfert    */
    /*  ------------------------    */

    v_lon           = p_pc[0].pf_lon;
    v_lat           = p_pc[0].pf_lat;
    v_differe       = p_pc[0].s_differe;
    v_site_max_requis   = p_pc[0].site_max_requis;
    v_site_min_requis   = p_pc[0].site_min_requis;
    v_marge_temporelle  = p_pc[0].marge_temporelle;
    v_marge_geog_lat        = p_pc[0].marge_geog_lat;
    v_marge_geog_lon        = p_pc[0].marge_geog_lon;
    Npass_max = p_pc[0].Npass_max;

    /* ...  input parameter conversion  */
    /*  --------------------------- */

    v_lon = v_lon * deg_rad;
    v_lat = v_lat * deg_rad;

    x_pf = cos (v_lat) * cos (v_lon);
    y_pf = cos (v_lat) * sin (v_lon);
    z_pf = sin (v_lat);

    v_site_min_requis = v_site_min_requis * deg_rad;
    visi_min = demi_pi - v_site_min_requis - asin(rt / rs * cos(v_site_min_requis));
    visi_min = 2. * sin(visi_min / 2.);
    visi_min = visi_min * visi_min;

    visi_max = v_site_max_requis * deg_rad;
    visi_max   = demi_pi - visi_max - asin(rt / rs * cos(visi_max));
    visi_max   = 2. * sin(visi_max / 2.);
    visi_max   = visi_max * visi_max;


    s_deb = p_pc[0].time_start - TIME_CONVERTOR;
    s_fin = p_pc[0].time_end - TIME_CONVERTOR;


    v_marge_temporelle = v_marge_temporelle + 5.;
    v_marge_temporelle = v_marge_temporelle / 259200.;

    v_marge_geog_lat = v_marge_geog_lat * 7.857142;
    v_marge_geog_lon = 0;

    /* ...  reading of OP */

    isat = 1;
//  t_sel = 999999999;

    //while (((isat-1) < 8) && (strncmp(p_po[isat-1].sat, "  ",2)!=0 )) {
    for (isat = 1; isat <= number_sat; isat ++)
    {

        p_pp[isat - 1].tpp = 0;
        sin_i    = sin(p_po[isat - 1].inc * deg_rad);
        cos_i    = cos(p_po[isat - 1].inc * deg_rad);

        v_dgap     = p_po[isat - 1].dgap / 1000;    /*  conversion m/jr --> km/jr */


        s_bul = p_po[isat - 1].time_bul - TIME_CONVERTOR;
        v_ts       = p_po[isat - 1].ts * 60.;
        ws0      = two_pi / v_ts;                   /* tour/sec */
        delta_lon = p_po[isat - 1].d_noeud * deg_rad;       /* distance between 2 asc nodes */
        wt       = delta_lon / v_ts;            /* tour/secondes */
        asc_node = p_po[isat - 1].lon_asc * deg_rad;

        /* ... recherche du prochain passage dont le site max > site max requis */

        Npass = 0;
        passage = 0;
        site = 0;
        duree = 0;
        site_max = 0;
        d2_min = 999.;
        t1 = s_deb - s_bul - (30 * 60);
        t2 = s_fin - s_bul;

        t0 = t1;
        k = (long)(t1 / v_ts);
        d_ws = -.75 * (v_dgap / p_po[isat - 1].dga / 86400.) * two_pi * k;
        ws = ws0 + d_ws;

        /* ... on saute le passage courrant */

        su_distance(    t1,
                        x_pf,
                        y_pf,
                        z_pf,
                        ws,
                        sin_i,
                        cos_i,
                        asc_node,
                        wt,
                        &d2);

        d2_mem     = d2;

        while ( (t1 < t2) && (Npass < Npass_max)  )
        {

            d2_mem_mem = d2_mem;
            d2_mem     = d2;

            su_distance(    t1,
                            x_pf,
                            y_pf,
                            z_pf,
                            ws,
                            sin_i,
                            cos_i,
                            asc_node,
                            wt,
                            &d2);

            if (d2 < visi_min)
            {

                passage = 1;

//           printf("********* passage en visibilite ***********\n");

                duree = duree + pas;
                v = 2.*asin(sqrt(d2) / 2.);
                temp =  (rs * sin(v)) / sqrt(rt * rt + rs * rs - 2 * rt * rs * cos(v));
                temp =  rad_deg * acos(temp);
                site = (int) (temp);
                if (site > site_max) site_max = site;
                if (d2 < d2_min)     d2_min = d2;

                step = pas;
            }
            else
            {

//           printf("********* passage hors visibilite ******");

                if (passage == 1)
                {

//              printf("****** on sort juste d'une visibilite ***********\n");

                    if (site_max < v_site_max_requis)
                    {

                        // on memorise lorsque l'on quitte la visibilit
                        duree_passage = duree;
                        date_milieu_passage = s_bul + t1 - duree / 2;
//                  date_debut_passage = date_milieu_passage - duree/2;
                        site_max_passage = site_max;

                        Npass += 1;


                        /* ...  Donnees en sortie :                 */
                        /*  - date de debut du prochain passage (sec)       */
                        /*  - duree du prochain passage, marge comprises (sec)  */

                        temp = (int)(v_marge_temporelle * t1 + v_marge_geog_lat + v_marge_geog_lon);
                        marge = (int)(temp);
                        tpp = date_milieu_passage - duree_passage / 2 - marge;
                        duree_passage_MC = duree_passage + 2 * marge;


                        strcpy(p_pp[isat - 1].sat, p_po[isat - 1].sat);
                        p_pp[isat - 1].tpp      = tpp + TIME_CONVERTOR;
                        p_pp[isat - 1].duree    = duree_passage_MC;
                        p_pp[isat - 1].site_max = site_max_passage;
                        passage = 2;

                    }
                    else
                    {

//                 printf("********* site max > site max requis ***********\n");

                        passage = 0;

                    }

                    site_max = 0;
                    duree = 0;
                    step = 30 * 60;        // 75 min
                    d2_min = 999999.;

                }
                else
                {

//              printf("********* on ne sort pas juste d'une visibilite ***********\n");

                    /* on ajuste le pas en fonction de la distance */

                    if ( d2 < ( 4 * visi_min))          step = pas;

                    if ((d2 >= ( 4 * visi_min)) &&
                        (d2 < (16 * visi_min)) )   step = 4 * pas;

                    if ((d2 >= (16 * visi_min))  && 
                         (d2 <= (32 * visi_min)) )    step = 8 * pas;

                     if ( d2 > (32 * visi_min))  step = 16 * pas;

                }

            }

            t1 = t1 + step;

        }       /* END WHILE sur le temps   */

    } /* lecture de chaque bulletin de satellite */




    return 0;
}


void su_distance(   long    t1,     /* input */

                    float   x_pf,
                    float   y_pf,
                    float   z_pf,
                    float   ws,
                    float   sin_i,
                    float   cos_i,
                    float   asc_node,
                    float   wt,

                    float   *d2)        /* output */

{
    float   lat_sat;    /*  latitude of the satellite beetween the a.n  */
    float   long_sat_an;    /*  longitude of the satellite beetween the a.n */
    float   long_sat;   /*  longitude of the satellite          */
    float   x, y, z;        /*  satellite position              */

    /* ...  calculation of the satellite latitude */

    lat_sat =  asin( sin(ws * (float)(t1)) * sin_i );

    /* ...  calculation of the satellite longitude */

    long_sat_an = atan( tan(ws * (float)(t1)) * cos_i);


    /*
        printf(" pi %lf \n", pi);
    */

    if (cos(ws * t1) < 0.)
    {
        long_sat_an = long_sat_an + pi;
    }

    long_sat = asc_node + long_sat_an + wt * (float)(t1);
    long_sat = fmod(long_sat, two_pi);

    if (long_sat < 0.)
    {
        long_sat = long_sat + two_pi;
    }

    /* ...  spheric satellite positions calculation in TR */

    x = cos (lat_sat) * cos (long_sat);
    y = cos (lat_sat) * sin (long_sat);
    z = sin (lat_sat);

    *d2 = (x - x_pf) * (x - x_pf) +
          (y - y_pf) * (y - y_pf) +
          (z - z_pf) * (z - z_pf) ;

}

uint8_t select_closest(pp *pt_pp, int number_sat, uint32_t desired_time)
{
    uint32_t min = 0xFFFFFFFF;
    uint8_t index = 0;
    uint32_t current = 0;

    for (int i = 0; i < number_sat; ++i)
    {
        current = pt_pp[i].tpp + (pt_pp[i].duree / 2);

        if (current > desired_time && ((current - desired_time) < min) )
        {
            min = (current - desired_time);
            index = i;
        }
    }
#ifdef DEBUG_SAT
    printf("SAT SELECTED: %2s,\t INDEX: %d,\t DESIRED TIME: %d\t, NEXT PASS: %d\n", pt_pp[index].sat, index, desired_time, (pt_pp[index].tpp + (pt_pp[index].duree / 2)));
#endif

    return index;
}
#ifdef DEBUG_SAT
void print_list(pp * p_pp,  int number_sat)
{


    for (int i = 0; i < number_sat; ++i)
    {
        printf("sat %2s  duration (min)  %i site Max (deg)  %i \t prox: %d\t middle %d\n",
               p_pp[i].sat,
               p_pp[i].duree / 60,
               p_pp[i].site_max,
               p_pp[i].tpp, // from 1990 to 1970
               (p_pp[i].tpp + (p_pp[i].duree / 2))); // from 1990 to 1970
    }

}

void print_config(pc    *p_pc)
{

    printf("------------CONFIG STRUCT------------\n\n\n");
    printf("lon %f, lat %f\n", p_pc[0].pf_lon, p_pc[0].pf_lat);
    printf("START: time %d,", p_pc[0].time_start);
    printf("FIN: time %d,", p_pc[0].time_end);
    printf("s_differe %f\t", p_pc[0].s_differe);
    printf("site_min_requis:  %f\t", p_pc[0].site_min_requis);
    printf("site_max_requis %f\t", p_pc[0].site_max_requis);
    printf("marge_temporelle %f\t", p_pc[0].marge_temporelle);
    printf("marge_geod_lat %f\t", p_pc[0].marge_geog_lat);
    printf("marge_geog_lon %f\t", p_pc[0].marge_geog_lon);
    printf("npass_max %d\n", p_pc[0].Npass_max);
}

void print_sat(po *p_po, int number_sat)
{

    printf("\n\n\n------------CONFIG SAT------------\n\n\n");
    for (int i = 0; i < number_sat; ++i)
    {
        printf("NAME: %s\n\n", p_po[i].sat);
        printf("START: time %d", p_po[i].time_bul);
        printf("VALUES: dga %f \tinc %f\t  lon_asc %f\t d_noeud %f\t ts %f\t, dgap %f\t\n\n", p_po[i].dga, p_po[i].inc,
               p_po[i].lon_asc,     p_po[i].d_noeud,
               p_po[i].ts,      p_po[i].dgap);

    }
}
#endif

#pragma GCC diagnostic pop