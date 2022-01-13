#include <iostream>
#include <sstream>

#include <fort.h>
#include <cpr/cpr.h>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct apiData {
    std::string name;
    std::string label;
    std::string payment;
    std::string price;    
};

class API {

public:
    std::string getHTTPRequest(std::string url, std::list<std::string> header, bool verbose = false) const
    {
   
        try
	    {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;

            boost::algorithm::replace_all(url, " ", "0%20");
            request.setOpt(new curlpp::options::Url(url)); 
            request.setOpt(new curlpp::options::Verbose(verbose)); 
               
            header.push_back("User-Agent: PostmanRuntime/7.26.10"); 
            header.push_back("Content-Type: charset=utf-8");
            request.setOpt(new curlpp::options::HttpHeader(header)); 
     
            std::stringstream os;
		    curlpp::options::WriteStream ws(&os);
            request.setOpt(ws);

            os << request;

            return os.str();
        }
	    catch(curlpp::RuntimeError & e)
	    {
		    return e.what();
	    }

	    catch(curlpp::LogicError & e)
	    {
		    return e.what();
	    }

    }

    std::vector<apiData> getAPI(const std::string &fuelType, const std::string &lat, const std::string &lon)
    {
        std::vector<apiData> data;
       
        std::string body = getHTTPRequest(
        "https://api.e-control.at/sprit/1.0/search/gas-stations/by-address?latitude="+lat+"&longitude="+lon+"&fuelType="+fuelType+"&includeClosed=false", 
            {
              "Content-Type: application/octet-stream"
            }
        );
        
        for( auto &e : json::parse(body) )
       {   
           if( e["prices"][0]["label"].is_string() ) 

              data.push_back( {  e["name"].get<std::string>()
                                ,e["prices"][0]["label"].get<std::string>() 
                                ,e["paymentMethods"]["others"].is_string() ? e["paymentMethods"]["others"].get<std::string>() : "Unbekannt"
                                ,e["prices"][0]["amount"].dump() } 
           );
       }
          
        return data;
    }

};

int main()
{

    std::string place, plz, type;

    std::cout << "Geben Sie ihren Ort an: ";
    std::cin >> place;
    std::cout << "Geben Sie ihre Postleitzahl an: ";
    std::cin >> plz;
    std::cout << "Geben Sie ihren Treibstoff an: Super oder Diesel? ";
    std::cin >> type;

    type = type == "Super" ? "SUP" : "DIE";
    
    API api;
    std::string out = api.getHTTPRequest(
        "https://nominatim.openstreetmap.org/search?q="+plz+"0%20"+place+"&format=json&polygon=1&addressdetails=1",
        {
        "Content-Type: application/octet-stream", 
        "Host: www.example.com"
        } 
    );
    
    if( out.length() > 10 )
    {
        json j = json::parse(out);
 
        ft_table_t *table = ft_create_table();
        ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);

        ft_write_ln(table, "Tankstelle", "Treibstoff", "Zahlung", "Preis");

        for(auto &e : api.getAPI( type, j[0]["lat"].get<std::string>(), j[0]["lon"].get<std::string>() ) )
                ft_write_ln( table, e.name.c_str(), e.label.c_str(), e.payment.c_str(), e.price.c_str() );
        
        printf("%s\n", ft_to_string(table));
        ft_destroy_table(table);
    }
    else
    {
        std::cout << "Es wurde nichts gefunden!";
    }
}
