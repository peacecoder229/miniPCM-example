#pragma once

#include "imc.h"
#include "cha.h"
#include "iio.h"
#include <string>
#include "utils.h"
#include <vector>

inline void iioPost(pcm::IIO & iio, double n_sample_in_sec)
{
	if(iio.eventCount == 0) return;
	uint64_t iiocompbufocc, iiocompbufinsrt, iioclks;
	double iio_rndtrip_lat;

		
	static std::vector<std::vector<pcm::uint64>> counter0, prev0;
	static std::vector<std::vector<pcm::uint64>> counter1, prev1;
	static std::vector<std::vector<pcm::uint64>> counter2, prev2;
	static std::vector<std::vector<pcm::uint64>> counter3, prev3;

	if (prev0.empty()) {
	iio.getCounter(prev0, 0);
	iio.getCounter(prev1, 1);
	iio.getCounter(prev2, 2);
	iio.getCounter(prev3, 3);
	}

	iio.getCounter(counter0, 0);
	iio.getCounter(counter1, 1);
	iio.getCounter(counter2, 2);
	iio.getCounter(counter3, 3);
	for(int i = 0; i < pcm::sockets; i++){
		iiocompbufocc = 0;
		iiocompbufinsrt = 0;
		iioclks = 0;
		for(int j =0 ; j < counter0[i].size(); j++){
			iiocompbufocc += counter0[i][j] - prev0[i][j];
			iiocompbufinsrt += counter1[i][j] - prev1[i][j];
			iioclks += counter2[i][j] - prev2[i][j];
		}

		//iio_rndtrip_lat = (iiocompbufocc / iiocompbufinsrt / iioclks) * 1e9;
		iio_rndtrip_lat = ((static_cast<double>(iiocompbufocc) / iiocompbufinsrt) / (iioclks / pcm::SPR_M2IOSF_NUM)) * 1e9;
		printf("socket%d: IIO rnd trip lat " , i);
		//printf("OCC=%1f, INSRT=%1f, CTICK=%1f , lat=%1f\n", iiocompbufocc, iiocompbufinsrt, iioclks, iio_rndtrip_lat);
		printf("OCC=%llu, INSRT=%llu, CTICK=%llu, lat=%.5f\n", iiocompbufocc, iiocompbufinsrt, iioclks, iio_rndtrip_lat);

	}
	prev0 = counter0;
	prev1 = counter1;
	prev2 = counter2;
	prev3 = counter3;
}




inline void chaPost(pcm::CHA& cha, double n_sample_in_sec, const std::string& analysis)
{
    if(cha.eventCount == 0) return;

    uint64_t io_wr, io_rd, torocc, torinsrt, chaclks;
    double IO_WR_BW, IO_RD_BW, tor_latency;

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
	torocc = 0;
	torinsrt = 0;
	chaclks = 0;

	if( analysis == "iobw"){

		for(int j = 0; j < counter0[i].size(); j++){
		    //io_wr += counter0[i][j] - prev0[i][j];
		    //io_rd += counter1[i][j] - prev1[i][j];
		    //
		    io_rd += counter0[i][j] - prev0[i][j];   //pciRDcur  this matches system RD BW or CAS.RD
		    io_wr += counter1[i][j] - prev1[i][j];  //itoM  this matches CAS.WR
		}

		//printf("socket IIO rnd trip latency: ");
		IO_WR_BW = n_sample_in_sec * io_wr * 64 / 1E9;
		IO_RD_BW = n_sample_in_sec * io_rd * 64 / 1E9;
		
		printf("IO_WR_BW = %lf  IO_RD_BW = %lf  ", IO_WR_BW, IO_RD_BW);

	}
	else if (analysis == "latency"){

		for(int j = 0; j < counter0[i].size(); j++){
			torocc += counter0[i][j] - prev0[i][j];
			torinsrt += counter1[i][j] - prev1[i][j];
			chaclks += counter2[i][j] - prev2[i][j];
		}
		int cha_count =  counter0[0].size();
		//printf("No of CHA is %d\n", cha_count);

		if(torinsrt < 1){
			tor_latency = 0;
		}
		else {
		tor_latency = ((static_cast<double>(torocc) / torinsrt) / (chaclks / cha_count )) * 1e9;
		}
		printf("CHA latency is %.3f " , tor_latency);
	}

	else {
		printf("CHA analysis doesn't match\n");
	}


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
    //Bloew division by n_sample_in_sec is just a hack.. at the end total number of RPQ nad WPQ are convreted in per second. 
    double ddrcyclecount = (1e9 * (1*60) / (1/2.4)) * (1 / n_sample_in_sec);

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


inline void imcPostnutanix(pcm::IMC& imc, double n_sample_in_sec)
{
    if(imc.eventCount == 0) return;

    //Bloew division by n_sample_in_sec is just a hack.. at the end total number of RPQ nad WPQ are convreted in per second.
    //Actual ddr cycles are counted here by 2nd counter need to make sure 2nd counter is supplied with this ddr cycle event
    ///IMC-raw.x -e imc/config=0x000000000000ff05,name=UNC_M_CAS_COUNT.ALL -e imc/config=0x0000000000000101,name=UNC_M_CLOCKTICKS  -e imc/config=0x0000000000000082,name=UNC_M_WPQ_OCCUPANCY_PCH0 -e imc/config=0x0000000000000080,name=UNC_M_RPQ_OCCUPANCY_PCH0

    pcm::uint32 cpu_model = pcm::getCPUModel();
    double tbw_multiplier, wpq_multiplier, rpq_multiplier;
    std::tie(tbw_multiplier, wpq_multiplier, rpq_multiplier) = pcm::getMultipliersForModel(cpu_model);	

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

    int imc_count = counter0[0].size();

    for(int soc = 0; soc < pcm::sockets; soc++){
                double tbw = 0, ncyc = 0, wpq=0, rpq=0;
                //uint64 tbw_p = 0, rbw_p=0, wbw_p=0, wpq_p=0, rpq_p=0;
                printf("  socket%d_BW=", soc);

                for(int i = 0; i < counter0[soc].size(); i++){
                        tbw += (counter0[soc][i] - prev0[soc][i]);
                        ncyc += (counter1[soc][i] - prev1[soc][i]);
                        wpq += (counter2[soc][i] - prev2[soc][i]);
                    rpq += (counter3[soc][i] - prev3[soc][i]);
                }
                ncyc /= imc_count;
                tbw=(( tbw * 64) / 1e9) * n_sample_in_sec;

		tbw *= tbw_multiplier;
        	wpq *= wpq_multiplier;
        	rpq *= rpq_multiplier;

                printf("tot_bw=%.2f memWR_lvl=%.2f memRD_lvl=%.2f ", tbw, (wpq / ncyc / 126) , (rpq / ncyc / 126));

    }
    printf("\n");

    prev0 = counter0;
    prev1 = counter1;
    prev2 = counter2;
    prev3 = counter3;

}

