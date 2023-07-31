#include "utils.h"
#include "exprtk.hpp"

namespace pcm
{


void printMap(const std::map<std::string, double>& values)
{
    for (const auto& pair : values)
    {
        std::cout << "Key: " << pair.first << " Value: " << pair.second << std::endl;
    }
}



double calculate_metric(const std::string& formula, std::map<std::string, double>& values, double n_sample_in_sec, bool multiplyFlag)
{
    exprtk::symbol_table<double> symbolTable;
    //printMap(values);

    for ( auto& pair : values)
    {
	//double nonConstCopy = pair.second;
        symbolTable.add_variable(pair.first, pair.second);
    }

    printf("tot_bw=%.2f memWR_lvl=%.2f memRD_lvl=%.2f ", ((values["c0"] * 64) / 1e9 ) * n_sample_in_sec  , (values["c2"] / (values["c1"]/8) / 126) , (values["c3"] / (values["c1"]/8) / 126));
    exprtk::expression<double> expression;
    expression.register_symbol_table(symbolTable);

    exprtk::parser<double> parser;
    if (!parser.compile(formula, expression))
    {

        std::cerr << "Failed to compile formula: " << formula << std::endl;
        exit(EXIT_FAILURE);
    }



    double result = expression.value();

    // Print debugging information
    //std::cout << "Formula: " << formula << std::endl;
    //std::cout << "Initial expression value: " << expression.value() << std::endl;
    //std::cout << "Multiply flag: " << (multiplyFlag ? "true" : "false") << std::endl;
    //std::cout << "n_sample_in_sec: " << n_sample_in_sec << std::endl;
// Apply n_sample_in_sec multiplication if flag is true
    if (multiplyFlag) 
    {
        result *= n_sample_in_sec;
    }
//std::cout << "Final result: " << result << std::endl;
    return result;
}




std::vector<std::string> split(const std::string & str, const char delim)
{
    std::string token;
    std::vector<std::string> result;
    std::istringstream strstr(str);
    while (std::getline(strstr, token, delim))
    {
        result.push_back(token);
    }
    return result;
}

std::tuple<double, double, double> getMultipliersForModel(uint32 cpu_model) {
        switch (cpu_model) {
            case GNR:
            case SRF:
                return std::make_tuple(2.0, 2.0, 2.0);
            default:
                return std::make_tuple(1.0, 1.0, 1.0);
        }
}



std::string cpuModelToString(SupportedCPUModels model)
{
    std::map<SupportedCPUModels, std::string> cpuModelToString;
    cpuModelToString[ICX] = "ICX";
    cpuModelToString[SPR] = "SPR";
    cpuModelToString[EMR] = "EMR";
    cpuModelToString[GNR] = "GNR";
    cpuModelToString[GNR_D] = "GNR_D";
    cpuModelToString[GRR] = "GRR";
    cpuModelToString[SRF] = "SRF";
    cpuModelToString[UNSUPPORTED] = "UNSUPPORTED";

    if (cpuModelToString.find(model) != cpuModelToString.end())
    {
        return cpuModelToString[model];
    }
    else
    {
        return "UNKNOWN";
    }
}




bool match(const std::string& subtoken, const std::string& sname, std::string& result)
{

	std::regex rgx(subtoken);
	std::smatch matched;
	if (std::regex_search(sname.begin(), sname.end(), matched, rgx)){
		std::cout << "match: " << matched[1] << '\n';
		result= matched[1];
		return true;
	}
	return false;

}

void s_expect::match(std::istream & istr) const
{
    istr >> std::noskipws;
    const auto len = length();
    char * buffer = new char[len + 2];
    buffer[0] = 0;
    istr.get(buffer, len+1);
    if (*this != std::string(buffer))
    {
        istr.setstate(std::ios_base::failbit);
    }
    delete [] buffer;
}

inline std::istream & operator >> (std::istream & istr, s_expect && s)
{
    s.match(istr);
    return istr;
}

inline std::istream & operator >> (std::istream && istr, s_expect && s)
{
    s.match(istr);
    return istr;
}

FILE * tryOpen(const char * path, const char * mode)
{
    FILE * f = fopen(path, mode);
    if (!f)
    {
        f = fopen((std::string("/pcm") + path).c_str(), mode);
    }
    return f;
}

std::string readSysFS(const char * path, bool silent = false)
{
    FILE * f = tryOpen(path, "r");
    if (!f)
    {
        if (silent == false) std::cerr << "ERROR: Can not open " << path << " file.\n";
        return std::string();
    }
    char buffer[1024];
    if(NULL == fgets(buffer, 1024, f))
    {
        if (silent == false) std::cerr << "ERROR: Can not read from " << path << ".\n";
        fclose(f);
        return std::string();
    }
    fclose(f);
    return std::string(buffer);
}

bool writeSysFS(const char * path, const std::string & value, bool silent = false)
{
    FILE * f = tryOpen(path, "w");
    if (!f)
    {
        if (silent == false) std::cerr << "ERROR: Can not open " << path << " file.\n";
        return false;
    }
    if (fputs(value.c_str(), f) < 0)
    {
        if (silent == false) std::cerr << "ERROR: Can not write to " << path << ".\n";
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

uint32 getCPUModel() {
    PCM_CPUID_INFO cpuinfo; // You'll need to define cpuinfo

    pcm_cpuid(1, cpuinfo);
    uint32 cpu_family = (((cpuinfo.array[0]) >> 8) & 0xf) | ((cpuinfo.array[0] & 0xf00000) >> 16);
    uint32 model = (((cpuinfo.array[0]) & 0xf0) >> 4) | ((cpuinfo.array[0] & 0xf0000) >> 12);
    uint32 cpu_stepping = cpuinfo.array[0] & 0x0f;

    if (cpu_family != 6)
    {
        std::cerr << "CPU Family is not supported by PCM" << " CPU Family: " << cpu_family << "\n";
        return UNSUPPORTED;  // Or another value to indicate error
    }

    return model;
}



int32 getNumCores()
{
    static int num_cores = -1;
    if(num_cores >= 0) return num_cores;

    // Only works for linux systems
    num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    
    return num_cores;
}

}   // namespace pcm
