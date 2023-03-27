#pragma once

#include "imc.h"
#include "cha.h"
#include "iio.h"

#include <vector>

inline void chaPost(pcm::CHA& cha, double n_sample_in_sec)
{
    if(cha.eventCount == 0) return;

    uint64_t io_wr, io_rd;
    double IO_WR_BW, IO_RD_BW;

    static std::vector<std::vector<pcm::uint64>> counter0, prev0;
    static std::vector<std::vector<pcm::uint64>> counter1, prev1;
    static std::vector<std::vector<pcm::uint64>> counter2, prev2;
    static std::vector<std::vector<pcm::uint64>> counter3, prev3;

    if (prev0.empty()) {
    	cha.getCounter(prev0, 0);
    	cha.getCounter(prev1, 1);
    	cha.getCounter(prev2, 2);
    	cha.getCounter(prev3, 3);
    }

    cha.getCounter(counter0, 0);
    cha.getCounter(counter1, 1);
    cha.getCounter(counter2, 2);
    cha.getCounter(counter3, 3);

//counter0 PCIRDCUR counter1 itom

    for(int i = 0; i < pcm::sockets; i++){
        io_wr = 0;
        io_rd = 0;

        for(int j = 0; j < counter0[i].size(); j++){
            //io_wr += counter0[i][j] - prev0[i][j];
            //io_rd += counter1[i][j] - prev1[i][j];
            io_rd += counter0[i][j] - prev0[i][j];   //pciRDcur  this matches system RD BW or CAS.RD
            io_wr += counter1[i][j] - prev1[i][j];  //itoM  this matches CAS.WR
        }

	//printf("socket %d: ");
        IO_WR_BW = n_sample_in_sec * io_wr * 64 / 1E9;
        IO_RD_BW = n_sample_in_sec * io_rd * 64 / 1E9;
        
        printf("IO_WR_BW = %lf  IO_RD_BW = %lf  ", IO_WR_BW, IO_RD_BW);
    }
    //printf("\n");

    prev0 = counter0;
    prev1 = counter1;
    prev2 = counter2;
    prev3 = counter3;
}

inline void imcPost(pcm::IMC& imc, double n_sample_in_sec)
{
    if(imc.eventCount == 0) return;

    uint64_t io_wr, io_rd;
    double IO_WR_BW, IO_RD_BW;
    double ddrcyclecount = (1e9 * (1*60) / (1/2.4))*n_sample_in_sec;

    static std::vector<std::vector<pcm::uint64>> counter0, prev0;
    static std::vector<std::vector<pcm::uint64>> counter1, prev1;
    static std::vector<std::vector<pcm::uint64>> counter2, prev2;
    static std::vector<std::vector<pcm::uint64>> counter3, prev3;

    if (prev0.empty()) {
    	imc.getCounter(prev0, 0);
    	imc.getCounter(prev1, 1);
    	imc.getCounter(prev2, 2);
    	imc.getCounter(prev3, 3);
    }

    imc.getCounter(counter0, 0);
    imc.getCounter(counter1, 1);
    imc.getCounter(counter2, 2);
    imc.getCounter(counter3, 3);

    for(int soc = 0; soc < pcm::sockets; soc++){
		double tbw = 0, rbw=0, wbw=0, wpq=0, rpq=0;
		//uint64 tbw_p = 0, rbw_p=0, wbw_p=0, wpq_p=0, rpq_p=0;
		printf("  socket%d_BW=", soc);

		for(int i = 0; i < counter0[soc].size(); i++){
			wbw += (counter0[soc][i] - prev0[soc][i]);
			rbw += (counter1[soc][i] - prev1[soc][i]);
			wpq += (counter2[soc][i] - prev2[soc][i]);
		    rpq += (counter3[soc][i] - prev3[soc][i]);
		}
		tbw=(((wbw + rbw) * 64) / 1e9) * n_sample_in_sec;
		printf("%.2f wr_bw=%.2f rd_bw=%.2f wpq=%.2f rpq=%.2f ", tbw, (wbw * 64 / 1e9)*n_sample_in_sec  , (rbw * 64 / 1e9)*n_sample_in_sec , (wpq / ddrcyclecount) , (rpq / ddrcyclecount));

    }
    printf("\n");

    prev0 = counter0;
    prev1 = counter1;
    prev2 = counter2;
    prev3 = counter3;

}
