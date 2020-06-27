// test_prepas.cpp - Prepas unit tests
//
// Copyright (C) 2019 Arribada
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

extern "C"

{
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "syshal_sat.h"
#include "prepas.h"
}

#include <gtest/gtest.h>

class Test_prepas : public ::testing::Test
{
public:
    FILE    *ul_conf;       /* configuration file   */
    FILE    *ul_bull;       /* orbit file       */
    FILE    *ul_resu;       /* result file      */

    int Nsat;                   /* number of satellites in orbit file   */
    int status;

    pc  tab_PC[1];              /* array of configuration parameter */
    po  tab_PO[8];              /* array of orbit parameter     */
    //pp    tab_PP[1];              /* array of prediction parameters   */

    po  *pt_po;                 /* pointer on tab_po            */
    pc  *pt_pc;                 /* pointer on tab_pc            */
    pp  *pt_pp;                 /* pointer on tab_pp            */

    int isat;

    int i;
    float min_elevation = 45;
    std::string nf_conf = "../prepas/prepas.cfg";
    std::string nf_bull = "../prepas/bulletin.dat";

    //char  message [256];
    char    ligne [MAXLU];

    int tsat[8];        /* Order of satellite in orbit file */
    char    csat[2];

    //long  s_pp;       /* beginning of prediction (sec) */
    bulletin_data_t bulletin[8];
    uint8_t number_sat;
    float lon;
    float lat;
    uint32_t minimum_time;
    uint32_t desired_time;
    struct tm t_start, t_fin;
    uint32_t t_of_day_start, t_of_day_fin;

    void SetUp() override
    {
        /* Clear Orbit Paramater */
        for (isat = 0; isat < 8; isat++)
        {
            strcpy(tab_PO[isat].sat, "  ");
        }

        /*-------- Read Data Configuration ---------*/
        ul_conf = fopen(nf_conf.c_str(), "r");
        ASSERT_TRUE(ul_conf);
        lat = 51.47;
        lon = -2.609;
        t_of_day_start = 1552586400; // 14/04/2019 @ 6:00pm (UTC)

        /*-------- Read Orbit Parameter ---------*/
        struct tm t_bul;
        ul_bull = fopen(nf_bull.c_str(), "r");
        ASSERT_TRUE(ul_bull);

        time_t conver_time;
        fgets(ligne, MAXLU, ul_bull);
        sscanf(ligne, "%s%ld%f%f%f%f%f%f",
               &bulletin[0].sat,    &bulletin[0].time_bulletin,
               &bulletin[0].params[0],
               &bulletin[0].params[1],      &bulletin[0].params[2],
               &bulletin[0].params[3],  &bulletin[0].params[4],
               &bulletin[0].params[5]);

        isat = 0;

        while (!feof(ul_bull))
        {

            isat++;
            fgets(ligne, MAXLU, ul_bull);
            strcpy(csat, "  ");
            sscanf(ligne, "%s%ld%f%f%f%f%f%f",
                   &bulletin[isat].sat, &bulletin[isat].time_bulletin,
                   &bulletin[isat].params[0],
                   &bulletin[isat].params[1],       &bulletin[isat].params[2],
                   &bulletin[isat].params[3],   &bulletin[isat].params[4],
                   &bulletin[isat].params[5]);
        } /* lecture de chaque bulletin de satellite */
        /* END WHILE */

        fclose (ul_bull);

        number_sat = isat;
    }
};

TEST_F(Test_prepas, DISABLED_SimplePrediction_1)
{
    uint32_t time = next_predict(bulletin,min_elevation, number_sat, lon, lat, 1564508687 + 1);
    EXPECT_EQ(time , 1552590810); //03/14/2019 @ 7:13pm (UTC)
}



TEST_F(Test_prepas, SimplePrediction_windows_same_day)
{
    int window_start = 1564508601;  //Tuesday, July 30, 2019 6:43:21 PM GMT+01:00 DST
    int window_middle = 1564508688; //Tuesday, July 30, 2019 6:44:42 PM GMT+01:00 DST
    int window_end = 1564508773;    //Tuesday, July 30, 2019 6:46:13 PM GMT+01:00 DST
    int next_sat = 1564517774;      //Tuesday, July 30, 2019 9:16:12 PM GMT+01:00 DST

    uint32_t time;
    //Using ARGOS web-page WINDOW  [18:43:23 - 18:44:41 - 18:45:59 - ] 21:14:29

    for (int i = window_start - 100; i < window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1); i++)
    {
        time = next_predict(bulletin, min_elevation, number_sat, lon, lat, i);
        EXPECT_TRUE(time >= window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1));
        if (!((time >= (window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1))) && (time <= (window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1)))))
        {
            printf("desired: %d\ttime: %d\twinndow_middle: %d\n", i, time, window_middle);
            break;
        }
    }
    for (int i = window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1) ; i < window_end + 100 ; i++)
    {
        time = next_predict(bulletin, min_elevation, number_sat, lon, lat, i);
        EXPECT_TRUE(time >= next_sat - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= next_sat + (SYSHAL_SAT_MINIMUM_PAS - 1));
        if (!(time >= next_sat - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= next_sat + (SYSHAL_SAT_MINIMUM_PAS - 1)))
        {
            printf("desired: %d\ttime: %d\tnext_sat: %d\n", i, time, next_sat);
            break;
        }
    }
    //03/14/2019 @ 7:13pm (UTC)
}

TEST_F(Test_prepas, SimplePrediction_windows_month)
{
    int window_start = 1567103310;  //Thursday, August 29, 2019 7:28:30 PM GMT+01:00 DST
    int window_middle = 1567103448; //Thursday, August 29, 2019 7:30:51 PM GMT+01:00 DST
    int window_end = 1567103592;    //Thursday, August 29, 2019 7:33:12 PM GMT+01:00 DST

    //Using ARGOS web-page WINDOW  [19:29:18 - 19:30:46 - 19:32:14]
    int next_sat = 1567108550;     // Thursday, August 29, 2019 7:34:50 PM GMT+01:00 DST

    uint32_t time;

    for (int i = window_start - 100; i < window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1); i++)
    {
        time = next_predict(bulletin, min_elevation, number_sat, lon, lat, i);
        EXPECT_TRUE(time >= window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1));
        if (!((time >= (window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1))) && (time <= (window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1)))))
        {
            printf("desired: %d\ttime: %d\twinndow_middle: %d\n", i, time, window_middle);
            break;
        }
    }
    for (int i = window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1) ; i < window_end + 100 ; i++)
    {
        time = next_predict(bulletin, min_elevation, number_sat, lon, lat, i);
        EXPECT_TRUE(time >= next_sat - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= next_sat + (SYSHAL_SAT_MINIMUM_PAS - 1));
        if (!(time >= next_sat - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= next_sat + (SYSHAL_SAT_MINIMUM_PAS - 1)))
        {
            printf("current_time: %d\tpredicted: %d\tnext_sat: %d\n", i, time, next_sat);
            break;
        }
    }
    //03/14/2019 @ 7:13pm (UTC)
}

TEST_F(Test_prepas, SimplePrediction_windows_six_moth)
{
    int window_start    = 1580068019;     //Sunday, January 26, 2020 8:46:59 PM GMT+01:00 DST
    int window_middle   = 1580068421;     //Sunday, January 26, 2020 8:53:37 PM GMT+01:00 DST
    int window_end      = 1580068821;     //Sunday, January 26, 2020 9:00:21 PM GMT+01:00 DST
    int next_sat        = 1580070821;     //Sunday, January 26, 2020 9:33:38 PM GMT+01:00 DST

    uint32_t time;


    //Using ARGOS web-page WINDOW  [20:51:57 - 20:53:36 - 20:55:15 ]-Next 21:31:57

    for (int i = window_start - 100; i < window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1) - 10; i++)
    {
        time = next_predict(bulletin, min_elevation, number_sat, lon, lat, i);
        EXPECT_TRUE(time >= window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1));
        if (!((time >= (window_middle - (SYSHAL_SAT_MINIMUM_PAS - 1))) && (time <= (window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1)))))
        {
            printf("desired: %d\ttime: %d\twinndow_middle: %d\n", i, time, window_middle);
            break;
        }
    }
    for (int i = window_middle + (SYSHAL_SAT_MINIMUM_PAS - 1) ; i < window_end + 100 ; i++)
    {
        time = next_predict(bulletin, min_elevation, number_sat, lon, lat, i);
        EXPECT_TRUE(time >= next_sat - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= next_sat + (SYSHAL_SAT_MINIMUM_PAS - 1));
        if (!(time >= next_sat - (SYSHAL_SAT_MINIMUM_PAS - 1) && time <= next_sat + (SYSHAL_SAT_MINIMUM_PAS - 1)))
        {
            printf("desired: %d\ttime: %d\tnext_sat: %d\n", i, time, next_sat);
            break;
        }
    }
    //03/14/2019 @ 7:13pm (UTC)
}

TEST_F(Test_prepas, Projection_2year)
{
    int test_start =  1564508601;  //Tuesday, July 30, 2019 6:43:21 PM GMT+01:00 DST
    int test_finish = 1564508601 + (2 * 365 * 24 * 60 * 60);  //Tuesday, July 30, 2021 6:43:21 PM GMT+01:00 DST
    int interval_gps = 4 * 60 * 60;
    int maximum_time_sat = 24 * 60 * 60;
    int current_time = test_start;
    int total_connection = 0;
    int total_gps = 0;

    int next_gps = 1564508601;
    int next_prediction = 0;
    while (current_time < test_finish)
    {
        if (current_time == next_prediction)
        {
            total_connection++;
        }
        if (current_time == next_gps)
        {
            if (next_prediction <= current_time)
            {

                next_prediction = next_predict(bulletin, min_elevation, number_sat, lon, lat, current_time + 30);
                EXPECT_TRUE(next_prediction < ((current_time + 30) + maximum_time_sat));
            }

            next_gps += interval_gps;
            total_gps++;
        }

        if ((current_time < next_prediction) && (next_prediction < next_gps))
        {
            current_time = next_prediction;
        }
        else
        {
            current_time = next_gps;
        }
    }

}

TEST_F(Test_prepas, number_prepas_first_test)
{
    int test_start =  1564765200; // 2/8/19 - 23:59:59
    int test_finish = 1564959599; // 4/8/19 - 23:59:59
    int current_time = test_start;

    int next_prediction = next_predict(bulletin, min_elevation,number_sat, lon, lat, current_time + 30);
    int number_sat = 0;

    while (current_time < test_finish)
    {
        if (next_prediction < current_time)
        {
            next_prediction = next_predict(bulletin, min_elevation, 7, lon, lat, current_time + 30);
            number_sat++;
        }
        current_time += 60;
        // printf("current_time: %d\n", current_time - test_start);
    }

    EXPECT_TRUE(number_sat > (15));
}

TEST_F(Test_prepas, Check_software)
{
    unsigned int test_start =  1565374573; // 9/8/19 - 19:14:11
    unsigned int last_sat = 0;
    unsigned int current_time = test_start;

    while (current_time < (test_start + 48 * 60 * 60))
    {
        std::time_t result = (std::time_t)next_predict(bulletin, min_elevation, number_sat, lon, lat, current_time);

        current_time = (unsigned int ) result + 20;
        std::cout << std::asctime(std::localtime(&result));
    }


}



TEST_F(Test_prepas, DISABLED_SimplePrediction_1_2)
{
    t_of_day_start = 1555261200; // 14/04/2019 @ 6:00pm (UTC)
    uint32_t time = next_predict(bulletin, min_elevation, number_sat, lon, lat, 1555263060 - 10);
    EXPECT_EQ(time , 1555263060);
}

TEST_F(Test_prepas, DISABLED_SimplePrediction_1_3)
{
    t_of_day_start = 1555261200; // 14/04/2019 @ 6:00pm (UTC)
    uint32_t time = next_predict(bulletin, min_elevation, number_sat, lon, lat, 1555263060 - 20);
    EXPECT_EQ(time , 1555263060);
}



TEST_F(Test_prepas, DISABLED_SimplePrediction_2)
{
    t_of_day_start = 1555261200; // 14/04/2019 @ 6:00pm (UTC)
    uint32_t time = next_predict(bulletin, min_elevation, number_sat, lon, lat, t_of_day_start);
    EXPECT_EQ(time , 1555263060);
}

TEST_F(Test_prepas, DISABLED_SimplePrediction_3_month)
{
    t_of_day_start = 1560531600; // 14/06/2019 @ 6:00pm (UTC)
    uint32_t time = next_predict(bulletin, min_elevation, number_sat, lon, lat, t_of_day_start);
    EXPECT_EQ(time , 1560537495);
}

TEST_F(Test_prepas, DISABLED_SimplePrediction_6_month)
{
    t_of_day_start = 1568480400; // 14/09/2019 @ 6:00pm (UTC)
    uint32_t time = next_predict(bulletin, min_elevation, number_sat, lon, lat, t_of_day_start);
    EXPECT_EQ(time , 1568485365);
}

TEST_F(Test_prepas, DISABLED_SimplePrediction_12_month)
{
    t_of_day_start = 1584208800; // 14/03/2020 @ 6:00pm (UTC)
    uint32_t time = next_predict(bulletin, min_elevation, number_sat, lon, lat, t_of_day_start);
    EXPECT_EQ(time , 1584210630);
}

TEST_F(Test_prepas, DISABLED_comp_original)
{
    lon = 1;
    lat = 52;
    t_of_day_start = 1552586400; // 14/04/2019 @ 6:00pm (UTC)#
    long minimum_time = t_of_day_start;
    pc  tab_PC[1];              /* array of configuration parameter */
    po  tab_PO[number_sat];
    pp tab_PP[number_sat];        /* array of result */
    pp ref_tab_PP[number_sat];
    tab_PC[0].pf_lon = lon;
    tab_PC[0].pf_lat = lat;

    tab_PC[0].time_start = minimum_time;
    tab_PC[0].time_end = minimum_time + (48 * 60 * 60);
    tab_PC[0].s_differe = 0;

    tab_PC[0].site_min_requis = 45.0f;
    tab_PC[0].site_max_requis = 90.0f;

    tab_PC[0].marge_temporelle = 0;
    tab_PC[0].marge_geog_lat = 0;
    tab_PC[0].marge_geog_lon = 0;

    tab_PC[0].Npass_max = 1;

    po  *pt_po;                 /* pointer on tab_po            */
    pc  *pt_pc;                 /* pointer on tab_pc            */
    pp  *pt_pp;                 /* pointer on tab_pp            */
    pp *ref_pt_pp;
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
    ref_pt_pp  = &ref_tab_PP[0];

    //Load REF VALUES
    memcpy(ref_pt_pp[0].sat, "MA", 2);
    ref_pt_pp[0].tpp = 1552641863;      /* date du prochain passage (sec90) */
    ref_pt_pp[0].duree = 3;     /* duree (sec) */
    ref_pt_pp[0].site_max = 59; /* site max dans le passage (deg) */

    memcpy(ref_pt_pp[1].sat, "MB", 2);
    ref_pt_pp[1].tpp = 1552596204;      /* date du prochain passage (sec90) */
    ref_pt_pp[1].duree = 3;     /* duree (sec) */
    ref_pt_pp[1].site_max = 83; /* site max dans le passage (deg) */

    memcpy(ref_pt_pp[2].sat, "MC", 2);
    ref_pt_pp[2].tpp = 1552594224;      /* date du prochain passage (sec90) */
    ref_pt_pp[2].duree = 2;     /* duree (sec) */
    ref_pt_pp[2].site_max = 55; /* site max dans le passage (deg) */

    memcpy(ref_pt_pp[3].sat, "15", 2);
    ref_pt_pp[3].tpp = 1552633013;      /* date du prochain passage (sec90) */
    ref_pt_pp[3].duree = 2;     /* duree (sec) */
    ref_pt_pp[3].site_max = 48; /* site max dans le passage (deg) */

    memcpy(ref_pt_pp[4].sat, "18", 2);
    ref_pt_pp[4].tpp = 1552590714;      /* date du prochain passage (sec90) */
    ref_pt_pp[4].duree = 3;     /* duree (sec) */
    ref_pt_pp[4].site_max = 60; /* site max dans le passage (deg) */

    memcpy(ref_pt_pp[5].sat, "19", 2);
    ref_pt_pp[5].tpp = 1552626713;      /* date du prochain passage (sec90) */
    ref_pt_pp[5].duree = 3;     /* duree (sec) */
    ref_pt_pp[5].site_max = 74; /* site max dans le passage (deg) */

    memcpy(ref_pt_pp[6].sat, "SR", 2);
    ref_pt_pp[6].tpp = 1552627223;      /* date du prochain passage (sec90) */
    ref_pt_pp[6].duree = 3;     /* duree (sec) */
    ref_pt_pp[6].site_max = 73; /* site max dans le passage (deg) */

    prepas(pt_pc, pt_po, pt_pp, number_sat);

    for (int i = 0; i < number_sat; ++i)
    {
        EXPECT_EQ(pt_pp[i].sat[0] , ref_pt_pp[i].sat[0]);
        EXPECT_EQ(pt_pp[i].sat[1] , ref_pt_pp[i].sat[1]);
        EXPECT_EQ(pt_pp[i].tpp , ref_pt_pp[i].tpp);
        EXPECT_EQ(pt_pp[i].duree / 60 , ref_pt_pp[i].duree);
        EXPECT_EQ(pt_pp[i].site_max , ref_pt_pp[i].site_max);
    }
}